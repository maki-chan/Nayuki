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
#ifndef NAYUKI_BYTEARRAYFLACINPUT_H
#define NAYUKI_BYTEARRAYFLACINPUT_H

#include "AbstractFlacLowLevelInput.h"

namespace Nayuki {
    namespace FLAC {
        namespace Decode {
            class ByteArrayFlacInput final : public AbstractFlacLowLevelInput {
            private:
                /**
                 * The underlying byte array to read from.
                 */
                uint_fast8_t *data;

                /**
                 * The length of the underlying byte array.
                 */
                uint_fast64_t length;

                /**
                 * The current offset in the underlying byte array.
                 */
                uint_fast64_t offset;

            protected:
                virtual int_fast32_t readUnderlying(uint_fast8_t buf[], uint_fast64_t off, uint_fast64_t len);

            public:
                /**
                 * Creates a new FLAC input stream with a given array of bytes.
                 * @param[in] b   the FLAC data for the input stream as byte array
                 * @param[in] len the length of the given FLAC data in bytes
                 */
                ByteArrayFlacInput(uint_fast8_t *b, uint_fast64_t len);

                virtual uint_fast64_t getLength();

                virtual void seekTo(uint_fast64_t pos);

                virtual void close();
            };
        }
    }
}

#endif
