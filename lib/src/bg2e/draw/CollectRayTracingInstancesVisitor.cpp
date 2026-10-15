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
            next.push_back({item, {item.tracingMesh.get(), item.worldMatrix, item.objectId, 0xff}});
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
