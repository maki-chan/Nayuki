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
#include "FrameInfo.h"

#include <cassert>

#include "Utilities.h"

#include "../decode/DataFormatException.h"

namespace Nayuki {
    namespace FLAC {
        namespace Common {
            const std::vector<std::array<int_fast32_t, 2>> FrameInfo::BLOCK_SIZE_CODES = {
                {  192,  1},
                {  576,  2},
                { 1152,  3},
                { 2304,  4},
                { 4608,  5},
                {  256,  8},
                {  512,  9},
                { 1024, 10},
                { 2048, 11},
                { 4096, 12},
                { 8192, 13},
                {16384, 14},
                {32768, 15}
            };

            const std::vector<std::array<int_fast32_t, 2>> FrameInfo::SAMPLE_DEPTH_CODES = {
                { 8, 1},
                {12, 2},
                {16, 4},
                {20, 5},
                {24, 6}
            };

            const std::vector<std::array<int_fast32_t, 2>> FrameInfo::SAMPLE_RATE_CODES = {
                { 88200,  1},
                {176400,  2},
                {192000,  3},
                {  8000,  4},
                { 16000,  5},
                { 22050,  6},
                { 24000,  7},
                { 32000,  8},
                { 44100,  9},
                { 48000, 10},
                { 96000, 11}
            };

            FrameInfo::FrameInfo() {
                frameIndex = -1;
                sampleOffset = -1;
                numChannels = -1;
                channelAssignment = -1;
                blockSize = -1;
                sampleRate = -1;
                sampleDepth = -1;
                frameSize = -1;
            }

            FrameInfo* FrameInfo::readFrame(Decode::FlacLowLevelInput *in) {
                // Preliminaries
                in->resetCrcs();
                int_fast16_t temp = in->readByte();
                if (temp == -1)
                    return nullptr;
                auto result = new FrameInfo();
                result->frameSize = -1;

                // Read sync bits
                auto sync = (uint_fast16_t )(temp << 6 | in->readUint(6)); // Uint14
                if (sync != 0x3FFE)
                    throw Decode::DataFormatException("Sync code expected");

                // Read various simple fields
                if (in->readUint(1) != 0)
                    throw Decode::DataFormatException("Reserved bit");
                uint_fast32_t blockStrategy  = in->readUint(1);
                uint_fast32_t blockSizeCode  = in->readUint(4);
                uint_fast32_t sampleRateCode = in->readUint(4);
                uint_fast32_t chanAsgn       = in->readUint(4);
                result->channelAssignment = chanAsgn;
                if (chanAsgn < 8)
                    result->numChannels = chanAsgn + 1;
                else if (8 <= chanAsgn && chanAsgn <= 10)
                    result->numChannels = 2;
                else
                    throw Decode::DataFormatException("Reserved channel assignment");
                result->sampleDepth = decodeSampleDepth((uint_fast8_t)in->readUint(3));
                if (in->readUint(1) != 0)
                    throw Decode::DataFormatException("Reserved bit");

                // Read and check the frame/sample position field
                uint_fast64_t position = readUtf8Integer(in); // Reads 1 to 7 bytes
                if (blockStrategy == 0) {
                    if ((position >> 31) != 0)
                        throw Decode::DataFormatException("Frame index too large");
                    result->frameIndex = (int_fast32_t)position;
                    result->sampleOffset = -1;
                } else if (blockStrategy == 1) {
                    result->sampleOffset = position;
                    result->frameIndex = -1;
                } else
                    throw Decode::DataFormatException("Unknown block strategy");
                
                // Read variable-length data for some fields
                result->blockSize = decodeBlockSize((uint_fast8_t)blockSizeCode, in);
                result->sampleRate = decodeSampleRate((uint_fast8_t)sampleRateCode, in);
                uint_fast8_t computedCrc8 = in->getCrc8();
                if (in->readUint(8) != computedCrc8)
                    throw Decode::DataFormatException("CRC-8 mismatch");
                return result;
            }

