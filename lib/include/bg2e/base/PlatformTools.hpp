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

#include <string>
#include <filesystem>

#include <bg2e/common.hpp>

#ifdef __APPLE__

#define BG2E_IS_MAC 1

static const bool is_mac = true;
static const bool is_windows = false;
static const bool is_linux = false;

#elif defined(_WIN32)

#define BG2E_IS_WINDOWS 1

static const bool is_mac = false;
static const bool is_windows = true;
static const bool is_linux = false;

#else

#define BG2E_IS_LINUX 1

static const bool is_mac = false;
static const bool is_windows = false;
static const bool is_linux = true;

#endif

namespace bg2e {
namespace base {

enum class Platform { macOS, Windows, Linux };

class BG2E_API PlatformTools {
public:
    static Platform currentPlatform();
    static std::filesystem::path shaderPath();
    static std::filesystem::path assetPath();
    static std::filesystem::path settingsPath();
    static std::filesystem::path homePath();
};

}
}
