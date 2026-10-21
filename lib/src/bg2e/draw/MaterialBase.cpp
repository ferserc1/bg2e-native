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

#include <bg2e/draw/MaterialBase.hpp>
#include <bg2e/utils/TextureCache.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/base/CpuProfile.hpp>

#include <stdexcept>
#include <utility>

namespace bg2e::draw {
namespace {
using Getter = std::shared_ptr<base::Texture> (base::MaterialAttributes::*)() const;
using Setter = void (base::MaterialAttributes::*)(std::shared_ptr<base::Texture>);
using Updated = bool (base::MaterialAttributes::*)() const;
const std::array<Getter, 6> getters = {&base::MaterialAttributes::albedoTexture, &base::MaterialAttributes::metalnessTexture,
    &base::MaterialAttributes::roughnessTexture, &base::MaterialAttributes::normalTexture,
    &base::MaterialAttributes::aoTexture, &base::MaterialAttributes::lightEmissionTexture};
const std::array<Setter, 6> setters = {&base::MaterialAttributes::setAlbedoTexture, &base::MaterialAttributes::setMetalnessTexture,
    &base::MaterialAttributes::setRoughnessTexture, &base::MaterialAttributes::setNormalTexture,
    &base::MaterialAttributes::setAoTexture, &base::MaterialAttributes::setLightEmissionTexture};
const std::array<Updated, 6> updated = {&base::MaterialAttributes::albedoTextureUpdated, &base::MaterialAttributes::metalnessTextureUpdated,
    &base::MaterialAttributes::roughnessTextureUpdated, &base::MaterialAttributes::normalTextureUpdated,
    &base::MaterialAttributes::aoTextureUpdated, &base::MaterialAttributes::lightEmissionTextureUpdated};
}

base::MaterialAttributes cloneMaterialAttributes(const base::MaterialAttributes& source)
{
    auto attributes = source;
    for (size_t i = 0; i < getters.size(); ++i)
    {
        const auto texture = (source.*getters[i])();
        (attributes.*setters[i])(texture ? cloneTextureSettings(*texture) : nullptr);
    }
    return attributes;
}

MaterialBase::MaterialBase(Engine* engine) : _engine(engine)
{
    if (!engine)
    {
        throw std::invalid_argument("draw::MaterialBase requires an engine");
    }
    engine->device();
}

void MaterialBase::setMaterialAttributes(const base::MaterialAttributes& attributes)
{
    const auto previous = _attributes;
    _attributes = attributes;
    for (size_t i = 0; i < getters.size(); ++i)
    {
        if ((previous.*getters[i])() != (_attributes.*getters[i])())
        {
            (_attributes.*setters[i])((_attributes.*getters[i])());
        }
    }
}

void MaterialBase::updateTextures(std::function<void()> onTextureLoaded)
{
    // No native owner changes and no upload/callback work for realized revisions.
    if (texturesUpdated())
    {
        _attributes.setUpdated();
        base::CpuProfileScope::count("material.realizationNoops", 1);
        return;
    }
    auto textures = _textures;
    auto settingsOwners = _realizedSettings;
    auto settingsRevisions = _realizedSettingsRevisions;
    for (size_t i = 0; i < textures.size(); ++i)
    {
        const auto settings = (_attributes.*getters[i])();
        const auto settingsRevision = settings ? settings->revision() : 0;
        if (textures[i] && settings == settingsOwners[i] &&
            settingsRevision == settingsRevisions[i])
        {
            continue;
        }
        if (!settings)
        {
            textures[i] = Texture::colorTexture(_engine, i == size_t(TextureSlot::Normal)
                ? base::Color(0.5f, 0.5f, 1.0f, 1.0f) : base::Color::White());
        }
        else
        {
            textures[i] = _useTextureCache && settings->type() == base::Texture::TypeFilesystem
                ? utils::TextureCache::get().load(_engine, *settings) : std::make_shared<Texture>(_engine, settings);
            if (onTextureLoaded)
            {
                onTextureLoaded();
            }
        }
        settingsOwners[i] = settings;
        settingsRevisions[i] = settingsRevision;
    }
    {
        base::CpuProfileScope profile("material.waitIdle.updateTextures");
        base::CpuProfileScope::count("material.drains.updateTextures", 1, "drains");
        _engine->device()->waitIdle();
    }
    _textures = std::move(textures);
    _realizedSettings = std::move(settingsOwners);
    _realizedSettingsRevisions = settingsRevisions;
    _attributes.setUpdated();
}

bool MaterialBase::texturesUpdated() const
{
    // Legacy pending flags are preserved on the attributes. Draw compares actual
    // source revisions so assigning the same settings does not force a reload.
    for (size_t i = 0; i < getters.size(); ++i)
    {
        const auto settings = (_attributes.*getters[i])();
        if (!_textures[i] || settings != _realizedSettings[i] ||
            (settings && settings->revision() != _realizedSettingsRevisions[i]))
        {
            return false;
        }
    }
    return true;
}

void MaterialBase::setTexture(TextureSlot slot, std::shared_ptr<Texture> texture)
{
    if (!texture || texture->engine() != _engine || !texture->image() || texture->image()->device() != _engine->device())
    {
        throw std::invalid_argument("Material texture belongs to an incompatible context");
    }
    const auto index = size_t(slot);
    auto settings = cloneTextureSettings(*texture->base());
    {
        base::CpuProfileScope profile("material.waitIdle.setTexture");
        base::CpuProfileScope::count("material.drains.setTexture", 1, "drains");
        _engine->device()->waitIdle();
    }
    // Keep the supplied realization, including GPU-authored mip levels. The
    // settings value records its sampler/source, not a second realization.
    (_attributes.*setters.at(index))(std::move(settings));
    _textures.at(index) = std::move(texture);
    _realizedSettings[index] = (_attributes.*getters[index])();
    _realizedSettingsRevisions[index] = _realizedSettings[index]->revision();
    // Only the selected slot is realized; do not mask other pending edits.
    auto pending = _attributes;
    _attributes.setUpdated();
    for (size_t i = 0; i < getters.size(); ++i)
    {
        if (i != index && !(pending.*updated[i])())
        {
            (_attributes.*setters[i])((pending.*getters[i])());
        }
    }
}

std::shared_ptr<MaterialBase> MaterialBase::clone() const
{
    {
        base::CpuProfileScope profile("material.waitIdle.clone");
        base::CpuProfileScope::count("material.drains.clone", 1, "drains");
        _engine->device()->waitIdle();
    }
    auto result = std::make_shared<MaterialBase>(_engine);
    result->_attributes = cloneMaterialAttributes(_attributes);
    for (size_t i = 0; i < _textures.size(); ++i)
    {
        if (_textures[i])
        {
            result->_textures[i] = _textures[i]->clone();
            result->_realizedSettings[i] = (result->_attributes.*getters[i])();
            result->_realizedSettingsRevisions[i] = result->_realizedSettings[i] ? result->_realizedSettings[i]->revision() : 0;
        }
    }
    // Preserve uncommitted source changes for the next explicit update.
    result->_attributes.setUpdated();
    for (size_t i = 0; i < getters.size(); ++i)
    {
        const auto settings = (_attributes.*getters[i])();
        if (!(_attributes.*updated[i])())
        {
            (result->_attributes.*setters[i])((result->_attributes.*getters[i])());
        }
        if (settings != _realizedSettings[i] ||
            (settings && settings->revision() != _realizedSettingsRevisions[i]))
        {
            (result->_attributes.*setters[i])((result->_attributes.*getters[i])());
            result->_realizedSettings[i] = _realizedSettings[i];
            result->_realizedSettingsRevisions[i] = _realizedSettingsRevisions[i];
        }
    }
    return result;
}
}
