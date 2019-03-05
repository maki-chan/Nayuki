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
#include "AbstractFlacLowLevelInput.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdexcept>

#include "DataFormatException.h"

namespace Nayuki {
    namespace FLAC {
        namespace Decode {
            uint_fast8_t *AbstractFlacLowLevelInput::CRC8_TABLE = [] {
                uint_fast8_t *crc8_table = new uint_fast8_t[CRC_TABLE_LEN];
                for (int i = 0; i < CRC_TABLE_LEN; i++) {
                    uint_fast32_t temp8 = i;
                    for (int j = 0; j < 8; j++) {
                        temp8 = (temp8 << 1) ^ ((temp8 >> 7) * 0x107);
                    }
                    crc8_table[i] = (uint_fast8_t) temp8;
                }
                return crc8_table;
            }();

            uint_fast16_t *AbstractFlacLowLevelInput::CRC16_TABLE = [] {
                uint_fast16_t *crc16_table = new uint_fast16_t[CRC_TABLE_LEN];
                for (int i = 0; i < CRC_TABLE_LEN; i++) {
                    uint_fast32_t temp16 = i << 8;
                    for (int j = 0; j < 8; j++) {
                        temp16 = (temp16 << 1) ^ ((temp16 >> 15) * 0x18005);
                    }
                    crc16_table[i] = (uint_fast16_t) temp16;
                }
                return crc16_table;
            }();

            uint_fast8_t **AbstractFlacLowLevelInput::RICE_DECODING_CONSUMED_TABLES = [] {
                uint_fast8_t **consumed_tables = new uint_fast8_t *[RICE_DECODING_TABLE_LEN];
                for (int_fast32_t param = 0; param < RICE_DECODING_TABLE_LEN; param++) {
                    consumed_tables[param] = new uint_fast8_t[1 << RICE_DECODING_TABLE_BITS];
                    for (uint_fast32_t i = 0;; i++) {
                        uint_fast32_t numBits = (i >> param) + 1 + param;
                        if (numBits > RICE_DECODING_TABLE_BITS)
                            break;
                        uint_fast32_t bits = ((1 << param) | (i & ((1 << param) - 1)));
                        uint_fast32_t shift = RICE_DECODING_TABLE_BITS - numBits;
                        for (int_fast32_t j = 0; j < (1 << shift); j++) {
                            consumed_tables[param][(bits << shift) | j] = (uint_fast8_t) numBits;
                        }
                    }
                }
                return consumed_tables;
            }();

            int_fast32_t **AbstractFlacLowLevelInput::RICE_DECODING_VALUE_TABLES = [] {
                int_fast32_t **values_tables = new int_fast32_t *[RICE_DECODING_TABLE_LEN];
                for (int_fast32_t param = 0; param < RICE_DECODING_TABLE_LEN; param++) {
                    values_tables[param] = new int_fast32_t[1 << RICE_DECODING_TABLE_BITS];
                    for (uint_fast32_t i = 0;; i++) {
                        uint_fast32_t numBits = (i >> param) + 1 + param;
                        if (numBits > RICE_DECODING_TABLE_BITS)
                            break;
                        uint_fast32_t bits = ((1 << param) | (i & ((1 << param) - 1)));
                        uint_fast32_t shift = RICE_DECODING_TABLE_BITS - numBits;
                        for (int_fast32_t j = 0; j < (1 << shift); j++) {
                            values_tables[param][(bits << shift) | j] = (i >> 1) ^ -(i & 1);
                        }
                    }
                }
                return values_tables;
            }();

            AbstractFlacLowLevelInput::AbstractFlacLowLevelInput() {
                byteBuffer = new uint_fast8_t[BUF_SIZE];
                positionChanged(0);
            }

            uint_fast64_t AbstractFlacLowLevelInput::getPosition() {
                return byteBufferStartPos + byteBufferIndex - (bitBufferLen + 7) / 8;
            }

            uint_fast8_t AbstractFlacLowLevelInput::getBitPosition() {
                return (-bitBufferLen) & 7;
            }

