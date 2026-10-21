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
#include <bg2e/scene/Node.hpp>
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace bg2e::draw::tests {
// Stopped initialized engine; returns false when the backend cannot run this
// fixture. This checks CPU tables backed by real GPU resources, not rendered pixels.
inline bool runTracingTableProbe(Engine& engine)
{
    using namespace pipeline;
    auto& device = *engine.device();
    const auto available = device.resourceArrayCapabilities().profileCapacity(2, 1, 10);
    if (!device.rayTracingCapabilities().accelerationStructure || available < 6)
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
    auto mesh = std::make_shared<Drawable::Mesh>();
    mesh->vertices = {{{0,0,0},{0,0,1},{0,0},{0.2f,0.3f},{1,0,0}},
        {{1,0,0},{0,0,1},{1,0},{0.6f,0.3f},{1,0,0}},
        {{0,1,0},{0,0,1},{0,1},{0.2f,0.8f},{1,0,0}}};
    // Nonzero submesh firstIndex exercises the packed index-base distinction.
    mesh->indices = {0,1,2, 2,1,0};
    mesh->submeshes = {{3,3}};
    auto a = std::make_shared<Drawable>();
    a->setMesh(mesh);
    a->material().setAlphaCutoff(0.5f);
    a->material().setAlbedoUVSet(1);
    a->material().setAlbedoScale(glm::vec2(2,3));
    a->load(&engine);
    for (uint32_t i = 0; i < 6; ++i)
    {
        // Give both sources the same explicit texture-presence flags as well
        // as the same native resources, including the AO slot.
        a->drawMaterial(0)->setTexture(TextureSlot(i), a->drawMaterial(0)->texture(TextureSlot(i)));
    }
    auto b = a->clone();
    for (uint32_t i = 0; i < 6; ++i)
    {
        // Explicit native resource sharing must not merge editable materials.
        b->drawMaterial(0)->setTexture(TextureSlot(i), a->drawMaterial(0)->texture(TextureSlot(i)));
    }
    auto first = std::make_shared<scene::Node>("independent B");
    auto second = std::make_shared<scene::Node>("reference A1");
    auto third = std::make_shared<scene::Node>("reference A2");
    RenderQueue queue;
    const auto extract = [&]()
    {
        queue.beginFrame(engine);
        queue.enqueue(*first, b, 0, glm::mat4(1));
        queue.enqueue(*second, a, 0, glm::mat4(1));
        queue.enqueue(*third, a, 0, glm::mat4(1));
    };
    TracingSceneResources resources(device, 6);
    b->material().setIsTransparent(true);
    extract();
    const auto omitted = resources.prepare(queue, 2);
    require(omitted->instances.size() == 2 && omitted->materials.size() == 1 &&
        omitted->instances[0].identity.x == 2 && omitted->instances[1].identity.x == 3 &&
        omitted->instances[0].ranges.w == 0 && omitted->instances[1].ranges.w == 0,
        "Transparent exclusion or repeated material identity mapping differs");

    b->material().setIsTransparent(false);
    extract();
    const auto snapshot = resources.prepare(queue, 2);
    require(snapshot->instances.size() == 3 && snapshot->materials.size() == 2 && snapshot->geometry->owners.size() == 2,
        "Distinct equal-valued materials must stay separate; repeated identities must share one entry");
    require(std::memcmp(&snapshot->materials[0].values, &snapshot->materials[1].values, sizeof(MaterialData)) == 0,
        "Fixture must use equal-valued independent materials");
    for (uint32_t i = 0; i < 3; ++i)
    {
        require(snapshot->scene->instances()[i].instanceId == i && snapshot->instances[i].identity.x == i+1,
            "Dense TLAS indices must remain separate from canonical identities");
    }
    require(snapshot->instances[0].ranges.w == 0 && snapshot->instances[1].ranges.w == 1 && snapshot->instances[2].ranges.w == 1,
        "Material addresses must follow first encounter order");
    require(snapshot->instances[1].ranges.x == mesh->indices.size()+3 && snapshot->instances[1].ranges.y == mesh->vertices.size() &&
        snapshot->instances[1].ranges == snapshot->instances[2].ranges,
        "Repeated mesh lookup lost nonzero packed geometry bases or submesh ranges");
    require(snapshot->images.front().get() == b->drawMaterial(0)->albedoTexture()->image() &&
        snapshot->samplers.front().get() == b->drawMaterial(0)->albedoTexture()->sampler() && !snapshot->fallbackTextureSlots,
        "Resource first-admission order or exact-capacity admission differs");
    require(snapshot->shadowTable.alphaTextures.size() == 1 && !snapshot->shadowTable.fallbackTextureSlots,
        "Explicitly shared alpha pairs must reuse the same shadow slot");
    const auto stable = resources.prepare(queue, 2);
    require(stable->geometry == snapshot->geometry && stable->materials.size() == snapshot->materials.size() &&
        std::memcmp(stable->materials.data(), snapshot->materials.data(), snapshot->materials.size()*sizeof(TracingMaterialData)) == 0,
        "Repeated construction must produce deterministic material tables");

    require(stable->materialBuffer == snapshot->materialBuffer && stable->shadowInstances == snapshot->shadowInstances &&
        stable->materials.data() == snapshot->materials.data() && stable->images.data() == snapshot->images.data() &&
        stable->instanceBuffer != snapshot->instanceBuffer && stable->scene != snapshot->scene,
        "Warm snapshots must retain immutable tables without reusing per-frame instance/TLAS state");
    RenderQueue movedQueue;
    movedQueue.beginFrame(engine);
    auto moved = glm::mat4(1);
    moved[3].x = 4;
    movedQueue.enqueue(*first, b, 0, moved);
    movedQueue.enqueue(*second, a, 0, glm::mat4(1));
    movedQueue.enqueue(*third, a, 0, glm::mat4(1));
    const auto transformed = resources.prepare(movedQueue, 2);
    require(transformed->materialBuffer == snapshot->materialBuffer && transformed->shadowInstances == snapshot->shadowInstances &&
        transformed->instances.front().model[3].x == 4 && snapshot->instances.front().model[3].x == 0,
        "Transform edits must preserve material and alpha generations without mutating previous instances");

    RenderQueue revisionsQueue;
    revisionsQueue.beginFrame(engine);
    revisionsQueue.enqueue(*second, a, 0, glm::mat4(1));
    const float originalRoughness = a->material().roughness();
    a->material().setRoughness(0.456f);
    revisionsQueue.enqueue(*third, a, 0, glm::mat4(1));
    TracingSceneResources revisionsResources(device, 6);
    const auto revisions = revisionsResources.prepare(revisionsQueue);
    require(revisions->materials.size() == 2 && revisions->instances[0].ranges.w == 0 && revisions->instances[1].ranges.w == 1 &&
        revisions->materials[0].values.factors.y == originalRoughness && revisions->materials[1].values.factors.y == 0.456f,
        "Different captured effective revisions of one entity must retain distinct derived entries");
    a->material().setRoughness(originalRoughness);

    b->material().setRoughness(0.123f);
    extract();
    const auto edited = resources.prepare(queue, 2);
    require(edited->geometry == snapshot->geometry && edited->materials[0].values.factors.y == 0.123f &&
        edited->materials[1].values.factors.y == snapshot->materials[1].values.factors.y &&
        snapshot->materials[0].values.factors.y != 0.123f,
        "Independent material edits must not change A or retained snapshots");
    require(edited->materialBuffer != snapshot->materialBuffer && edited->shadowInstances == snapshot->shadowInstances &&
        edited->images.data() == snapshot->images.data(),
        "Scalar edits must create immutable material buffers while retaining native resource tables and alpha metadata");
    const auto layout = device.createPipelineLayout(tracingBindingLayout(6));
    const auto shadowLayout = device.createPipelineLayout(softShadowBindingLayout(2));
    device.immediateSubmit([&](gpu::CommandBuffer* command)
    {
        snapshot->build(*command);
        edited->build(*command);
    });
    const auto arrays = snapshot->materialResourceSet(*layout);
    const auto alphaArrays = snapshot->shadowAlphaResourceSet(shadowLayout, 2);
    require(arrays == edited->materialResourceSet(*layout) &&
        alphaArrays == edited->shadowAlphaResourceSet(shadowLayout, 2) &&
        snapshot->resourceSet(*layout) != edited->resourceSet(*layout),
        "Scalar edits must reuse complete array sets independently of changing scene sets");
    require(arrays != edited->materialResourceSet(*layout, 6, false) && arrays == edited->materialResourceSet(*layout),
        "Fresh diagnostic arrays must not replace the persistent cache");
    resources.setPersistenceEnabled(false);
    const auto baseline = resources.prepare(queue, 2);
    require(baseline->materialBuffer != edited->materialBuffer && baseline->shadowInstances != edited->shadowInstances &&
        std::memcmp(baseline->materials.data(), edited->materials.data(), edited->materials.size()*sizeof(TracingMaterialData)) == 0,
        "Diagnostic generations must rebuild identical table contents");
    resources.setPersistenceEnabled(true);
    const auto restored = resources.prepare(queue, 2);
    RenderQueue reorderedQueue;
    reorderedQueue.beginFrame(engine);
    reorderedQueue.enqueue(*second, a, 0, glm::mat4(1));
    reorderedQueue.enqueue(*first, b, 0, glm::mat4(1));
    const auto reordered = resources.prepare(reorderedQueue, 2);
    require(reordered->instances[0].ranges.w == 0 && reordered->instances[1].ranges.w == 1 &&
        reordered->materials[1].values.factors.y == 0.123f && reordered->shadowInstances != restored->shadowInstances,
        "Membership/order changes must regenerate dense metadata and material mappings");
    for (size_t i = 0; i < reordered->instances.size(); ++i)
    {
        require(reordered->shadowTable.instances[i].ranges.w == reordered->instances[i].identity.x &&
            reordered->shadowTable.instances[i].ranges.x == reordered->instances[i].ranges.x &&
            reordered->shadowTable.instances[i].options.z == reordered->instances[i].ranges.y,
            "Reordered shadow metadata must match the new dense geometry mapping");
    }
    resources.prepare(queue, 2);
    TracingSceneResources bounded(device, 1);
    const auto overflow = bounded.prepare(queue);
    uint64_t missing = 0;
    for (const auto& instance : overflow->instances)
    {
        const auto& material = overflow->materials.at(instance.ranges.w);
        for (uint32_t i = 0; i < 6; ++i)
        {
            const auto image = material.images[i/4][i%4];
            const auto sampler = material.samplers[i/4][i%4];
            require((image == UINT32_MAX) == (sampler == UINT32_MAX), "Resource pairs must be admitted atomically");
            if (image == UINT32_MAX)
            {
                ++missing;
            }
            else
            {
                require(image < overflow->images.size() && sampler < overflow->samplers.size(), "Descriptor address exceeds capacity");
            }
        }
    }
    require(missing > 0 && missing == overflow->fallbackTextureSlots && overflow->instances.size() == 3 &&
        overflow->materials.size() == 2 && overflow->images.size() == 1 && overflow->samplers.size() == 1,
        "Overflow must preserve instances and per-reference fallback accounting");

    RenderQueue subset;
    subset.beginFrame(engine);
    subset.enqueue(*second, a, 0, glm::mat4(1));
    const auto beforeFailure = resources.prepare(queue, 2);
    bool rejected = false;
    try
    {
        resources.prepare(subset, MaxShadowAlphaTextures+1);
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    const auto afterFailure = resources.prepare(queue, 2);
    require(rejected && afterFailure->materialBuffer == beforeFailure->materialBuffer &&
        afterFailure->shadowInstances == beforeFailure->shadowInstances && afterFailure->geometry == beforeFailure->geometry,
        "Failed preparation must preserve cached material/shadow generations");

    b->drawMaterial(0)->setTexture(TextureSlot::Albedo, a->drawMaterial(0)->albedoTexture()->clone());
    extract();
    CollectRayTracingInstancesVisitor collection;
    collection.collect(queue);
    const auto shadowOverflow = buildShadowQueryTable(collection, 1);
    require(shadowOverflow.alphaTextures.size() == 1 && shadowOverflow.fallbackTextureSlots == 2 &&
        shadowOverflow.instances[0].ranges.z == 0 && shadowOverflow.instances[1].ranges.z == UINT32_MAX &&
        shadowOverflow.instances[2].ranges.z == UINT32_MAX,
        "Shadow pair overflow must preserve deterministic first-admission order");

    a->drawMaterial(0)->albedoTexture()->reload();
    rejected = false;
    try
    {
        resources.prepare(queue);
    }
    catch (const std::logic_error&)
    {
        rejected = true;
    }
    require(rejected, "Resource replacement must reject stale queue snapshots");
    extract();
    require(resources.prepare(queue)->materials.size() == 2 && snapshot->images.front()->isValid(),
        "Resource replacement must preserve retained snapshots and source identities");
    queue.beginFrame(engine);
    const auto empty = resources.prepare(queue, 2);
    require(empty->empty() && empty->device == &device && empty->capacity == 6 && !empty->scene && !empty->materialBuffer && !empty->instanceBuffer && empty->materials.empty(),
        "Empty scenes must allocate no tracing table buffers");
    resources.clear();
    require(snapshot->materials.size() == 2 && snapshot->geometry->vertexBuffer->isValid(), "Cache clearing must preserve retained snapshots");

    // Fill only the sampler table. A rejected pair must not consume the free
    // image slot, even when its image would otherwise fit.
    auto atomic = a->clone();
    const auto albedo = a->drawMaterial(0)->albedoTexture();
    auto alternateSampler = std::make_shared<Texture>(&engine);
    alternateSampler->load(std::make_shared<base::Texture>(*albedo->base()), albedo->imageResource());
    for (uint32_t i = 0; i < 6; ++i)
    {
        atomic->drawMaterial(0)->setTexture(TextureSlot(i), albedo);
    }
    atomic->drawMaterial(0)->setTexture(TextureSlot::Metalness, alternateSampler);
    atomic->drawMaterial(0)->setTexture(TextureSlot::Roughness, a->drawMaterial(0)->roughnessTexture());
    queue.beginFrame(engine);
    queue.enqueue(*second, atomic, 0, glm::mat4(1));
    TracingSceneResources asymmetric(device, 2);
    const auto atomicTable = asymmetric.prepare(queue);
    require(atomicTable->images.size() == 1 && atomicTable->samplers.size() == 2 && atomicTable->fallbackTextureSlots == 1 &&
        atomicTable->materials[0].images[0].z == UINT32_MAX && atomicTable->materials[0].samplers[0].z == UINT32_MAX,
        "Sampler overflow must not admit an orphan image");
    return true;
}
}
