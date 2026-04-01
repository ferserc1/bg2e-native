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
#include <bg2e/base/Color.hpp>
#include <bg2e/base/Texture.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#define GLM_FORCE_LEFT_HANDED 1
#include <glm/vec2.hpp>

#include <memory>
#include <array>

namespace bg2e {
namespace base {

class BG2E_API MaterialAttributes {
public:
    
    MaterialAttributes();
    
    inline bool isTransparent() const { return _isTransparent; }
    inline void setIsTransparent(bool t) { _isTransparent = t; }
    
    inline bool isSolid() const { return _isSolid; }
    inline void setIsSolid(bool s) { _isSolid = s; }
    
    inline const Color & albedo() const { return _albedo; }
    inline void setAlbedo(const Color & c) { _albedo = c; }
    inline void setAlbedo(const std::array<float, 3>& s) { _albedo = base::Color{ s.at(0), s.at(1), s.at(2), 1.0f }; }
    inline void setAlbedo(const std::array<float, 4>& s) { _albedo = base::Color{ s.at(0), s.at(1), s.at(2), s.at(3)}; }
    
    inline std::shared_ptr<Texture> albedoTexture() const { return _albedoTexture; }
    inline void setAlbedoTexture(Texture * t) { setAlbedoTexture(std::shared_ptr<Texture>(t)); }
    inline void setAlbedoTexture(std::shared_ptr<Texture> t) { _albedoTextureUpdated = false; _albedoTexture = t; }
    inline bool albedoTextureUpdated() const { return _albedoTextureUpdated; }
    
    inline const glm::vec2 & albedoScale() const { return _albedoScale; }
    inline void setAlbedoScale(const glm::vec2& offset) { _albedoScale = offset; }
    inline void setAlbedoScale(const std::array<float, 2>& s) { _albedoScale = {s[0], s[1]}; }

    inline uint32_t albedoUVSet() const { return _albedoUVSet; }
    inline void setAlbedoUVSet(uint32_t uvSet) { _albedoUVSet = uvSet; }
    
    inline float metalness() const { return _metalness; }
    inline void setMetalness(float m) { _metalness = m; }

    inline std::shared_ptr<Texture> metalnessTexture() const { return _metalnessTexture; }
    inline void setMetalnessTexture(Texture * t) { setMetalnessTexture(std::shared_ptr<Texture>(t)); }
    inline void setMetalnessTexture(std::shared_ptr<Texture> t) { _metalnessTextureUpdated = false; _metalnessTexture = t; }
    inline bool metalnessTextureUpdated() const { return _metalnessTextureUpdated; }
    
    inline const glm::vec2 & metalnessScale() const { return _metalnessScale; }
    inline void setMetalnessScale(const glm::vec2& s) { _metalnessScale = s; }
    inline void setMetalnessScale(const std::array<float, 2>& s) { _metalnessScale = {s[0], s[1]}; }
    
    inline uint32_t metalnessChannel() const { return _metalnessChannel; }
    inline void setMetalnessChannel(uint32_t c) { _metalnessChannel = c; }

    inline uint32_t metalnessUVSet() const { return _metalnessUVSet; }
    inline void setMetalnessUVSet(uint32_t uvSet) { _metalnessUVSet = uvSet; }
    
    inline float roughness() const { return _roughness; }
    inline void setRoughness(float r) { _roughness = r; }

    inline std::shared_ptr<Texture> roughnessTexture() const { return _roughnessTexture; }
    inline void setRoughnessTexture(Texture * t) { setRoughnessTexture(std::shared_ptr<Texture>(t)); }
    inline void setRoughnessTexture(std::shared_ptr<Texture> t) { _roughnessTextureUpdated = false; _roughnessTexture = t; }
    inline bool roughnessTextureUpdated() const { return _roughnessTextureUpdated; }

    inline const glm::vec2 & roughnessScale() const { return _roughnessScale; }
    inline void setRoughnessScale(const glm::vec2& s) { _roughnessScale = s; }
    inline void setRoughnessScale(const std::array<float, 2>& s) { _roughnessScale = {s[0], s[1]}; }

    inline uint32_t roughnessChannel() const { return _roughnessChannel; }
    inline void setRoughnessChannel(uint32_t c) { _roughnessChannel = c; }

    inline uint32_t roughnessUVSet() const { return _roughnessUVSet; }
    inline void setRoughnessUVSet(uint32_t uvSet) { _roughnessUVSet = uvSet; }

