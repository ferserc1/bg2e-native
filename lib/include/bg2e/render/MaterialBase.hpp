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

#include <bg2e/base/MaterialAttributes.hpp>
#include <bg2e/render/Texture.hpp>

#include <functional>

namespace bg2e {
namespace render {

class BG2E_API MaterialBase {
public:
    MaterialBase(Engine *);
    virtual ~MaterialBase();

    void updateTextures();
    void updateTextures(std::function<void()> onTextureLoaded);

    inline void setMaterialAttributes(const base::MaterialAttributes& att) { _materialAttributes = att; }
    inline base::MaterialAttributes& materialAttributes() { return _materialAttributes; }
    inline const base::MaterialAttributes& materialAttributes() const { return _materialAttributes; }
    
    inline std::shared_ptr<Texture> albedoTexture() { return _albedoTexture; }
    inline std::shared_ptr<Texture> metalnessTexture() { return _metalnessTexture; }
    inline std::shared_ptr<Texture> roughnessTexture() { return _roughnessTexture; }
    inline std::shared_ptr<Texture> normalTexture() { return _normalTexture; }
    inline std::shared_ptr<Texture> aoTexture() { return _aoTexture; }
    
    inline bool useTextureCache() const { return _useTextureCache; }
    inline void setUseTextureCache(bool tc) { _useTextureCache = tc; }

protected:
    Engine * _engine;
    
    base::MaterialAttributes _materialAttributes;
    
    std::shared_ptr<Texture> _albedoTexture;
    std::shared_ptr<Texture> _metalnessTexture;
    std::shared_ptr<Texture> _roughnessTexture;
    std::shared_ptr<Texture> _normalTexture;
    std::shared_ptr<Texture> _aoTexture;
    
    bool _useTextureCache = false;
    
    // Called by destructor
    void cleanup();
};

}
}
