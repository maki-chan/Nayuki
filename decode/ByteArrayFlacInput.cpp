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
#include "ByteArrayFlacInput.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace Nayuki {
    namespace FLAC {
        namespace Decode {
            ByteArrayFlacInput::ByteArrayFlacInput(uint_fast8_t *b, uint_fast64_t len) : AbstractFlacLowLevelInput() {
                if (b == nullptr)
                    throw std::invalid_argument("FLAC data array cannot be null");
                data = b;
                length = len;
                offset = 0;
            }

            uint_fast64_t ByteArrayFlacInput::getLength() {
                return length;
            }

            void ByteArrayFlacInput::seekTo(uint_fast64_t pos) {
                offset = pos;
                positionChanged(pos);
            }

            int_fast32_t ByteArrayFlacInput::readUnderlying(uint_fast8_t buf[], uint_fast64_t off, uint_fast64_t len) {
                int n = std::min(length - offset, len);
                if (n == 0)
                    return -1;
                std::memcpy(buf + off, data + offset, n * sizeof(uint_fast8_t));
                offset += n;
                return n;
            }

            void ByteArrayFlacInput::close() {
                if (data != nullptr) {
                    data = nullptr;
                    AbstractFlacLowLevelInput::close();
                }
            }
        }
    }
}
