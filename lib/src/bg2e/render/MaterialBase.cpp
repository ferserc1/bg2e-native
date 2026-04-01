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

#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/utils/TextureCache.hpp>

namespace bg2e::render {

MaterialBase::MaterialBase(Engine * engine)
    :_engine(engine)
{
    _albedoTexture = Texture::whiteTexture(engine);
    _normalTexture = Texture::whiteTexture(engine);
    _metalnessTexture = Texture::whiteTexture(engine);
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
    bool useCache,
    std::shared_ptr<Texture> fallbackTexture
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
    else {
        outTexture = fallbackTexture;
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
        updateTexture(_engine, _materialAttributes.albedoTexture(), _albedoTexture, useTextureCache(), Texture::whiteTexture(_engine));
    }
    
    if (!_materialAttributes.metalnessTextureUpdated())
    {
        updateTexture(_engine, _materialAttributes.metalnessTexture(), _metalnessTexture, useTextureCache(), Texture::whiteTexture(_engine));
    }
    
    if (!_materialAttributes.roughnessTextureUpdated())
    {
        updateTexture(_engine, _materialAttributes.roughnessTexture(), _roughnessTexture, useTextureCache(), Texture::whiteTexture(_engine));
    }
    
    if (!_materialAttributes.normalTextureUpdated())
    {
        updateTexture(_engine, _materialAttributes.normalTexture(), _normalTexture, useTextureCache(), Texture::normalTexture(_engine));
    }
    
    if (!_materialAttributes.aoTextureUpdated())
    {
        updateTexture(_engine, _materialAttributes.aoTexture(), _aoTexture, useTextureCache(), Texture::whiteTexture(_engine));
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
