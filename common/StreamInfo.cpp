/*
 * Nayuki++ is a FLAC encoding library aiming for better compression than
 * the reference implementation by Xiph.Org.
 *
 * Copyright (C) 2019 Michael Armbruster
 * Copyright (C) Project Nayuki
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The following code is a derivative work of the code from the Nayuki project,
 * which is licensed under the terms of the LGPLv3.
 */
#include "StreamInfo.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

#include "../decode/ByteArrayFlacInput.h"
#include "../decode/DataFormatException.h"
#include "../decode/FlacLowLevelInput.h"

namespace Nayuki {
    namespace FLAC {
        namespace Common {
            StreamInfo::StreamInfo() {
                // Set these fields to legal unknown values
                minFrameSize = 0;
                maxFrameSize = 0;
                numSamples = 0;
                std::memset(md5Hash, 0, sizeof(md5Hash));

                // Set these fields to invalid (not reserved) values
                minBlockSize = 0;
                maxBlockSize = 0;
                sampleRate = 0;
            }

            StreamInfo::StreamInfo(std::vector<uint_fast8_t> b) : StreamInfo(b.data(), b.size()) {
                // Nothing extra to do
            }

            StreamInfo::StreamInfo(uint_fast8_t b[], uint_fast32_t length) {
                if (b == nullptr)
                    throw std::invalid_argument("Given metadata block is null");
                if (length != 34)
                    throw std::invalid_argument("Invalid data length");
                Decode::FlacLowLevelInput *in = new Decode::ByteArrayFlacInput(b, length);
                minBlockSize = in->readUint(16);
                maxBlockSize = in->readUint(16);
                minFrameSize = in->readUint(24);
                maxFrameSize = in->readUint(24);
                if (minBlockSize < 16)
                    throw Decode::DataFormatException("Minimum block size less than 16");
                if (maxBlockSize < minBlockSize)
                    throw Decode::DataFormatException("Maximum block size less than minimum block size");
                if (minFrameSize != 0 && maxFrameSize != 0 && maxFrameSize < minFrameSize)
                    throw Decode::DataFormatException("Maximum frame size less than minimum frame size");
                sampleRate = in->readUint(20);
                if (sampleRate == 0 || sampleRate > 655350)
                    throw Decode::DataFormatException("Invalid sample rate");
                numChannels = in->readUint(3) + 1;
                sampleDepth = in->readUint(5) + 1;
                numSamples = (long) in->readUint(18) << 18 | in->readUint(18); // uint36
                in->readFully(md5Hash, MD5_DIGEST_LENGTH);
                // Skip closing the in-memory stream
            }

            void StreamInfo::checkValues() {
                if ((minBlockSize >> 16) != 0)
                    throw std::runtime_error("Invalid minimum block size");
                if ((maxBlockSize >> 16) != 0)
                    throw std::runtime_error("Invalid maximum block size");
                if ((minFrameSize >> 24) != 0)
                    throw std::runtime_error("Invalid minimum frame size");
                if ((maxFrameSize >> 24) != 0)
                    throw std::runtime_error("Invalid maximum frame size");
                if (sampleRate == 0 || (sampleRate >> 20) != 0)
                    throw std::runtime_error("Invalid sample rate");
                if (numChannels < 1 || numChannels > 8)
                    throw std::runtime_error("Invalid number of channels");
                if (sampleDepth < 4 || sampleDepth > 32)
                    throw std::runtime_error("Invalid sample depth");
                if ((numSamples >> 36) != 0)
                    throw std::runtime_error("Invalid number of samples");
            }

            void StreamInfo::checkFrame(FrameInfo *meta) {
                if (meta->numChannels != numChannels)
                    throw Decode::DataFormatException("Channel count mismatch");
                if (meta->sampleRate != -1 && (uint_fast32_t) meta->sampleRate != sampleRate)
                    throw Decode::DataFormatException("Sample rate mismatch");
                if (meta->sampleDepth != -1 && meta->sampleDepth != sampleDepth)
                    throw Decode::DataFormatException("Sample depth mismatch");
                if (numSamples != 0 && (uint_fast64_t) meta->blockSize > numSamples)
                    throw Decode::DataFormatException("Block size exceeds total number of samples");

                if (meta->blockSize > maxBlockSize)
                    throw Decode::DataFormatException("Block size exceeds maximum");
                // Note: If minBlockSize == maxBlockSize, then the final block
                // in the stream is allowed to be smaller than minBlockSize

                if (minFrameSize != 0 && (uint_fast32_t) meta->frameSize < minFrameSize)
                    throw Decode::DataFormatException("Frame size less than minimum");
                if (maxFrameSize != 0 && (uint_fast32_t) meta->frameSize > maxFrameSize)
                    throw Decode::DataFormatException("Frame size exceeds maximum");
            }

            void StreamInfo::write(bool last, Nayuki::FLAC::Encode::BitOutputStream *out) {
                // Check arguments and state
                if (out == nullptr)
                    throw std::invalid_argument("Output stream cannot be null");
                checkValues();

                // Write metadata block header
                out->writeInt(1, last ? 1 : 0);
                out->writeInt(7, 0);  // Type
                out->writeInt(24, 34);  // Length

                // Write stream info block fields
                out->writeInt(16, minBlockSize);
                out->writeInt(16, maxBlockSize);
                out->writeInt(24, minFrameSize);
                out->writeInt(24, maxFrameSize);
                out->writeInt(20, sampleRate);
                out->writeInt(3, numChannels - 1);
                out->writeInt(5, sampleDepth - 1);
                out->writeInt(18, (int_fast32_t) (numSamples >> 18));
                out->writeInt(18, (int_fast32_t) (numSamples >> 0));
                for (unsigned char b : md5Hash)
                    out->writeInt(8, b);
            }

            unsigned char *StreamInfo::getMd5Hash(int_fast32_t *samples[], uint_fast8_t chans, uint_fast64_t numSamples,
                                                  uint_fast8_t depth) {
                // Check arguments
                if (samples == nullptr)
                    throw std::invalid_argument("Samples cannot be null");
                for (uint_fast8_t i = 0; i < chans; i++)
                    if (samples[i] == nullptr)
                        throw std::invalid_argument("No channel can have null samples");
                if (depth > 32 || depth % 8 != 0)
                    throw std::invalid_argument("Unsupported bit depth");

                // Create hasher
                MD5_CTX ctx;
                MD5_Init(&ctx);

                // Convert samples to a stream of bytes, compute hash
                uint_fast8_t numBytes = depth / 8;
                uint_fast64_t bufSize = chans * numBytes * std::min(numSamples, 2048ULL);
                uint_fast8_t *buf = new uint_fast8_t[bufSize];
                for (uint_fast64_t i = 0, l = 0; i < numSamples; i++) {
                    for (int j = 0; j < chans; j++) {
                        int_fast32_t val = samples[j][i];
                        for (int k = 0; k < numBytes; k++, l++)
                            buf[l] = (uint_fast8_t)((uint_fast32_t)val >> (k << 3));
                    }
                    if (l == bufSize || i == numSamples - 1) {
                        MD5_Update(&ctx, buf, l);
                        l = 0;
                    }
                }

                // Return final hasher result
                unsigned char *result = new unsigned char[MD5_DIGEST_LENGTH];
                MD5_Final(result, &ctx);
                return result;
            }
        }
    }
}
