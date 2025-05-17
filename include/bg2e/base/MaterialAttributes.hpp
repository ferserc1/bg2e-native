#pragma once

#include <bg2e/common.hpp>
#include <bg2e/base/Color.hpp>
#include <bg2e/base/Texture.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#define GLM_FORCE_LEFT_HANDED 1
#include <glm/vec2.hpp>

#include <memory>

namespace bg2e {
namespace base {

class BG2E_API MaterialAttributes {
public:
    
    MaterialAttributes();
    
    inline const Color & albedo() const { return _albedo; }
    inline void setAlbedo(const Color & c) { _albedo = c; }
    
    inline std::shared_ptr<Texture> albedoTexture() const { return _albedoTexture; }
    inline void setAlbedo(const std::shared_ptr<Texture>& t) { _albedoTexture = t; }
    
    inline const glm::vec2 & albedoScale() const { return _albedoScale; }
    inline void setAlbedoScale(const glm::vec2& offset) { _albedoScale = offset; }

    inline uint32_t albedoUVSet() const { return _albedoUVSet; }
    inline void setAlbedoUVSet(uint32_t uvSet) { _albedoUVSet = uvSet; }
    
    inline float metalness() const { return _metalness; }
    inline void setMetalness(float m) { _metalness = m; }

    inline std::shared_ptr<Texture> metalnessTexture() const { return _metalnessTexture; }
    inline void setMetalness(const std::shared_ptr<Texture>& t) { _metalnessTexture = t; }
    
    inline const glm::vec2 & metalnessScale() const { return _metalnessScale; }
    inline void setMetalnessScale(const glm::vec2& s) { _metalnessScale = s; }
    
    inline const uint32_t metalnessChannel() const { return _metalnessChannel; }
    inline void setMetalnessChannel(uint32_t c) { _metalnessChannel = c; }

    inline uint32_t metalnessUVSet() const { return _metalnessUVSet; }
    inline void setMetalnessUVSet(uint32_t uvSet) { _metalnessUVSet = uvSet; }
    
    inline float roughness() const { return _roughness; }
    inline void setRoughness(float r) { _roughness = r; }

    inline std::shared_ptr<Texture> roughnessTexture() const { return _roughnessTexture; }
    inline void setRoughness(const std::shared_ptr<Texture>& t) { _roughnessTexture = t; }

    inline const glm::vec2 & roughnessScale() const { return _roughnessScale; }
    inline void setRoughnessScale(const glm::vec2& s) { _roughnessScale = s; }

    inline const uint32_t roughnessChannel() const { return _roughnessChannel; }
    inline void setRoughnessChannel(uint32_t c) { _roughnessChannel = c; }

    inline uint32_t roughnessUVSet() const { return _roughnessUVSet; }
    inline void setRoughnessUVSet(uint32_t uvSet) { _roughnessUVSet = uvSet; }

    inline std::shared_ptr<Texture> normalTexture() const { return _normalTexture; }
    inline void setNormalTexture(const std::shared_ptr<Texture>& t) { _normalTexture = t; }

    inline const glm::vec2 & normalScale() const { return _normalScale; }
    inline void setNormalScale(const glm::vec2& s) { _normalScale = s; }

    inline uint32_t normalUVSet() const { return _normalUVSet; }
    inline void setNormalUVSet(uint32_t uvSet) { _normalUVSet = uvSet; }

    inline std::shared_ptr<Texture> aoTexture() const { return _aoTexture; }
    inline void setAoTexture(const std::shared_ptr<Texture>& t) { _aoTexture = t; }

    inline const glm::vec2 & aoScale() const { return _aoScale; }
    inline void setAoScale(const glm::vec2& s) { _aoScale = s; }

    inline const uint32_t aoChannel() const { return _aoChannel; }
    inline void setAoChannel(uint32_t c) { _aoChannel = c; }

    inline uint32_t aoUVSet() const { return _aoUVSet; }
    inline void setAoUVSet(uint32_t uvSet) { _aoUVSet = uvSet; }
    


protected:
    
    Color _albedo = Color::White();
    std::shared_ptr<Texture> _albedoTexture;
    glm::vec2 _albedoScale{ 1.0f, 1.0f };
    uint32_t _albedoUVSet = 0;
    
    float _metalness = 0.0f;
    std::shared_ptr<Texture> _metalnessTexture;
    glm::vec2 _metalnessScale{ 1.0f, 1.0f };
    uint32_t _metalnessChannel = 0;
    uint32_t _metalnessUVSet = 0;

    float _roughness = 0.0f;
    std::shared_ptr<Texture> _roughnessTexture;
    glm::vec2 _roughnessScale{ 1.0f, 1.0f };
    uint32_t _roughnessChannel = 0;
    uint32_t _roughnessUVSet = 0;

    std::shared_ptr<Texture> _normalTexture;
    glm::vec2 _normalScale{ 1.0f, 1.0f };
    uint32_t _normalUVSet = 0;

    std::shared_ptr<Texture> _aoTexture;
    glm::vec2 _aoScale{ 1.0f, 1.0f };
    uint32_t _aoChannel = 0;
    uint32_t _aoUVSet = 0;
    
};

}
}
