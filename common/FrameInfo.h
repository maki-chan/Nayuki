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
#ifndef NAYUKI_FRAMEINFO_H
#define NAYUKI_FRAMEINFO_H

#include <array>
#include <cstdint>
#include <vector>

#include "../decode/FlacLowLevelInput.h"

#include "../encode/BitOutputStream.h"

namespace Nayuki {
    namespace FLAC {
        namespace Common {
            /**
             * Represents most fields in a frame header, in decoded (not raw) form. Mutable structure, not thread safe.
             * Also has methods for parsing and serializing this structure to/from bytes. All fields can be modified
             * freely when no method call is active.
             */
            class FrameInfo final {
            public:
                /**
	             * The index of this frame, where the foremost frame has index 0 and each subsequent frame increments
                 * it. This is either a `uint31` value or -1 if unused. Exactly one of the fields frameIndex and
                 * sampleOffset is equal to -1 (not both nor neither). This value can only be used if the stream info's
                 * `minBlockSize = maxBlockSize` (constant block size encoding style).
	             */
                int_fast32_t frameIndex;

                /**
	             * The offset of the first sample in this frame with respect to the beginning of the audio stream. This
                 * is either a `uint36` value or -1 if unused. Exactly one of the fields frameIndex and sampleOffset is
                 * equal to -1 (not both nor neither).
	             */
                int_fast64_t sampleOffset;

                /**
                 * The number of audio channels in this frame, in the range 1 to 8 inclusive. This value is fully
                 * determined by the channelAssignment field.
                 */
                int_fast32_t numChannels;

                /**
                 * The raw channel assignment value of this frame, which is a `uint4` value. This indicates the number
                 * of channels, but also tells the stereo coding mode.
                 */
                int_fast32_t channelAssignment;

                /**
                 * The number of samples per channel in this frame, in the range 1 to 65536 inclusive.
                 */
                int_fast32_t blockSize;

                /**
                 * The sample rate of this frame in hertz (Hz), in the range 1 to 655360 inclusive, or -1 if unavailable
                 * (i.e. the stream info should be consulted).
                 */
                int_fast32_t sampleRate;

                /**
                 * The sample depth of this frame in bits, in the range 8 to 24 inclusive, or -1 if unavailable (i.e.
                 * the stream info should be consulted).
                 */
                int_fast32_t sampleDepth;

                /**
                 * The size of this frame in bytes, from the start of the sync sequence to the end of the trailing
                 * CRC-16 checksum. A valid value is at least 10, or -1 if unavailable (e.g. the frame header was parsed
                 * but not the entire frame).
                 */
                int_fast32_t frameSize;

                /**
                 * Constructs a blank frame metadata structure, setting all fields to unknown or invalid values.
                 */
                explicit FrameInfo();

                /**
                 * Reads the next FLAC frame header from the specified input stream, either returning a new frame info
                 * object or `null`. The stream must be aligned to a byte boundary and start at a sync sequence. If EOF
                 * is immediately encountered before any bytes were read, then this returns `null`.
                 *
                 * Otherwise this reads between 6 to 16 bytes from the stream - starting from the sync code, and ending
                 * after the CRC-8 value is read (but before reading any subframes). It tries to parse the frame header
                 * data. After the values are successfully decoded, a new frame info object is created, almost all
                 * fields are set to the parsed values, and it is returned. (This doesn't read to the end of the frame,
                 * so the frameSize field is set to -1.)
                 * @param[in,out] in the input stream to read from (not `null`)
                 * @return a new frame info object or `null`
                 */
                static FrameInfo* readFrame(Decode::FlacLowLevelInput *in);

                /**
                 * Writes the current state of this object as a frame header to the specified output stream, from the
                 * sync field through to the CRC-8 field (inclusive). This does not write the data of subframes, the bit
                 * padding, nor the CRC-16 field.
                 * 
                 * The stream must be byte-aligned before this method is called, and will be aligned upon returning
                 * (i.e. it writes a whole number of bytes). This method initially resets the stream's CRC computations,
                 * which is useful behavior for the caller because it will need to write the CRC-16 at the end of the
                 * frame.
                 * @param[in,out] out the output stream to write to (not `null`)
                 */
                void writeHeader(Encode::BitOutputStream *out);

