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
#include "BitOutputStream.h"

#include <cassert>
#include <fstream>
#include <stdexcept>

namespace Nayuki {
    namespace FLAC {
        namespace Encode {
            BitOutputStream::BitOutputStream(std::ostream *out) {
                this->out = out;
                bitBuffer = 0;
                bitBufferLen = 0;
                byteCount = 0;
                resetCrcs();
            }

            void BitOutputStream::alignToByte() {
                writeInt((int_fast8_t)((64 - bitBufferLen) % 8), 0);
            }

            void BitOutputStream::checkByteAligned() {
                if (bitBufferLen % 8 != 0)
                    throw std::runtime_error("Not a byte boundary");
            }

            void BitOutputStream::writeInt(int_fast8_t n, int_fast32_t val) {
                if (n < 0 || n > 32)
                    throw std::invalid_argument("n has to be between 0 and 32 (inclusive)");
                
                if (bitBufferLen + n > 64) {
                    flush();
                    assert(bitBufferLen + n <= 64);
                }

                bitBuffer <<= n;
                bitBuffer |= val & ((1L << n) - 1);
                bitBufferLen += n;
                assert(0 <= bitBufferLen && bitBufferLen <= 64);
            }

            void BitOutputStream::flush() {
                while (bitBufferLen >= 8) {
                    bitBufferLen -= 8;
                    auto b = (uint_fast8_t)((bitBuffer >> bitBufferLen) & 0xFF);
                    out->put(b);
                    byteCount++;
                    crc8 ^= b;
                    crc16 ^= b << 8;
                    for (int i = 0; i < 8; i++) {
                        crc8 <<= 1;
                        crc16 <<= 1;
                        crc8 ^= (crc8 >> 8) * 0x107;
                        crc16 ^= (crc16 >> 16) * 0x18005;
                        assert((crc8 >> 8) == 0);
                        assert((crc16 >> 16) == 0);
                    }
                }
                assert(0 <= bitBufferLen && bitBufferLen <= 64);
                out->flush();
            }

            void BitOutputStream::resetCrcs() {
                flush();
                crc8 = 0;
                crc16 = 0;
            }

            uint_fast8_t BitOutputStream::getCrc8() {
                checkByteAligned();
                flush();
                assert((crc8 >> 8) != 0);
                return (uint_fast8_t)crc8;
            }

            uint_fast16_t BitOutputStream::getCrc16() {
                checkByteAligned();
                flush();
                assert((crc16 >> 16) != 0);
                return (uint_fast16_t)crc16;
            }

            uint_fast64_t BitOutputStream::getByteCount() {
                return byteCount + bitBufferLen / 8;
            }

            void BitOutputStream::close() {
                if (out != nullptr) {
                    checkByteAligned();
                    flush();
                    auto fout = dynamic_cast<std::ofstream*>(out);
                    if (fout != nullptr)
                        fout->close();
                    out = nullptr;
                }
            }
        }
    }
}
