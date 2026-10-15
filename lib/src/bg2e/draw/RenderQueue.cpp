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

#include <bg2e/draw/RenderQueue.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/scene/Node.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace bg2e::draw {
void RenderQueue::beginFrame(Engine& engine)
{
    clear();
    _engine = &engine;
}
void RenderQueue::enqueue(scene::Node& node, const std::shared_ptr<Drawable>& drawable, uint32_t submesh, const glm::mat4& world)
{
    if (!_engine || !drawable || !drawable->isLoaded() || drawable->engine() != _engine || !drawable->drawMesh())
    {
        throw std::invalid_argument("Draw queue requires loaded resources belonging to its engine");
    }
    if (submesh >= drawable->submeshesCount() || submesh >= drawable->drawMesh()->submeshCount())
    {
        throw std::out_of_range("Draw submesh topology requires a reload");
    }
    if (!drawable->drawMesh()->vertexBuffer() || !drawable->drawMesh()->indexBuffer() ||
        !drawable->drawMesh()->vertexBuffer()->isValid() || !drawable->drawMesh()->indexBuffer()->isValid() ||
        drawable->drawMesh()->vertexBuffer()->device() != _engine->device() || drawable->drawMesh()->indexBuffer()->device() != _engine->device())
    {
        throw std::invalid_argument("Draw mesh buffers are stale or belong to another device");
    }
    const auto material = drawable->drawMaterial(submesh);
    if (!material || material->engine() != _engine)
    {
        throw std::invalid_argument("Draw material belongs to an incompatible context");
    }
    if (!drawable->submeshVisibility(submesh) || !material->materialAttributes().visible())
    {
        return;
    }
    for (int c = 0; c < 4; ++c)
    {
        for (int r = 0; r < 4; ++r)
        {
            if (!std::isfinite(world[c][r]))
            {
                throw std::invalid_argument("Draw transform must be finite");
            }
        }
    }
    const float determinant = glm::determinant(world);
    if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f ||
        world[0][3] != 0 || world[1][3] != 0 || world[2][3] != 0 || world[3][3] != 1)
    {
        throw std::invalid_argument("Draw transform must be affine and invertible");
    }
    if (_items.size() >= 0x00ffffffu)
    {
        throw std::length_error("Draw object IDs exceed the portable 24-bit instance range");
    }
    Item item;
    item.node = node.weak_from_this();
    item.nodeId = node.identifier();
    item.objectId = uint32_t(_items.size()) + 1;
    item.submeshIndex = submesh;
    item.drawMesh = drawable->drawMesh();
    item.drawMaterial = material;
    item.materialData = packMaterial(*material);
    item.worldMatrix = world;
    item.rayTracingEnabled = drawable->rayTracingEnabled();
    if (submesh < drawable->rayTracingMeshes().size())
    {
        item.tracingMesh = drawable->rayTracingMesh(submesh);
    }
    for (size_t i = 0; i < item.textures.size(); ++i)
    {
        item.textures[i] = material->texture(TextureSlot(i));
        item.textureImages[i] = item.textures[i]->image();
        item.textureSamplers[i] = item.textures[i]->sampler();
        if (item.textures[i]->engine() != _engine || !item.textures[i]->image()->isValid() ||
            item.textures[i]->image()->device() != _engine->device() || item.textures[i]->image()->isCubemap() ||
            gpu::isDepthFormat(item.textures[i]->image()->pixelFormat()) || gpu::isUnsignedIntegerFormat(item.textures[i]->image()->pixelFormat()) ||
            gpu::isSignedIntegerFormat(item.textures[i]->image()->pixelFormat()) || !item.textures[i]->sampler()->isValid() ||
            item.textures[i]->sampler()->device() != _engine->device() ||
            item.textures[i]->image()->currentLayout() != gpu::ImageLayout::ShaderReadOnly)
        {
            throw std::invalid_argument("Draw textures must be ready on the same device");
        }
    }
    const auto& a = material->materialAttributes();
    _items.push_back(item);
    if (a.isTransparent() && a.isSolid())
    {
        _solidTransparent.push_back(std::move(item));
    }
    else if (a.isTransparent())
    {
        _transparent.push_back(std::move(item));
    }
    else
    {
        _opaque.push_back(std::move(item));
    }
}
void RenderQueue::validate(Engine& engine) const
{
    if (_engine != &engine)
    {
        throw std::invalid_argument("Draw queue belongs to another engine");
    }
    for (const auto& item : _items)
    {
        if (!item.drawMesh || !item.drawMesh->vertexBuffer() || !item.drawMesh->indexBuffer() ||
            !item.drawMesh->vertexBuffer()->isValid() || !item.drawMesh->indexBuffer()->isValid())
        {
            throw std::logic_error("Draw queue geometry is stale");
        }
        for (size_t i = 0; i < item.textures.size(); ++i)
        {
            const auto& texture = item.textures[i];
            if (!texture || texture->image() != item.textureImages[i] || texture->sampler() != item.textureSamplers[i] ||
                !texture->image() || !texture->sampler() || !texture->image()->isValid() || !texture->sampler()->isValid() ||
                texture->image()->currentLayout() != gpu::ImageLayout::ShaderReadOnly)
            {
                throw std::logic_error("Re-extract the draw queue after texture/sampler realization changes");
            }
        }
    }
}
void RenderQueue::sort(const glm::vec3& cameraPosition)
{
    if (!std::isfinite(cameraPosition.x) || !std::isfinite(cameraPosition.y) || !std::isfinite(cameraPosition.z))
    {
        throw std::invalid_argument("Camera position must be finite");
    }
    auto backToFront = [&](const Item& a, const Item& b)
    {
        const auto da = glm::vec3(a.worldMatrix[3]) - cameraPosition;
        const auto db = glm::vec3(b.worldMatrix[3]) - cameraPosition;
        return glm::dot(da, da) > glm::dot(db, db);
    };
    std::stable_sort(_transparent.begin(), _transparent.end(), backToFront);
    std::stable_sort(_solidTransparent.begin(), _solidTransparent.end(), backToFront);
}
const std::vector<RenderQueue::Item>& RenderQueue::items() const
{
    return _items;
}
const std::vector<RenderQueue::Item>& RenderQueue::queue(RenderQueueType type) const
{
    switch (type)
    {
    case RenderQueueType::Opaque: return _opaque;
    case RenderQueueType::Transparent: return _transparent;
    case RenderQueueType::SolidTransparent: return _solidTransparent;
    }
    throw std::invalid_argument("Unknown draw queue type");
}
Engine* RenderQueue::engine() const
{
    return _engine;
}
LightUniforms& RenderQueue::lights()
{
    return _lights;
}
const LightUniforms& RenderQueue::lights() const
{
    return _lights;
}
void RenderQueue::setEnvironment(std::shared_ptr<const scene::EnvironmentComponent> environment)
{
    _environment = std::move(environment);
}
std::shared_ptr<const scene::EnvironmentComponent> RenderQueue::environment() const
{
    return _environment;
}
void RenderQueue::clear()
{
    _items.clear();
    _opaque.clear();
    _transparent.clear();
    _solidTransparent.clear();
    _lights = {};
    _environment.reset();
    _engine = nullptr;
}
}
