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

#include <bg2e/draw/pipeline/SoftShadowPass.hpp>
#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace bg2e::draw::pipeline {
namespace {
struct alignas(16) ShadowUniforms {
    glm::mat4 inverseViewProjection{1};
    glm::vec4 parameters{0}; // bias, directional distance, reserved, reserved
    glm::uvec4 counts{0}; // lights, instances, alpha textures, maximum samples
    LightUniforms lights;
};
static_assert(sizeof(ShadowUniforms) == 752 && offsetof(ShadowUniforms, lights) == 96);
void storage(Engine& engine, std::shared_ptr<gpu::Buffer>& buffer, const void* data, size_t size)
{
    if (!buffer || buffer->byteSize() < size)
    {
        auto next = engine.device()->createBuffer("draw query table");
        next->createStorageBuffer(data, size);
        buffer = std::move(next);
    }
    else
    {
        buffer->updateStorageBuffer(data, size);
    }
}
}

ShadowQueryTable buildShadowQueryTable(const CollectRayTracingInstancesVisitor& collection)
{
    ShadowQueryTable table;
    for (const auto& instance : collection.instances())
    {
        const auto& item = instance.item;
        const auto& material = item.materialData;
        const auto& mesh = item.drawMesh->meshData();
        const auto& submesh = mesh.submeshes.at(item.submeshIndex);
        ShadowInstanceData data;
        data.ranges = {0, submesh.indexCount / 3, std::numeric_limits<uint32_t>::max(), item.objectId};
        data.alpha = {material.uvScale[0].x, material.uvScale[0].y, material.emissionAlpha.y, material.albedo.a};
        data.options = {material.uvSets[0].x, material.uvSets[1].z, 0, 0};
        if ((data.options.y & MaterialAlphaTest) != 0)
        {
            const auto& texture = item.textures[0];
            auto found = std::find_if(table.alphaTextures.begin(), table.alphaTextures.end(), [&](const auto& other)
            {
                return other->image() == texture->image() && other->sampler() == texture->sampler();
            });
            if (found == table.alphaTextures.end())
            {
                if (table.alphaTextures.size() >= MaxShadowAlphaTextures)
                {
                    throw ShadowTextureCapacityError("Query cutouts support eight distinct albedo image/sampler pairs; no occluders are truncated");
                }
                table.alphaTextures.push_back(texture);
                data.ranges.z = uint32_t(table.alphaTextures.size() - 1);
            }
            else
            {
                data.ranges.z = uint32_t(found - table.alphaTextures.begin());
            }
            if (submesh.indexCount % 3 != 0 || uint64_t(submesh.firstIndex) + submesh.indexCount > mesh.indices.size() ||
                uint64_t(table.triangles.size()) + submesh.indexCount / 3 > std::numeric_limits<uint32_t>::max())
            {
                throw std::length_error("Query UV triangle range is invalid or exceeds the table address space");
            }
            data.ranges.x = uint32_t(table.triangles.size());
            for (uint32_t offset = 0; offset < submesh.indexCount; offset += 3)
            {
                ShadowTriangleUV uv;
                glm::vec4* values[] = {&uv.a, &uv.b, &uv.c};
                for (uint32_t j = 0; j < 3; ++j)
                {
                    const auto& vertex = mesh.vertices.at(mesh.indices.at(size_t(submesh.firstIndex) + offset + j));
                    *values[j] = {vertex.texCoord0, vertex.texCoord1};
                }
                table.triangles.push_back(uv);
            }
        }
        table.instances.push_back(data);
    }
    return table;
}

PassContract softShadowContract(gpu::PixelFormat depth)
{
    const auto hdr = gpu::PixelFormat::R16G16B16A16_SFLOAT;
    return {"draw.query.softShadows.v1", {{"normalEmission", hdr}, {"depth", depth}},
        {{"shadow0", hdr, ExtentDomain::Internal, gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment},
         {"shadow1", hdr, ExtentDomain::Internal, gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment}}};
}

