
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/utils/TextureCache.hpp>

namespace bg2e::render {

MaterialBase::MaterialBase(Engine * engine)
    :_engine(engine)
{
    _albedoTexture = Texture::whiteTexture(engine);
    _normalTexture = Texture::whiteTexture(engine);
    _roughnessTexture = Texture::whiteTexture(engine);
    _normalTexture = Texture::normalTexture(engine);
    _aoTexture = Texture::whiteTexture(engine);
}

MaterialBase::~MaterialBase()
{
    cleanup();
}

void updateTexture(
    Engine * engine,
    std::shared_ptr<base::Texture> texData,
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
                engine,
                texData
            );
    }
    else if (texData != nullptr)
    {
        if (texData->imageFilePath() == "")
        {
            throw std::runtime_error("TextureCache: could not load cached texture because the source texture data does not contains a file path");
        }
        outTexture = utils::TextureCache::get().load(engine, texData->imageFilePath());
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
        updateTexture(_engine, _materialAttributes.albedoTexture(), _albedoTexture, useTextureCache());
    }
    
    if (!_materialAttributes.metalnessTextureUpdated())
    {
        updateTexture(_engine, _materialAttributes.metalnessTexture(), _metalnessTexture, useTextureCache());
    }
    
    if (!_materialAttributes.roughnessTextureUpdated())
    {
        updateTexture(_engine, _materialAttributes.roughnessTexture(), _roughnessTexture, useTextureCache());
    }
    
    if (!_materialAttributes.normalTextureUpdated())
    {
        updateTexture(_engine, _materialAttributes.normalTexture(), _normalTexture, useTextureCache());
    }
    
    if (!_materialAttributes.aoTextureUpdated())
    {
        updateTexture(_engine, _materialAttributes.aoTexture(), _aoTexture, useTextureCache());
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
