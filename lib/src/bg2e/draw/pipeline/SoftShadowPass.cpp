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
#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include "TracingTableLookup.hpp"
#include <bg2e/draw/pipeline/TracingSceneResources.hpp>
#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/draw/pipeline/RasterFrame.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/base/Log.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace bg2e::draw::pipeline {
namespace {
struct alignas(16) ShadowUniforms {
    glm::mat4 inverseViewProjection{1};
    glm::vec4 parameters{0}; // bias, directional distance, reserved, reserved
    glm::uvec4 counts{0}; // lights, instances, alpha textures, maximum samples
    LightUniforms lights;
};
static_assert(sizeof(ShadowUniforms) == 752 && offsetof(ShadowUniforms, lights) == 96);
}

ShadowQueryTable buildShadowQueryTable(const CollectRayTracingInstancesVisitor& collection, uint32_t capacity)
{
    return buildShadowQueryTable(collection.instances(), capacity);
}

ShadowQueryTable buildShadowQueryTable(const std::vector<CollectRayTracingInstancesVisitor::Instance>& instances, uint32_t capacity)
{
    base::CpuProfileScope buildProfile("shadowTable.build");
    if (!capacity || capacity > MaxShadowAlphaTextures)
    {
        throw std::invalid_argument("Shadow alpha capacity must be in [1, 16384]");
    }
    ShadowQueryTable table;
    uint64_t alphaTestInstances = 0;
    uint64_t alphaTextureLookups = 0;
    table.instances.reserve(instances.size());
    std::unordered_map<detail::AlphaPairKey, uint32_t, detail::AlphaPairHash> alphaLookup;
    base::CpuProfileScope stageProfile("shadowTable.instanceMetadata");
    for (const auto& instance : instances)
    {
        const auto& item = instance.item;
        const auto& material = item.materialData;
        const auto& mesh = item.drawMesh->meshData();
        const auto& submesh = mesh.submeshes.at(item.submeshIndex);
        ShadowInstanceData data;
        if (!submesh.indexCount || submesh.indexCount % 3 != 0 ||
            uint64_t(submesh.firstIndex) + submesh.indexCount > mesh.indices.size())
        {
            throw std::length_error("Query submesh must contain a valid triangle index range");
        }
        data.ranges = {submesh.firstIndex, submesh.indexCount / 3, std::numeric_limits<uint32_t>::max(), item.objectId};
        data.alpha = {material.uvScale[0].x, material.uvScale[0].y, material.emissionAlpha.y, material.albedo.a};
        data.options = {material.uvSets[0].x, material.uvSets[1].z, 0, 0};
        if ((data.options.y & MaterialAlphaTest) != 0)
        {
            ++alphaTestInstances;
            ++alphaTextureLookups;
            stageProfile.next("shadowTable.alphaTextureDeduplication");
            const auto& texture = item.textures[0];
            if (!texture || !texture->image() || !texture->sampler() || !texture->image()->isValid() || !texture->sampler()->isValid() ||
                texture->image() != item.textureImages[0] || texture->sampler() != item.textureSamplers[0] ||
                texture->bindingRevision() != item.textureRevisions[0] || texture->image()->bindingGeneration() != item.imageGenerations[0])
            {
                throw std::invalid_argument("Shadow alpha lookup requires current retained image/sampler resources");
            }
            const detail::AlphaPairKey key{{texture->image(), texture->image()->bindingGeneration()}, texture->sampler()};
            const auto found = alphaLookup.find(key);
            if (found == alphaLookup.end())
            {
                if (table.alphaTextures.size() >= capacity)
                {
                    // Keep scalar alpha/cutoff when this pair cannot be admitted.
                    ++table.fallbackTextureSlots;
                }
                else
                {
                    data.ranges.z = uint32_t(table.alphaTextures.size());
                    table.alphaTextures.push_back(texture);
                    alphaLookup.emplace(key, data.ranges.z);
                }
            }
            else
            {
                data.ranges.z = found->second;
            }
            stageProfile.next("shadowTable.instanceMetadata");
        }
        table.instances.push_back(data);
    }
    base::CpuProfileScope::count("shadowTable.instances", instances.size());
    base::CpuProfileScope::count("shadowTable.alphaTestInstances", alphaTestInstances);
    base::CpuProfileScope::count("shadowTable.opaqueInstances", instances.size() - alphaTestInstances);
    base::CpuProfileScope::count("shadowTable.uniqueAlphaTextures", table.alphaTextures.size());
    base::CpuProfileScope::count("shadowTable.entries", table.instances.size());
    base::CpuProfileScope::count("shadowTable.capacitySlots", capacity);
    base::CpuProfileScope::count("shadowTable.overflowSlots", table.fallbackTextureSlots);
    base::CpuProfileScope::count("shadowTable.instanceBytes", table.instances.size() * sizeof(ShadowInstanceData), "bytes");
    base::CpuProfileScope::count("shadowTable.alphaTextureLookups", alphaTextureLookups);
    base::CpuProfileScope::count("shadowTable.alphaTexturesAtCapacity", table.alphaTextures.size() >= capacity ? 1 : 0);
    return table;
}