gpu::PipelineLayoutDescription softShadowBindingLayout()
{
    using T = gpu::ResourceType;
    gpu::PipelineLayoutDescription result;
    const auto add = [&](uint32_t vk, uint32_t metal, T type)
    {
        result.resourceBindings.push_back({0, {vk, metal}, type, gpu::ShaderStage::Compute, 1});
    };
    add(0, 1, T::UniformBuffer);
    add(1, 2, T::StorageBuffer);
    add(2, 3, T::StorageBuffer);
    add(3, 4, T::AccelerationStructure);
    add(4, 0, T::SampledImage);
    add(5, 1, T::SampledImage);
    add(6, 8, T::Sampler);
    add(7, 2, T::StorageImage);
    add(8, 3, T::StorageImage);
    for (uint32_t i = 0; i < MaxShadowAlphaTextures; ++i)
    {
        add(16 + i, 4 + i, T::SampledImage);
        add(24 + i, i, T::Sampler);
    }
    return result;
}

SoftShadowPass::SoftShadowPass(std::filesystem::path shaderRoot, uint32_t slots, gpu::PixelFormat depth)
    : _shaderRoot(shaderRoot.empty() ? base::PlatformTools::shaderPath() : std::move(shaderRoot)),
      _requestedSlots(slots), _contract(softShadowContract(depth))
{
}

SoftShadowSettings& SoftShadowPass::settings()
{
    return _settings;
}

const SoftShadowSettings& SoftShadowPass::settings() const
{
    return _settings;
}

bool SoftShadowPass::available() const
{
    return bool(_pipeline);
}

const std::string& SoftShadowPass::status() const
{
    return _status;
}

const PassContract& SoftShadowPass::contract() const
{
    return _contract;
}

void SoftShadowPass::initialize(Engine& engine, const LayerConfiguration& configuration)
{
    if (_engine)
    {
        throw std::logic_error("Clean up SoftShadowPass before initialization");
    }
    _engine = &engine;
    _contract = softShadowContract(configuration.contract.inputs.at(1).format);
    const auto caps = engine.device()->rayTracingCapabilities();
    const auto usages = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment;
    if (!caps.querySupported() || !engine.device()->formatCapabilities(gpu::PixelFormat::R16G16B16A16_SFLOAT).supports(usages))
    {
        _status = "Query shadows unavailable: logical query/acceleration or visibility format support is missing";
        return;
    }
    const uint32_t slots = _requestedSlots ? _requestedSlots : (engine.surface() ? engine.surface()->inFlightFrames() : 0);
    if (!slots)
    {
        throw std::invalid_argument("Query shadows require an explicit acquired slot count without a surface");
    }
    try
    {
        _layout = engine.device()->createPipelineLayout(softShadowBindingLayout());
        gpu::ShaderLib library(_shaderRoot, engine.backendType());
        _shader = library.compute("draw/query/soft_shadows", engine.device());
        gpu::ComputePipelineDescription description;
        description.layout = _layout.get();
        description.computeShader = _shader.get();
        _pipeline = engine.device()->createComputePipeline(description);
        gpu::SamplerDescription nearest;
        nearest.minFilter = nearest.magFilter = gpu::Filter::Nearest;
        nearest.addressModeU = nearest.addressModeV = gpu::AddressMode::ClampToEdge;
        nearest.maxLod = 0;
        _nearest = engine.device()->createSampler(nearest);
        _slots.resize(slots);
        _status = "Query shadows ready";
    }
    catch (const std::exception& error)
    {
        _pipeline.reset();
        _layout.reset();
        _shader.reset();
        _nearest.reset();
        _slots.clear();
        _status = std::string("Query shadows unavailable: ") + error.what();
    }
}