            void AbstractFlacLowLevelInput::positionChanged(uint_fast64_t pos) {
                byteBufferStartPos = pos;
                std::memset(byteBuffer, 0, BUF_SIZE * sizeof(uint_fast8_t));
                byteBufferLen = 0;
                byteBufferIndex = 0;
                bitBuffer = 0;
                bitBufferLen = 0;
                resetCrcs();
            }

            void AbstractFlacLowLevelInput::checkByteAligned() {
                if (bitBufferLen % 8 != 0)
                    throw std::runtime_error("Not at a byte boundary");
            }

            uint_fast32_t AbstractFlacLowLevelInput::readUint(uint_fast8_t n) {
                if (n > 32)
                    throw std::invalid_argument("Cannot read more than 32 bits of a `uint32` value");
                while (bitBufferLen < n) {
                    int_fast32_t b = readUnderlying();
                    if (b == -1)
                        throw std::runtime_error("End of data");
                    bitBuffer = (bitBuffer << 8) | b;
                    bitBufferLen += 8;
                    assert(0 <= bitBufferLen && bitBufferLen <= 64);
                }
                uint_fast32_t result = (uint_fast32_t) (bitBuffer >> (bitBufferLen - n));
                if (n != 32) {
                    result &= (1 << n) - 1;
                    assert((result >> n) == 0);
                }
                bitBufferLen -= n;
                assert(0 <= bitBufferLen && bitBufferLen <= 64);
                return result;
            }

            int_fast32_t AbstractFlacLowLevelInput::readSignedInt(uint_fast8_t n) {
                if (n > 32)
                    throw std::invalid_argument("Cannot read more than 32 bits of a `uint32` value");
                int_fast32_t shift = 32 - n;
                return (readUint(n) << shift) >> shift;
            }

            void
            AbstractFlacLowLevelInput::readRiceSignedInts(int_fast32_t param, int_fast64_t result[], int_fast32_t start,
                                                          int_fast32_t end) {
                if (param < 0 || param > 31)
                    throw std::invalid_argument("Rice Code Parameter has to be between 0 and 31 inclusive");
                uint_fast64_t unaryLimit = 1UL << (53 - param);

                uint_fast8_t *consumeTable = RICE_DECODING_CONSUMED_TABLES[param];
                int_fast32_t *valueTable = RICE_DECODING_VALUE_TABLES[param];
                while (true) {
                    while (start <= end - RICE_DECODING_CHUNK) {
                        if (bitBufferLen < RICE_DECODING_CHUNK * RICE_DECODING_TABLE_BITS) {
                            if (byteBufferIndex <= byteBufferLen - 8) {
                                fillBitBuffer();
                            } else
                                break;
                        }
                        for (int_fast32_t i = 0; i < RICE_DECODING_CHUNK; i++, start++) {
                            // Fast decoder
                            int_fast32_t extractedBits =
                                    (int_fast32_t) (bitBuffer >> (bitBufferLen - RICE_DECODING_TABLE_BITS)) &
                                    RICE_DECODING_TABLE_MASK;
                            uint_fast8_t consumed = consumeTable[extractedBits];
                            if (consumed == 0)
                                goto middle;
                            bitBufferLen -= consumed;
                            result[start] = valueTable[extractedBits];
                        }
                    }

                    middle:
                    if (start >= end)
                        break;
                    int_fast64_t val = 0;
                    while (readUint(1) == 0) {
                        if (val >= unaryLimit) {
                            // At this point, the final decoded value would be so large that the result of the
                            // downstream restoreLpc() calculation would not fit in the output sample's bit depth -
                            // hence why we stop early and throw an exception. However, this check is conservative
                            // and doesn't catch all the cases where the post-LPC result wouldn't fit.
                            throw DataFormatException("Residual value too large");
                        }
                        val++;
                    }
                    val = (val << param) | readUint(param);  // Note: Long masking unnecessary because param <= 31
                    assert(((uint_fast64_t)val >> 53) == 0);  // Must fit a uint53 by design due to unaryLimit
                    val = ((uint_fast64_t)val >> 1) ^ -(val & 1);  // uint53 to int53 according to rice coding
                    assert((val >> 52) == 0 || (val >> 52) == -1);  // Must fit a signed int53 by design
                    result[start] = val;
                    start++;
                }
            }

