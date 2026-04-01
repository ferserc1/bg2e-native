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

#include <bg2e/common.hpp>
#include <bg2e/base/Image.hpp>
#include <bg2e/base/Texture.hpp>

#include <filesystem>

namespace bg2e {
namespace db {

extern BG2E_API bg2e::base::Image * loadImage(const std::filesystem::path& filePath);

extern BG2E_API bg2e::base::Image * loadImage(const std::filesystem::path& basePath, const std::string& fileName);

extern BG2E_API void saveImage(
    const std::filesystem::path& filePath,
    const uint8_t* data,
    uint32_t width,
    uint32_t height,
    uint32_t bpp
);

extern BG2E_API void saveImage(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    const uint8_t* data,
    uint32_t width,
    uint32_t height,
    uint32_t bpp
);

}
}
