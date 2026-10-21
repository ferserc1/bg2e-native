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

#include <memory>
#include <unordered_map>
#include <filesystem>

#include <bg2e/base/Texture.hpp>
#include <bg2e/draw/Texture.hpp>

namespace bg2e {
namespace utils {

class BG2E_API TextureCache {
  public:
    static TextureCache& get();
    static void destroy();

    // Reuse weak realizations only within their original live Engine/device.
    std::shared_ptr<draw::Texture> load(draw::Engine*, const std::filesystem::path& filePath);
    std::shared_ptr<draw::Texture> load(draw::Engine*, const std::filesystem::path& filePath, const std::string& fileName);
    std::shared_ptr<draw::Texture> load(draw::Engine*, const std::string& filePath);
    std::shared_ptr<draw::Texture> load(draw::Engine*, const base::Texture& settings);

    // Remove all objects from the cache
    void emptyCache();

  protected:
    TextureCache() = default;
    virtual ~TextureCache() = default;

    static TextureCache* g_singleton;

    // Weak draw entries: Engine/device must outlive every external realization.
    std::unordered_map<std::string, std::weak_ptr<draw::Texture>> _drawTextures;
};

}
}
