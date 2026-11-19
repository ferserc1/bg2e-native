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

#include "ReplaceDrawable.hpp"

#include <bg2e/gpu/RayTracingMesh.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/Image.hpp>
#include <cstdio>
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace bg2e_composer::command::scene {
using namespace bg2e::app;

ReplaceDrawable::ReplaceDrawable(const std::shared_ptr<StageScene>& stage,
    std::shared_ptr<bg2e::manipulation::SelectionManager> selection,
    std::shared_ptr<bg2e::scene::DrawableComponent> owner, std::shared_ptr<bg2e::draw::Drawable> replacement)
    : _stage(stage), _owner(owner), _selection(selection), _after(std::move(replacement))
{
    if (!stage || !owner || !selection || !stage->contains(owner->ownerNode()) || !_after || !_after->isLoaded())
    {
        throw std::invalid_argument("Drawable replacement requires a live document owner and prepared model");
    }
    _before = owner->drawable();
    _handle = owner->resourceHandle();
    const auto cache = _handle.context();
    if (!_before || !cache || _before->engine() != _after->engine() || _after->cacheUsage().registered || _after->hasOwner())
    {
        throw std::invalid_argument("Replacement must be independent geometry from the owner's engine");
    }
    const std::weak_ptr<bg2e::draw::Drawable> weak = _after;
    auto use = cache->retain(_handle, bg2e::draw::DrawableUse::History, "owner-replacement:" + _handle.identifier(), [weak]() noexcept
    {
        if (const auto resource = weak.lock(); resource && !resource->cacheUsage().registered)
        {
            try
            {
                resource->cleanup();
            }
            catch (...)
            {
                std::fprintf(stderr, "Could not release an unpublished replacement drawable during context shutdown.\n");
            }
        }
    });
    if (!use)
    {
        throw std::runtime_error(use.diagnostic.message);
    }
    _use = std::move(use.lease);
    _selectionBefore = selection->captureSelection();
    std::function<void(const std::shared_ptr<bg2e::scene::Node>&)> collect = [&](const auto& node)
    {
        if (auto* instance = node->drawableInstance(); instance && instance->resourceHandle() == _handle)
        {
            auto prepared = instance->cloneForResourceReplacement(*_after);
            Instance item;
            item.node = node;
            item.before = stage->retainForCommand(instance->shared_from_this());
            item.after = stage->retainForCommand(prepared);
            item.beforeComponents = node->orderedComponents();
            item.afterComponents = item.beforeComponents;
            for (auto& component : item.afterComponents)
            {
                if (component.get() == instance)
                {
                    component = prepared;
                }
            }
            _instances.push_back(std::move(item));
        }
        for (const auto& child : node->children())
        {
            collect(child);
        }
    };
    collect(stage->document().root);
}


const CommandInfo& ReplaceDrawable::info() const noexcept
{
    return _info;
}


CommandMemory ReplaceDrawable::memory() const
{
    CommandMemory result{sizeof(*this) + 65536, {}};
    // Geometry tickets are per retained realization, never per instance.
    for (const auto& resource : {_before, _after})
    {
        const auto mesh = resource->mesh();
        const auto bytes = mesh->vertices.capacity() * sizeof(bg2e::geo::VertexPNUUT) +
            mesh->indices.capacity() * sizeof(uint32_t) + mesh->submeshes.capacity() * sizeof(bg2e::geo::Submesh);
        result.resources.push_back({mesh, bytes});
        result.resources.push_back({resource->drawMesh(), bytes * 2 + 65536});
        for (const auto& blas : resource->rayTracingMeshes())
        {
            result.resources.push_back({blas, blas->retainedBytes() + 65536});
        }
        for (uint32_t sub = 0; sub < resource->submeshesCount(); ++sub)
        {
            result.localBytes += 131072 + resource->submeshName(sub).capacity() + resource->submeshGroupName(sub).capacity();
            auto material = resource->drawMaterial(sub);
            for (uint32_t slot = 0; slot < 6; ++slot)
            {
                const auto texture = material->texture(static_cast<bg2e::draw::TextureSlot>(slot));
                const auto image = texture ? texture->imageResource() : nullptr;
                if (texture && texture->base()->sourceOwner())
                {
                    result.resources.push_back({texture->base()->sourceOwner(), texture->base()->sourceBytes()});
                }
                if (texture && texture->base()->type() == bg2e::base::Texture::TypeProcedural)
                {
                    const auto* source = texture->base()->proceduralTextureGenerator();
                    if (source)
                    {
                        result.localBytes += 3 * size_t(source->width()) * source->height() * source->bytesPerPixel();
                    }
                }
                if (image)
                {
                    const auto bytes = size_t(image->width()) * image->height() * image->arrayLayers() *
                        bg2e::gpu::pixelFormatBytes(image->pixelFormat()) * 2;
                    result.resources.push_back({image, bytes});
                }
            }
        }
    }
    for (const auto& item : _instances)
    {
        for (const auto& state : {item.before, item.after})
        {
            result.resources.push_back({state, state->bytes});
            result.localBytes += state->growthBytes();
        }
        result.localBytes += 4 * (item.beforeComponents.capacity() + item.afterComponents.capacity()) * sizeof(std::shared_ptr<bg2e::scene::Component>);
    }
    for (const auto* state : {&_selectionBefore, &_selectionAfter})
    {
        result.localBytes += 4 * (state->items.capacity() * sizeof(bg2e::manipulation::SelectionItem) +
            state->nodes.capacity() * sizeof(std::weak_ptr<bg2e::scene::Node>));
    }
    if (const auto selection = _selection.lock())
    {
        result.localBytes += 4 * (selection->selectedItems().size() + selection->selectedNodes().size()) *
            (sizeof(bg2e::manipulation::SelectionItem) + sizeof(std::weak_ptr<bg2e::scene::Node>));
    }
    return result;
}


CommandResult ReplaceDrawable::execute() noexcept
{
    return apply(true);
}


CommandResult ReplaceDrawable::undo() noexcept
{
    return apply(false);
}


CommandResult ReplaceDrawable::redo() noexcept
{
    return apply(true);
}


CommandResult ReplaceDrawable::apply(bool forward) noexcept
{
    if (_applied == forward)
    {
        return {};
    }
    const auto owner = _owner.lock();
    const auto stage = _stage.lock();
    const auto selection = _selection.lock();
    const auto& expected = forward ? _before : _after;
    const auto& next = forward ? _after : _before;
    if (!owner || !stage || !selection || !stage->contains(owner->ownerNode()) || owner->resourceHandle() != _handle || owner->drawable() != expected)
    {
        return {CommandStatus::Rejected, "Drawable owner or resource changed"};
    }
    bool changed = false;
    bg2e::manipulation::SelectionState previousSelection;
    try
    {
        previousSelection = selection->captureSelection();
        for (const auto& item : _instances)
        {
            const auto node = item.node.lock();
            const auto& components = forward ? item.beforeComponents : item.afterComponents;
            if (!node || !stage->contains(node.get()) || node->orderedComponents() != components)
            {
                return {CommandStatus::Rejected, "An affected instance changed membership"};
            }
        }
        const auto assigned = owner->setDrawable(next);
        if (!assigned.ok())
        {
            return {CommandStatus::Rejected, "The replacement resource could not be assigned"};
        }
        changed = true;
        const auto cache = _handle.context();
        if (!cache)
        {
            throw std::runtime_error("The owner resource context expired");
        }
        if (!forward || _afterUse.valid())
        {
            const auto defaults = cache->restoreOriginalDefinitions(_handle, forward ? _afterUse : _use);
            if (!defaults)
            {
                throw std::runtime_error(defaults.diagnostic.message);
            }
        }
        else
        {
            auto retained = cache->retain(_handle, bg2e::draw::DrawableUse::History, "owner-replacement-after:" + _handle.identifier());
            if (!retained)
            {
                throw std::runtime_error(retained.diagnostic.message);
            }
            _afterUse = std::move(retained.lease);
        }
        for (const auto& item : _instances)
        {
            const auto state = std::static_pointer_cast<bg2e::scene::DrawableInstanceComponent>((forward ? item.after : item.before)->component);
            if (!state->synchronizeResourceRevision(*next))
            {
                throw std::runtime_error("Prepared instance appearance no longer matches the shared geometry");
            }
            item.node.lock()->setComponentsOrdered(forward ? item.afterComponents : item.beforeComponents);
        }
        selection->refreshSelection();
        if (!forward || _captured)
        {
            selection->restoreSelection(forward ? _selectionAfter : _selectionBefore, [stage](auto* node)
            {
                return stage->contains(node);
            });
        }
        else
        {
            _selectionAfter = selection->captureSelection();
            _captured = true;
        }
        _applied = forward;
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        if (changed)
        {
            try
            {
                const auto restored = owner->setDrawable(expected);
                if (!restored.ok())
                {
                    return {CommandStatus::Faulted, "Could not restore shared drawable after replacement failure"};
                }
                const auto cache = _handle.context();
                const auto defaults = cache->restoreOriginalDefinitions(_handle, forward ? _use : _afterUse);
                if (!defaults)
                {
                    return {CommandStatus::Faulted, "Could not restore original drawable definitions"};
                }
                for (const auto& item : _instances)
                {
                    auto state = std::static_pointer_cast<bg2e::scene::DrawableInstanceComponent>((forward ? item.before : item.after)->component);
                    if (!state->synchronizeResourceRevision(*expected))
                    {
                        return {CommandStatus::Faulted, "Could not restore instance appearance after replacement failure"};
                    }
                    item.node.lock()->setComponentsOrdered(forward ? item.beforeComponents : item.afterComponents);
                }
                selection->restoreSelection(previousSelection, [stage](auto* node)
                {
                    return stage->contains(node);
                });
            }
            catch (...)
            {
                return {CommandStatus::Faulted, "Drawable replacement recovery failed"};
            }
        }
        return {CommandStatus::Failed, "Drawable replacement rejected; previous state retained"};
    }
}

}