void SoftShadowPass::attachScene(std::shared_ptr<scene::Node>)
{
    if (_engine)
    {
        _engine->device()->waitIdle();
    }
    for (auto& slot : _slots)
    {
        slot = Slot{};
    }
}

void SoftShadowPass::resize(const LayerConfiguration&)
{
    attachScene(nullptr);
}

void SoftShadowPass::update(const FrameContext&)
{
}

void SoftShadowPass::prepare(const FrameContext& frame, const RenderQueue& queue,
    const glm::mat4& view, const glm::mat4& projection)
{
    if (&frame.engine != _engine)
    {
        throw std::invalid_argument("Query shadows belong to another engine");
    }
    if (!available())
    {
        return;
    }
    auto& slot = _slots.at(frame.frameSlot);
    if (slot.prepared && slot.serial == frame.frameNumber)
    {
        throw std::logic_error("Prepare query data once per acquired presentation");
    }
    if (!std::isfinite(_settings.bias) || _settings.bias <= 0 || !std::isfinite(_settings.directionalDistance) ||
        _settings.directionalDistance <= _settings.bias * 2 || _settings.maxSamples == 0 || _settings.maxSamples > MaxShadowSamples)
    {
        throw std::invalid_argument("Invalid query shadow bias/distance/sample budget");
    }
    if (queue.lights().count.x > MaxLights)
    {
        throw std::length_error("Query shadows support at most eight captured lights");
    }
    bool lights = false;
    for (uint32_t i = 0; i < queue.lights().count.x; ++i)
    {
        const auto& light = queue.lights().lights.at(i);
        if (_settings.enabled && light.flags.y)
        {
            if (!std::isfinite(light.parameters.y) || light.parameters.y < 0)
            {
                throw std::invalid_argument("Shadow-casting lights need finite nonnegative sourceSize");
            }
            lights = true;
        }
    }
    slot.sets.clear(); // Retire only this completed slot's previous bindings.
    slot.active = false;
    slot.built = false;
    _status = "Query shadows ready";
    _sceneSupported = true;
    if (_settings.enabled && lights)
    {
        slot.collection.collect(queue);
        try
        {
            slot.table = buildShadowQueryTable(slot.collection);
        }
        catch (const ShadowTextureCapacityError& error)
        {
            // Neutral shadow output for the whole scene; advanced tables are independent.
            slot.scene.reset();
            slot.collection = {};
            slot.table = {};
            slot.serial = frame.frameNumber;
            slot.prepared = true;
            _sceneSupported = false;
            _status = std::string("Scene query shadows unavailable; neutral visibility: ")+error.what();
            return;
        }
        if (!slot.table.instances.empty())
        {
            if (!slot.scene)
            {
                slot.scene = _engine->device()->createRayTracingScene("draw query shadow TLAS");
            }
            slot.scene->clearInstances();
            for (const auto& instance : slot.collection.instances())
            {
                const auto& item = instance.instance;
                const bool cutout = (instance.item.materialData.uvSets[1].z & MaterialAlphaTest) != 0;
                slot.scene->addInstance(item.mesh, item.transform, item.instanceId, item.mask,
                    cutout ? gpu::RayTracingOpacity::ForceNonOpaque : gpu::RayTracingOpacity::ForceOpaque);
            }
            ShadowUniforms data;
            data.inverseViewProjection = packFrame(frame, view, projection).inverseViewProjection;
            data.parameters = {_settings.bias, _settings.directionalDistance, 0, 0};
            data.counts = {queue.lights().count.x, uint32_t(slot.table.instances.size()), uint32_t(slot.table.alphaTextures.size()), _settings.maxSamples};
            data.lights = queue.lights();
            if (!slot.uniforms)
            {
                slot.uniforms = _engine->device()->createBuffer("draw query uniforms");
                slot.uniforms->createUniformBuffer(data);
            }
            else
            {
                slot.uniforms->updateUniformBuffer(data);
            }
            storage(*_engine, slot.instances, slot.table.instances.data(), slot.table.instances.size() * sizeof(ShadowInstanceData));
            const ShadowTriangleUV empty;
            storage(*_engine, slot.triangles, slot.table.triangles.empty() ? &empty : slot.table.triangles.data(),
                std::max(size_t(1), slot.table.triangles.size()) * sizeof(ShadowTriangleUV));
            slot.active = true;
        }
        else
        {
            slot.scene.reset();
        }
    }
    else
    {
        // Disabled/unused queries do not require built Drawable tracing geometry.
        slot.scene.reset();
        slot.collection = {};
        slot.table = {};
    }
    slot.serial = frame.frameNumber;
    slot.prepared = true;
}

