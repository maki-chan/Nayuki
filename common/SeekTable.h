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
#ifndef NAYUKI_SEEKTABLE_H
#define NAYUKI_SEEKTABLE_H

#include <vector>
#include <cstdint>

#include "../encode/BitOutputStream.h"

namespace Nayuki {
    namespace FLAC {
        namespace Common {
            /**
             * Represents precisely all the fields of a seek table metadata block. Mutable structure, not thread-safe.
             * Also has methods for parsing and serializing this structure to/from bytes. All fields and objects can be
             * modified freely when no method call is active.
             */
            class SeekTable final {
            public:
                /**
                 * Represents a seek point entry in a seek table. Mutable structure, not thread-safe. This class itself
                 * does not check the correctness of data, but other classes might.
                 *
                 * A seek point with data `(sampleOffset = x, fileOffset = y, frameSamples = z)` means that at byte
                 * position `(y + (byte offset of foremost audio frame))` in the file, a FLAC frame begins (with the
                 * sync sequence), that frame has sample offset `x` (where sample 0 is defined as the start of the audio
                 * stream), and the frame contains `z` samples per channel.
                 */
                class SeekPoint final {
                public:
                    /**
                     * The sample offset in the audio stream, a `uint64` value. A value of -1 (i.e. 0xFFFFFFFFFFFFFFFF)
                     * means this is a placeholder point.
                     */
                    uint_fast64_t sampleOffset;

                    /**
                     * The byte offset relative to the start of the foremost frame, a `uint64` value. If sampleOffset is
                     * -1, then this value is ignored.
                     */
                    uint_fast64_t fileOffset;

                    /**
                     * The number of audio samples in the target block/frame, a `uint16` value. If sampleOffset is -1,
                     * then this value is ignored.
                     */
                    uint_fast16_t frameSamples;
                };

                /**
                 * The list of seek points in this seek table. It is okay to replace this list as needed (the initially
                 * constructed list object is not special).
                 */
                std::vector<SeekPoint> points;

                /**
                 * Constructs a blank seek table with an initially empty list of seek points. (Note that the empty state
                 * is legal.)
                 */
                SeekTable();

                /**
                 * Constructs a seek table by parsing the given byte array representing the metadata block. (The array
                 * must contain only the metadata payload, without the type or length fields.)
                 *
                 * This constructor does not check the validity of the seek points, namely the ordering of seek point
                 * offsets, so calling `checkValues()` on the freshly constructed object can fail. However, this does
                 * guarantee that every point's `frameSamples` field is a `uint16`.
                 * @param[in] b the metadata block's payload data to parse (not `null`)
                 */
                explicit SeekTable(std::vector<uint_fast8_t> b);

                /**
                 * Constructs a seek table by parsing the given byte array representing the metadata block. (The array
                 * must contain only the metadata payload, without the type or length fields.)
                 *
                 * This constructor does not check the validity of the seek points, namely the ordering of seek point
                 * offsets, so calling `checkValues()` on the freshly constructed object can fail. However, this does
                 * guarantee that every point's `frameSamples` field is a `uint16`.
                 * @param[in] b      the metadata block's payload data to parse (not `null`)
                 * @param[in] length the length of the payload data
                 */
                SeekTable(uint_fast8_t b[], uint_fast32_t length);

                /**
                 * Checks the state of this object and returns silently if all these criteria pass:
                 *
	             * - No object is `null`
                 * - The `frameSamples` field of each point is a `uint16` value
                 * - All points with `sampleOffset == -1 (i.e. 0xFFF...FFF) are at the end of the list
                 * - All points with `sampleOffset != -1` have strictly increasing values of `sampleOffset` and
                 *   non-decreasing values of `fileOffset`
                 */
                void checkValues();

                /**
                 * Writes all the points of this seek table as a metadata block to the specified output stream, also
                 * indicating whether it is the last metadata block. (This does write the type and length fields for the
                 * metadata block, unlike the constructor which takes an array without those fields.)
                 * @param[in]     last whether the metadata block is the final one in the FLAC file
                 * @param[in,out] out  the output stream to write to (not `null`)
                 */
                void write(bool last, Encode::BitOutputStream *out);
            };
        }
    }
}

#endif
