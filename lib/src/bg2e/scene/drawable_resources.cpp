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

#include <bg2e/scene/drawable_resources.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/gpu/Common.hpp>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace bg2e::scene {
namespace {

void collect(const Component& component, draw::DrawableUse use, const std::string& role, DrawableResourceRetention& result,
    const std::shared_ptr<draw::DrawableDependencies>& dependencies)
{
    if (const auto* renderable = dynamic_cast<const DrawableRenderComponent*>(&component))
    {
        for (uint32_t sub = 0; sub < renderable->submeshesCount(); ++sub)
        {
            draw::appendMaterialDependencies(renderable->effectiveMaterial(sub), *dependencies);
            if (renderable->resourcesLoaded())
            {
                if (const auto material = renderable->drawMaterial(sub))
                {
                    draw::appendMaterialDependencies(material->materialAttributes(), *dependencies);
                }
            }
        }
    }
    draw::DrawableResourceHandle handle;
    if (const auto* owner = dynamic_cast<const DrawableComponent*>(&component); owner && owner->isSceneGeometry())
    {
        handle = owner->resourceHandle();
    }
    else if (const auto* instance = dynamic_cast<const DrawableInstanceComponent*>(&component))
    {
        handle = instance->resourceHandle();
    }
    if (!handle.valid() || std::find(result.identities.begin(), result.identities.end(), handle) != result.identities.end())
    {
        return;
    }
    const auto context = handle.context();
    if (!context || !context->active())
    {
        throw std::runtime_error("Drawable resource context is no longer active");
    }
    auto retained = context->retain(handle, use, role + ":" + handle.identifier(), {}, [dependencies]()
    {
        return *dependencies;
    });
    if (!retained)
    {
        throw std::runtime_error(retained.diagnostic.message);
    }
    result.identities.push_back(handle);
    result.leases.push_back(std::move(retained.lease));
}


void collect(const Node& root, draw::DrawableUse use, const std::string& role, DrawableResourceRetention& result,
    const std::shared_ptr<draw::DrawableDependencies>& dependencies)
{
    for (const auto& component : root.components())
    {
        collect(*component.second, use, role, result, dependencies);
    }
    for (const auto& child : root.children())
    {
        collect(*child, use, role, result, dependencies);
    }
}


template <typename T>
DrawableResourceRetention retain(const T& value, draw::DrawableUse use, const std::string& role)
{
    DrawableResourceRetention result;
    try
    {
        gpu::requireDebugName(role);
        if (use != draw::DrawableUse::History && use != draw::DrawableUse::Job && use != draw::DrawableUse::Frame)
        {
            throw std::invalid_argument("Resource retention does not create scene owners or instances");
        }
        // Aggregate appearances before returning, even for repeated identities.
        // All identity-deduplicated leases share this immutable CPU snapshot.
        const auto dependencies = std::make_shared<draw::DrawableDependencies>();
        collect(value, use, role, result, dependencies);
    }
    catch (const std::exception& error)
    {
        result.leases.clear();
        result.identities.clear();
        result.diagnostic = {draw::DrawableCacheCode::ResourceFailed, error.what()};
    }
    catch (...)
    {
        result.leases.clear();
        result.identities.clear();
        result.diagnostic = {draw::DrawableCacheCode::ResourceFailed, "Cannot retain drawable resources"};
    }
    return result;
}

}


void collectDrawableCacheContexts(const bg2e::scene::Node& node, std::vector<std::shared_ptr<bg2e::draw::DrawableCache>>& contexts)
{
    const auto append = [&](const auto& context)
    {
        if (context && context->active() && std::find(contexts.begin(), contexts.end(), context) == contexts.end())
        {
            contexts.push_back(context);
        }
    };
    append(node.drawableCacheContext());
    for (const auto& context : node.retainedDrawableCacheContexts())
    {
        append(context);
    }
    for (const auto& component : node.orderedComponents())
    {
        if (const auto* owner = dynamic_cast<const bg2e::scene::DrawableComponent*>(component.get()); owner && owner->isSceneGeometry())
        {
            append(owner->resourceHandle().context());
        }
        else if (const auto* instance = dynamic_cast<const bg2e::scene::DrawableInstanceComponent*>(component.get()))
        {
            append(instance->resourceHandle().context());
        }
    }
    for (const auto& child : node.children())
    {
        collectDrawableCacheContexts(*child, contexts);
    }
}



DrawableResourceRetention retainDrawableResources(const Component& component, draw::DrawableUse use, const std::string& debugRole)
{
    return retain(component, use, debugRole);
}


DrawableResourceRetention retainDrawableResources(const Node& root, draw::DrawableUse use, const std::string& debugRole)
{
    return retain(root, use, debugRole);
}

}