            uint_fast64_t FrameInfo::readUtf8Integer(Decode::FlacLowLevelInput *in) {
                int_fast32_t head = in->readUint(8);
                int_fast32_t n = numberOfLeadingZeros((uint32_t)~(head << 24));
                assert(0 <= n && n <= 8);
                if (n == 0)
                    return (uint_fast64_t)head;
                else if (n == 1 || n == 8)
                    throw Decode::DataFormatException("Invalid UTF-8 coded number");
                else {
                    uint_fast64_t result = head & ((uint_fast64_t)0x7F >> n);
                    for (int i = 0; i < n - 1; i++) {
                        uint_fast32_t temp = in->readUint(8);
                        if ((temp & 0xC0) != 0x80)
                            throw Decode::DataFormatException("Invalid UTF-8 coded number");
                        result = (result << 6) | (temp & 0x3F);
                    }
                    if ((result >> 36) != 0)
                        throw Decode::DataFormatException("Decoded value does not fit into uint36 type");
                    return result;
                }
            }

            int_fast32_t FrameInfo::decodeBlockSize(uint_fast8_t code, Decode::FlacLowLevelInput *in) {
                if ((code >> 4) != 0)
                    throw std::invalid_argument("Invalid block size");
                switch (code) {
                    case 0:
                        throw Decode::DataFormatException("Reserved block size");
                    case 6:
                        return in->readUint(8) + 1;
                    case 7:
                        return in->readUint(16) + 1;
                    default:
                        int_fast32_t result = searchSecond(BLOCK_SIZE_CODES, code);
                        assert(result >= 1 && result <= 65536);
                        return result;
                }
            }

            int_fast32_t FrameInfo::decodeSampleRate(uint_fast8_t code, Decode::FlacLowLevelInput *in) {
                if ((code >> 4) != 0)
                    throw std::invalid_argument("Invalid sample rate");
                switch (code) {
                    case 0:
                        return -1; // Caller should obtain value from stream info metadata block
                    case 12:
                        return in->readUint(8);
                    case 13:
                        return in->readUint(16);
                    case 14:
                        return in->readUint(16) * 10;
                    case 15:
                        throw Decode::DataFormatException("Invalid sample rate");
                    default:
                        int_fast32_t result = searchSecond(SAMPLE_RATE_CODES, code);
                        assert(result >= 1 && result <= 655350);
                        return result;
                }
            }

            int_fast32_t FrameInfo::decodeSampleDepth(uint_fast8_t code) {
                if ((code >> 3) != 0)
                    throw std::invalid_argument("Invalid sample depth");
                else if (code == 0)
                    return -1; // Caller should obtain value from stream info metadata block
                else {
                    int_fast32_t result = searchSecond(SAMPLE_DEPTH_CODES, code);
                    if (result == -1)
                        throw Decode::DataFormatException("Reserved bit depth");
                    assert(result >= 1 && result <= 32);
                    return result;
                }
            }

            void FrameInfo::writeHeader(Encode::BitOutputStream *out) {
                if (out == nullptr)
                    throw std::invalid_argument("Output stream cannot be null");
                out->resetCrcs();
                out->writeInt(14, 0x3FFE); // Sync
                out->writeInt(1, 0); // Reserved
                out->writeInt(1, 1); // Blocking strategy

                uint_fast8_t blockSizeCode = getBlockSizeCode(blockSize);
                out->writeInt(4, blockSizeCode);
                uint_fast8_t sampleRateCode = getSampleRateCode(sampleRate);
                out->writeInt(4, sampleRateCode);

                out->writeInt(4, channelAssignment);
                out->writeInt(3, getSampleDepthCode(sampleDepth));
                out->writeInt(1, 0); // Reserved

                // Variable-length: 1 to 7 bytes
                if ((frameIndex != -1 && sampleOffset == -1) || (sampleOffset != -1 && frameIndex == -1))
                    writeUtf8Integer((uint_fast64_t)sampleOffset, out);
                else
                    throw std::runtime_error("Frame index and sample offset are mutually exclusive");

                // Variable-length: 0 to 2 bytes
                if (blockSizeCode == 6)
                    out->writeInt(8, blockSize - 1);
                else if (blockSizeCode == 7)
                    out->writeInt(16, blockSize - 1);

                // Variable-length: 0 to 2 bytes
                if (sampleRateCode == 12)
                    out->writeInt(8, sampleRate);
                else if (sampleRateCode == 13)
                    out->writeInt(16, sampleRate);
                else if (sampleRateCode == 14)
                    out->writeInt(16, sampleRate / 10);
                
                out->writeInt(8, out->getCrc8());
            }

