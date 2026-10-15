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

#include <bg2e/draw/Texture.hpp>
#include <bg2e/base/MaterialAttributes.hpp>
#include <array>
#include <functional>

namespace bg2e::draw {
BG2E_API base::MaterialAttributes cloneMaterialAttributes(const base::MaterialAttributes& source);

enum class TextureSlot
{
    Albedo, Metalness, Roughness, Normal, AmbientOcclusion, LightEmission
};

// Value attributes are live material state. GPU textures are stable resources,
// not frame-ring bindings. Cache sharing is explicit and disabled for clones.
class BG2E_API MaterialBase {
  public:
    explicit MaterialBase(Engine* engine);
    virtual ~MaterialBase() = default;
    MaterialBase(const MaterialBase&) = delete;
    MaterialBase& operator=(const MaterialBase&) = delete;
    base::MaterialAttributes& materialAttributes()
    {
        return _attributes;
    }
    const base::MaterialAttributes& materialAttributes() const
    {
        return _attributes;
    }
    void setMaterialAttributes(const base::MaterialAttributes& attributes);
    Engine* engine() const
    {
        return _engine;
    }
    void updateTextures(std::function<void()> onTextureLoaded = nullptr);
    void setTexture(TextureSlot slot, std::shared_ptr<Texture> texture);
    std::shared_ptr<Texture> texture(TextureSlot slot) const
    {
        return _textures.at(size_t(slot));
    }
    std::shared_ptr<Texture> albedoTexture() const
    {
        return texture(TextureSlot::Albedo);
    }
    std::shared_ptr<Texture> metalnessTexture() const
    {
        return texture(TextureSlot::Metalness);
    }
    std::shared_ptr<Texture> roughnessTexture() const
    {
        return texture(TextureSlot::Roughness);
    }
    std::shared_ptr<Texture> normalTexture() const
    {
        return texture(TextureSlot::Normal);
    }
    std::shared_ptr<Texture> aoTexture() const
    {
        return texture(TextureSlot::AmbientOcclusion);
    }
    std::shared_ptr<Texture> lightEmissionTexture() const
    {
        return texture(TextureSlot::LightEmission);
    }
    bool useTextureCache() const
    {
        return _useTextureCache;
    }
    void setUseTextureCache(bool value)
    {
        _useTextureCache = value;
    }
    virtual std::shared_ptr<MaterialBase> clone() const;

  private:
    Engine* _engine;
    base::MaterialAttributes _attributes;
    std::array<std::shared_ptr<Texture>, 6> _textures;
    bool _useTextureCache = false;
};
}