PassContract softShadowContract(gpu::PixelFormat depth)
{
    const auto hdr = gpu::PixelFormat::R16G16B16A16_SFLOAT;
    return {"draw.query.softShadows.v5", {{"normalEmission", hdr}, {"depth", depth}},
        {{"shadow0", hdr, ExtentDomain::Internal, gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment},
         {"shadow1", hdr, ExtentDomain::Internal, gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment}}};
}

gpu::PipelineLayoutDescription softShadowBindingLayout(uint32_t capacity)
{
    if (!capacity || capacity > MaxShadowAlphaTextures)
    {
        throw std::invalid_argument("Shadow alpha capacity must be in [1, 16384]");
    }
    using T = gpu::ResourceType;
    gpu::PipelineLayoutDescription result;
    const auto add = [&](uint32_t set, uint32_t vk, uint32_t metal, T type)
    {
        result.resourceBindings.push_back({set, {vk, metal}, type, gpu::ShaderStage::Compute, 1});
    };
    add(0, 0, 1, T::UniformBuffer);
    add(1, 1, 2, T::StorageBuffer);
    add(1, 2, 3, T::StorageBuffer); // Shared tracing vertices, including UV0/UV1.
    add(1, 3, 4, T::AccelerationStructure);
    add(0, 4, 0, T::SampledImage);
    add(0, 5, 1, T::SampledImage);
    add(0, 6, 8, T::Sampler);
    add(0, 7, 2, T::StorageImage);
    add(0, 8, 3, T::StorageImage);
    add(1, 9, 7, T::StorageBuffer); // Shared tracing indices.
    result.resourceBindings.push_back({2,{16,0},T::SampledImage,gpu::ShaderStage::Compute,capacity,{},5,true,false});
    result.resourceBindings.push_back({2,{24,0},T::Sampler,gpu::ShaderStage::Compute,capacity,{},6,true,false});
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
    _alphaCapacity = engine.device()->resourceArrayCapabilities().profileCapacity(2,1,10);
    const auto usages = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment;
    if (!caps.querySupported() || !_alphaCapacity || !engine.device()->formatCapabilities(gpu::PixelFormat::R16G16B16A16_SFLOAT).supports(usages))
    {
        _alphaCapacity = 0;
        _status = "Query shadows unavailable: query/acceleration, sampled arrays or visibility format support is missing";
        return;
    }
    const uint32_t slots = _requestedSlots ? _requestedSlots : engine.frameSlotCount();
    if (!slots)
    {
        throw std::invalid_argument("Query shadows require an explicit acquired slot count without a surface");
    }
    try
    {
        _layout = engine.device()->createPipelineLayout(softShadowBindingLayout(_alphaCapacity));
        gpu::ShaderLib library(_shaderRoot, engine.backendType());
        _shader = engine.backendType() == gpu::BackendType::Vulkan ?
            library.specialized("draw/query/soft_shadows",gpu::ShaderStage::Compute,engine.device(),{{0,_alphaCapacity}}) :
            library.compute("draw/query/soft_shadows", engine.device());
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
        _status = "Query shadows ready (alpha capacity=" + std::to_string(_alphaCapacity) + ")";
    }
    catch (const std::exception& error)
    {
        _pipeline.reset();
        _layout.reset();
        _shader.reset();
        _nearest.reset();
        _slots.clear();
        _alphaCapacity = 0;
        _status = std::string("Query shadows unavailable: ") + error.what();
    }
}

