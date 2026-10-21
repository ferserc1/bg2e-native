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

#include <bg2e/draw/CollectRayTracingInstancesVisitor.hpp>
#include <bg2e/gpu/Device.hpp>
#include <stdexcept>
#include <utility>

namespace bg2e::draw {

void CollectRayTracingInstancesVisitor::collect(const RenderQueue& queue, RayTracingCollectionOptions options)
{
    if (!queue.engine())
    {
        throw std::invalid_argument("Initialize the raster queue before collecting query instances");
    }
    queue.validate(*queue.engine());
    std::vector<Instance> next;
    if (queue.engine()->device()->rayTracingCapabilities().accelerationStructure)
    {
        for (const auto& item : queue.items())
        {
            const uint32_t flags = item.materialData.uvSets[1].z;
            const bool transparent = (flags & MaterialTransparent) != 0;
            const bool solid = (flags & MaterialSolid) != 0;
            if (!item.rayTracingEnabled || (transparent && !(solid ? options.includeSolidTransparent : options.includeTransparent)))
            {
                continue;
            }
            if (!item.tracingMesh || !item.tracingMesh->isBuilt() || item.tracingMesh->device() != queue.engine()->device() ||
                item.tracingMesh->description().vertexBuffer != item.drawMesh->vertexBuffer() ||
                item.tracingMesh->description().indexBuffer != item.drawMesh->indexBuffer() ||
                item.tracingMesh->description().firstIndex != item.drawMesh->meshData().submeshes.at(item.submeshIndex).firstIndex ||
                item.tracingMesh->description().indexCount != item.drawMesh->meshData().submeshes.at(item.submeshIndex).indexCount)
            {
                throw std::logic_error("Reload enabled tracing geometry before collecting instances");
            }
            if (next.size() >= options.maxInstances)
            {
                throw std::length_error("Ray instance capacity exceeded; extraction does not silently truncate");
            }
            const auto opacity = (flags & MaterialAlphaTest) != 0 ?
                gpu::RayTracingOpacity::ForceNonOpaque : gpu::RayTracingOpacity::ForceOpaque;
            next.push_back({item, {item.tracingMesh.get(), item.worldMatrix, item.objectId, 0xff, opacity}});
        }
    }
    _engine = queue.engine();
    _instances = std::move(next);
}

void CollectRayTracingInstancesVisitor::collect(scene::Node* root, Engine& engine, RayTracingCollectionOptions options)
{
    RenderQueue queue;
    RenderQueueVisitor visitor;
    visitor.enqueue(root, engine, queue);
    collect(queue, options);
}

const std::vector<CollectRayTracingInstancesVisitor::Instance>& CollectRayTracingInstancesVisitor::instances() const
{
    return _instances;
}

void CollectRayTracingInstancesVisitor::populate(gpu::RayTracingScene& scene) const
{
    if (!_engine || scene.device() != _engine->device())
    {
        throw std::invalid_argument("Ray scene belongs to another device");
    }
    scene.clearInstances();
    for (const auto& item : _instances)
    {
        scene.addInstance(item.instance.mesh, item.instance.transform, item.instance.instanceId, item.instance.mask, item.instance.opacity);
    }
}
}
