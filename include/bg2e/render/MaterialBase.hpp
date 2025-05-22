#pragma once

#include <bg2e/base/MaterialAttributes.hpp>
#include <bg2e/render/Texture.hpp>

namespace bg2e {
namespace render {

class BG2E_API MaterialBase {
public:
    MaterialBase(Vulkan *);
    virtual ~MaterialBase();

    void updateTextures();

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
    Vulkan * _vulkan;
    
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