void SoftShadowPass::attachScene(std::shared_ptr<scene::Node>)
{
    _capacityWarningActive = false;
    if (_engine)
    {
        _engine->device()->waitIdleForResourceUpdate();
    }
    for (auto& slot : _slots)
    {
        slot = Slot{};
    }
    if (_pool)
    {
        _pool->collect();
    }
    _standaloneResources.reset();
}

void SoftShadowPass::resize(const LayerConfiguration&)
{
    attachScene(nullptr);
}

void SoftShadowPass::update(const FrameContext&)
{
}

void SoftShadowPass::prepare(const FrameContext& frame, const RenderQueue& queue,
    const glm::mat4& view, const glm::mat4& projection, std::shared_ptr<TracingSnapshot> snapshot)
{
    base::CpuProfileScope profile("effect.shadows.prepare");
    base::CpuProfileScope::count("shadow.prepareCalls", 1);
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
    slot.prepared = false;
    slot.sets.clear(); // Retire only this completed slot's previous bindings.
    if (_pool)
    {
        _pool->collect();
    }
    slot.snapshot = std::move(snapshot);
    slot.active = false;
    slot.built = false;
    _status = "Query shadows ready (alpha capacity=" + std::to_string(_alphaCapacity) + ")";
    if (_settings.enabled && lights)
    {
        if (slot.snapshot)
        {
            if (slot.snapshot->device != _engine->device() ||
                (!slot.snapshot->empty() && (!slot.snapshot->shadowInstances || !slot.snapshot->geometry ||
                    !slot.snapshot->geometry->vertexBuffer || !slot.snapshot->geometry->indexBuffer || !slot.snapshot->scene)) ||
                slot.snapshot->shadowTable.instances.size() != slot.snapshot->instances.size() ||
                slot.snapshot->shadowTable.alphaTextures.size() > _alphaCapacity)
            {
                throw std::invalid_argument("Shadow snapshot requires compatible prepared shadow resources");
            }
            base::CpuProfileScope::count("shadow.prepareFromSharedSnapshot", 1);
        }
        else
        {
            base::CpuProfileScope::count("shadow.prepareStandaloneSnapshot", 1);
            if (!_standaloneResources)
            {
                _standaloneResources = std::make_shared<TracingSceneResources>(*_engine->device(), _alphaCapacity);
            }
            slot.snapshot = _standaloneResources->prepare(queue, _alphaCapacity);
        }
        const auto& table = slot.snapshot->shadowTable;
        if (table.fallbackTextureSlots)
        {
            _status = "Query shadows ready; neutral alpha for " + std::to_string(table.fallbackTextureSlots) +
                " overflowed texture slots (capacity=" + std::to_string(_alphaCapacity) + ")";
            if (!_capacityWarningActive)
            {
                bg2e_log_warning << _status << "; all occluder instances retained" << bg2e_log_end;
            }
        }
        _capacityWarningActive = table.fallbackTextureSlots != 0;
        if (!table.instances.empty())
        {
            ShadowUniforms data;
            data.inverseViewProjection = packFrame(frame, view, projection).inverseViewProjection;
            data.parameters = {_settings.bias, _settings.directionalDistance, 0, 0};
            data.counts = {queue.lights().count.x, uint32_t(table.instances.size()), uint32_t(table.alphaTextures.size()), _settings.maxSamples};
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
            slot.active = true;
        }
    }
    else
    {
        // Disabled/unused queries do not require built Drawable tracing geometry.
        slot.snapshot.reset();
        _capacityWarningActive = false;
    }
    slot.serial = frame.frameNumber;
    slot.prepared = true;
}

