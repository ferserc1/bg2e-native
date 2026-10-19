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

#include <bg2e/gpu/Image.hpp>
#include <filesystem>
#include <vector>

namespace bg2e::db {
// CPU representation of native-format color texels, ordered layer then mip.
// Version 1 preserves every authored mip/layer; no gamma or HDR conversion.
struct TextureArchive {
    gpu::ImageDescription description;
    std::vector<std::vector<uint8_t>> subresources;
};

BG2E_API TextureArchive readTextureArchive(const std::filesystem::path& path);
BG2E_API void writeTextureArchive(const std::filesystem::path& path, gpu::Image& image);

}