void SoftShadowPass::record(const PassContext& context)
{
    if (!available() || &context.frame.engine != _engine)
    {
        throw std::logic_error("Record only an available query shadow pass in its engine");
    }
    auto& slot = _slots.at(context.frame.frameSlot);
    if (!slot.prepared || slot.serial != context.frame.frameNumber)
    {
        throw std::logic_error("Prepare the acquired query shadow slot before recording");
    }
    auto& command = context.frame.commandBuffer;
    if (!slot.active)
    {
        for (const auto& output : context.outputs)
        {
            RenderTarget::clear(command, *output.image, {1, 1, 1, 1});
        }
        return;
    }
    if (!slot.built)
    {
        command.buildRayTracingScene(slot.scene.get());
        slot.built = true;
    }
    for (const auto& input : context.inputs)
    {
        command.transition(input.image, gpu::ImageLayout::ShaderReadOnly);
    }
    for (const auto& output : context.outputs)
    {
        command.transition(output.image, gpu::ImageLayout::General);
    }
    {
        auto set = _engine->device()->createResourceSet(_layout.get(), 0);
        set->setUniformBuffer({0, 1}, slot.uniforms.get());
        set->setStorageBuffer({1, 2}, slot.instances.get());
        set->setStorageBuffer({2, 3}, slot.triangles.get());
        set->setRayTracingScene({3, 4}, slot.scene.get());
        set->setSampledImage({4, 0}, context.inputs.at(0).image);
        set->setSampledImage({5, 1}, context.inputs.at(1).image);
        set->setSampler({6, 8}, _nearest.get());
        set->setStorageImage({7, 2}, context.outputs.at(0).image);
        set->setStorageImage({8, 3}, context.outputs.at(1).image);
        // Fill unused fixed bindings with one retained compatible albedo pair.
        const auto fallback = slot.collection.instances().front().item.textures[0];
        for (uint32_t i = 0; i < MaxShadowAlphaTextures; ++i)
        {
            const auto texture = i < slot.table.alphaTextures.size() ? slot.table.alphaTextures[i] : fallback;
            set->setSampledImage({16 + i, 4 + i}, texture->image());
            set->setSampler({24 + i, i}, texture->sampler());
        }
        set->update();
        slot.sets.push_back(std::move(set));
    }
    command.beginCompute();
    command.bindPipeline(_pipeline.get());
    command.bindResourceSet(_pipeline.get(), 0, slot.sets.back().get());
    const auto extent = context.outputs.at(0).image->size();
    command.dispatch((extent.width + 15) / 16, (extent.height + 15) / 16, 1);
    command.endCompute();
    for (const auto& output : context.outputs)
    {
        command.transition(output.image, gpu::ImageLayout::ShaderReadOnly);
    }
}

void SoftShadowPass::cleanup()
{
    if (_engine)
    {
        _engine->device()->waitIdle();
    }
    _slots.clear();
    _pipeline.reset();
    _shader.reset();
    _layout.reset();
    _nearest.reset();
    _engine = nullptr;
    _status = "Not initialized";
}

bool SoftShadowPass::sceneSupported() const
{
    return available() && _sceneSupported;
}

}
