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

#include <bg2e/draw/FrameDataBinding.hpp>
#include <bg2e/draw/ObjectDataBinding.hpp>
#include <bg2e/draw/LightDataBinding.hpp>
#include <bg2e/draw/EnvironmentDataBinding.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace bg2e::draw {
namespace {
uint32_t slotCount(Engine& engine, uint32_t count)
{
    if (count == 0)
    {
        count = engine.frameSlotCount();
    }
    if (count == 0)
    {
        throw std::invalid_argument("Data bindings need at least one acquired frame slot");
    }
    return count;
}

void validateLayout(Engine& engine, const std::shared_ptr<gpu::PipelineLayout>& layout)
{
    if (!layout || !layout->isValid() || layout->device() != engine.device())
    {
        throw std::invalid_argument("Draw binding layout must belong to its engine");
    }
}

void validateTexture(const std::shared_ptr<Texture>& texture, Engine& engine)
{
    if (!texture || texture->engine() != &engine || !texture->image() || !texture->image()->isValid() ||
        texture->image()->device() != engine.device() || texture->image()->isCubemap() ||
        !texture->sampler() || !texture->sampler()->isValid() || texture->sampler()->device() != engine.device() ||
        texture->image()->currentLayout() != gpu::ImageLayout::ShaderReadOnly)
    {
        throw std::invalid_argument("Object texture realization is stale or belongs to another context");
    }
}
}

gpu::PipelineLayoutDescription drawBindingLayout()
{
    gpu::PipelineLayoutDescription result;
    auto add = [&](uint32_t set, uint32_t vk, uint32_t metal, gpu::ResourceType type, gpu::ShaderStage stage)
    {
        result.resourceBindings.push_back({set, {.vulkan = vk, .metal = metal}, type, stage, 1});
    };
    add(0, 0, 2, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex);
    add(0, 1, 6, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment);
    add(1, 0, 3, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex);
    add(1, 1, 7, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment);
    for (uint32_t i = 0; i < 6; ++i)
    {
        add(1, 2 + i, i, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment);
        add(1, 8 + i, i, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment);
    }
    add(2, 0, 4, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment);
    for (uint32_t i = 0; i < 4; ++i)
    {
        add(3, i, 6 + i, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment);
        add(3, 4 + i, 6 + i, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment);
    }
    add(3, 8, 5, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment);
    result.debugName = "draw.bindings.v1";
    return result;
}

void DataBinding::initializeUniforms(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout,
    uint32_t setIndex, size_t byteSize, const std::vector<gpu::ShaderBinding>& bindings, uint32_t frameSlots)
{
    if (_engine && _engine != &engine)
    {
        throw std::logic_error("Clean up data bindings before changing engines");
    }
    validateLayout(engine, layout);
    const auto count = slotCount(engine, frameSlots);
    cleanup();
    _engine = &engine;
    _layout = std::move(layout);
    _byteSize = byteSize;
    _slots.resize(count);
    std::vector<uint8_t> zero(byteSize, 0);
    for (auto& slot : _slots)
    {
        slot.buffer = engine.device()->createBuffer("Draw frame uniform data");
        slot.buffer->createUniformBuffer(zero.data(), byteSize);
        slot.set = engine.device()->createResourceSet(_layout.get(), setIndex);
        for (const auto binding : bindings)
        {
            slot.set->setUniformBuffer(binding, slot.buffer.get());
        }
        slot.set->update();
    }
}

DataBinding::Slot& DataBinding::updateBytes(const FrameContext& frame, const void* data, size_t byteSize)
{
    if (_engine != &frame.engine || frame.frameSlot >= _slots.size() || byteSize != _byteSize || !data)
    {
        throw std::invalid_argument("Data binding update does not match its acquired frame slot");
    }
    auto& slot = _slots.at(frame.frameSlot);
    if (slot.prepared && slot.frameNumber == frame.frameNumber)
    {
        throw std::logic_error("Do not overwrite mutable data already prepared for this frame");
    }
    if (!slot.buffer || !slot.set)
    {
        throw std::logic_error("Data binding initialization did not complete");
    }
    slot.buffer->updateUniformBuffer(data, byteSize);
    slot.frameNumber = frame.frameNumber;
    slot.prepared = true;
    return slot;
}

