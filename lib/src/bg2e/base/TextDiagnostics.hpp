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
#include <bg2e/base/Log.hpp>
#include <cstdio>

namespace bg2e::base::detail {

template<class... Values>
void textWarning(const Values&... values) noexcept
{
    try
    {
        auto log = bg2e_log_warning;
        log << "Text warning: ";
        ((log << values), ...);
        log << bg2e_log_end;
    }
    catch (...)
    {
        std::fputs("Text warning: diagnostic unavailable; continuing without text.\n", stderr);
    }
}

}
