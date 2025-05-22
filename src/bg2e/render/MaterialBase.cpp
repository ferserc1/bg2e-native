
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
    base::Texture * texData,
    std::shared_ptr<Texture>& outTexture,
    bool useCache
) {
    if (outTexture.get())
    {
        outTexture.reset();
    }
    
    if (texData != nullptr && !useCache)
    {
        outTexture = std::make_shared<Texture>(
                vulkan,
                texData
            );
    }
    else if (texData != nullptr)
    {
        if (texData->imageFilePath() == "")
        {
            throw std::runtime_error("TextureCache: could not load cached texture because the source texture data does not contains a file path");
        }
        outTexture = utils::TextureCache::get().load(vulkan, texData->imageFilePath());
    }
}

void MaterialBase::updateTextures()
{
    if (_materialAttributes.isUpdated())
    {
        return;
    }
    
    if (!_materialAttributes.albedoTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.albedoTexture().get(), _albedoTexture, useTextureCache());
    }
    else {
        _albedoTexture = render::Texture::whiteTexture(_vulkan);
    }
    
    if (!_materialAttributes.metalnessTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.metalnessTexture().get(), _metalnessTexture, useTextureCache());
    }
    else {
        _metalnessTexture = render::Texture::whiteTexture(_vulkan);
    }
    
    if (!_materialAttributes.roughnessTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.roughnessTexture().get(), _roughnessTexture, useTextureCache());
    }
    else {
        _roughnessTexture = render::Texture::whiteTexture(_vulkan);
    }
    
    if (!_materialAttributes.normalTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.normalTexture().get(), _normalTexture, useTextureCache());
    }
    else {
        _normalTexture = render::Texture::normalTexture(_vulkan);
    }
    
    if (!_materialAttributes.aoTextureUpdated())
    {
        updateTexture(_vulkan, _materialAttributes.aoTexture().get(), _aoTexture, useTextureCache());
    }
    else {
        _normalTexture = render::Texture::whiteTexture(_vulkan);
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