gpu::ResourceSet* DataBinding::set(const FrameContext& frame) const
{
    if (&frame.engine != _engine || frame.frameSlot >= _slots.size())
    {
        throw std::invalid_argument("Data binding belongs to another context/frame slot");
    }
    const auto& slot = _slots.at(frame.frameSlot);
    if (!slot.prepared || slot.frameNumber != frame.frameNumber || !slot.set)
    {
        throw std::logic_error("Prepare this slot before binding its frame data");
    }
    return slot.set.get();
}

void DataBinding::cleanup()
{
    if (_engine)
    {
        _engine->device()->waitIdle();
    }
    _slots.clear();
    _layout.reset();
    _byteSize = 0;
    _engine = nullptr;
}

void FrameDataBinding::initialize(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout, uint32_t frameSlots)
{
    initializeUniforms(engine, std::move(layout), 0, sizeof(FrameData), {{.vulkan = 0, .metal = 2}, {.vulkan = 1, .metal = 6}}, frameSlots);
}

void FrameDataBinding::update(const FrameContext& frame, const glm::mat4& view, const glm::mat4& projection)
{
    const auto data = packFrame(frame, view, projection);
    updateBytes(frame, &data, sizeof(data));
}

void LightDataBinding::initialize(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout, uint32_t frameSlots)
{
    initializeUniforms(engine, std::move(layout), 2, sizeof(LightUniforms), {{.vulkan = 0, .metal = 4}}, frameSlots);
}

void LightDataBinding::update(const FrameContext& frame, const LightUniforms& lights)
{
    if (lights.count.x > MaxLights)
    {
        throw std::length_error("Draw light binding supports at most eight lights");
    }
    for (uint32_t n = 0; n < lights.count.x; ++n)
    {
        for (const auto& value : {lights.lights[n].positionIntensity, lights.lights[n].color, lights.lights[n].directionAngle, lights.lights[n].parameters})
        {
            for (int i = 0; i < 4; ++i)
            {
                if (!std::isfinite(value[i]))
                {
                    throw std::invalid_argument("Light data must be finite");
                }
            }
        }
    }
    updateBytes(frame, &lights, sizeof(lights));
}

void EnvironmentDataBinding::initialize(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout, uint32_t frameSlots, uint32_t metalTextureBase)
{
    initializeUniforms(engine, std::move(layout), 3, sizeof(EnvironmentData), {{.vulkan = 8, .metal = 5}}, frameSlots);
    _metalTextureBase = metalTextureBase;
    _owners.resize(_slots.size());
}

void EnvironmentDataBinding::update(const FrameContext& frame, std::shared_ptr<const environment::EnvironmentResources> environment)
{
    if (!environment || _engine != &frame.engine || frame.frameSlot >= _owners.size())
    {
        throw std::invalid_argument("Environment frame binding requires a compatible resource snapshot");
    }
    environment->validate(frame.engine.device());
    EnvironmentData data;
    data.parameters = {float(environment->prefilteredSpecular->mipLevels() - 1), environment->skyboxBlur, environment->skyboxVisible ? 1.0f : 0.0f, 0};
    auto& slot = updateBytes(frame, &data, sizeof(data));
    slot.prepared = false; // Do not expose partially rebound environment sets.
    const std::array<std::shared_ptr<gpu::Image>, 4> images = {environment->sky, environment->irradiance, environment->prefilteredSpecular, environment->brdf};
    for (uint32_t i = 0; i < images.size(); ++i)
    {
        slot.set->setSampledImage({.vulkan = i, .metal = _metalTextureBase + i}, images[i].get());
        slot.set->setSampler({.vulkan = 4 + i, .metal = _metalTextureBase + i}, i == 3 ? environment->brdfSampler.get() : environment->sampler.get());
    }
    slot.set->update();
    _owners.at(frame.frameSlot) = std::move(environment);
    slot.prepared = true;
}

