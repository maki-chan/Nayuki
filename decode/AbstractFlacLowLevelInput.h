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
#ifndef NAYUKI_ABSTRACTFLACLOWLEVELINPUT_H
#define NAYUKI_ABSTRACTFLACLOWLEVELINPUT_H

#include "FlacLowLevelInput.h"

namespace Nayuki {
    namespace FLAC {
        namespace Decode {
            class AbstractFlacLowLevelInput : FlacLowLevelInput {
            private:
                /**
                 * Constant representing the length of the byte buffer.
                 */
                static const int_fast16_t BUF_SIZE = 4096;

                /**
                 * Unknown variable, ported from original work.
                 *
                 * Configurable, must be positive.
                 */
                static const int_fast8_t RICE_DECODING_TABLE_BITS = 13;

                /**
                 * Unknown variable, ported from original work.
                 */
                static const int_fast32_t RICE_DECODING_TABLE_MASK = (1 << RICE_DECODING_TABLE_BITS) - 1;

                /**
                 * Length of rice decoding tables.
                 */
                static const int_fast8_t RICE_DECODING_TABLE_LEN = 31;

                /**
                 * Unknown variable, ported from original work.
                 */
                static uint_fast8_t **RICE_DECODING_CONSUMED_TABLES;

                /**
                 * Unknown variable, ported from original work.
                 */
                static int_fast32_t **RICE_DECODING_VALUE_TABLES;

                /**
                 * Unknown variable, ported from original work.
                 *
                 * Configurable, must be positive, and `RICE_DECODING_CHUNK * RICE_DECODING_TABLE_BITS <= 64`.
                 */
                static const int_fast32_t RICE_DECODING_CHUNK = 4;

                /**
                 * For CRC calculations.
                 */
                static const int_fast32_t CRC_TABLE_LEN = 256;

                /**
                 * For CRC calculations.
                 */
                static uint_fast8_t *CRC8_TABLE;

                /**
                 * For CRC calculations.
                 */
                static uint_fast16_t *CRC16_TABLE;

                /**
                 * Unknown variable, ported from original work.
                 */
                uint_fast64_t byteBufferStartPos;

                /**
                 * Data from the underlying stream is first stored into this byte buffer before further processing.
                 */
                uint_fast8_t *byteBuffer;

                /**
                 * Unknown variable, ported from original work.
                 */
                int_fast32_t byteBufferLen;

                /**
                 * Unknown variable, ported from original work
                 */
                int_fast32_t byteBufferIndex;

                /**
                 * The buffer of next bits to return to a reader. Note that `byteBufferIndex` is incremented when byte
                 * values are put into the bit buffer, but they might not have been consumed by the ultimate reader yet.
                 *
                 * Only the bottom `bitBufferLen` bits are valid; the top bits are garbage.
                 */
                uint_fast64_t bitBuffer;

                /**
                 * Number of bits currently in the `bitBuffer`. Always in the range [0, 64].
                 */
                uint_fast8_t bitBufferLen;

                /**
                 * Current state of the CRC calculations. Always a `uint8` value.
                 */
                uint_fast16_t crc8;

                /**
                 * Current state of the CRC calculations. Always a `uint16` value.
                 */
                uint_fast32_t crc16;

                /**
                 * Current state of the CRC calculations. In the range [0, `byteBufferLen`], unless
                 * `byteBufferLen == -1`.
                 */
                int_fast32_t crcStartIndex;

                /**
                 * Either returns silently or throws an exception.
                 */
                void checkByteAligned();

                /**
                 * Appends at least 8 bits to the bit buffer, or throws an exception.
                 */
                void fillBitBuffer();

                /**
                 * Reads a byte from the byte buffer (if available) or from the underlying stream, returning either a
                 * `uint8` or -1.
                 * @return read byte from the underlying stream or -1
                 */
                int_fast16_t readUnderlying();

                /**
                 * Updates the two CRC values with data from the byte buffer in the range
                 * [`crcStartIndex`, `byteBufferIndex - unusedTrailingBytes`].
                 * @param unusedTrailingBytes negative offset for bytes used in CRC calculation
                 */
                void updateCrcs(int_fast32_t unusedTrailingBytes);

            protected:
                /**
                 * When a subclass handles `seekTo()` and didn't throw an exception, it must call this method to flush
                 * the buffers of upcoming data.
                 * @param pos new start position of the byte buffer
                 */
                void positionChanged(uint_fast64_t pos);

                /**
                 * Reads up to `len` bytes from the underlying byte-based input stream into the given array subrange.
                 * Returns a value in the range [0, `len`] for a successful read, or -1 if the end of stream was
                 * reached.
                 * @param buf the byte array which should be filled
                 * @param off the starting offset in the byte array to fill
                 * @param len the number of bytes to read from the input stream and to write to the given array
                 * @return number of bytes read and written, or -1
                 */
                virtual int_fast32_t readUnderlying(uint_fast8_t buf[], int off, int len) = 0;

            public:
                /**
                 * Creates a default and empty input stream.
                 */
                AbstractFlacLowLevelInput();

                virtual uint_fast64_t getPosition();

                virtual uint_fast8_t getBitPosition();

                virtual uint_fast32_t readUint(uint_fast8_t n);

                virtual int_fast32_t readSignedInt(uint_fast8_t n);

                virtual void
                readRiceSignedInts(int_fast32_t param, int_fast64_t result[], int_fast32_t start, int_fast32_t end);

                virtual int_fast16_t readByte();

                virtual void readFully(uint_fast8_t b[], uint_fast64_t length);

                virtual void resetCrcs();

                virtual uint_fast8_t getCrc8();

                virtual uint_fast16_t getCrc16();

                virtual void close();
            };
        }
    }
}

#endif
