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
#ifndef NAYUKI_BITOUTPUTSTREAM_H
#define NAYUKI_BITOUTPUTSTREAM_H

#include <cstdint>
#include <ostream>

namespace Nayuki {
    namespace FLAC {
        namespace Encode {
            /**
             * A bit-oriented output stream, with other methods tailored for FLAC usage (such as CRC calculation).
             */
            class BitOutputStream final {
            public:
                /**
                 * Constructs a FLAC-oriented bit output stream from the given byte-based output stream.
                 * @param[in,out] out the byte-based output stream which will be used
                 */
                explicit BitOutputStream(std::ostream *out);

                /**
                 * Writes between 0 to 7 zero bits, to align the current bit position to a byte boundary.
                 */
                void alignToByte();

                /**
                 * Writes the lowest `n` bits of the given value to this bit output stream. This doesn't care whether
                 * `val` represents a signed or unsigned integer.
                 * @param n   number of bits to write
                 * @param val integer containing the bits to write
                 */
                void writeInt(int_fast8_t n, int_fast32_t val);

                /**
                 * Writes out whole bytes from the bit buffer to the underlying stream. After this is done, only 0 to 7
                 * bits remain in the bit buffer. Also updates the CRCs on each byte written.
                 */
                void flush();

                /**
                 * Marks the current position (which must be byte-aligned) as the start of both CRC calculations.
                 */
                void resetCrcs();

                /**
                 * Returns the CRC-8 hash of all the bytes written since the last call to `resetCrcs()` (or from the
                 * beginning of stream if reset was never called).
                 * @return the calculated CRC-8 hash 
                 */
                uint_fast8_t getCrc8();

                /**
                 * Returns the CRC-16 hash of all the bytes written since the last call to `resetCrcs()` (or from the
                 * beginning of stream if reset was never called).
                 * @return the calculated CRC-16 hash 
                 */
                uint_fast16_t getCrc16();

                /**
                 * Returns the number of bytes written since the start of the stream.
                 * @return the number of bytes written 
                 */
                uint_fast64_t getByteCount();

                /**
                 * Writes out any internally buffered bit data, closes the underlying output stream, and invalidates
                 * this bit output stream object for any future operation. Note that a `BitOutputStream` only uses
                 * memory but does not have native resources. It is okay to `flush()` the pending data and simply let a
                 * `BitOutputStream` be garbage collected without calling `close()`, but the parent is still responsible
                 * for calling `close()` on the underlying output stream if it uses native resources.
                 */
                void close();

            private:
                /**
                 * The underlying byte-based output stream to write to.
                 */
                std::ostream *out;

                /**
                 * Only the bottom `bitBufferLen` bits are valid; the top bits are garbage.
                 */
                uint_fast64_t bitBuffer;

                /**
                 * Number of bits currently in the buffer. Always in the range [0, 64].
                 */
                uint_fast8_t bitBufferLen;

                /**
                 * Number of bytes written since the start of stream.
                 */
                uint_fast64_t byteCount;

                /**
                 * Current state of the CRC calculations. Always a `uint8` value.
                 */
                uint_fast8_t crc8;

                /**
                 * Current state of the CRC calculations. Always a `uint16` value.
                 */
                uint_fast16_t crc16;

                /**
                 * Either returns silently or throws an exception.
                 */
                void checkByteAligned();
            };
        }
    }
}

#endif
