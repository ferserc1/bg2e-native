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

#include <bg2e/draw/ShaderData.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
void verifyMaterialRevisions(const bg2e::draw::MaterialBase& source)
{
    using namespace bg2e;
    const auto require = [](bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::logic_error(message);
        }
    };
    const auto equal = [](draw::MaterialRevisions a, draw::MaterialRevisions b)
    {
        return a.values == b.values && a.bindings == b.bindings && a.classification == b.classification;
    };
    base::MaterialAttributes defaults;
    require(defaults.alphaCutoff() == 0, "Default cutoff must disable alpha test");
    const auto sourceRevision = source.revisions();
    auto material = source.clone();
    auto& attributes = material->materialAttributes();
    const auto initial = attributes;
    auto previous = material->revisions();
    const auto authored = attributes.revision();
    attributes.setRoughness(attributes.roughness());
    attributes.setAlbedo(attributes.albedo());
    attributes.setAlbedoScale(attributes.albedoScale());
    attributes = attributes;
    require(authored == attributes.revision() && equal(previous, material->revisions()), "Unchanged setters must preserve revisions");
    attributes.setAlbedoTexture(attributes.albedoTexture());
    material->updateTextures();
    require(equal(previous, material->revisions()), "Assigning unchanged texture settings must not replace GPU bindings");

    attributes.setAlphaCutoff(0.001f);
    previous = material->revisions();
    const auto beforeSmallEdit = attributes.revision();
    attributes.setAlphaCutoff(0.005f);
    require(attributes.revision() > beforeSmallEdit && attributes.alphaCutoff() == 0.005f, "Authored subthreshold edit must survive");
    require(equal(previous, material->revisions()), "Subthreshold edits must preserve effective GPU revisions");
    require(draw::packMaterial(*material).emissionAlpha.y == 0, "Disabled cutoff must pack zero");
    attributes.setAlphaCutoff(draw::AlphaTestCutoffThreshold);
    auto changed = material->revisions();
    require(changed.values > previous.values && changed.classification > previous.classification &&
        changed.bindings == previous.bindings, "Threshold transition must change values/classification only");

    for (float invalid : {-1.0f, 2.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
    {
        attributes.setAlphaCutoff(invalid);
        bool rejected = false;
        try
        {
            draw::packMaterial(*material);
        }
        catch (const std::invalid_argument&)
        {
            rejected = true;
        }
        require(rejected, "Invalid edit must not be hidden by the material cache");
    }
    attributes.setAlphaCutoff(draw::AlphaTestCutoffThreshold);
    require(equal(changed, material->revisions()), "Rejected candidates must not publish revisions");

    attributes = initial;
    previous = material->revisions();
    material->setMaterialAttributes(initial);
    require(equal(previous, material->revisions()), "Equal whole-attribute replacement must preserve effective revisions");
    auto replacement = initial;
    replacement.setIsTransparent(!initial.isTransparent());
    attributes = replacement;
    changed = material->revisions();
    require(changed.values > previous.values && changed.classification > previous.classification &&
        changed.bindings == previous.bindings, "Reference assignment must invalidate classification");
    attributes = initial;
    previous = material->revisions();

    auto texture = material->albedoTexture();
    const auto binding = texture->bindingRevision();
    texture->updateSampler(*texture->base());
    require(texture->bindingRevision() == binding, "Unchanged sampler assignment must preserve bindings");
    auto samplerSettings = *texture->base();
    samplerSettings.setMagFilter(samplerSettings.magFilter() == base::Texture::FilterNearest ? base::Texture::FilterLinear : base::Texture::FilterNearest);
    texture->updateSampler(samplerSettings);
    changed = material->revisions();
    require(changed.bindings > previous.bindings && changed.values == previous.values, "Sampler replacement must invalidate bindings only");
    material->setTexture(draw::TextureSlot::Albedo, texture);
    require(equal(changed, material->revisions()), "Reassigning the same realization must preserve effective revisions");

    // Mutating referenced source settings must require realization even though
    // MaterialAttributes' pending flags and pointer identities did not change.
    auto sourceSettings = attributes.albedoTexture();
    require(bool(sourceSettings), "Use the textured canopy for revision checks");
    const auto settingsRevision = sourceSettings->revision();
    *sourceSettings = *sourceSettings;
    sourceSettings->setMagFilter(sourceSettings->magFilter());
    require(sourceSettings->revision() == settingsRevision, "Equal texture settings must preserve source revisions");
    sourceSettings->setMagFilter(sourceSettings->magFilter() == base::Texture::FilterNearest ? base::Texture::FilterLinear : base::Texture::FilterNearest);
    require(!material->texturesUpdated(), "Referenced settings edits must invalidate realization");
    bool pendingRejected = false;
    try
    {
        draw::packMaterial(*material);
    }
    catch (const std::logic_error&)
    {
        pendingRejected = true;
    }
    require(pendingRejected, "Pending settings must not reuse stale packed data");
    material->updateTextures();
    require(material->texturesUpdated() && material->revisions().bindings > changed.bindings, "Realization must publish new bindings");
    auto* generator = sourceSettings->proceduralTextureGenerator();
    require(generator != nullptr, "Use a procedural canopy texture for generator checks");
    const auto generatorRevision = generator->revision();
    generator->setDimensions(generator->width(), generator->height(), generator->bytesPerPixel());
    require(generator->revision() == generatorRevision && material->texturesUpdated(), "Equal generator dimensions must preserve realization");
    generator->markModified();
    require(!material->texturesUpdated(), "Generator content edits must invalidate realization");
    auto pendingClone = material->clone();
    require(!pendingClone->texturesUpdated(), "Cloning must preserve unrealized source changes");
    pendingClone->updateTextures();
    require(pendingClone->texturesUpdated() && !material->texturesUpdated(), "Clone realization must not consume source changes");
    material->updateTextures();
    require(equal(sourceRevision, source.revisions()), "Clone edits must not affect source revisions");

    previous = material->revisions();
    material->setMaterialCacheEnabled(false);
    require(equal(previous, material->revisions()), "Diagnostic repacking must preserve effective revisions");
    material->setMaterialCacheEnabled(true);
    require(equal(previous, material->revisions()), "Multiple consumers must observe the same revisions");
}
}