void EnvironmentDataBinding::cleanup()
{
    DataBinding::cleanup();
    _owners.clear();
}

void ObjectDataBinding::initialize(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout, uint32_t frameSlots)
{
    if (_engine && _engine != &engine)
    {
        throw std::logic_error("Clean up object bindings before changing engines");
    }
    validateLayout(engine, layout);
    const auto count = slotCount(engine, frameSlots);
    cleanup();
    _engine = &engine;
    _layout = std::move(layout);
    _slots.resize(count);
}

void ObjectDataBinding::prepare(const FrameContext& frame, const RenderQueue& queue)
{
    if (_engine != &frame.engine || queue.engine() != _engine || frame.frameSlot >= _slots.size())
    {
        throw std::invalid_argument("Object queue and acquired frame slot must share an engine");
    }
    queue.validate(frame.engine);
    auto& slot = _slots.at(frame.frameSlot);
    if (slot.prepared && slot.frameNumber == frame.frameNumber)
    {
        throw std::logic_error("Do not rewrite object data already prepared for this frame");
    }
    for (const auto& item : queue.items())
    {
        for (const auto& texture : item.textures)
        {
            validateTexture(texture, frame.engine);
        }
    }
    slot.records.resize(queue.items().size());
    for (size_t i = 0; i < queue.items().size(); ++i)
    {
        const auto& item = queue.items()[i];
        if (item.objectId != i + 1)
        {
            throw std::invalid_argument("Object IDs must index the complete queue snapshot");
        }
        auto& record = slot.records[i];
        record.owner = item;
        ObjectData data;
        data.model = item.worldMatrix;
        data.normal = glm::transpose(glm::inverse(item.worldMatrix));
        data.material = item.materialData;
        data.identity = {item.objectId, item.submeshIndex, 0, 0};
        if (!record.buffer)
        {
            record.buffer = frame.engine.device()->createBuffer("Draw object uniform data");
            record.buffer->createUniformBuffer(data);
        }
        else
        {
            record.buffer->updateUniformBuffer(data);
        }
        if (!record.set)
        {
            record.set = frame.engine.device()->createResourceSet(_layout.get(), 1);
            record.set->setUniformBuffer({.vulkan = 0, .metal = 3}, record.buffer.get());
            record.set->setUniformBuffer({.vulkan = 1, .metal = 7}, record.buffer.get());
        }
        for (uint32_t texture = 0; texture < item.textures.size(); ++texture)
        {
            record.set->setSampledImage({.vulkan = 2 + texture, .metal = texture}, item.textures[texture]->image());
            record.set->setSampler({.vulkan = 8 + texture, .metal = texture}, item.textures[texture]->sampler());
        }
        record.set->update();
    }
    slot.frameNumber = frame.frameNumber;
    slot.prepared = true;
}

gpu::ResourceSet* ObjectDataBinding::set(const FrameContext& frame, uint32_t objectId) const
{
    if (_engine != &frame.engine || frame.frameSlot >= _slots.size())
    {
        throw std::invalid_argument("Object data belongs to another acquired frame slot");
    }
    const auto& slot = _slots.at(frame.frameSlot);
    if (!slot.prepared || slot.frameNumber != frame.frameNumber || objectId == 0 || objectId > slot.records.size())
    {
        throw std::out_of_range("Prepare the object snapshot before binding its one-based ID");
    }
    return slot.records.at(objectId - 1).set.get();
}

void ObjectDataBinding::cleanup()
{
    if (_engine)
    {
        _engine->device()->waitIdle();
    }
    _slots.clear();
    _layout.reset();
    _engine = nullptr;
}
}
