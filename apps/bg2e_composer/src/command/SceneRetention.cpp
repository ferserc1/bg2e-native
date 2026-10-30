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

#include "SceneRetention.hpp"
#include "../StageScene.hpp"

#include <bg2e/scene/PolarTransformController.hpp>
#include <bg2e/scene/FixedScaleTransformController.hpp>
#include <bg2e/scene/ChainJoint.hpp>
#include <bg2e/manipulation/GizmoComponent.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <limits>
#include <bg2e/plugin/PluginManager.hpp>
#include <set>
#include <array>
#include <stdexcept>
#include <typeinfo>

namespace bg2e_composer::command {
namespace {
size_t add(size_t a, size_t b)
{
    if (b > std::numeric_limits<size_t>::max() - a)
    {
        throw std::length_error("Retained scene memory overflow");
    }
    return a + b;
}

size_t multiply(size_t a, size_t b)
{
    if (a && b > std::numeric_limits<size_t>::max() / a)
    {
        throw std::length_error("Retained scene memory overflow");
    }
    return a * b;
}

size_t drawableBytes(const std::shared_ptr<bg2e::draw::Drawable>& drawable)
{
    if (!drawable)
    {
        return 0;
    }
    size_t bytes = sizeof(*drawable);
    if (const auto& mesh = drawable->mesh())
    {
        bytes = add(bytes, multiply(mesh->vertices.capacity(), sizeof(bg2e::geo::VertexPNUUT)));
        bytes = add(bytes, multiply(mesh->indices.capacity(), sizeof(uint32_t)));
        bytes = add(bytes, multiply(mesh->submeshes.capacity(), sizeof(bg2e::geo::Submesh)));
        // Realization keeps a CPU geometry copy as well as native buffers.
        bytes = multiply(bytes, 3);
    }
    std::set<const bg2e::gpu::RayTracingMesh*> acceleration;
    for (const auto& blas : drawable->rayTracingMeshes())
    {
        if (!blas || !blas->isBuilt() || !drawable->drawMesh() ||
            blas->description().vertexBuffer != drawable->drawMesh()->vertexBuffer() ||
            blas->description().indexBuffer != drawable->drawMesh()->indexBuffer())
        {
            throw std::invalid_argument("Retained acceleration geometry must match its raster buffers");
        }
        if (acceleration.insert(blas.get()).second)
        {
            bytes = add(bytes, add(sizeof(*blas), blas->retainedBytes()));
        }
    }
    std::set<const bg2e::gpu::Image*> images;
    std::set<const void*> sources;
    using Attributes = bg2e::base::MaterialAttributes;
    using Getter = std::shared_ptr<bg2e::base::Texture> (Attributes::*)() const;
    const std::array<Getter, 6> getters{&Attributes::albedoTexture, &Attributes::metalnessTexture,
        &Attributes::roughnessTexture, &Attributes::normalTexture, &Attributes::aoTexture, &Attributes::lightEmissionTexture};
    for (uint32_t sub = 0; sub < drawable->submeshesCount(); ++sub)
    {
        bytes = add(bytes, 65536 + drawable->submeshName(sub).capacity() + drawable->submeshGroupName(sub).capacity());
        for (auto getter : getters)
        {
            const auto settings = (drawable->materialDefinition(sub).*getter)();
            if (settings && settings->sourceOwner() && sources.insert(settings->sourceOwner().get()).second)
            {
                bytes = add(bytes, settings->sourceBytes());
            }
        }
        if (!drawable->isLoaded())
        {
            continue;
        }
        for (unsigned slot = 0; slot < 6; ++slot)
        {
            auto texture = drawable->drawMaterial(sub)->texture(static_cast<bg2e::draw::TextureSlot>(slot));
            if (texture && texture->base()->sourceOwner() && sources.insert(texture->base()->sourceOwner().get()).second)
            {
                bytes = add(bytes, texture->base()->sourceBytes());
            }
            auto* image = texture ? texture->image() : nullptr;
            if (image && images.insert(image).second)
            {
                const auto texelBytes = bg2e::gpu::pixelFormatBytes(image->pixelFormat());
                if (!texelBytes)
                {
                    throw std::invalid_argument("The retained image format has no payload size contract");
                }
                size_t storage = 0;
                uint32_t width = image->width(), height = image->height();
                for (uint32_t level = 0; level < image->mipLevels(); ++level)
                {
                    storage = add(storage, multiply(multiply(width, height), texelBytes));
                    width = std::max(1u, width / 2);
                    height = std::max(1u, height / 2);
                }
                storage = multiply(storage, image->arrayLayers());
                if (texture->base()->type() == bg2e::base::Texture::TypeProcedural ||
                    texture->base()->imageFilePath().empty())
                {
                    // Owning pixel snapshots can keep decoded RGBA32F data in
                    // addition to the native image; filesystem sources do not.
                    storage = add(storage, multiply(multiply(image->width(), image->height()), 32));
                }
                bytes = add(bytes, add(storage, 65536));
            }
        }
    }
    return bytes;
}

size_t componentBytes(const std::shared_ptr<bg2e::scene::Component>& component, bool preparedEnvironment = false)
{
    if (!supportedComponent(*component))
    {
        throw std::invalid_argument("This component requires an explicit lifetime/memory/undo adapter");
    }
    if (auto environment = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(component);
        environment && !environment->environmentImage().empty() && !preparedEnvironment)
    {
        throw std::invalid_argument("Environment images require the retained environment resource adapter");
    }
    if (auto camera = std::dynamic_pointer_cast<bg2e::scene::CameraComponent>(component))
    {
        const auto* projection = camera->projection();
        if (projection && typeid(*projection) != typeid(bg2e::math::PerspectiveProjection) &&
            typeid(*projection) != typeid(bg2e::math::OpticalProjection) &&
            typeid(*projection) != typeid(bg2e::math::OrthographicProjection))
        {
            throw std::invalid_argument("Custom projections require a lifetime/restoration adapter");
        }
    }
    size_t bytes = 65536; // Built-in component, authored strings, metadata and tool buffers.
    if (auto environment = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(component))
    {
        bytes = add(bytes, environment->environmentImage().capacity());
    }
    if (auto drawable = std::dynamic_pointer_cast<bg2e::scene::DrawableComponent>(component))
    {
        bytes = add(bytes, drawableBytes(drawable->drawable()));
    }
    if (auto gizmo = std::dynamic_pointer_cast<bg2e::manipulation::GizmoComponent>(component))
    {
        for (const auto& cached : gizmo->retainedDrawables())
        {
            bytes = add(bytes, drawableBytes(cached));
        }
    }
    return bytes;
}

size_t nodeBytes(const std::shared_ptr<bg2e::scene::Node>& node, bool preparedEnvironment = false)
{
    const auto* instance = node.get();
    if (typeid(*instance) != typeid(bg2e::scene::Node))
    {
        throw std::invalid_argument("Custom nodes require a retained lifetime adapter");
    }
    size_t bytes = add(sizeof(*node), node->name().capacity() + node->identifier().capacity());
    for (const auto& component : node->orderedComponents())
    {
        bytes = add(bytes, componentBytes(component, preparedEnvironment));
    }
    for (const auto& child : node->children())
    {
        bytes = add(bytes, nodeBytes(child, preparedEnvironment));
    }
    return bytes;
}
}

bool supportedComponent(const bg2e::scene::Component& component)
{
    using namespace bg2e::scene;
    const auto& type = typeid(component);
    return type == typeid(TransformComponent) || type == typeid(DrawableComponent) ||
        type == typeid(CameraComponent) || type == typeid(LightComponent) ||
        type == typeid(EnvironmentComponent) || type == typeid(OrbitCameraComponent) ||
        type == typeid(PolarTransformControllerComponent) || type == typeid(FixedScaleTransformControllerComponent) ||
        type == typeid(InputChainJointComponent) || type == typeid(OutputChainJointComponent) ||
        type == typeid(bg2e::manipulation::SelectableComponent) || type == typeid(bg2e::manipulation::GizmoComponent);
}

void requireSupportedTree(const std::shared_ptr<bg2e::scene::Node>& node)
{
    const auto* instance = node.get();
    if (typeid(*instance) != typeid(bg2e::scene::Node))
    {
        throw std::invalid_argument("Custom nodes require a structural restoration adapter");
    }
    for (const auto& component : node->orderedComponents())
    {
        if (!supportedComponent(*component))
        {
            // No plugin object, weak control block or callback is retained by
            // an admitted command without a lifetime and memory adapter.
            throw std::invalid_argument("Custom/plugin components require a structural restoration adapter");
        }
        (void)componentBytes(component); // Validate native/resource eligibility before cloning.
    }
    for (const auto& child : node->children())
    {
        requireSupportedTree(child);
    }
}

SceneRetention::SceneRetention(std::shared_ptr<bg2e::scene::Node> value, bool preparedEnvironment)
    : _preparedEnvironment(preparedEnvironment), node(std::move(value))
{
    auto& plugins = bg2e::plugin::PluginManager::get();
    if (!plugins.plugins().empty())
    {
        _registryLease = plugins.registryReadLease();
    }
    if (!_preparedEnvironment)
    {
        requireSupportedTree(node);
    }
    // Prepared environment adoption separately accounts retained GPU snapshots.
    // nodeBytes still rejects custom components and unsupported native payloads.
    refreshBudget();
}

SceneRetention::SceneRetention(std::shared_ptr<bg2e::scene::Component> value) : component(std::move(value))
{
    auto& plugins = bg2e::plugin::PluginManager::get();
    if (!plugins.plugins().empty())
    {
        _registryLease = plugins.registryReadLease();
    }
    refreshBudget();
}

size_t SceneRetention::currentBytes() const
{
    return node ? nodeBytes(node, _preparedEnvironment) : componentBytes(component);
}

size_t SceneRetention::growthBytes() const
{
    const auto current = currentBytes();
    return current > bytes ? current - bytes : 0;
}

void SceneRetention::refreshBudget()
{
    bytes = currentBytes();
}

bool SceneRetention::detached() const
{
    return node ? !node->parent() && !node->scene() : component && !component->ownerNode();
}

void SceneRetention::release(StageScene& stage)
{
    if (node && detached())
    {
        stage.cleanupRetainedNode(node);
    }
    if (component && !component->ownerNode())
    {
        if (auto drawable = std::dynamic_pointer_cast<bg2e::scene::DrawableComponent>(component))
        {
            if (drawable->drawable())
            {
                drawable->drawable()->cleanup();
            }
        }
    }
}

}