            void FrameInfo::writeUtf8Integer(uint_fast64_t val, Encode::BitOutputStream *out) {
                if ((val >> 36) != 0)
                    throw std::invalid_argument("Given value does not fit into uint36 type");
                int_fast32_t bitLen = 64 - numberOfLeadingZeros((uint64_t)val);
                if (bitLen <= 7)
                    out->writeInt(8, (int_fast32_t)val);
                else {
                    int_fast32_t n = (bitLen - 2) / 5;
                    out->writeInt(8, ((uint_fast32_t)0xFF80 >> n) | (int_fast32_t)(val >> (n * 6)));
                    for (int_fast32_t i = n - 1; i >= 0; i--)
                        out->writeInt(8, 0x80 | ((int_fast32_t)(val >> (i * 6)) & 0x3F));
                }
            }

            uint_fast8_t FrameInfo::getBlockSizeCode(int_fast32_t blockSize) {
                int_fast32_t result = searchFirst(BLOCK_SIZE_CODES, blockSize);
                if (result != -1); // Already done
                else if (1 <= blockSize && blockSize <= 256)
                    result = 6;
                else if (1 <= blockSize && blockSize <= 65536)
                    result = 7;
                else
                    throw std::invalid_argument("Invalid block size");

                assert(((uint_fast32_t)result >> 4) == 0);
                return (uint_fast8_t)result;
            }

            uint_fast8_t FrameInfo::getSampleRateCode(int_fast32_t sampleRate) {
                if (sampleRate <= 0)
                    throw std::invalid_argument("Invalid sample rate");
                int_fast32_t result = searchFirst(SAMPLE_RATE_CODES, sampleRate);
                if (result != -1); // Already done
                else if (0 <= sampleRate && sampleRate < 256)
                    result = 12;
                else if (0 <= sampleRate && sampleRate < 65536)
                    result = 13;
                else if (0 <= sampleRate && sampleRate < 655360 && sampleRate % 10 == 0)
                    result = 14;
                else
                    result = 0;
                
                assert(((uint_fast32_t)result >> 4) == 0);
                return (uint_fast8_t)result;
            }

            uint_fast8_t FrameInfo::getSampleDepthCode(int_fast32_t sampleDepth) {
                if (sampleDepth != -1 && (sampleDepth < 1 || sampleDepth > 32))
                    throw std::invalid_argument("Invalid bit depth");
                int_fast32_t result = searchFirst(SAMPLE_DEPTH_CODES, sampleDepth);
                if (result == -1)
                    result = 0;
                assert(((uint_fast32_t)result >> 3) == 0);
                return (uint_fast8_t)result;
            }

            int_fast32_t FrameInfo::searchFirst(std::vector<std::array<int_fast32_t, 2>> table, int_fast32_t key) {
                for (const auto pair : table) {
                    if (pair[0] == key)
                        return pair[1]; 
                }
                return -1;
            }

            int_fast32_t FrameInfo::searchSecond(std::vector<std::array<int_fast32_t, 2>> table, int_fast32_t key)
            {
                for (const auto pair : table) {
                    if (pair[1] == key)
                        return pair[0];
                }
                return -1;
            }
        }
    }
}