            void AbstractFlacLowLevelInput::fillBitBuffer() {
                int_fast32_t i = byteBufferIndex;
                int_fast32_t n = std::min((int_fast32_t)((64U - bitBufferLen) >> 3), byteBufferLen - i);
                uint_fast8_t *b = byteBuffer;
                if (n > 0) {
                    for (int j = 0; j < n; j++, i++)
                        bitBuffer = (bitBuffer << 8) | (b[i] & 0xFF);
                    bitBufferLen += n << 3;
                } else if (bitBufferLen <= 56) {
                    int temp = readUnderlying();
                    if (temp == -1)
                        throw std::runtime_error("End of data");
                    bitBuffer = (bitBuffer << 8) | temp;
                    bitBufferLen += 8;
                }
                assert(8 <= bitBufferLen && bitBufferLen <= 64);
                byteBufferIndex += n;
            }

            int_fast16_t AbstractFlacLowLevelInput::readByte() {
                checkByteAligned();
                if (bitBufferLen >= 8)
                    return readUint(8);
                else {
                    assert(bitBufferLen == 0);
                    return readUnderlying();
                }
            }

            void AbstractFlacLowLevelInput::readFully(uint_fast8_t b[], uint_fast64_t length) {
                if (b == nullptr)
                    throw std::invalid_argument("Output buffer cannot be null");
                checkByteAligned();
                for (uint_fast64_t i = 0; i < length; i++)
                    b[i] = (uint_fast8_t)readUint(8);
            }

            int_fast16_t AbstractFlacLowLevelInput::readUnderlying() {
                if (byteBufferIndex >= byteBufferLen) {
                    if (byteBufferLen == -1)
                        return -1;
                    byteBufferStartPos += byteBufferLen;
                    updateCrcs(0);
                    byteBufferLen = readUnderlying(byteBuffer, 0, BUF_SIZE);
                    crcStartIndex = 0;
                    if (byteBufferLen <= 0)
                        return -1;
                    byteBufferIndex = 0;
                }
                assert(byteBufferIndex < byteBufferLen);
                int_fast16_t temp = byteBuffer[byteBufferIndex] & 0xFF;
                byteBufferIndex++;
                return temp;
            }

            void AbstractFlacLowLevelInput::resetCrcs() {
                checkByteAligned();
                crcStartIndex = byteBufferIndex - bitBufferLen / 8;
                crc8 = 0;
                crc16 = 0;
            }

            uint_fast8_t AbstractFlacLowLevelInput::getCrc8() {
                checkByteAligned();
                updateCrcs(bitBufferLen / 8);
                assert((crc8 >> 8) != 0);
                return crc8;
            }

            uint_fast16_t AbstractFlacLowLevelInput::getCrc16() {
                checkByteAligned();
                updateCrcs(bitBufferLen / 8);
                assert((crc16 >> 16) != 0);
                return crc16;
            }

            void AbstractFlacLowLevelInput::updateCrcs(int_fast32_t unusedTrailingBytes) {
                int_fast32_t end = byteBufferIndex - unusedTrailingBytes;
                for (int_fast32_t i = crcStartIndex; i < end; i++) {
                    uint_fast8_t b = byteBuffer[i] & 0xFF;
                    crc8 = CRC8_TABLE[crc8 ^ b] & 0xFF;
                    crc16 = CRC16_TABLE[(crc16 >> 8) ^ b] ^ ((crc16 & 0xFF) << 8);
                    assert((crc8 >> 8) == 0);
                    assert((crc16 >> 16) == 0);
                }
                crcStartIndex = end;
            }

            void AbstractFlacLowLevelInput::close() {
                byteBuffer = nullptr;
                byteBufferLen = -1;
                byteBufferIndex = -1;
                bitBuffer = 0;
                bitBufferLen = -1;
                crc8 = -1;
                crc16 = -1;
                crcStartIndex = -1;
            }
        }
    }
}
