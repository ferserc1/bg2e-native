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

#include "MaterialCommands.hpp"

#include <bg2e/gpu/Common.hpp>
#include <bg2e/draw/ShaderData.hpp>
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>
#include <typeinfo>

namespace bg2e_composer::command::material {
namespace {
using namespace bg2e;
using Attributes = base::MaterialAttributes;
using Setter = void (Attributes::*)(std::shared_ptr<base::Texture>);
const std::array<Setter, 6> setters{&Attributes::setAlbedoTexture, &Attributes::setMetalnessTexture,
    &Attributes::setRoughnessTexture, &Attributes::setNormalTexture, &Attributes::setAoTexture,
    &Attributes::setLightEmissionTexture};

size_t add(size_t a, size_t b)
{
    if (b > std::numeric_limits<size_t>::max() - a)
    {
        throw std::length_error("Material history memory overflow");
    }
    return a + b;
}

size_t multiply(size_t a, size_t b)
{
    if (a && b > std::numeric_limits<size_t>::max() / a)
    {
        throw std::length_error("Material history memory overflow");
    }
    return a * b;
}

void accountDefinition(app::CommandMemory& memory, const Attributes& attributes)
{
    using Getter = std::shared_ptr<base::Texture> (Attributes::*)() const;
    const std::array<Getter, 6> getters{&Attributes::albedoTexture, &Attributes::metalnessTexture,
        &Attributes::roughnessTexture, &Attributes::normalTexture, &Attributes::aoTexture, &Attributes::lightEmissionTexture};
    for (auto getter : getters)
    {
        auto settings = (attributes.*getter)();
        if (!settings)
        {
            continue;
        }
        memory.localBytes = add(memory.localBytes, 1024 + settings->imageFilePath().capacity());
        if (settings->sourceOwner())
        {
            memory.resources.push_back({settings->sourceOwner(), settings->sourceBytes()});
        }
        if (settings->type() == base::Texture::TypeProcedural)
        {
            auto* generator = settings->proceduralTextureGenerator();
            if (!generator)
            {
                throw std::invalid_argument("Missing retained procedural source");
            }
            memory.localBytes = add(memory.localBytes,
                multiply(multiply(generator->width(), generator->height()), generator->bytesPerPixel()));
        }
    }
}

void account(app::CommandMemory& memory, const std::shared_ptr<draw::MaterialBase>& material)
{
    accountDefinition(memory, material->materialAttributes());
    memory.localBytes = add(memory.localBytes, 65536 + material->materialAttributes().name().capacity() +
        material->materialAttributes().groupName().capacity());
    for (unsigned i = 0; i < 6; ++i)
    {
        auto texture = material->texture(static_cast<draw::TextureSlot>(i));
        if (!texture || !texture->image())
        {
            throw std::invalid_argument("Material history requires realized texture bindings");
        }
        if (texture->base()->sourceOwner())
        {
            memory.resources.push_back({texture->base()->sourceOwner(), texture->base()->sourceBytes()});
        }
        auto image = texture->imageResource();
        auto texelBytes = gpu::pixelFormatBytes(image->pixelFormat());
        if (!texelBytes)
        {
            throw std::invalid_argument("Unsupported retained material image format");
        }
        size_t bytes = 0;
        auto width = image->width(), height = image->height();
        for (uint32_t level = 0; level < image->mipLevels(); ++level)
        {
            bytes = add(bytes, multiply(multiply(width, height), texelBytes));
            width = std::max(1u, width / 2);
            height = std::max(1u, height / 2);
        }
        memory.resources.push_back({image, multiply(bytes, image->arrayLayers())});
        // Wrapper, sampler and retained decoded/procedural source. Deliberately
        // conservative; image tickets separately deduplicate native image data.
        const bool retainedPixels = texture->base()->type() == base::Texture::TypeProcedural ||
            texture->base()->imageFilePath().empty();
        const size_t source = retainedPixels ? multiply(multiply(image->width(), image->height()), 32) : 0;
        memory.resources.push_back({texture, add(65536 + texture->base()->imageFilePath().capacity(), source)});
        // Editable procedural settings are copied per command, independently of
        // the shared native image ticket. Do not deduplicate their CPU budget.
        if (texture->base()->type() == base::Texture::TypeProcedural)
        {
            memory.localBytes = add(memory.localBytes, source);
        }
    }
}

bool apply(draw::MaterialBase& material, Attributes& definition, const ui::MaterialEditIntent& intent)
{
    bool changed = false;
    const auto initialLive = material.materialAttributes().revision();
    const auto initialDefinition = definition.revision();
    if (intent.patch)
    {
        const auto liveRevision = material.materialAttributes().revision();
        const auto definitionRevision = definition.revision();
        intent.patch->apply(material.materialAttributes());
        intent.patch->apply(definition);
        changed = liveRevision != material.materialAttributes().revision() || definitionRevision != definition.revision();
        if (!intent.cpuValuesOnly)
        {
            material.updateTextures();
        }
    }
    auto replaceSettings = [&](draw::TextureSlot slot, bool linear)
    {
        auto source = material.texture(slot);
        auto settings = draw::cloneTextureSettings(*source->base());
        const auto revision = settings->revision();
        if (intent.sampling)
        {
            intent.sampling->apply(*settings);
        }
        if (linear)
        {
            settings->setColorType(base::Color::TypeLinear);
        }
        if (revision == settings->revision())
        {
            return;
        }
        auto replacement = std::make_shared<draw::Texture>(material.engine());
        replacement->load(settings, source->imageResource());
        material.setTexture(slot, replacement);
        (definition.*setters.at(size_t(slot)))(draw::cloneTextureSettings(*settings));
        changed = true;
    };
    if (intent.sampling)
    {
        replaceSettings(intent.samplingSlot, intent.samplingSlot == draw::TextureSlot::AmbientOcclusion &&
            material.materialAttributes().indirectTextureMode() != Attributes::IndirectTextureMode::AmbientOcclusion);
    }
    if (intent.linearSlot >= 0)
    {
        replaceSettings(static_cast<draw::TextureSlot>(intent.linearSlot), true);
    }
    if (intent.ordinaryAo || intent.indirectMode)
    {
        const auto mode = intent.ordinaryAo ? Attributes::IndirectTextureMode::AmbientOcclusion : *intent.indirectMode;
        if (intent.ordinaryAo || mode != Attributes::IndirectTextureMode::AmbientOcclusion)
        {
            if (!intent.ordinaryAo && !material.materialAttributes().aoTexture())
            {
                throw std::invalid_argument("Assign an indirect map before choosing a baked interpretation");
            }
            replaceSettings(draw::TextureSlot::AmbientOcclusion, true);
            if (!intent.ordinaryAo)
            {
                material.materialAttributes().setAoUVSet(1);
                definition.setAoUVSet(1);
            }
        }
        changed = changed || material.materialAttributes().indirectTextureMode() != mode || definition.indirectTextureMode() != mode;
        material.materialAttributes().setIndirectTextureMode(mode);
        definition.setIndirectTextureMode(mode);
    }
    if (intent.clearSlot >= 0)
    {
        const auto slot = size_t(intent.clearSlot);
        using Getter = std::shared_ptr<base::Texture> (Attributes::*)() const;
        const std::array<Getter, 6> getters{&Attributes::albedoTexture, &Attributes::metalnessTexture,
            &Attributes::roughnessTexture, &Attributes::normalTexture, &Attributes::aoTexture,
            &Attributes::lightEmissionTexture};
        changed = changed || bool((material.materialAttributes().*getters.at(slot))()) || bool((definition.*getters.at(slot))());
        (material.materialAttributes().*setters.at(slot))(nullptr);
        (definition.*setters.at(slot))(nullptr);
        material.updateTextures();
        if (slot == size_t(draw::TextureSlot::AmbientOcclusion))
        {
            material.materialAttributes().setIndirectTextureMode(Attributes::IndirectTextureMode::AmbientOcclusion);
            definition.setIndirectTextureMode(Attributes::IndirectTextureMode::AmbientOcclusion);
        }
    }
    return changed || initialLive != material.materialAttributes().revision() || initialDefinition != definition.revision();
}
}

EditMaterials::EditMaterials(std::weak_ptr<StageScene> stage, const bg2e::ui::MaterialEditTargets& targets,
    const bg2e::ui::MaterialEditIntent& intent, const std::shared_ptr<bg2e::draw::MaterialBase>& paste)
    : _stage(std::move(stage))
{
    auto owner = _stage.lock();
    if (!owner || targets.selection.empty() || !targets.direct.expired())
    {
        throw std::invalid_argument("Composer material edits require selected document submeshes");
    }
    std::set<std::pair<bg2e::draw::Drawable*, uint32_t>> seen;
    std::set<bg2e::draw::MaterialBase*> owners;
    _memory.localBytes = sizeof(*this) + targets.selection.size() * sizeof(Target) * 2;
    for (const auto& item : targets.selection)
    {
        auto node = item.node.lock();
        auto mesh = item.lockMesh();
        if (!node || !owner->contains(node.get()) || !mesh || !mesh->isLoaded())
        {
            throw std::invalid_argument("Material target is stale");
        }
        if (!seen.emplace(mesh.get(), item.submesh).second)
        {
            continue;
        }
        auto live = mesh->drawMaterial(item.submesh);
        const auto* instance = live.get();
        const auto* pasteInstance = paste.get();
        if (typeid(*instance) != typeid(bg2e::draw::MaterialBase) ||
            (pasteInstance && typeid(*pasteInstance) != typeid(bg2e::draw::MaterialBase)))
        {
            throw std::invalid_argument("Custom materials require an explicit state adapter");
        }
        if (!owners.insert(live.get()).second)
        {
            throw std::invalid_argument("Shared live materials require independent ownership before editing");
        }
        Target target;
        target.item = item;
        target.live = live;
        if (!live->texturesUpdated())
        {
            throw std::invalid_argument("Realize pending material sources before editing with history");
        }
        target.expectedRevision = live->materialAttributes().revision();
        target.expectedDefinition = mesh->materialDefinition(item.submesh).revision();
        target.alternate = (paste ? paste : live)->snapshot();
        if (target.alternate->engine() != live->engine())
        {
            throw std::invalid_argument("Material clipboard belongs to another device");
        }
        target.definition = bg2e::draw::cloneMaterialAttributes(paste ? paste->materialAttributes() : mesh->materialDefinition(item.submesh));
        _effective = apply(*target.alternate, target.definition, intent) || paste || _effective;
        if (!target.alternate->texturesUpdated())
        {
            throw std::invalid_argument("A CPU material intent cannot contain unresolved resource changes");
        }
        target.alternate->materialAttributes().setName(live->materialAttributes().name());
        target.alternate->materialAttributes().setGroupName(live->materialAttributes().groupName());
        target.alternate->materialAttributes().setVisible(live->materialAttributes().visible());
        (void)bg2e::draw::packMaterial(*target.alternate);
        // Submesh metadata has its own commands and must survive material paste.
        target.definition.setName(mesh->materialDefinition(item.submesh).name());
        target.definition.setGroupName(mesh->materialDefinition(item.submesh).groupName());
        target.definition.setVisible(mesh->materialDefinition(item.submesh).visible());
        accountDefinition(_memory, mesh->materialDefinition(target.item.submesh));
        accountDefinition(_memory, target.definition);
        account(_memory, live);
        account(_memory, target.alternate);
        _targets.push_back(std::move(target));
    }
}

EditMaterials::EditMaterials(std::weak_ptr<StageScene> stage, const std::vector<PreparedMaterial>& prepared,
    std::function<bool()> sourceValid)
    : _stage(std::move(stage)), _sourceValid(std::move(sourceValid))
{
    auto owner = _stage.lock();
    if (!owner || prepared.empty())
    {
        throw std::invalid_argument("Prepared materials require document receivers");
    }
    _info.name = "Apply prepared lightmaps";
    std::set<bg2e::draw::MaterialBase*> seen;
    _memory.localBytes = sizeof(*this) + prepared.size() * sizeof(Target) * 2 + 65536;
    for (const auto& value : prepared)
    {
        auto node = value.target.node.lock();
        auto mesh = value.target.lockMesh();
        if (!node || !owner->contains(node.get()) || !mesh || !mesh->isLoaded() || !value.material)
        {
            throw std::invalid_argument("Prepared material receiver is stale");
        }
        auto live = mesh->drawMaterial(value.target.submesh);
        const auto* liveInstance = live.get();
        const auto* preparedInstance = value.material.get();
        if (typeid(*liveInstance) != typeid(bg2e::draw::MaterialBase) ||
            typeid(*preparedInstance) != typeid(bg2e::draw::MaterialBase))
        {
            throw std::invalid_argument("Prepared custom materials require a state adapter");
        }
        if (!seen.insert(live.get()).second || live->engine() != value.material->engine() || !live->texturesUpdated())
        {
            throw std::invalid_argument("Prepared material aliases, device or source state changed");
        }
        Target target;
        target.item = value.target;
        target.live = live;
        target.expectedRevision = live->materialAttributes().revision();
        target.expectedDefinition = mesh->materialDefinition(value.target.submesh).revision();
        target.alternate = value.material->snapshot();
        target.definition = bg2e::draw::cloneMaterialAttributes(value.definition);
        (void)bg2e::draw::packMaterial(*target.alternate);
        accountDefinition(_memory, mesh->materialDefinition(target.item.submesh));
        accountDefinition(_memory, target.definition);
        account(_memory, live);
        account(_memory, target.alternate);
        _targets.push_back(std::move(target));
    }
    _effective = true;
}


const bg2e::app::CommandInfo& EditMaterials::info() const noexcept
{
    return _info;
}

bg2e::app::CommandMemory EditMaterials::memory() const
{
    return _memory;
}

bg2e::app::CommandResult EditMaterials::execute() noexcept
{
    try
    {
        if (_sourceValid && !_sourceValid())
        {
            return {bg2e::app::CommandStatus::Rejected, "Prepared material job or sources changed"};
        }
        return _effective ? exchange(true) : bg2e::app::CommandResult{};
    }
    catch (...)
    {
        return {bg2e::app::CommandStatus::Rejected, "Prepared material validation failed"};
    }
}

bg2e::app::CommandResult EditMaterials::undo() noexcept
{
    return exchange(false);
}

bg2e::app::CommandResult EditMaterials::redo() noexcept
{
    return exchange(true);
}

bg2e::app::CommandResult EditMaterials::exchange(bool applied) noexcept
{
    using namespace bg2e::app;
    if (_applied == applied)
    {
        return {};
    }
    // Validate every target before the first allocation-free publication.
    auto stage = _stage.lock();
    for (const auto& target : _targets)
    {
        auto node = target.item.node.lock();
        auto mesh = target.item.lockMesh();
        auto live = target.live.lock();
        if (!stage || !node || !stage->contains(node.get()) || !mesh || !live ||
            mesh->drawMaterial(target.item.submesh) != live || live->engine() != target.alternate->engine() ||
            live->materialAttributes().revision() != target.expectedRevision ||
            mesh->materialDefinition(target.item.submesh).revision() != target.expectedDefinition)
        {
            return {CommandStatus::Rejected, "Prepared material target changed"};
        }
    }
    for (auto& target : _targets)
    {
        auto mesh = target.item.lockMesh();
        auto live = target.live.lock();
        live->exchangeState(*target.alternate);
        using std::swap;
        swap(mesh->materialDefinition(target.item.submesh), target.definition);
        target.expectedRevision = live->materialAttributes().revision();
        target.expectedDefinition = mesh->materialDefinition(target.item.submesh).revision();
    }
    _applied = applied;
    return {CommandStatus::Applied, {}};
}

}
