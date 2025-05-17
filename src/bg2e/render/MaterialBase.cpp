
#include <bg2e/render/MaterialBase.hpp>

namespace bg2e::render {

MaterialBase::MaterialBase(Vulkan * vulkan)
    :_vulkan(vulkan)
{
    
}

MaterialBase::~MaterialBase()
{
    cleanup();
}

void updateTexture(
    Vulkan * vulkan,
    const std::shared_ptr<base::Texture>& texData,
    std::shared_ptr<Texture>& outTexture
) {
    if (outTexture.get())
    {
        outTexture.reset();
    }
    
    if (texData.get() != nullptr)
    {
        outTexture = std::make_shared<Texture>(
            vulkan,
            texData
        );
    }
}

void MaterialBase::update()
{
    if (_materialAttributes.isUpdated())
    {
        return;
    }
    
    if (!_materialAttributes.albedoTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.albedoTexture(), _albedoTexture);
    }
    
    if (!_materialAttributes.metalnessTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.metalnessTexture(), _metalnessTexture);
    }
    
    if (!_materialAttributes.roughnessTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.roughnessTexture(), _roughnessTexture);
    }
    
    if (!_materialAttributes.normalTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.normalTexture(), _normalTexture);
    }
    
    if (!_materialAttributes.aoTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.aoTexture(), _aoTexture);
    }
    
    _materialAttributes.setUpdated();
}

void MaterialBase::cleanup()
{
    _albedoTexture.reset();
    _metalnessTexture.reset();
    _roughnessTexture.reset();
    _normalTexture.reset();
    _aoTexture.reset();
}

}
