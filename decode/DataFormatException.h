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
#ifndef NAYUKI_DATAFORMATEXCEPTION_H
#define NAYUKI_DATAFORMATEXCEPTION_H

#include <stdexcept>

namespace Nayuki {
    namespace FLAC {
        namespace Decode {
            class DataFormatException : std::runtime_error {
            public:
                explicit DataFormatException(const std::string& what_arg) : std::runtime_error(what_arg) { }
                explicit DataFormatException(const char *what_arg) : std::runtime_error(what_arg) { }
            };
        }
    }
}

#endif
