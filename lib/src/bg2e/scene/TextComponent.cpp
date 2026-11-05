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

#include <bg2e/scene/TextComponent.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/draw/ShaderData.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/utils/MaterialSerializer.hpp>
#include "../base/TextDiagnostics.hpp"
#include <algorithm>
#include <cmath>
#include <functional>

namespace bg2e::scene {
namespace {
base::MaterialAttributes copyMaterial(const base::MaterialAttributes& source)
{
    auto value = source;
    const auto albedo = value.albedoTexture();
    value.setAlbedoTexture(nullptr);
    value = draw::cloneMaterialAttributes(value);
    // Albedo is sampling metadata here: never evaluate a source that the text
    // owner will replace, including an incoming pasted procedural generator.
    value.setAlbedoTexture(albedo ? std::make_shared<base::Texture>(*albedo) : nullptr);
    return value;
}


float worldDimension(float value, float fallback)
{
    if (!std::isfinite(value) || value <= 0)
    {
        base::detail::textWarning("Invalid text world dimension; using default.");
        return fallback;
    }
    if (value < 0.001f || value > 1000000)
    {
        base::detail::textWarning("Text world dimension clamped to [0.001,1000000].");
    }
    return std::clamp(value, 0.001f, 1000000.0f);
}
}


TextComponent::TextComponent()
{
    _fallbackMaterial.setDoubleSided(true);
    _fallbackMaterial.setAlphaCutoff(0.1f);
    try
    {
        _generator = std::make_shared<base::TextTextureGenerator>(_text, nullptr);
        auto source = base::createTextTexture(_generator);
        if (source)
        {
            source->setSourceEditable(false);
            source->setIgnoreSerialization(true);
            _fallbackMaterial.setAlbedoTexture(source);
        }
        _drawable = std::make_shared<draw::Drawable>();
        _drawable->setMesh(makeQuad());
        _drawable->setName("Text");
        _drawable->setSubmeshName("Text");
        _drawable->materialDefinition() = _fallbackMaterial;
    }
    catch (...)
    {
        _drawable.reset();
        base::detail::textWarning("Cannot allocate initial text quad; text remains editable but unavailable.");
    }
}


bool TextComponent::requiresBg2Export() const
{
    return false;
}


size_t TextComponent::retainedCpuBytes() const noexcept
{
    size_t bytes = sizeof(*this) + _text.content().capacity() + _text.fontIdentifier().capacity();
    if (_generator)
    {
        bytes += _generator->retainedBytes();
    }
    if (_font && (!_generator || _font != _generator->font()))
    {
        bytes += _font->byteSize();
    }
    return bytes;
}


base::Text& TextComponent::text()
{
    return _text;
}


const base::Text& TextComponent::text() const
{
    return _text;
}


void TextComponent::setText(const base::Text& value) noexcept
{
    _text = value;
}


glm::vec2 TextComponent::size() const noexcept
{
    auto result = _size;
    if (_preserveAspect)
    {
        const auto paragraph = _text.paragraphSize();
        result.x = static_cast<float>(double(result.y) * paragraph.width / paragraph.height);
        const float maximum = std::max(result.x, result.y);
        if (maximum > 1000000)
        {
            result *= 1000000.0f / maximum;
        }
    }
    return result;
}


void TextComponent::setSize(const glm::vec2& value) noexcept
{
    _size = {worldDimension(value.x, 2), worldDimension(value.y, 1)};
    _preserveAspect = false;
}


void TextComponent::setHeight(float value) noexcept
{
    _size.y = worldDimension(value, 1);
}


bool TextComponent::preserveAspect() const noexcept
{
    return _preserveAspect;
}


void TextComponent::setPreserveAspect(bool value) noexcept
{
    _preserveAspect = value;
}


glm::vec2 TextComponent::pivot() const noexcept
{
    return _pivot;
}


void TextComponent::setPivot(const glm::vec2& value) noexcept
{
    if (!std::isfinite(value.x) || !std::isfinite(value.y))
    {
        base::detail::textWarning("Invalid text pivot; using center.");
        _pivot = glm::vec2(0.5f);
        return;
    }
    _pivot = glm::clamp(value, glm::vec2(0), glm::vec2(1));
    if (_pivot != value)
    {
        base::detail::textWarning("Text pivot clamped to [0,1].");
    }
}


base::MaterialAttributes& TextComponent::material()
{
    return _pendingMaterial ? *_pendingMaterial : _drawable ? _drawable->material() : _fallbackMaterial;
}


const base::MaterialAttributes& TextComponent::material() const
{
    return _pendingMaterial ? *_pendingMaterial : _drawable ? _drawable->material() : _fallbackMaterial;
}


void TextComponent::setMaterial(const base::MaterialAttributes& value) noexcept
try
{
    _pendingMaterial = copyMaterial(value);
    ++_editVersion;
}
catch (...)
{
    base::detail::textWarning("Cannot stage text material; previous material retained.");
}


std::shared_ptr<draw::Drawable::Mesh> TextComponent::makeQuad() const
{
    auto mesh = std::make_shared<draw::Drawable::Mesh>();
    const auto dimensions = size();
    const float left = -_pivot.x * dimensions.x, right = left + dimensions.x;
    const float bottom = -_pivot.y * dimensions.y, top = bottom + dimensions.y;
    // Readable front is +Z: viewed from that side, +X is screen-right and
    // +Y is screen-up. Keep both top-left UV sets unchanged, not mirrored.
    const glm::vec3 normal{0, 0, 1}, tangent{1, 0, 0};
    mesh->vertices = {
        {{left, top, 0}, normal, {0, 0}, {0, 0}, tangent},
        {{right, top, 0}, normal, {1, 0}, {1, 0}, tangent},
        {{right, bottom, 0}, normal, {1, 1}, {1, 1}, tangent},
        {{left, bottom, 0}, normal, {0, 1}, {0, 1}, tangent}};
    mesh->indices = {0, 2, 1, 0, 3, 2};
    mesh->submeshes = {{0, 6}};
    return mesh;
}


bool TextComponent::resourcesDirty() const noexcept
try
{
    auto* owner = ownerNode() ? ownerNode()->scene() : nullptr;
    if (!_attempted || _attemptText != _text.revision() || _attemptEdit != _editVersion ||
        _attemptScene != owner || _attemptSize != size() || _attemptPivot != _pivot ||
        (owner && (!owner->textFontsReady() || _attemptFonts != owner->textFontVersion())))
    {
        return true;
    }
    // Immutable font identity also detects replacement Scene objects with a
    // coincident generation value; no file read or resource mutation here.
    if (owner && owner->textFont(_text.fontIdentifier()) != _font)
    {
        return true;
    }
    return !_ready && material().revision() != _attemptMaterial;
}
catch (...)
{
    return false;
}


bool TextComponent::hasQuadResources() const noexcept
{
    return _drawable && _drawable->isLoaded() && _ready;
}


bool TextComponent::canRender() const noexcept
{
    if (!hasQuadResources() || !_hasInk || !_attempted)
    {
        return false;
    }
    auto* owner = ownerNode() ? ownerNode()->scene() : nullptr;
    // Ordinary CPU edits retain the last complete version until prepareFrame,
    // including continuously animated labels. A changed/unavailable font must
    // not keep drawing old glyphs or their shadows while awaiting preparation.
    return owner == _attemptScene && (!owner || (owner->textFontsReady() &&
        owner->textFontVersion() == _attemptFonts && owner->textFont(_text.fontIdentifier()) == _font));
}


void TextComponent::retryResources() noexcept
{
    ++_editVersion;
}


void TextComponent::releaseResources() noexcept
try
{
    auto previous = _drawable;
    std::optional<gpu::Device::ResourceUpdateScope> boundary;
    if (previous && previous->engine())
    {
        boundary.emplace(*previous->engine()->device());
    }
    auto definition = copyMaterial(material());
    auto candidate = std::make_shared<draw::Drawable>();
    candidate->setMesh(makeQuad());
    candidate->setName("Text");
    candidate->setSubmeshName("Text");
    candidate->materialDefinition() = definition;
    _fallbackMaterial = std::move(definition);
    _drawable = std::move(candidate);
    _ready = false;
    _attempted = false;
    _attemptEngine = nullptr;
}
catch (...)
{
    _ready = false;
    base::detail::textWarning("Cannot release text realization safely; old owners retained, retry before engine teardown.");
}


void TextComponent::checkpoint(draw::Engine& engine, const std::shared_ptr<const base::TextFont>& font) noexcept
{
    _attemptEngine = &engine;
    _attemptScene = ownerNode() ? ownerNode()->scene() : nullptr;
    _attemptFonts = _attemptScene ? _attemptScene->textFontVersion() : 0;
    _attemptText = _text.revision();
    _attemptEdit = _editVersion;
    _attemptSize = size();
    _attemptPivot = _pivot;
    _font = font;
    _attempted = true;
    _attemptMaterial = material().revision();
}


bool TextComponent::prepareResources(draw::Engine& engine) noexcept
{
    auto* owner = ownerNode() ? ownerNode()->scene() : nullptr;
    if (owner)
    {
        owner->prepareTextFonts();
    }
    auto font = owner ? owner->textFont(_text.fontIdentifier()) : nullptr;
    if (_attemptEngine == &engine && !resourcesDirty())
    {
        return false;
    }
    const bool loaded = _drawable && _drawable->isLoaded() && _drawable->engine() == &engine;
    const bool pixelsChanged = !loaded || !_ready || _attemptText != _text.revision() ||
        _font != font || _attemptScene != owner || _attemptFonts != (owner ? owner->textFontVersion() : 0);
    const bool geometryChanged = !loaded || _realizedSize != size() || _realizedPivot != _pivot;
    const bool materialChanged = bool(_pendingMaterial);
    checkpoint(engine, font); // Cache unsuccessful attempts too; no per-frame retry.
    _ready = false;
    try
    {
        if (!engine.device() || !engine.device()->isValid())
        {
            base::detail::textWarning("Text preparation needs a live device; text omitted until explicit retry.");
            return true;
        }
        if (_drawable && _drawable->engine() && _drawable->engine() != &engine)
        {
            base::detail::textWarning("Text belongs to another engine; releaseResources at the old boundary before rebinding.");
            return true;
        }
        gpu::Device::ResourceUpdateScope boundary(*engine.device());
        base::MaterialAttributes attributes;
        if (pixelsChanged || materialChanged || !loaded)
        {
            attributes = copyMaterial(material());
        }
        auto nextGenerator = _generator;
        std::shared_ptr<draw::Texture> texture;
        bool hasInk = _hasInk;
        if (pixelsChanged)
        {
            nextGenerator = std::make_shared<base::TextTextureGenerator>(_text, font, _attemptFonts);
            auto sampling = attributes.albedoTexture();
            if (!sampling && _drawable)
            {
                sampling = _drawable->material().albedoTexture();
            }
            auto source = sampling ? std::make_shared<base::Texture>(*sampling) :
                base::createTextTexture(nextGenerator);
            if (!source)
            {
                base::detail::textWarning("Text source allocation failed; text omitted.");
                return true;
            }
            const auto colorType = source->colorType();
            source->setProceduralGenerator(nextGenerator);
            source->setColorType(colorType);
            source->setSourceEditable(false);
            source->setIgnoreSerialization(true);
            std::unique_ptr<base::Image> pixels(source->generateImage());
            // generateImage records the generator's native encoding. Sampling
            // interpretation is authored material data and must survive typing.
            source->setColorType(colorType);
            if (!pixels || !pixels->isValid())
            {
                base::detail::textWarning("Text bitmap unavailable; text omitted.");
                return true;
            }
            hasInk = false;
            for (size_t i = 3; i < size_t(pixels->width()) * pixels->height() * 4; i += 4)
            {
                hasInk = hasInk || pixels->data()[i] != 0;
            }
            if (loaded)
            {
                texture = std::make_shared<draw::Texture>(&engine);
                texture->load(*pixels, *source);
            }
            attributes.setAlbedoTexture(draw::cloneTextureSettings(*source));
        }
        else if (materialChanged && loaded)
        {
            // Whole-material replacement may change sampling, never the owned source.
            const auto protectedSource = _drawable->material().albedoTexture();
            if (protectedSource)
            {
                auto source = std::make_shared<base::Texture>(*protectedSource);
                if (auto incoming = attributes.albedoTexture())
                {
                    source->setColorType(incoming->colorType());
                    source->setMagFilter(incoming->magFilter());
                    source->setMinFilter(incoming->minFilter());
                    source->setUseMipmaps(incoming->useMipmaps());
                    source->setAddressMode(incoming->addressModeU(), incoming->addressModeV(), incoming->addressModeW());
                    source->setMinLod(incoming->minLod());
                    source->setMaxLod(incoming->maxLod());
                    source->setMipLodBias(incoming->mipLodBias());
                }
                texture = std::make_shared<draw::Texture>(&engine);
                texture->load(source, _drawable->drawMaterial(0)->albedoTexture()->imageResource());
                attributes.setAlbedoTexture(source);
            }
        }

        if (!loaded)
        {
            auto candidate = std::make_shared<draw::Drawable>();
            candidate->setMesh(makeQuad());
            candidate->setName("Text");
            candidate->setSubmeshName("Text");
            candidate->materialDefinition() = attributes;
            candidate->load(&engine);
            (void)draw::packMaterial(*candidate->drawMaterial(0));
            _drawable = std::move(candidate); // Old owners are idle at this boundary.
        }
        else
        {
            std::shared_ptr<draw::MaterialBase> preparedMaterial;
            std::optional<draw::Drawable::PreparedGeometry> geometry;
            std::shared_ptr<draw::Drawable::Mesh> mesh;
            std::optional<base::MaterialAttributes> definition;
            if (pixelsChanged || materialChanged)
            {
                preparedMaterial = _drawable->drawMaterial(0)->snapshot();
                if (materialChanged)
                {
                    preparedMaterial->setMaterialAttributes(attributes);
                }
                if (texture)
                {
                    preparedMaterial->setTexture(draw::TextureSlot::Albedo, texture);
                }
                preparedMaterial->updateTextures();
                (void)draw::packMaterial(*preparedMaterial);
                definition = draw::cloneMaterialAttributes(preparedMaterial->materialAttributes());
            }
            if (geometryChanged)
            {
                mesh = makeQuad();
                geometry.emplace(_drawable->prepareGeometry(*mesh));
                if (!_drawable->canExchangeGeometry(*geometry))
                {
                    base::detail::textWarning("Prepared text geometry is stale; text omitted until retry.");
                    return true;
                }
            }
            // Preparation may submit uploads/BLAS. Drain before discarding
            // displaced geometry/material states still referenced by commands.
            engine.device()->waitIdleForResourceUpdate();
            if (geometry)
            {
                if (!_drawable->exchangeGeometry(*geometry))
                {
                    base::detail::textWarning("Cannot publish prepared text geometry; text omitted until retry.");
                    return true;
                }
                using std::swap;
                swap(*_drawable->mesh(), *mesh);
            }
            if (preparedMaterial)
            {
                if (!_drawable->drawMaterial(0)->exchangeState(*preparedMaterial))
                {
                    base::detail::textWarning("Cannot publish text material; text omitted until retry.");
                    return true;
                }
                using std::swap;
                swap(_drawable->materialDefinition(), *definition);
            }
        }
        _generator = std::move(nextGenerator);
        _hasInk = hasInk;
        _realizedSize = size();
        _realizedPivot = _pivot;
        _pendingMaterial.reset();
        _ready = true;
        _attemptMaterial = material().revision();
        return true;
    }
    catch (...)
    {
        base::detail::textWarning("Cannot prepare text GPU/material resources; only this text is omitted until source change/retry.");
        return true;
    }
}


bool TextComponent::prepareResources(Node* root, draw::Engine& engine) noexcept
try
{
    if (!root)
    {
        return false;
    }
    if (auto* owner = root->scene())
    {
        owner->prepareTextFonts();
    }
    bool changed = false;
    std::function<void(Node*)> prepare = [&](Node* node)
    {
        if (auto* text = node->getComponent<TextComponent>())
        {
            changed = text->prepareResources(engine) || changed;
        }
        for (const auto& child : node->children())
        {
            prepare(child.get());
        }
    };
    prepare(root);
    return changed;
}
catch (...)
{
    base::detail::textWarning("Cannot traverse pending text resources; preparation deferred.");
    return false;
}


void TextComponent::reportRenderFailure() noexcept
{
    if (_ready)
    {
        base::detail::textWarning("Invalid text draw state; text omitted until material/source change or retry.");
    }
    _ready = false;
    _attemptMaterial = material().revision();
}


void TextComponent::update(float)
{
    // CPU notification only. Uploads must never run in update/draw traversal.
    if (resourcesDirty())
    {
        try
        {
            if (auto* loop = app::MainLoop::current())
            {
                loop->requestSceneFrame();
            }
        }
        catch (...)
        {
            // Detached/direct renderer use may have no active main loop.
        }
    }
}


void TextComponent::loadResources(draw::Engine& engine, SceneLoadProgress*) noexcept
{
    prepareResources(engine);
}


std::shared_ptr<Component> TextComponent::clone() const noexcept
try
{
    auto copy = std::make_shared<TextComponent>();
    copy->_text = _text;
    copy->_size = _size;
    copy->_pivot = _pivot;
    copy->_preserveAspect = _preserveAspect;
    copy->_fallbackMaterial = copyMaterial(material());
    copy->_generator = _generator ? _generator->clone() : nullptr;
    if (!copy->_generator)
    {
        copy->_generator = std::make_shared<base::TextTextureGenerator>(_text, _font, _attemptFonts);
    }
    copy->_generator->configure(_text, _font, _attemptFonts);
    auto albedo = copy->_fallbackMaterial.albedoTexture() ?
        std::make_shared<base::Texture>(*copy->_fallbackMaterial.albedoTexture()) : base::createTextTexture(copy->_generator);
    if (albedo)
    {
        const auto color = albedo->colorType();
        albedo->setProceduralGenerator(copy->_generator);
        albedo->setColorType(color);
        albedo->setSourceEditable(false);
        albedo->setIgnoreSerialization(true);
        copy->_fallbackMaterial.setAlbedoTexture(albedo);
    }
    copy->_pendingMaterial = copy->_fallbackMaterial;
    copy->_priority = _priority;
    copy->_ignoreSerialization = _ignoreSerialization;
    if (_drawable)
    {
        try
        {
            copy->_drawable = _drawable->clone();
            copy->_hasInk = _hasInk;
            copy->_realizedSize = _realizedSize;
            copy->_realizedPivot = _realizedPivot;
            copy->_ready = copy->_drawable->isLoaded();
            if (copy->_ready && !_pendingMaterial)
            {
                copy->_pendingMaterial.reset(); // Keep independently cloned native non-albedo resources.
            }
        }
        catch (...)
        {
            base::detail::textWarning("Text GPU clone unavailable; retaining independent CPU text/material for later realization.");
            copy->_drawable = std::make_shared<draw::Drawable>();
            copy->_drawable->setMesh(copy->makeQuad());
            copy->_drawable->materialDefinition() = copy->_fallbackMaterial;
        }
    }
    return copy;
}
catch (...)
{
    base::detail::textWarning("Cannot clone text component; clone unavailable.");
    return nullptr;
}


void TextComponent::deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath) noexcept
try
{
    deserializeCommon(data);
    json::ObjectReader reader(data);
    if (!reader.isValid())
    {
        base::detail::textWarning("Invalid text representation; previous/default data retained.");
        return;
    }
    if (auto text = reader.getObject("text"))
    {
        _text.deserialize(text->node());
    }
    const bool aspect = reader.getBool("preserveAspect").value_or(_preserveAspect);
    if (auto size = reader.getVec2("size"))
    {
        setSize({(*size)[0], (*size)[1]});
    }
    setPreserveAspect(aspect);
    if (auto pivot = reader.getVec2("pivot"))
    {
        setPivot({(*pivot)[0], (*pivot)[1]});
    }
    if (auto embedded = reader.getObject("material"))
    {
        // Copy only the representation. Drop an untrusted albedo reference
        // before the conventional parser can construct or resolve that source.
        auto object = embedded->node()->objectValue();
        object.erase("albedoTexture");
        const auto mode = embedded->getString("indirectTextureMode");
        if (mode && *mode != "ao" && *mode != "bakedAO" && *mode != "bakedGI")
        {
            object.erase("indirectTextureMode");
            base::detail::textWarning("Invalid text material indirect mode; using previous/default value.");
        }
        for (const char* key : {"albedoTextureSettings", "metalnessTextureSettings", "roughnessTextureSettings",
            "normalTextureSettings", "ambientOcclussionSettings", "lightEmissionTextureSettings"})
        {
            if (auto settings = embedded->getObject(key))
            {
                auto repaired = settings->node()->objectValue();
                for (const char* field : {"mag", "min", "u", "v", "w", "color"})
                {
                    if (settings->isUndefined(field))
                    {
                        continue;
                    }
                    auto value = settings->getInteger<uint32_t>(field);
                    const std::string name(field);
                    const uint32_t maximum = name == "mag" || name == "min" ? 1 : name == "color" ? 2 : 3;
                    if (!value || *value > maximum || (name == "color" && *value == 0))
                    {
                        repaired.erase(field);
                        base::detail::textWarning("Invalid text material sampler enum; using previous/default value.");
                    }
                }
                object[key] = json::JSON(repaired);
            }
        }
        auto attributes = material();
        auto generator = std::make_shared<base::TextTextureGenerator>(_text, nullptr);
        auto source = base::createTextTexture(generator);
        if (!source)
        {
            base::detail::textWarning("Cannot restore text material source; previous material retained.");
        }
        else
        {
            source->setSourceEditable(false);
            source->setIgnoreSerialization(true);
            attributes.setAlbedoTexture(source);
            try
            {
                utils::MaterialSerializer serializer;
                if (!serializer.deserializeMaterial(json::JSON(object)->serialize(), basePath, attributes))
                {
                    base::detail::textWarning("Invalid embedded text material; defaults retained.");
                }
            }
            catch (...)
            {
                // Keep fields already recovered by the conventional parser;
                // a malformed optional material must not discard the paragraph.
                base::detail::textWarning("Malformed text material; retaining recovered/default fields.");
            }
            _pendingMaterial = std::move(attributes);
            _generator = std::move(generator);
        }
    }
    retryResources();
}
catch (...)
{
    base::detail::textWarning("Cannot read text representation; previous/default data retained.");
}


