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
#ifndef NAYUKI_FLACLOWLEVELINPUT_H
#define NAYUKI_FLACLOWLEVELINPUT_H

#include <cstdint>

namespace Nayuki {
    namespace FLAC {
        namespace Decode {
            /**
             * A low-level input stream tailored to the needs of FLAC decoding. An overview of methods includes bit
             * reading, CRC calculation, Rice decoding, and positioning and seeking (partly optional).
             */
            class FlacLowLevelInput {
            public:
                /**
                 * Default destructor for defined behavior
                 */
                virtual ~FlacLowLevelInput() = default;

                /**
                 * Returns the total number of bytes in the FLAC file represented by this input stream. This number
                 * should not change for the lifetime of this object. Implementing this is optional; it's intended to
                 * support blind seeking without the use of seek tables, such as binary searching the whole file. A
                 * class may choose to throw an exception instead, such as for a non-seekable network input stream of
                 * unknown length.
                 * @return the length of the stream in bytes
                 */
                virtual uint_fast64_t getLength() = 0;

                /**
                 * Returns the current byte position in the stream, a non-negative value. This increments after every 8
                 * bits read, and a partially read byte is treated as unread. This value is 0 initially, is set directly
                 * by `seekTo()`, and potentially increases after every call to a `read*()` method. Other methods do not
                 * affect this value.
                 * @return the current position in the stream
                 */
                virtual uint_fast64_t getPosition() = 0;

                /**
                 * Returns the current number of consumed bits in the current byte. This starts at 0, increments for
                 * each bit consumed, maxes out at 7, then resets to 0 and repeats.
                 * @return the number of consumed bits of the current byte
                 */
                virtual uint_fast8_t getBitPosition() = 0;

                /**
                 * Changes the position of the next read to the given byte offset from the start of the stream. This
                 * also resets CRCs and sets the bit position to 0. Implementing this is optional; it is intended to
                 * support playback seeking. A class may choose to throw an exception instead, such as for a
                 * non-seekable network input stream.
                 * @param[in] pos the new position for the stream
                 */
                virtual void seekTo(uint_fast64_t pos) = 0;

                /**
                 * Reads the next given number of bits (`0 <= n <= 32`) as an unsigned integer. However, in the case of
                 * `n = 32`, the result will be a normal unsigned integer (`uint32`).
                 * @param[in] n the number of bits to read
                 * @return the read unsigned integer
                 */
                virtual uint_fast32_t readUint(uint_fast8_t n) = 0;

                /**
                 * Reads the next given number of bits (`0 <= n <= 32`) as a signed integer (i.e. sign-extended to
                 * `int32`).
                 * @param[in] n the number of bits to read
                 * @return the read signed integer
                 */
                virtual int_fast32_t readSignedInt(uint_fast8_t n) = 0;

                /**
                 * Reads and decodes the next batch of Rice-coded signed integers. Note that any Rice-coded integer
                 * might read a large number of bits from the underlying stream (but not in practice because it would be
                 * a very inefficient encoding). Every new value stored into the array is guaranteed to fit into a
                 * signed `int53` - see FrameDecoder.restoreLpc() for an explanation of why this is a necessary (but not
                 * sufficient) bound on the range of decoded values.
                 * @param[in]  param  the rice coding parameter
                 * @param[out] result the decoded signed integers
                 * @param[in]  start  unknown, copied from original work
                 * @param[in]  end    unknown, copied from original work
                 */
                virtual void
                readRiceSignedInts(int_fast32_t param, int_fast64_t result[], int_fast32_t start, int_fast32_t end) = 0;

                /**
                 * Returns the next unsigned byte value (in the range [0, 255]) or -1 for `EOF`. Must be called at a
                 * byte boundary (i.e. `getBitPosition() == 0`), otherwise an exception is thrown.
                 * @return the read byte value or -1
                 */
                virtual int_fast16_t readByte() = 0;

                /**
                 * Discards any partial bits, then reads the given array for a specified length or throws an exception.
                 * Must be called at a byte boundary (i.e. `getBitPosition() == 0`), otherwise an exception is thrown.
                 * @param[out] b      the array to fill with read bytes
                 * @param[in]  length the length of the array `b`
                 */
                virtual void readFully(uint_fast8_t b[], uint_fast64_t length) = 0;

                /**
                 * Marks the current byte position as the start of both CRC calculations. The effect of `resetCrcs()` is
                 * implied at the beginning of stream and when `seekTo()` is called. Must be called at a byte boundary
                 * (i.e. `getBitPosition() == 0`), otherwise an exception is thrown.
                 */
                virtual void resetCrcs() = 0;

                /**
                 * Returns the CRC-8 hash of all the bytes read since the most recent time one of these events occurred:
                 * 
                 * - a call to `resetCrcs()`
                 * - a call to `seekTo()`
                 * - the beginning of stream
                 * 
	             * Must be called at a byte boundary (i.e. `getBitPosition() == 0`), otherwise an exception is thrown.
                 * @return the CRC-8 hash of the bytes read
                 */
                virtual uint_fast8_t getCrc8() = 0;

                /**
                 * Returns the CRC-16 hash of all the bytes read since the most recent time one of these events
                 * occurred:
                 * 
                 * - a call to `resetCrcs()`
                 * - a call to `seekTo()`
                 * - the beginning of stream
                 * 
                 * Must be called at a byte boundary (i.e. `getBitPosition() == 0`), otherwise an exception is thrown.
                 * @return the CRC-16 hash of the bytes read
                 */
                virtual uint_fast16_t getCrc16() = 0;

                /**
                 * Closes underlying objects / native resources, and possibly discards memory buffers. Generally
                 * speaking, this operation invalidates this input stream, so calling methods (other than `close()`) or
                 * accessing fields thereafter should be forbidden. The `close()` method must be idempotent and safe
                 * when called more than once. If an implementation does not have native or time-sensitive resources, it
                 * is okay for the class user to skip calling `close()` and simply let the object be garbage-collected.
                 * But out of good habit, it is recommended to always close a FlacLowLevelInput stream so that the logic
                 * works correctly on all types.
                 */
                virtual void close() = 0;
            };
        }
    }
}

#endif