void SoftShadowPass::record(const PassContext& context)
{
    base::CpuProfileScope profile("effect.shadows.record");
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
    if (context.inputs.size() != 2 || context.outputs.size() != 2)
    {
        throw std::invalid_argument("Shadow invocations require two inputs and two outputs");
    }
    for (const auto& input : context.inputs)
    {
        if (!input.image || !input.image->isValid() || input.image->device() != _engine->device())
        {
            throw std::invalid_argument("Shadow input must be a valid same-device image");
        }
    }
    for (const auto& output : context.outputs)
    {
        if (!output.image || !output.image->isValid() || output.image->device() != _engine->device())
        {
            throw std::invalid_argument("Shadow output must be a valid same-device image");
        }
    }
    auto receiverExtent = context.outputs.at(0).image->size();
    const auto fullExtent = receiverExtent;
    // Pointwise shadows need no temporal/filter halo. Keep the existing shader
    // ABI and dispatch a conservative origin-anchored rectangle of workgroups.
    // Clear the complete outputs so pixels outside the dispatch remain neutral.
    if (context.raster && context.raster->region)
    {
        const auto& r = *context.raster->region;
        bool eligible = r.width && r.height && r.x <= fullExtent.width && r.y <= fullExtent.height &&
            r.width <= fullExtent.width-r.x && r.height <= fullExtent.height-r.y;
        for (const auto& input : context.inputs)
        {
            eligible = eligible && input.image->size() == fullExtent;
        }
        for (const auto& output : context.outputs)
        {
            eligible = eligible && output.image->size() == fullExtent &&
                (output.image->usage() & gpu::ImageUsage::ColorAttachment) == gpu::ImageUsage::ColorAttachment;
        }
        if (eligible)
        {
            receiverExtent = {r.x+r.width, r.y+r.height};
            for (const auto& output : context.outputs)
            {
                RenderTarget::clear(command, *output.image, {1, 1, 1, 1});
            }
        }
    }
    if (!slot.built)
    {
        slot.snapshot->build(command);
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
        base::CpuProfileScope bindingProfile("shadow.prepareBindings");
        Slot::Invocation invocation;
        invocation.stable = slot.snapshot->shadowResourceSet(_layout, _alphaCapacity, 1, _bindingReuseEnabled);
        invocation.alpha = slot.snapshot->shadowAlphaResourceSet(_layout, _alphaCapacity, _bindingReuseEnabled);
        if (!_pool)
        {
            _pool = std::make_shared<TemporaryResourcePool>(*_engine->device());
        }
        _pool->setEnabled(_poolingEnabled);
        invocation.pooled = _pool->bindings(command, _layout, 0, 0, {context.frame.frameSlot, slot.sets.size(), 0, 0});
        auto set = invocation.pooled->resources();
        invocation.pooled->setUniformBuffer({0, 1}, slot.uniforms.get());
        invocation.pooled->setSampledImage({4, 0}, context.inputs.at(0).image);
        invocation.pooled->setSampledImage({5, 1}, context.inputs.at(1).image);
        invocation.pooled->setSampler({6, 8}, _nearest.get());
        invocation.pooled->setStorageImage({7, 2}, context.outputs.at(0).image);
        invocation.pooled->setStorageImage({8, 3}, context.outputs.at(1).image);
        invocation.pooled->update();
        invocation.resources = std::move(set);
        for (const auto& input : context.inputs)
        {
            invocation.owners.push_back(input.image->weak_from_this().lock());
        }
        for (const auto& output : context.outputs)
        {
            invocation.owners.push_back(output.image->weak_from_this().lock());
        }
        slot.sets.push_back(std::move(invocation));
        base::CpuProfileScope::count("shadow.invocationBindings", 1);
    }
    command.beginCompute();
    command.bindPipeline(_pipeline.get());
    command.bindResourceSet(_pipeline.get(), 0, slot.sets.back().resources.get());
    command.bindResourceSet(_pipeline.get(), 1, slot.sets.back().stable.get());
    command.bindResourceSet(_pipeline.get(), 2, slot.sets.back().alpha.get());
    const auto groupsX = (receiverExtent.width+15)/16;
    const auto groupsY = (receiverExtent.height+15)/16;
    base::CpuProfileScope::count("shadow.receiverInvocations", uint64_t(groupsX)*groupsY*256);
    base::CpuProfileScope::count("shadow.fullReceiverPixels", uint64_t(fullExtent.width)*fullExtent.height, "pixels");
    command.dispatch(groupsX, groupsY, 1);
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
        _engine->device()->waitIdleForResourceUpdate();
    }
    _slots.clear();
    _pool.reset();
    _standaloneResources.reset();
    _pipeline.reset();
    _shader.reset();
    _layout.reset();
    _nearest.reset();
    _alphaCapacity = 0;
    _capacityWarningActive = false;
    _engine = nullptr;
    _status = "Not initialized";
}

bool SoftShadowPass::sceneSupported() const
{
    return available(); // Exhausted texture slots no longer disable scene shadows.
}

uint32_t SoftShadowPass::alphaTextureCapacity() const
{
    return _alphaCapacity;
}


TemporaryPoolStatistics SoftShadowPass::poolStatistics() const
{
    return _pool ? _pool->statistics() : TemporaryPoolStatistics{};
}

}
