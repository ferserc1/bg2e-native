/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/draw/all.hpp>
#include <bg2e/draw/pipeline/TracingSceneResources.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/Queue.hpp>
#include <bg2e/scene/Node.hpp>
#include <algorithm>
#include <stdexcept>

namespace bg2e::draw::tests {
// Stopped initialized engine, with no background submitters. Builds real TLAS
// generations; false denotes missing capabilities, never a passing native test.
inline bool runAccelerationReuseProbe(Engine& engine)
{
    using namespace pipeline;
    auto& device = *engine.device();
    const auto capacity = std::min(6u, device.resourceArrayCapabilities().profileCapacity(2,1,10));
    if (!device.rayTracingCapabilities().accelerationStructure || !capacity)
    {
        return false;
    }
    const auto require = [](bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    };
    auto drawable = std::make_shared<Drawable>();
    drawable->setMesh(geo::createPlane(2,2));
    drawable->load(&engine);
    auto node = std::make_shared<scene::Node>("acceleration generation probe");
    RenderQueue queue;
    const auto extract = [&](float x)
    {
        queue.beginFrame(engine);
        auto transform = glm::mat4(1);
        transform[3].x = x;
        queue.enqueue(*node, drawable, 0, transform);
    };
    TracingSceneResources resources(device, capacity);
    extract(0);
    const auto first = resources.prepare(queue, capacity);
    const auto notRecorded = resources.prepare(queue, capacity);
    require(first->scene != notRecorded->scene, "Unrecorded TLAS must not be reusable");
    auto abandoned = device.graphicsQueue().createCommandBuffer("abandoned tracing build");
    abandoned->begin();
    first->build(*abandoned);
    abandoned->end();
    const auto abandonedTicket = abandoned->recordingCompletion();
    require(abandonedTicket && !abandonedTicket->completed(), "Recording must not certify submission completion");
    require(resources.prepare(queue, capacity)->scene != first->scene, "Build-recorded TLAS must not enter completed reuse");
    abandoned.reset();
    require(!abandonedTicket->completed(), "Destroyed unsubmitted recordings must remain incomplete");

    const auto built = resources.prepare(queue, capacity);
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        built->build(*command);
        require(resources.prepare(queue, capacity)->scene != built->scene,
            "A build inside an unsubmitted immediate callback is still pending");
    });
    const auto warm = resources.prepare(queue, capacity);
    require(warm->scene == built->scene && warm->instanceBuffer == built->instanceBuffer &&
        warm->instances.data() == built->instances.data(), "Completed unchanged scene must reuse acceleration and instance storage");
    // New wrappers still transition their own current material resources.
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        warm->build(*command);
    });
    const auto layout = device.createPipelineLayout(tracingBindingLayout(capacity));
    require(bool(warm->resourceSet(*layout)), "Completed reused scene must bind in a fresh recording");

    drawable->material().setRoughness(0.321f);
    extract(0);
    const auto scalar = resources.prepare(queue, capacity);
    require(scalar->scene == built->scene && scalar->instanceBuffer == built->instanceBuffer &&
        scalar->materialBuffer != built->materialBuffer, "Scalar edits must preserve acceleration while versioning material buffers");
    drawable->drawMaterial(0)->albedoTexture()->reload();
    extract(0);
    const auto texture = resources.prepare(queue, capacity);
    require(texture->scene == built->scene && texture->instanceBuffer == built->instanceBuffer,
        "Texture replacement without an opacity/address change must preserve acceleration");
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        texture->build(*command);
    });
    require(texture->materialResourceSet(*layout) != warm->materialResourceSet(*layout),
        "Reused acceleration must still bind the new texture generation");

    const auto blas = drawable->rayTracingMesh(0);
    const auto blasGeneration = blas->buildGeneration();
    extract(3);
    const auto moved = resources.prepare(queue, capacity);
    require(moved->scene != built->scene && moved->instanceBuffer != built->instanceBuffer &&
        moved->owners.front().item.tracingMesh == blas && blas->buildGeneration() == blasGeneration &&
        moved->instances.front().model[3].x == 3 && built->instances.front().model[3].x == 0,
        "Transform changes must build new TLAS storage without rebuilding or changing BLAS");
    drawable->material().setAlphaCutoff(AlphaTestCutoffThreshold);
    extract(0);
    const auto cutout = resources.prepare(queue, capacity);
    require(cutout->scene != built->scene && cutout->scene->instances().front().opacity == gpu::RayTracingOpacity::ForceNonOpaque,
        "Opacity threshold crossings must invalidate acceleration");
    drawable->material().setAlphaCutoff(0);
    extract(0);
    require(resources.prepare(queue, capacity)->scene == built->scene,
        "Returning to a retained complete key may reuse its immutable acceleration");

    auto otherNode = std::make_shared<scene::Node>("second correspondence");
    queue.enqueue(*otherNode, drawable, 0, glm::mat4(1));
    const auto added = resources.prepare(queue, capacity);
    require(added->scene != built->scene && added->instances.size() == 2,
        "Membership changes must invalidate acceleration");
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        added->build(*command);
    });
    queue.beginFrame(engine);
    queue.enqueue(*otherNode, drawable, 0, glm::mat4(1));
    queue.enqueue(*node, drawable, 0, glm::mat4(1));
    const auto reordered = resources.prepare(queue, capacity);
    require(reordered->scene != added->scene, "Ordered node correspondence must distinguish identical mesh/transform instances");

    queue.beginFrame(engine);
    queue.enqueue(*node, drawable, 0, glm::mat4(1));
    drawable->material().setRoughness(0.654f);
    queue.enqueue(*otherNode, drawable, 0, glm::mat4(1));
    const auto addressed = resources.prepare(queue, capacity);
    require(addressed->scene != added->scene && addressed->instances[0].ranges.w == 0 && addressed->instances[1].ranges.w == 1,
        "Material addressing changes must invalidate shared instance storage");

    resources.setAccelerationReuseEnabled(false);
    extract(0);
    const auto baseline = resources.prepare(queue, capacity);
    require(baseline->scene != built->scene && baseline->instanceBuffer != built->instanceBuffer &&
        baseline->instances.front().ranges == built->instances.front().ranges,
        "Diagnostic fallback must retain full-build correctness");
    resources.setAccelerationReuseEnabled(true);
    drawable->material().setVisible(false);
    extract(0);
    require(resources.prepare(queue, capacity)->empty(), "Visibility exclusion must clear scene cache ownership");
    drawable->material().setVisible(true);
    extract(0);
    const auto afterEmpty = resources.prepare(queue, capacity);
    require(afterEmpty->scene != built->scene && built->scene->isValid(),
        "Empty scenes must evict caches while preserving borrowed snapshots");
    resources.clear();
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        afterEmpty->build(*command);
    });
    require(afterEmpty->scene->isValid(), "Cache clearing must preserve pending snapshot owners");
    const auto beforeRebuild = resources.prepare(queue, capacity);
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        beforeRebuild->build(*command);
    });
    require(resources.prepare(queue, capacity)->scene == beforeRebuild->scene,
        "BLAS invalidation fixture must start from a reusable completed generation");
    const auto priorGeneration = blas->buildGeneration();
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        command->buildRayTracingMesh(blas.get());
    });
    const auto rebuiltBlas = resources.prepare(queue, capacity);
    require(blas->buildGeneration() != priorGeneration && rebuiltBlas->scene != beforeRebuild->scene,
        "A native BLAS rebuild must invalidate cached scene generations at the same object address");
    drawable->setMesh(geo::createPlane(4,4));
    drawable->load(&engine);
    extract(0);
    const auto reloaded = resources.prepare(queue, capacity);
    require(reloaded->geometry != rebuiltBlas->geometry && reloaded->scene != rebuiltBlas->scene &&
        afterEmpty->scene->isValid(), "Geometry reload must version acceleration without destroying retained scenes");
    queue.beginFrame(engine);
    resources.clear();
    node.reset();
    drawable.reset();
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        reloaded->build(*command);
    });
    require(reloaded->scene->isValid(), "Source destruction must not invalidate independently retained scene generations");
    return true;
}
}
