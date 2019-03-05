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
#ifndef NAYUKI_STREAMINFO_H
#define NAYUKI_STREAMINFO_H

#include <cstdint>
#include <vector>

#include <openssl/md5.h>

#include "FrameInfo.h"

#include "../encode/BitOutputStream.h"

namespace Nayuki {
    namespace FLAC {
        namespace Common {
            /**
             * Represents precisely all the fields of a stream info metadata block. Mutable structure, not thread-safe.
             * Also has methods for parsing and serializing this structure to/from bytes. All fields can be modified
             * freely when no method call is active.
             */
            class StreamInfo final {
            public:
                /**
                 * Minimum block size (in samples per channel) among the whole stream, a `uint16` value. However when
                 * `minBlockSize == maxBlockSize` (constant block size encoding style), the final block is allowed to be
                 * smaller than minBlockSize.
                 */
                uint_fast16_t minBlockSize;

                /**
                 * Maximum block size (in samples per channel) among the whole stream, a `uint16` value.
                 */
                uint_fast16_t maxBlockSize;

                /**
                 * Minimum frame size (in bytes) among the whole stream, a `uint24` value. However, a value of 0
                 * signifies that the value is unknown.
                 */
                uint_fast32_t minFrameSize;

                /**
                 * Maximum frame size (in bytes) among the whole stream, a `uint24` value. However, a value of 0
                 * signifies that the value is unknown.
                 */
                uint_fast32_t maxFrameSize;

                /**
                 * The sample rate of the audio stream (in hertz (Hz)), a positive `uint20` value. Note that 0 is an
                 * invalid value.
                 */
                uint_fast32_t sampleRate;

                /**
                 * The number of channels in the audio stream, between 1 and 8 inclusive. 1 means mono, 2 means stereo,
                 * et cetera.
                 */
                uint_fast8_t numChannels;

                /**
                 * The bits per sample in the audio stream, in the range 4 to 32 inclusive.
                 */
                uint_fast8_t sampleDepth;

                /**
                 * The total number of samples per channel in the whole stream, a `uint36` value. The special value of 0
                 * signifies that the value is unknown (not empty zero-length stream).
                 */
                uint_fast64_t numSamples;

                /**
                 * The 16-byte MD5 hash of the raw uncompressed audio data serialized in little endian with channel
                 * interleaving (not planar). It can be all zeros to signify that the hash was not computed. It is okay
                 * to replace this array as needed (the initially constructed array object is not special).
                 */
                unsigned char md5Hash[MD5_DIGEST_LENGTH];

                /**
                 * Constructs a blank stream info structure with certain default values.
                 */
                StreamInfo();

                /**
                 * Constructs a stream info structure by parsing the specified 34-byte metadata block. (The vector must
                 * contain only the metadata payload, without the type or length fields.)
                 * @param[in] b the metadata block's payload data to parse (not `null`)
                 */
                explicit StreamInfo(std::vector<uint_fast8_t> b);

                /**
                 * Constructs a stream info structure by parsing the specified 34-byte metadata block. (The array must
                 * contain only the metadata payload, without the type or length fields.)
                 * @param[in] b      the metadata block's payload data to parse (not `null`)
                 * @param[in] length the length of the given array parameter `b`
                 */
                StreamInfo(uint_fast8_t b[], uint_fast32_t length);

                /**
                 * Checks the state of this object, and either returns silently or throws an exception.
                 */
                void checkValues();

                /**
                 * Checks whether the specified frame information is consistent with values in this stream info object,
                 * either returning silently or throwing an exception.
                 * @param[in] meta the frame info object to check (not `null`)
                 */
                void checkFrame(FrameInfo *meta);

                /**
                 * Writes this stream info metadata block to the specified output stream, including the metadata block
                 * header, writing exactly 38 bytes. (This is unlike the constructor, which takes an array without the
                 * type and length fields.) The output stream must initially be aligned to a byte boundary, and will
                 * finish at a byte boundary.
                 * @param[in]     last whether the metadata block is the final one in the FLAC file
                 * @param[in,out] out  the output stream to write to (not `null`)
                 */
                void write(bool last, Encode::BitOutputStream *out);

                /**
                 * Computes and returns the MD5 hash of the specified raw audio sample data at the specified bit depth.
                 * Currently, the bit depth must be a multiple of 8, between 8 and 32 inclusive. The returned array is
                 * an array of length `MD5_DIGEST_LENGTH`.
                 * @param[in] samples    the audio samples to hash, where each subarray is a channel (all not `null`)
                 * @param[in] chans      the number of audio channels (number of sample channels)
                 * @param[in] numSamples the number of samples per channel
                 * @param[in] depth      the bit depth of the audio samples (each value is a signed `depth`-bit integer)
                 * @return
                 */
                static unsigned char *
                getMd5Hash(int_fast32_t *samples[], uint_fast8_t chans, uint_fast64_t numSamples,
                           uint_fast8_t depth);
            };
        }
    }
}

#endif
