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

#include <bg2e/draw/pipeline/TracingSceneResources.hpp>
#include "TracingPersistentResources.hpp"
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/base/Log.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace bg2e::draw::pipeline {
namespace {
void range(size_t count, size_t stride, uint64_t limit)
{
    if (count > UINT32_MAX || uint64_t(count) > limit / stride)
    {
        throw std::length_error("Packed tracing data exceeds uint32 addressing/storage-buffer range");
    }
}

void finite(const glm::vec4& value)
{
    for (uint32_t i = 0; i < 4; ++i)
    {
        if (!std::isfinite(value[i]))
        {
            throw std::invalid_argument("Tracing geometry/material/transform must be finite");
        }
    }
}

std::shared_ptr<gpu::Buffer> buffer(gpu::Device& device, const void* bytes, size_t size)
{
    auto result = device.createBuffer("draw immutable tracing snapshot data");
    result->createStorageBuffer(bytes,size);
    return result;
}
}

TracingSnapshot::TracingSnapshot()
    : TracingSnapshot(std::make_shared<TracingMaterialGeneration>(), std::make_shared<TracingShadowGeneration>())
{
}

TracingSnapshot::TracingSnapshot(std::shared_ptr<const TracingMaterialGeneration> materialGeneration,
    std::shared_ptr<const TracingShadowGeneration> shadowGeneration,
    std::shared_ptr<TracingAccelerationGeneration> accelerationGeneration)
    : _materialGeneration(std::move(materialGeneration)), _shadowGeneration(std::move(shadowGeneration)),
      _accelerationGeneration(accelerationGeneration ? std::move(accelerationGeneration) : std::make_shared<TracingAccelerationGeneration>()),
      instances(_accelerationGeneration->instances), materials(_materialGeneration->materials), images(_materialGeneration->resources->images),
      samplers(_materialGeneration->resources->samplers), instanceBuffer(_accelerationGeneration->buffer), materialBuffer(_materialGeneration->buffer),
      scene(_accelerationGeneration->scene), shadowTable(_shadowGeneration->table), shadowImages(_shadowGeneration->resources->images),
      shadowSamplers(_shadowGeneration->resources->samplers), shadowImageGenerations(_shadowGeneration->resources->generations),
      shadowFallbackImage(_shadowGeneration->resources->fallbackImage), shadowFallbackSampler(_shadowGeneration->resources->fallbackSampler),
      shadowFallbackGeneration(_shadowGeneration->resources->fallbackGeneration), shadowInstances(_shadowGeneration->buffer)
{
}

std::shared_ptr<gpu::ResourceSet> TracingSnapshot::materialResourceSet(gpu::PipelineLayout& layout,
    uint32_t bindingCapacity, bool reuse) const
{
    if (!bindingCapacity)
    {
        bindingCapacity = capacity;
    }
    if (!_built || empty() || bindingCapacity < capacity)
    {
        throw std::invalid_argument("Material bindings require a built snapshot and full capacity");
    }
    return _materialGeneration->resources->bindings(layout, bindingCapacity, false, reuse);
}

std::shared_ptr<gpu::ResourceSet> TracingSnapshot::shadowAlphaResourceSet(std::shared_ptr<gpu::PipelineLayout> layout,
    uint32_t bindingCapacity, bool reuse) const
{
    if (!layout || !_built || empty() || !shadowInstances)
    {
        throw std::invalid_argument("Alpha bindings require a built shadow snapshot and layout");
    }
    return _shadowGeneration->resources->bindings(*layout, bindingCapacity, true, reuse);
}

bool TracingSnapshot::empty() const
{
    return instances.empty();
}

void TracingSnapshot::build(gpu::CommandBuffer& command)
{
    base::CpuProfileScope profile("tracing.buildTLASAndTransitions");
    if (command.hasActiveScope())
    {
        throw std::invalid_argument("Build tracing snapshots outside active command scopes");
    }
    if (_built)
    {
        return;
    }
    for (const auto& image : images)
    {
        command.transition(image.get(),gpu::ImageLayout::ShaderReadOnly);
    }
    // The dedicated shadow alpha table has an independent admission/fallback
    // policy from the six-slot material table, including in standalone use.
    for (const auto& image : shadowImages)
    {
        command.transition(image.get(),gpu::ImageLayout::ShaderReadOnly);
    }
    if (shadowFallbackImage)
    {
        command.transition(shadowFallbackImage.get(),gpu::ImageLayout::ShaderReadOnly);
    }
    if (scene)
    {
        if (_accelerationGeneration->recorded)
        {
            if (!_accelerationGeneration->completed())
            {
                throw std::logic_error("Discard a snapshot after aborted acceleration recording");
            }
            base::CpuProfileScope::count("tracing.skippedTLASBuilds", 1);
        }
        else
        {
            // Capture the exact recording before emitting commands. Failed or
            // abandoned recording never becomes a reusable completed generation.
            const auto completion = command.recordingCompletion();
            _accelerationGeneration->recorded = true;
            command.buildRayTracingScene(scene.get());
            // A failed native build leaves this generation poisoned, even if
            // someone erroneously submits the partially recorded command later.
            _accelerationGeneration->completion = completion;
            base::CpuProfileScope::count("tracing.TLASBuilds", 1);
        }
        _built = true;
    }
}

std::shared_ptr<gpu::ResourceSet> TracingSnapshot::resourceSet(gpu::PipelineLayout& layout, uint32_t setIndex,
    uint32_t bindingCapacity) const
{
    base::CpuProfileScope lookupProfile("tracing.getSceneBindings");
    if (setIndex != 1 || !_built || empty() || !geometry || layout.device() != device)
    {
        throw std::logic_error("Build a nonempty same-device tracing snapshot before binding");
    }
    if (!bindingCapacity)
    {
        bindingCapacity = capacity;
    }
    if (bindingCapacity < capacity)
    {
        throw std::invalid_argument("Tracing bindings cannot truncate a shared snapshot");
    }
    for (const auto& cached : _sceneBindings)
    {
        if (cached.layout == &layout && cached.setIndex == setIndex && cached.capacity == bindingCapacity)
        {
            return cached.resources;
        }
    }
    base::CpuProfileScope profile("tracing.createSceneBindings");
    auto set = device->createResourceSet(&layout,setIndex);
    set->setStorageBuffer({0,2},geometry->vertexBuffer.get());
    set->setStorageBuffer({1,3},geometry->indexBuffer.get());
    set->setStorageBuffer({2,4},instanceBuffer.get());
    set->setStorageBuffer({3,5},materialBuffer.get());
    set->setRayTracingScene({4,6},scene.get());
    set->update();
    // Publish only fully initialized bindings. Their lifetime is bounded by this
    // frame/job snapshot. Persistent arrays are bound independently in set 2.
    _sceneBindings.push_back({&layout, layout.weak_from_this().lock(), setIndex, bindingCapacity, set});
    return set;
}

std::shared_ptr<gpu::ResourceSet> TracingSnapshot::shadowResourceSet(std::shared_ptr<gpu::PipelineLayout> layout,
    uint32_t bindingCapacity, uint32_t setIndex, bool reuse) const
{
    base::CpuProfileScope profile("shadow.getStableBindings");
    if (!layout || !layout->isValid() || layout->device() != device || setIndex != 1 || !_built || empty() ||
        !geometry || !geometry->vertexBuffer || !geometry->indexBuffer || !shadowInstances || !scene ||
        !geometry->vertexBuffer->isValid() || !geometry->indexBuffer->isValid() || !shadowInstances->isValid() || !scene->isValid() ||
        !bindingCapacity || bindingCapacity > MaxShadowAlphaTextures ||
        shadowImages.size() > bindingCapacity || shadowImages.size() != shadowSamplers.size() ||
        shadowImages.size() != shadowImageGenerations.size() || shadowImages.size() != shadowTable.alphaTextures.size() ||
        !shadowFallbackImage || !shadowFallbackSampler)
    {
        throw std::invalid_argument("Shadow bindings require a built compatible snapshot and v5 layout/capacity");
    }
    const auto valid = [&](const std::shared_ptr<gpu::Image>& image, const std::shared_ptr<gpu::Sampler>& sampler, uint64_t generation)
    {
        return image && sampler && image->device() == device && sampler->device() == device &&
            image->isValid() && sampler->isValid() && image->bindingGeneration() == generation;
    };
    if (!valid(shadowFallbackImage, shadowFallbackSampler, shadowFallbackGeneration))
    {
        throw std::logic_error("Discard the shadow snapshot after native resource recreation");
    }
    for (size_t i = 0; i < shadowImages.size(); ++i)
    {
        if (!valid(shadowImages[i], shadowSamplers[i], shadowImageGenerations[i]))
        {
            throw std::logic_error("Discard the shadow snapshot after native alpha resource recreation");
        }
    }
    if (reuse)
    {
        for (const auto& cached : _shadowBindings)
        {
            if (cached.layout == layout.get() && cached.setIndex == setIndex && cached.capacity == bindingCapacity)
            {
                base::CpuProfileScope::count("shadow.stableBindingHits", 1);
                return cached.resources;
            }
        }
    }
    auto set = device->createResourceSet(layout.get(), setIndex);
    set->setStorageBuffer({1,2}, shadowInstances.get());
    set->setStorageBuffer({2,3}, geometry->vertexBuffer.get());
    set->setStorageBuffer({9,7}, geometry->indexBuffer.get());
    set->setRayTracingScene({3,4}, scene.get());
    set->update();
    // Retain layouts even on the fresh-per-invocation diagnostic path. Only the
    // reusable collection participates in lookup, and neither sees partial sets.
    auto& bindings = reuse ? _shadowBindings : _uncachedShadowBindings;
    bindings.push_back({layout.get(), layout, setIndex, bindingCapacity, set});
    base::CpuProfileScope::count("shadow.geometryBufferBindings", 2);
    return set;
}

gpu::PipelineLayoutDescription tracingBindingLayout(uint32_t capacity, gpu::ShaderStage stage,
    const std::vector<gpu::ShaderStage>& additionalStages)
{
    if (!capacity || capacity > gpu::MaxSampledResourceTableCapacity)
    {
        throw std::invalid_argument("Tracing arrays require capacity in [1, 16384]");
    }
    gpu::PipelineLayoutDescription result;
    for (uint32_t i = 0; i < 4; ++i)
    {
        result.resourceBindings.push_back({1,{i,2+i},gpu::ResourceType::StorageBuffer,stage,1,additionalStages});
    }
    result.resourceBindings.push_back({1,{4,6},gpu::ResourceType::AccelerationStructure,stage,1,additionalStages});
    result.resourceBindings.push_back({2,{5,0},gpu::ResourceType::SampledImage,stage,capacity,additionalStages,8,true,true});
    result.resourceBindings.push_back({2,{6,0},gpu::ResourceType::Sampler,stage,capacity,additionalStages,9,true,true});
    return result;
}

TracingSceneResources::TracingSceneResources(gpu::Device& device, uint32_t capacity)
    : _device(&device), _capacity(capacity)
{
    tracingBindingLayout(capacity);
    const auto limits = device.resourceArrayCapabilities();
    if (!device.isValid() || !device.rayTracingCapabilities().accelerationStructure || !limits.nonUniformSampledResources || capacity > limits.maxSampledImages || capacity > limits.maxSamplers)
    {
        throw std::invalid_argument("Tracing material arrays are unavailable on this device");
    }
}

std::shared_ptr<TracingSnapshot> TracingSceneResources::prepare(const RenderQueue& queue, uint32_t shadowCapacity)
{
    base::CpuProfileScope profile("tracing.prepare");
    base::CpuProfileScope stageProfile("tracing.collectAndValidate");
    if (shadowCapacity > MaxShadowAlphaTextures)
    {
        throw std::invalid_argument("Shadow alpha capacity exceeds the supported range");
    }
    if (!queue.engine() || queue.engine()->device() != _device)
    {
        throw std::invalid_argument("Tracing queue belongs to another device");
    }
    CollectRayTracingInstancesVisitor collection;
    collection.collect(queue);
    base::CpuProfileScope::count("tracing.collectedInstances", collection.instances().size());
    const auto& owners = collection.instances();
    const auto limit = _device->resourceArrayCapabilities().maxStorageBufferRange;
    range(owners.size(),sizeof(TracingInstanceData),limit);
    if (owners.empty())
    {
        auto empty = std::make_shared<TracingSnapshot>();
        empty->device = _device;
        empty->capacity = _capacity;
        clear();
        base::CpuProfileScope::count("tracing.emptySceneSnapshots", 1);
        return empty;
    }
    stageProfile.next("tracing.uniqueMeshCollection");
    std::vector<std::shared_ptr<Drawable::DrawMesh>> meshes;
    std::unordered_map<const Drawable::DrawMesh*, uint32_t> meshLookup;
    std::vector<uint32_t> meshIndices;
    meshIndices.reserve(owners.size());
    for (const auto& source : owners)
    {
        const auto [found, inserted] = meshLookup.try_emplace(source.item.drawMesh.get(), uint32_t(meshes.size()));
        if (inserted)
        {
            meshes.push_back(source.item.drawMesh);
        }
        meshIndices.push_back(found->second);
    }
    base::CpuProfileScope::count("tracing.meshLookups", owners.size());
    base::CpuProfileScope::count("tracing.uniqueMeshes", meshes.size());
    stageProfile.next("tracing.geometryPacking");
    std::shared_ptr<const TracingGeometry> geometry = _geometry;
    if (!geometry || geometry->owners != meshes)
    {
        auto packed = std::make_shared<TracingGeometry>();
        packed->owners = meshes;
        for (const auto& owner : meshes)
        {
            const auto& mesh = owner->meshData();
            range(packed->vertices.size()+mesh.vertices.size(),sizeof(TracingVertex),limit);
            range(packed->indices.size()+mesh.indices.size(),sizeof(uint32_t),limit);
            packed->bases.push_back({uint32_t(packed->vertices.size()),uint32_t(packed->indices.size())});
            for (const auto& source : mesh.vertices)
            {
                TracingVertex vertex;
                vertex.position = glm::vec4(source.position,1);
                vertex.normal = glm::vec4(source.normal,0);
                vertex.uv = glm::vec4(source.texCoord0,source.texCoord1);
                vertex.tangent = glm::vec4(source.tangent,1);
                finite(vertex.position);
                finite(vertex.normal);
                finite(vertex.uv);
                finite(vertex.tangent);
                packed->vertices.push_back(vertex);
            }
            for (auto index : mesh.indices)
            {
                if (index >= mesh.vertices.size())
                {
                    throw std::out_of_range("Tracing mesh index is outside its vertex range");
                }
                packed->indices.push_back(index);
            }
        }
        packed->vertexBuffer = buffer(*_device,packed->vertices.data(),packed->vertices.size()*sizeof(TracingVertex));
        packed->indexBuffer = buffer(*_device,packed->indices.data(),packed->indices.size()*sizeof(uint32_t));
        geometry = packed;
    }
    stageProfile.next("tracing.instanceMaterialTextureTables");
    auto stagedMaterials = std::make_shared<TracingMaterialGeneration>();
    auto resources = std::make_shared<TracingResourceTable>();
    resources->device = _device;
    resources->admissionCapacity = _capacity;
    std::unordered_map<detail::MaterialKey, uint32_t, detail::MaterialHash> sourceLookup;
    for (const auto& source : owners)
    {
        const auto& item = source.item;
        if (!item.drawMaterial)
        {
            throw std::invalid_argument("Tracing materials require a retained source identity");
        }
        const auto& r = item.materialRevisions;
        detail::MaterialKey key{item.drawMaterial.get(), r.values, r.bindings, r.classification};
        if (sourceLookup.try_emplace(key, uint32_t(stagedMaterials->keys.size())).second)
        {
            stagedMaterials->keys.push_back(key);
            stagedMaterials->owners.push_back(item.drawMaterial);
        }
    }
    const bool materialHit = _persistenceEnabled && _materials && _materials->keys == stagedMaterials->keys;
    std::shared_ptr<const TracingMaterialGeneration> materialGeneration = materialHit ? _materials : stagedMaterials;
    if (materialHit)
    {
        materialGeneration->resources->validate();
    }
    else
    {
        stagedMaterials->resources = resources;
    }
    auto acceleration = std::make_shared<TracingAccelerationGeneration>();
    acceleration->geometry = geometry;
    acceleration->keys.reserve(owners.size());
    acceleration->blasOwners.reserve(owners.size());
    for (size_t i = 0; i < owners.size(); ++i)
    {
        const auto& source = owners[i];
        const auto& item = source.item;
        const auto& submesh = item.drawMesh->meshData().submeshes.at(item.submeshIndex);
        const auto& r = item.materialRevisions;
        const auto address = sourceLookup.at({item.drawMaterial.get(), r.values, r.bindings, r.classification});
        const auto base = geometry->bases[meshIndices[i]];
        AccelerationIdentity key;
        key.mesh = item.drawMesh.get();
        key.blas = item.tracingMesh.get();
        key.blasGeneration = item.tracingMesh->buildGeneration();
        key.nodeId = item.nodeId;
        key.transform = item.worldMatrix;
        key.ranges = {base.y + submesh.firstIndex, base.x, submesh.indexCount / 3, address};
        key.identity = {item.objectId, item.submeshIndex, glm::determinant(glm::mat3(item.worldMatrix)) < 0 ? 1u : 0u, 0};
        key.mask = source.instance.mask;
        key.opacity = source.instance.opacity;
        key.tracingEnabled = item.rayTracingEnabled;
        acceleration->keys.push_back(std::move(key));
        acceleration->blasOwners.push_back(item.tracingMesh);
    }
    bool accelerationHit = false;
    if (_accelerationReuseEnabled)
    {
        for (auto it = _accelerations.rbegin(); it != _accelerations.rend(); ++it)
        {
            if ((*it)->geometry == geometry && (*it)->keys == acceleration->keys && (*it)->completed())
            {
                acceleration = *it;
                accelerationHit = true;
                break;
            }
        }
    }
    auto shadow = std::make_shared<TracingShadowGeneration>();
    shadow->geometry = geometry;
    shadow->capacity = shadowCapacity;
    auto alpha = std::make_shared<TracingResourceTable>();
    alpha->device = _device;
    alpha->admissionCapacity = shadowCapacity;
    if (shadowCapacity)
    {
        for (size_t i = 0; i < owners.size(); ++i)
        {
            const auto& item = owners[i].item;
            const auto& m = item.materialData;
            const auto& submesh = item.drawMesh->meshData().submeshes.at(item.submeshIndex);
            ShadowIdentity key;
            key.ranges = {geometry->bases[meshIndices[i]].y + submesh.firstIndex, submesh.indexCount / 3, 0, item.objectId};
            key.alpha = {m.uvScale[0].x, m.uvScale[0].y, m.emissionAlpha.y, m.albedo.a};
            key.options = {m.uvSets[0].x, m.uvSets[1].z, geometry->bases[meshIndices[i]].x, 0};
            if ((key.options.y & MaterialAlphaTest) != 0)
            {
                key.image = {item.textureImages[0], item.imageGenerations[0]};
                key.sampler = item.textureSamplers[0];
                key.texture = item.textures[0].get();
                key.textureRevision = item.textureRevisions[0];
                shadow->owners.push_back(item.textures[0]);
            }
            shadow->keys.push_back(key);
        }
        alpha->fallbackImage = owners.front().item.textures[0]->imageResource();
        alpha->fallbackSampler = owners.front().item.textures[0]->samplerResource();
        alpha->fallbackGeneration = alpha->fallbackImage->bindingGeneration();
    }
    const bool shadowHit = _persistenceEnabled && _shadows && _shadows->geometry == geometry &&
        _shadows->capacity == shadowCapacity && _shadows->keys == shadow->keys &&
        _shadows->resources->fallbackImage == alpha->fallbackImage &&
        _shadows->resources->fallbackSampler == alpha->fallbackSampler &&
        _shadows->resources->fallbackGeneration == alpha->fallbackGeneration;
    std::shared_ptr<const TracingShadowGeneration> shadowGeneration = shadowHit ? _shadows : shadow;
    if (shadowHit && shadowCapacity)
    {
        shadowGeneration->resources->validate();
    }
    else
    {
        shadow->resources = alpha;
    }
    auto next = std::make_shared<TracingSnapshot>(materialGeneration, shadowGeneration, acceleration);
    next->device = _device;
    next->capacity = _capacity;
    next->owners = owners;
    next->geometry = geometry;
    std::unordered_map<detail::ImageKey, uint32_t, detail::ImageHash> imageLookup;
    std::unordered_map<const gpu::Sampler*, uint32_t> samplerLookup;
    std::unordered_map<detail::MaterialKey, uint32_t, detail::MaterialHash> materialLookup;
    std::vector<uint32_t> materialFallbackSlots;
    if (materialHit)
    {
        materialLookup = sourceLookup;
        materialFallbackSlots = materialGeneration->fallbackSlots;
    }
    uint64_t imageLookups = 0, samplerLookups = 0, reusedMaterials = 0;
    if (!accelerationHit)
    {
        acceleration->instances.reserve(next->owners.size());
    }
    for (size_t sourceIndex = 0; sourceIndex < next->owners.size(); ++sourceIndex)
    {
        const auto& item = next->owners[sourceIndex].item;
        const auto& submesh = item.drawMesh->meshData().submeshes.at(item.submeshIndex);
        if (!submesh.indexCount || submesh.indexCount % 3 || uint64_t(submesh.firstIndex)+submesh.indexCount > item.drawMesh->meshData().indices.size())
        {
            throw std::invalid_argument("Tracing submesh must contain a valid triangle index range");
        }
        const auto meshIndex = meshIndices[sourceIndex];
        TracingInstanceData instance;
        if (accelerationHit)
        {
            instance = acceleration->instances[sourceIndex];
        }
        else
        {
            instance.model = item.worldMatrix;
            for (uint32_t i = 0; i < 4; ++i)
            {
                finite(instance.model[i]);
            }
            const float determinant = glm::determinant(glm::mat3(instance.model));
            if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f ||
                instance.model[0][3] != 0 || instance.model[1][3] != 0 || instance.model[2][3] != 0 || instance.model[3][3] != 1)
            {
                throw std::invalid_argument("Tracing requires a finite invertible affine model transform");
            }
            instance.normal = glm::transpose(glm::inverse(instance.model));
            for (uint32_t i = 0; i < 4; ++i)
            {
                finite(instance.normal[i]);
            }
            instance.ranges = {geometry->bases[meshIndex].y+submesh.firstIndex,geometry->bases[meshIndex].x,
                submesh.indexCount/3,0};
            instance.identity = {item.objectId,item.submeshIndex,determinant < 0 ? 1u : 0u,0};
        }
        if (!item.drawMaterial)
        {
            throw std::invalid_argument("Tracing materials require a retained source identity");
        }
        const auto& revision = item.materialRevisions;
        const detail::MaterialKey materialKey{item.drawMaterial.get(), revision.values, revision.bindings, revision.classification};
        const auto existingMaterial = materialLookup.find(materialKey);
        if (existingMaterial != materialLookup.end())
        {
            instance.ranges.w = existingMaterial->second;
            // Keep the previous per-instance warning/accounting semantics even
            // though immutable derived material entries are now shared.
            next->fallbackTextureSlots += materialFallbackSlots[existingMaterial->second];
            if (!accelerationHit)
            {
                acceleration->instances.push_back(instance);
            }
            ++reusedMaterials;
            continue;
        }
        range(next->materials.size()+1, sizeof(TracingMaterialData), limit);
        uint32_t fallbackSlots = 0;
        TracingMaterialData material;
        material.values = item.materialData;
        finite(material.values.albedo);
        finite(material.values.fresnelTint);
        finite(material.values.sheenColor);
        finite(material.values.factors);
        finite(material.values.emissionAlpha);
        for (const auto& value : material.values.uvScale)
        {
            finite(value);
        }
        for (uint32_t slot = 0; slot < 6; ++slot)
        {
            const auto uv = slot < 4 ? material.values.uvSets[0][slot] : material.values.uvSets[1][slot-4];
            if (uv > 1)
            {
                throw std::invalid_argument("Tracing materials support UV0 and UV1 only");
            }
            const auto& texture = item.textures[slot];
            if (!texture)
            {
                throw std::invalid_argument("Tracing materials require six realized textures");
            }
            auto image = texture->imageResource();
            auto sampler = texture->samplerResource();
            if (!image || !sampler || image.get() != item.textureImages[slot] || sampler.get() != item.textureSamplers[slot] ||
                image->device() != _device || sampler->device() != _device || !image->isValid() || !sampler->isValid() ||
                image->bindingGeneration() != item.imageGenerations[slot] || texture->bindingRevision() != item.textureRevisions[slot] ||
                image->imageType() != gpu::ImageType::Image2D || gpu::isDepthFormat(image->pixelFormat()) ||
                !gpu::hasFlag(image->usage(),gpu::ImageUsage::Sampled))
            {
                throw std::invalid_argument("Tracing snapshot requires current same-device color-2D material resources");
            }
            const detail::ImageKey imageKey{image.get(), image->bindingGeneration()};
            const auto imageIt = imageLookup.find(imageKey);
            const auto samplerIt = samplerLookup.find(sampler.get());
            ++imageLookups;
            ++samplerLookups;
            // Decide admission for both resources before adding either one.
            if ((imageIt == imageLookup.end() && next->images.size() == _capacity) ||
                (samplerIt == samplerLookup.end() && next->samplers.size() == _capacity))
            {
                material.images[slot/4][slot%4] = UINT32_MAX;
                material.samplers[slot/4][slot%4] = UINT32_MAX;
                ++fallbackSlots;
                continue;
            }
            uint32_t imageIndex;
            if (imageIt == imageLookup.end())
            {
                imageIndex = uint32_t(next->images.size());
                resources->images.push_back(image);
                resources->generations.push_back(image->bindingGeneration());
                imageLookup.emplace(imageKey, imageIndex);
            }
            else
            {
                imageIndex = imageIt->second;
            }
            uint32_t samplerIndex;
            if (samplerIt == samplerLookup.end())
            {
                samplerIndex = uint32_t(next->samplers.size());
                resources->samplers.push_back(sampler);
                samplerLookup.emplace(sampler.get(), samplerIndex);
            }
            else
            {
                samplerIndex = samplerIt->second;
            }
            material.images[slot/4][slot%4] = imageIndex;
            material.samplers[slot/4][slot%4] = samplerIndex;
        }
        instance.ranges.w = uint32_t(next->materials.size());
        materialLookup.emplace(materialKey, instance.ranges.w);
        stagedMaterials->materials.push_back(material);
        materialFallbackSlots.push_back(fallbackSlots);
        next->fallbackTextureSlots += fallbackSlots;
        if (!accelerationHit)
        {
            acceleration->instances.push_back(instance);
        }
    }
    base::CpuProfileScope::count("tracing.materialLookups", next->owners.size());
    base::CpuProfileScope::count("tracing.materialEntries", next->materials.size());
    base::CpuProfileScope::count("tracing.reusedMaterialReferences", reusedMaterials);
    base::CpuProfileScope::count("tracing.imageLookups", imageLookups);
    base::CpuProfileScope::count("tracing.samplerLookups", samplerLookups);
    base::CpuProfileScope::count("tracing.uniqueImages", next->images.size());
    base::CpuProfileScope::count("tracing.uniqueSamplers", next->samplers.size());
    base::CpuProfileScope::count("tracing.fallbackTextureSlots", next->fallbackTextureSlots);
    base::CpuProfileScope::count("tracing.materialBufferBytes", next->materials.size()*sizeof(TracingMaterialData));
    stageProfile.next("tracing.instanceBufferUpload");
    if (!accelerationHit)
    {
        acceleration->buffer = buffer(*_device,next->instances.data(),next->instances.size()*sizeof(TracingInstanceData));
    }
    base::CpuProfileScope::count("tracing.accelerationGenerationHits", accelerationHit ? 1 : 0);
    base::CpuProfileScope::count("tracing.instanceUploadedBytes", accelerationHit ? 0 : next->instances.size()*sizeof(TracingInstanceData));
    if (shadowCapacity && !shadowHit)
    {
        base::CpuProfileScope::count("shadowTable.prepared", 1);
        stageProfile.next("tracing.prepareShadowResources");
        {
            base::CpuProfileScope shadowTotalProfile("tracing.shadowTablesAndBuffers");
            {
                base::CpuProfileScope shadowBuildProfile("tracing.buildShadowQueryTable");
                shadow->table = buildShadowQueryTable(next->owners, shadowCapacity);
                for (const auto& texture : next->shadowTable.alphaTextures)
                {
                    auto image = texture->imageResource();
                    alpha->generations.push_back(image->bindingGeneration());
                    alpha->images.push_back(std::move(image));
                    alpha->samplers.push_back(texture->samplerResource());
                }
                // Match the shared geometry's index/vertex bases to dense TLAS order.
                for (size_t i = 0; i < next->instances.size(); ++i)
                {
                    shadow->table.instances[i].ranges.x = next->instances[i].ranges.x;
                    shadow->table.instances[i].options.z = next->instances[i].ranges.y;
                }
            }
            {
                base::CpuProfileScope shadowInstanceProfile("tracing.shadowInstanceBufferUpload");
                const size_t shadowInstanceBytes = next->shadowTable.instances.size() * sizeof(ShadowInstanceData);
                shadow->buffer = buffer(*_device, next->shadowTable.instances.data(), shadowInstanceBytes);
                base::CpuProfileScope::count("shadowTable.instanceBufferBytes", shadowInstanceBytes);
            }
        }
    }
    else if (!shadowCapacity)
    {
        base::CpuProfileScope::count("shadowTable.skippedNoShadowCapacity", 1);
    }
    stageProfile.next("tracing.allocateAndUploadMaterialBuffer");
    if (!materialHit)
    {
        stagedMaterials->fallbackSlots = materialFallbackSlots;
        stagedMaterials->buffer = buffer(*_device,next->materials.data(),next->materials.size()*sizeof(TracingMaterialData));
        resources->fallbackImage = resources->images.front();
        resources->fallbackSampler = resources->samplers.front();
        resources->fallbackGeneration = resources->generations.front();
        if (_persistenceEnabled && _materials && resources->matches(*_materials->resources))
        {
            stagedMaterials->resources = _materials->resources;
        }
    }
    if (!shadowHit && shadowCapacity && _persistenceEnabled && _shadows && alpha->matches(*_shadows->resources))
    {
        shadow->resources = _shadows->resources;
    }
    base::CpuProfileScope::count("tracing.materialGenerationHits", materialHit ? 1 : 0);
    base::CpuProfileScope::count("tracing.materialUploadedBytes", materialHit ? 0 : next->materials.size()*sizeof(TracingMaterialData));
    base::CpuProfileScope::count("shadow.metadataGenerationHits", shadowHit && shadowCapacity ? 1 : 0);
    stageProfile.next("tracing.createSceneAndInstances");
    if (!accelerationHit)
    {
        acceleration->scene = _device->createRayTracingScene("draw immutable tracing acceleration generation");
    }
    uint64_t alphaTestInstances = 0;
    for (uint32_t i = 0; i < next->owners.size(); ++i)
    {
        const auto& source = next->owners[i];
        const bool cutout = (source.item.materialData.uvSets[1].z & MaterialAlphaTest) != 0;
        alphaTestInstances += cutout ? 1u : 0u;
        if (!accelerationHit)
        {
            acceleration->scene->addInstance(source.instance.mesh,source.instance.transform,i,source.instance.mask,
                source.instance.opacity);
        }
    }
    base::CpuProfileScope::count("tracing.alphaTestInstances", alphaTestInstances);
    base::CpuProfileScope::count("tracing.opaqueInstances", next->owners.size() - alphaTestInstances);
    // Rebind public aliases after resource-table sharing decisions.
    auto published = std::make_shared<TracingSnapshot>(materialGeneration, shadowGeneration, acceleration);
    published->device = next->device;
    published->capacity = next->capacity;
    published->fallbackTextureSlots = next->fallbackTextureSlots;
    published->geometry = std::move(next->geometry);
    published->owners = std::move(next->owners);
    next = std::move(published);
    // Stage bounded cache changes before publishing any service state. Pending
    // generations remain candidates, but cannot satisfy reuse until completed.
    auto accelerations = _accelerations;
    if (!accelerationHit)
    {
        constexpr size_t maxGenerations = 8;
        if (accelerations.size() == maxGenerations)
        {
            accelerations.erase(accelerations.begin());
        }
        accelerations.push_back(acceleration);
    }
    _accelerations.swap(accelerations);
    _geometry = geometry; // Publish only after the complete snapshot succeeds.
    _materials = materialGeneration;
    _shadows = shadowGeneration;
    if (next->fallbackTextureSlots && !_capacityWarningActive)
    {
        bg2e_log_warning << "Tracing material capacity reached: images=" << next->images.size()
            << ", samplers=" << next->samplers.size() << ", capacity=" << _capacity
            << "; using neutral values for " << next->fallbackTextureSlots
            << " texture slots (raster materials unchanged)" << bg2e_log_end;
    }
    _capacityWarningActive = next->fallbackTextureSlots != 0;
    return next;
}

void TracingSceneResources::clear()
{
    _accelerations.clear();
    _geometry.reset();
    _materials.reset();
    _shadows.reset();
    _capacityWarningActive = false;
}

uint32_t TracingSceneResources::capacity() const
{
    return _capacity;
}
}
