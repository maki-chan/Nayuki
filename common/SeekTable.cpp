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
#include "SeekTable.h"

#include <stdexcept>

#include "Utilities.h"

namespace Nayuki {
    namespace FLAC {
        namespace Common {
            SeekTable::SeekTable() {
                points = std::vector<SeekPoint>();
            }

            SeekTable::SeekTable(std::vector<uint_fast8_t> b) : SeekTable(b.data(), b.size()) {
                // Nothing extra to do
            }

            SeekTable::SeekTable(uint_fast8_t b[], uint_fast32_t length) {
                points = std::vector<SeekPoint>();
                if (b == nullptr)
                    throw std::invalid_argument("Given payload data is null");
                if (length % 18 != 0)
                    throw std::invalid_argument("Data contains a partial seek point");
                for (uint_fast32_t i = 0; i < length; i += 18) {
                    SeekPoint p = SeekPoint();
                    p.sampleOffset = convertToUint64(b + i);
                    p.fileOffset = convertToUint64(b + i + 8);
                    p.frameSamples = convertToUint16(b + i + 16);
                    points.push_back(p);
                }
            }

            void SeekTable::checkValues() {
                // Check ordering of points
                for (uint_fast64_t i = 1; i < points.size(); i++) {
                    SeekPoint p = points.at(i);
                    if (p.sampleOffset != 0xFFFFFFFFFFFFFFFF) {
                        SeekPoint q = points.at(i - 1);
                        if (p.sampleOffset <= q.sampleOffset)
                            throw std::runtime_error("Sample offsets out of order");
                        if (p.fileOffset < q.fileOffset)
                            throw std::runtime_error("File offsets out of order");
                    }
                }
            }

            void SeekTable::write(bool last, Encode::BitOutputStream *out) {
                if (out == nullptr)
                    throw std::invalid_argument("Output stream cannot be null");
                if (points.size() > ((1 << 24) - 1) / 18)
                    throw std::runtime_error("Too many seek points");
                checkValues();

                // Write metadata block header
                out->writeInt(1, last ? 1 : 0);
                out->writeInt(7, 3);
                out->writeInt(24, points.size() * 18);

                // Write each seek point
                for (SeekPoint p : points) {
                    out->writeInt(32, (int_fast32_t)(p.sampleOffset >> 32));
                    out->writeInt(32, (int_fast32_t)(p.sampleOffset >>  0));
                    out->writeInt(32, (int_fast32_t)(p.fileOffset >> 32));
                    out->writeInt(32, (int_fast32_t)(p.fileOffset >>  0));
                    out->writeInt(16, p.frameSamples);
                }
            }
        }
    }
}
