
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/utils/TextureCache.hpp>

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
    std::shared_ptr<Texture>& outTexture,
    bool useCache
) {
    if (outTexture.get())
    {
        outTexture.reset();
    }
    
    if (texData.get() != nullptr && !useCache)
    {
        outTexture = std::make_shared<Texture>(
                vulkan,
                texData
            );
    }
    else if (texData.get() != nullptr)
    {
        if (texData->imageFilePath() == "")
        {
            throw std::runtime_error("TextureCache: could not load cached texture because the source texture data does not contains a file path");
        }
        outTexture = utils::TextureCache::get().load(vulkan, texData->imageFilePath());
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
        updateTexture(_vulkan, _materialAttributes.albedoTexture(), _albedoTexture, useTextureCache());
    }
    
    if (!_materialAttributes.metalnessTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.metalnessTexture(), _metalnessTexture, useTextureCache());
    }
    
    if (!_materialAttributes.roughnessTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.roughnessTexture(), _roughnessTexture, useTextureCache());
    }
    
    if (!_materialAttributes.normalTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.normalTexture(), _normalTexture, useTextureCache());
    }
    
    if (!_materialAttributes.aoTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.aoTexture(), _aoTexture, useTextureCache());
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
