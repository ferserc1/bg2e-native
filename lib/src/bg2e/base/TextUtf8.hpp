/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <cstddef>
#include <string_view>

namespace bg2e::base::detail {

// Caller supplies a nonempty remainder. Invalid sequences consume one byte,
// making replacement deterministic even for truncated/overlong/surrogate input.
inline char32_t nextTextScalar(std::string_view value, size_t& offset)
{
    const auto first = static_cast<unsigned char>(value[offset++]);
    if (first < 0x80)
    {
        return first;
    }
    const unsigned count = first >= 0xc2 && first <= 0xdf ? 2 :
        first >= 0xe0 && first <= 0xef ? 3 : first >= 0xf0 && first <= 0xf4 ? 4 : 0;
    if (!count || value.size() - offset < count - 1)
    {
        return U'\ufffd';
    }
    char32_t result = first & (0x7fu >> count);
    for (unsigned i = 0; i < count - 1; ++i)
    {
        const auto next = static_cast<unsigned char>(value[offset+i]);
        if ((next & 0xc0) != 0x80)
        {
            return U'\ufffd';
        }
        result = (result << 6) | (next & 0x3f);
    }
    if ((count == 2 && result < 0x80) || (count == 3 && result < 0x800) ||
        (count == 4 && result < 0x10000) || result > 0x10ffff || (result >= 0xd800 && result <= 0xdfff))
    {
        return U'\ufffd';
    }
    offset += count - 1;
    return result;
}

}