            private:
                /**
                 * Lookup table to translate block sizes to their code or vice versa.
                 */
                static const std::vector<std::array<int_fast32_t, 2>> BLOCK_SIZE_CODES;

                /**
                 * Lookup table to translate sample depths to their code or vice versa.
                 */
                static const std::vector<std::array<int_fast32_t, 2>> SAMPLE_DEPTH_CODES;

                /**
                 * Lookup table to translate sample rates to their code or vice versa.
                 */
                static const std::vector<std::array<int_fast32_t, 2>> SAMPLE_RATE_CODES;

                /**
                 * Reads 1 to 7 whole bytes from the input stream. Return value is a `uint36`.
                 * @param[in,out] in   the input stream to read from (not `null`)
                 * @return the decoded integer value
                 */
                static uint_fast64_t readUtf8Integer(Decode::FlacLowLevelInput *in);

                /**
                 * Decodes the block size code.
                 * @param[in]     code the encoded block size
                 * @param[in,out] in   the input stream to read from (not `null`)
                 * @return the decoded block size, a value in the range [1, 65536]
                 */
                static int_fast32_t decodeBlockSize(uint_fast8_t code, Decode::FlacLowLevelInput *in);

                /**
                 * Decodes the sample rate code.
                 * @param[in] code the encoded sample rate
                 * @param[in,out] in   the input stream to read from (not `null`)
                 * @return the decoded sample rate, a value in the range [-1, 655350]
                 */
                static int_fast32_t decodeSampleRate(uint_fast8_t code, Decode::FlacLowLevelInput *in);

                /**
                 * Decodes the sample depth.
                 * @param[in] code the encoded sample depth
                 * @return the decoded sample depth, a value in the range [-1, 24]
                 */
                static int_fast32_t decodeSampleDepth(uint_fast8_t code);

                /**
                 * Given a `uint36` value, this writes 1 to 7 whole bytes to the given output stream.
                 * @param[in]     val the `uint36` value to write
                 * @param[in,out] out the output stream to write to
                 */
                static void writeUtf8Integer(uint_fast64_t val, Encode::BitOutputStream *out);

                /**
                 * Returns a `uint4` value representing the given block size.
                 * @param[in] blockSize the block size to encode
                 * @return the `uint4` representation of the block size
                 */
                static uint_fast8_t getBlockSizeCode(int_fast32_t blockSize);

                /**
                 * Returns a uint4 value representing the given sample rate.
                 * @param[in] sampleRate the sample rate to encode
                 * @return the `uint4` representation of the sample rate
                 */
                static uint_fast8_t getSampleRateCode(int_fast32_t sampleRate);

                /**
                 * Returns a `uint3` value representing the given sample depth.
                 * @param[in] sampleDepth the sample depth to encode
                 * @return the `uint3` representation of the sample depth
                 */
                static uint_fast8_t getSampleDepthCode(int_fast32_t sampleDepth);

                /**
                 * Does a lookup in one of the code tables and tries to get the value in index 1 for the corresponding
                 * key in index 0.
                 * @param[in] table the code table to search in
                 * @param[in] key   the key to search for
                 * @return the result of the lookup or -1 if nothing was found
                 */
                static int_fast32_t searchFirst(std::vector<std::array<int_fast32_t, 2>> table, int_fast32_t key);

                /**
                 * Does a lookup in one of the code tables and tries to get the value in index 0 for the corresponding
                 * key in index 1.
                 * @param[in] table the code table to search in
                 * @param[in] key   the key to search for
                 * @return the result of the lookup or -1 if nothing was found
                 */
                static int_fast32_t searchSecond(std::vector<std::array<int_fast32_t, 2>> table, int_fast32_t key);
            };
        }
    }
}

#endif
