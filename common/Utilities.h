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
 */
#ifndef NAYUKI_UTILITIES_H
#define NAYUKI_UTILITIES_H

#include <cstdint>

namespace Nayuki {
    namespace FLAC {
        namespace Common {
            /**
             * Converts the next 8 bytes in a given byte array to a `uint64` value.
             * @param[in] buf the byte array from which 8 bytes will be converted
             * @return the converted `uint64` value
             */
            uint_fast64_t convertToUint64(uint_fast8_t buf[]) {
                return (((uint_fast64_t) (buf[0] & 0xff) << 56) | ((uint_fast64_t) (buf[1] & 0xff) << 48) |
                        ((uint_fast64_t) (buf[2] & 0xff) << 40) | ((uint_fast64_t) (buf[3] & 0xff) << 32) |
                        ((uint_fast64_t) (buf[4] & 0xff) << 24) | ((uint_fast64_t) (buf[5] & 0xff) << 16) |
                        ((uint_fast64_t) (buf[6] & 0xff) << 8) | ((uint_fast64_t) (buf[7] & 0xff)));
            }

            /**
             * Converts the next 2 bytes in a given byte array to a `uint16` value.
             * @param[in] buf the byte array from which 2 bytes will be converted
             * @return the converted `uint16` value
             */
            uint_fast16_t convertToUint16(uint_fast8_t buf[]) {
                return (((buf[0] & 0xff) << 8) | (buf[1] & 0xff));
            }

            /**
             * Counts the number of preceding zero bits in the given 32-bit value.
             * @param[in] i the value whose number of leading zeros is to be computed
             * @return the number of preceding zero bits
             */
            int_fast32_t numberOfLeadingZeros(uint_fast32_t i) {
                if (i == 0)
                    return 32;
                int n = 1;
                if (i >> 16 == 0) { n += 16; i <<= 16; }
                if (i >> 24 == 0) { n +=  8; i <<=  8; }
                if (i >> 28 == 0) { n +=  4; i <<=  4; }
                if (i >> 30 == 0) { n +=  2; i <<=  2; }
                n -= i >> 31;
                return n;
            }

            /**
             * Counts the number of preceding zero bits in the given 64-bit value.
             * @param[in] i the value whose number of leading zeros is to be computed
             * @return the number of preceding zero bits
             */
            int_fast32_t numberOfLeadingZeros(uint_fast64_t i) {
                if (i == 0)
                    return 64;
                int n = 1;
                int x = (int)(i >> 32);
                if (x == 0) { n += 32; x = (int)i; }
                if (x >> 16 == 0) { n += 16; x <<= 16; }
                if (x >> 24 == 0) { n +=  8; x <<=  8; }
                if (x >> 28 == 0) { n +=  4; x <<=  4; }
                if (x >> 30 == 0) { n +=  2; x <<=  2; }
                n -= x >> 31;
                return n;
            }
        }
    }
}

#endif
