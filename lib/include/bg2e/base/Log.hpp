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

#include <iostream>

#include <bg2e/common.hpp>
#include <bg2e/base/PlatformTools.hpp>

namespace bg2e {
namespace base {

class BG2E_API Log {
public:
    enum class Level {
        Info = 0,
        Warning = 1,
        Error = 2,
        Debug = 3
    };

    static bool isDebug();

    Log(Level l) : _level{l} {}

    template <typename T>
    Log& operator << (const T & value) {
        if ((_level == Level::Debug && isDebug()) || _level != Level::Debug) {
            std::cout << value;
        }
        return *this;
    }

protected:
    Level _level;
};

}
}

#define bg2e_log_info bg2e::base::Log(bg2e::base::Log::Level::Info)
#define bg2e_log_warning bg2e::base::Log(bg2e::base::Log::Level::Warning)
#define bg2e_log_error bg2e::base::Log(bg2e::base::Log::Level::Error)
#define bg2e_log_debug bg2e::base::Log(bg2e::base::Log::Level::Debug)
#define bg2e_log_end "\n"