    inline std::shared_ptr<Texture> normalTexture() const { return _normalTexture; }
    inline void setNormalTexture(Texture * t) { setNormalTexture(std::shared_ptr<Texture>(t)); }
    inline void setNormalTexture(std::shared_ptr<Texture> t) { _normalTextureUpdated = false; _normalTexture = t; }
    inline bool normalTextureUpdated() const { return _normalTextureUpdated; }

    inline const glm::vec2 & normalScale() const { return _normalScale; }
    inline void setNormalScale(const glm::vec2& s) { _normalScale = s; }
    inline void setNormalScale(const std::array<float, 2>& s) { _normalScale = {s[0], s[1]}; }

    inline uint32_t normalUVSet() const { return _normalUVSet; }
    inline void setNormalUVSet(uint32_t uvSet) { _normalUVSet = uvSet; }
    
    inline const Color & fresnelTint() const { return _fresnelTint; }
    inline void setFresnelTint(const Color& c) { _fresnelTint = c; }
    
    inline float sheenIntensity() const { return _sheenIntensity; }
    inline void setSheenIntensity(float s) { _sheenIntensity = s; }
    
    inline const Color & sheenColor() const { return _sheenColor; }
    inline void setSheenColor(const Color & color) { _sheenColor = color; }

    inline std::shared_ptr<Texture> aoTexture() const { return _aoTexture; }
    inline void setAoTexture(Texture * t) { setAoTexture(std::shared_ptr<Texture>(t)); }
    inline void setAoTexture(std::shared_ptr<Texture> t) { _aoTextureUpdated = false; _aoTexture = t; }
    inline bool aoTextureUpdated() const { return _aoTextureUpdated; }

    inline const glm::vec2 & aoScale() const { return _aoScale; }
    inline void setAoScale(const glm::vec2& s) { _aoScale = s; }
    inline void setAoScale(const std::array<float, 2>& s) { _aoScale = {s[0], s[1]}; }

    inline uint32_t aoChannel() const { return _aoChannel; }
    inline void setAoChannel(uint32_t c) { _aoChannel = c; }

    inline uint32_t aoUVSet() const { return _aoUVSet; }
    inline void setAoUVSet(uint32_t uvSet) { _aoUVSet = uvSet; }
    
    void setUpdated();
    
    inline bool isUpdated() const
    {
        return  _albedoTextureUpdated &&
                _metalnessTextureUpdated &&
                _roughnessTextureUpdated &&
                _normalTextureUpdated &&
                _aoTextureUpdated;
    }
    
    // Metadata: in a bg2 file, Materials store some data from their associated submesh.
    // This is metadata: it does not belong to the material, but to the submesh. It is
    // stored in materialAttributes for serialisation and deserialisation to bg2 files.
    inline const std::string& name() const { return _name; }
    inline void setName(const std::string& name) { _name = name; }
    
    inline const std::string& groupName() const { return _groupName; }
    inline void setGroupName(const std::string& groupName) { _groupName = groupName; }
    
    inline bool visible() const { return _visible; }
    inline void setVisible(bool v) { _visible = v; }

protected:
    
    bool _isTransparent = false;
    bool _isSolid = true;
    
    Color _albedo = Color::White();
    std::shared_ptr<Texture> _albedoTexture;
    glm::vec2 _albedoScale{ 1.0f, 1.0f };
    uint32_t _albedoUVSet = 0;
    bool _albedoTextureUpdated = true;
    
    float _metalness = 0.0f;
    std::shared_ptr<Texture> _metalnessTexture;
    glm::vec2 _metalnessScale{ 1.0f, 1.0f };
    uint32_t _metalnessChannel = 0;
    uint32_t _metalnessUVSet = 0;
    bool _metalnessTextureUpdated = true;

    float _roughness = 0.9f;
    std::shared_ptr<Texture> _roughnessTexture;
    glm::vec2 _roughnessScale{ 1.0f, 1.0f };
    uint32_t _roughnessChannel = 0;
    uint32_t _roughnessUVSet = 0;
    bool _roughnessTextureUpdated = true;

    std::shared_ptr<Texture> _normalTexture;
    glm::vec2 _normalScale{ 1.0f, 1.0f };
    uint32_t _normalUVSet = 0;
    bool _normalTextureUpdated = true;
    
    Color _fresnelTint = Color::White();

    float _sheenIntensity = 0.0f;
    Color _sheenColor = Color::White();
    
    std::shared_ptr<Texture> _aoTexture;
    glm::vec2 _aoScale{ 1.0f, 1.0f };
    uint32_t _aoChannel = 0;
    uint32_t _aoUVSet = 0;
    bool _aoTextureUpdated = true;
    
    // Material metadata
    std::string _name = "";
    std::string _groupName = "";
    bool _visible = true;
};

}
}
