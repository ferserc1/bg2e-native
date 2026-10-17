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
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

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

bool TracingSnapshot::empty() const
{
    return instances.empty();
}

void TracingSnapshot::build(gpu::CommandBuffer& command)
{
    if (command.hasActiveScope())
    {
        throw std::invalid_argument("Build tracing snapshots outside active command scopes");
    }
    for (const auto& image : images)
    {
        command.transition(image.get(),gpu::ImageLayout::ShaderReadOnly);
    }
    if (!_built && scene)
    {
        command.buildRayTracingScene(scene.get());
        _built = true;
    }
}

std::shared_ptr<gpu::ResourceSet> TracingSnapshot::resourceSet(gpu::PipelineLayout& layout, uint32_t setIndex) const
{
    if (setIndex != 1 || !_built || empty() || !geometry || layout.device() != device)
    {
        throw std::logic_error("Build a nonempty same-device tracing snapshot before binding");
    }
    auto set = device->createResourceSet(&layout,setIndex);
    set->setStorageBuffer({0,2},geometry->vertexBuffer.get());
    set->setStorageBuffer({1,3},geometry->indexBuffer.get());
    set->setStorageBuffer({2,4},instanceBuffer.get());
    set->setStorageBuffer({3,5},materialBuffer.get());
    set->setRayTracingScene({4,6},scene.get());
    std::vector<gpu::ImageView> views(capacity,images.front()->fullView());
    std::vector<gpu::Sampler*> states(capacity,samplers.front().get());
    for (size_t i = 0; i < images.size(); ++i)
    {
        views[i] = images[i]->fullView();
    }
    for (size_t i = 0; i < samplers.size(); ++i)
    {
        states[i] = samplers[i].get();
    }
    set->setSampledImages({5,0},views);
    set->setSamplers({6,capacity},states);
    set->update();
    return set;
}

gpu::PipelineLayoutDescription tracingBindingLayout(uint32_t capacity, gpu::ShaderStage stage,
    const std::vector<gpu::ShaderStage>& additionalStages)
{
    if (capacity != 32 && capacity != 128 && capacity != 256)
    {
        throw std::invalid_argument("Tracing arrays require a 32/128/256 shader profile");
    }
    gpu::PipelineLayoutDescription result;
    for (uint32_t i = 0; i < 4; ++i)
    {
        result.resourceBindings.push_back({1,{i,2+i},gpu::ResourceType::StorageBuffer,stage,1,additionalStages});
    }
    result.resourceBindings.push_back({1,{4,6},gpu::ResourceType::AccelerationStructure,stage,1,additionalStages});
    result.resourceBindings.push_back({1,{5,0},gpu::ResourceType::SampledImage,stage,capacity,additionalStages,8});
    result.resourceBindings.push_back({1,{6,capacity},gpu::ResourceType::Sampler,stage,capacity,additionalStages,8});
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

std::shared_ptr<TracingSnapshot> TracingSceneResources::prepare(const RenderQueue& queue)
{
    if (!queue.engine() || queue.engine()->device() != _device)
    {
        throw std::invalid_argument("Tracing queue belongs to another device");
    }
    CollectRayTracingInstancesVisitor collection;
    collection.collect(queue);
    auto next = std::make_shared<TracingSnapshot>();
    next->device = _device;
    next->capacity = _capacity;
    next->owners = collection.instances();
    const auto limit = _device->resourceArrayCapabilities().maxStorageBufferRange;
    range(next->owners.size(),sizeof(TracingMaterialData),limit);
    range(next->owners.size(),sizeof(TracingInstanceData),limit);
    if (next->owners.empty())
    {
        return next;
    }
    std::vector<std::shared_ptr<Drawable::DrawMesh>> meshes;
    for (const auto& source : next->owners)
    {
        if (std::find(meshes.begin(),meshes.end(),source.item.drawMesh) == meshes.end())
        {
            meshes.push_back(source.item.drawMesh);
        }
    }
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
    next->geometry = geometry;
    for (const auto& source : next->owners)
    {
        const auto& item = source.item;
        const auto& submesh = item.drawMesh->meshData().submeshes.at(item.submeshIndex);
        if (!submesh.indexCount || submesh.indexCount % 3 || uint64_t(submesh.firstIndex)+submesh.indexCount > item.drawMesh->meshData().indices.size())
        {
            throw std::invalid_argument("Tracing submesh must contain a valid triangle index range");
        }
        const auto meshIndex = size_t(std::find(meshes.begin(),meshes.end(),item.drawMesh)-meshes.begin());
        TracingInstanceData instance;
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
            submesh.indexCount/3,uint32_t(next->materials.size())};
        instance.identity = {item.objectId,item.submeshIndex,determinant < 0 ? 1u : 0u,0};
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
                image->imageType() != gpu::ImageType::Image2D || gpu::isDepthFormat(image->pixelFormat()) ||
                !gpu::hasFlag(image->usage(),gpu::ImageUsage::Sampled))
            {
                throw std::invalid_argument("Tracing snapshot requires current same-device color-2D material resources");
            }
            auto imageIt = std::find(next->images.begin(),next->images.end(),image);
            if (imageIt == next->images.end())
            {
                next->images.push_back(image);
                imageIt = next->images.end()-1;
            }
            auto samplerIt = std::find(next->samplers.begin(),next->samplers.end(),sampler);
            if (samplerIt == next->samplers.end())
            {
                next->samplers.push_back(sampler);
                samplerIt = next->samplers.end()-1;
            }
            if (next->images.size() > _capacity || next->samplers.size() > _capacity)
            {
                throw std::length_error("Tracing material capacity exceeded: images="+std::to_string(next->images.size())+
                    ", samplers="+std::to_string(next->samplers.size())+", capacity="+std::to_string(_capacity));
            }
            material.images[slot/4][slot%4] = uint32_t(imageIt-next->images.begin());
            material.samplers[slot/4][slot%4] = uint32_t(samplerIt-next->samplers.begin());
        }
        next->instances.push_back(instance);
        next->materials.push_back(material);
    }
    next->instanceBuffer = buffer(*_device,next->instances.data(),next->instances.size()*sizeof(TracingInstanceData));
    next->materialBuffer = buffer(*_device,next->materials.data(),next->materials.size()*sizeof(TracingMaterialData));
    next->scene = _device->createRayTracingScene("draw shared tracing snapshot");
    for (uint32_t i = 0; i < next->owners.size(); ++i)
    {
        const auto& source = next->owners[i];
        const bool cutout = (source.item.materialData.uvSets[1].z & MaterialAlphaTest) != 0;
        next->scene->addInstance(source.instance.mesh,source.instance.transform,i,source.instance.mask,
            cutout ? gpu::RayTracingOpacity::ForceNonOpaque : gpu::RayTracingOpacity::ForceOpaque);
    }
    _geometry = geometry; // Publish cache only after the complete snapshot succeeds.
    return next;
}

void TracingSceneResources::clear()
{
    _geometry.reset();
}

uint32_t TracingSceneResources::capacity() const
{
    return _capacity;
}
}
