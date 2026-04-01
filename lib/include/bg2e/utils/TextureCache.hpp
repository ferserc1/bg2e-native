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

#include <bg2e/render/Texture.hpp>
#include <bg2e/base/Texture.hpp>
#include <bg2e/render/Engine.hpp>

namespace bg2e {
namespace utils {

class BG2E_API TextureCache {
public:

    static TextureCache& get();
    static void destroy();

    // Load and catalogues a texture with default settings, unless it is already catalogued, in which case it returns the cached version
    std::shared_ptr<render::Texture> load(render::Engine *, const std::filesystem::path& filePath);
    
    std::shared_ptr<render::Texture> load(render::Engine *, const std::filesystem::path& filePath, const std::string& fileName);
    
    std::shared_ptr<render::Texture> load(render::Engine *, const std::string& filePath);
    
    // Load and catalogues a texture using the supplied Texture object as texture settings. In this case, the Texture object is only used to
    // configure the resulting Texture. The texture image is loaded from the filePath
    std::shared_ptr<render::Texture> load(render::Engine *, const base::Texture& settings);
        
    // Remove all objects from the cache
    void emptyCache();
    
protected:
    TextureCache() = default;
    virtual ~TextureCache() = default;
    
    static TextureCache * g_singleton;
    
    std::unordered_map<std::string, std::shared_ptr<render::Texture>> _textures;
};

}
}