std::shared_ptr<json::JsonNode> TextComponent::serializeReferences(const std::filesystem::path& basePath) noexcept
try
{
    auto result = Component::serialize(basePath);
    auto paragraph = _text.serialize();
    if (!result || !paragraph)
    {
        base::detail::textWarning("Cannot serialize editable text; save refused.");
        return nullptr;
    }
    auto attributes = material();
    // Ownership is component logic, never trust a replaced source's export flag.
    auto source = attributes.albedoTexture() ?
        std::make_shared<base::Texture>(*attributes.albedoTexture()) : base::createTextTexture(_generator);
    if (!source)
    {
        base::detail::textWarning("Cannot serialize text sampling; save refused.");
        return nullptr;
    }
    source->setIgnoreSerialization(true);
    attributes.setAlbedoTexture(source);
    for (const auto& texture : {attributes.metalnessTexture(), attributes.roughnessTexture(),
        attributes.normalTexture(), attributes.aoTexture(), attributes.lightEmissionTexture()})
    {
        if (texture && !texture->ignoreSerialization() && texture->type() != base::Texture::TypeFilesystem)
        {
            base::detail::textWarning("Text material has a procedural image; use a complete scene save.");
            return nullptr;
        }
    }
    utils::MaterialSerializer serializer;
    std::vector<std::shared_ptr<base::Texture>> textures;
    auto embedded = json::JsonParser(serializer.serializeMaterial(attributes, textures, false)).parse();
    const auto directory = std::filesystem::absolute(basePath.empty() ? "." : basePath).lexically_normal();
    for (auto& [key, value] : embedded->objectValue())
    {
        if (key == "metalnessTexture" || key == "roughnessTexture" || key == "normalTexture" ||
            key == "ambientOcclussion" || key == "lightEmissionTexture")
        {
            if (!value || !value->isString() || value->stringValue().empty())
            {
                base::detail::textWarning("Text material has an unexported image; use a complete scene save.");
                return nullptr;
            }
            auto path = std::filesystem::absolute(value->stringValue()).lexically_normal();
            auto relative = path.lexically_relative(directory);
            value = json::JSON((relative.empty() ? path : relative).generic_string());
        }
    }
    auto& object = result->objectValue();
    object["text"] = paragraph;
    object["material"] = embedded;
    object["size"] = json::JSON(std::array<float, 2>{_size.x, _size.y});
    object["preserveAspect"] = json::JSON(_preserveAspect);
    object["pivot"] = json::JSON(std::array<float, 2>{_pivot.x, _pivot.y});
    return result;
}
catch (...)
{
    base::detail::textWarning("Cannot serialize editable text/material; save refused.");
    return nullptr;
}


std::shared_ptr<json::JsonNode> TextComponent::serialize(const std::filesystem::path& basePath) noexcept
{
    return serializeReferences(basePath);
}


void TextComponent::didAddToNode(Node*)
{
    _attempted = false;
}


void TextComponent::didRemoveFromNode(Node*)
{
    _attempted = false;
}


BG2E_SCENE_REGISTER_COMPONENT(TextComponent);

}
