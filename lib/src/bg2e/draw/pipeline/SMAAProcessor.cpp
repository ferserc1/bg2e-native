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

#include <bg2e/draw/pipeline/SMAAProcessor.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <AreaTex.h>
#include <SearchTex.h>
#include <cmath>
#include <glm/vec4.hpp>

namespace bg2e::draw::pipeline {
namespace {
struct alignas(16) SMAAUniforms {
    glm::vec4 metrics{0};
    glm::vec4 parameters{0}; // luma threshold, search steps, reserved.
};
static_assert(sizeof(SMAAUniforms) == 32);
}

SMAAProcessor::SMAAProcessor(SMAAConfiguration configuration)
    : _configuration(std::move(configuration))
{
    if (_configuration.shaderRoot.empty())
    {
        _configuration.shaderRoot = base::PlatformTools::shaderPath();
    }
}

void SMAAProcessor::initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots)
{
    if (_device || !device.isValid() || !slots || _configuration.shaderInterface != "draw.smaa.v1")
    {
        throw std::invalid_argument("SMAA requires a clean lifecycle, valid slots/device and draw.smaa.v1");
    }
    _device = &device;
    const auto target = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
    for (auto format : {gpu::PixelFormat::R8G8_UNORM,gpu::PixelFormat::R8G8B8A8_UNORM,gpu::PixelFormat::R16G16B16A16_SFLOAT})
    {
        const auto caps = device.formatCapabilities(format);
        if (!caps.supports(target) || !caps.linearFiltering)
        {
            _status = "SMAA unavailable: filterable RG8/RGBA8/RGBA16 storage formats are required";
            return;
        }
    }
    if (!device.formatCapabilities(gpu::PixelFormat::R8_UNORM).supports(gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst) ||
        !device.formatCapabilities(gpu::PixelFormat::R8_UNORM).linearFiltering ||
        !device.formatCapabilities(gpu::PixelFormat::R8G8_UNORM).supports(gpu::ImageUsage::TransferDst))
    {
        _status = "SMAA unavailable: filterable R8/RG8 lookup upload formats are required";
        return;
    }
    try
    {
        gpu::PipelineLayoutDescription layout;
        layout.resourceBindings.push_back({0,{0,1},gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Compute});
        for (uint32_t i = 0; i < 3; ++i)
        {
            layout.resourceBindings.push_back({0,{1+i,i},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
        }
        layout.resourceBindings.push_back({0,{4,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute});
        layout.resourceBindings.push_back({0,{5,3},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
        _layout = device.createPipelineLayout(layout);
        gpu::ShaderLib library(_configuration.shaderRoot,backend);
        const char* names[] = {"edge","weights","blend"};
        for (uint32_t i = 0; i < 3; ++i)
        {
            _shaders[i] = library.compute(_configuration.shaderPrefix+"/"+names[i],&device);
            _pipelines[i] = device.createComputePipeline({_shaders[i].get(),_layout.get()});
        }
        gpu::SamplerDescription linear;
        linear.minFilter = linear.magFilter = gpu::Filter::Linear;
        linear.addressModeU = linear.addressModeV = gpu::AddressMode::ClampToEdge;
        linear.maxLod = 0;
        _linear = device.createSampler(linear);
        gpu::ImageDescription description;
        description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
        description.size = {AREATEX_WIDTH,AREATEX_HEIGHT};
        description.format = gpu::PixelFormat::R8G8_UNORM;
        _area = device.createImage(description);
        _area->upload({areaTexBytes,sizeof(areaTexBytes),AREATEX_PITCH,0,0});
        description.size = {SEARCHTEX_WIDTH,SEARCHTEX_HEIGHT};
        description.format = gpu::PixelFormat::R8_UNORM;
        _search = device.createImage(description);
        _search->upload({searchTexBytes,sizeof(searchTexBytes),SEARCHTEX_PITCH,0,0});
        _slots.resize(slots);
        _ready = true;
        _status = "SMAA 1x orthogonal lookup passes ready";
    }
    catch (const std::exception& error)
    {
        _status = std::string("SMAA unavailable: ")+error.what();
        _pipelines = {};
        _shaders = {};
        _layout.reset();
        _linear.reset();
        _area.reset();
        _search.reset();
        _slots.clear();
    }
}

void SMAAProcessor::initialize(Engine& engine, gpu::PixelFormat format, uint32_t slots)
{
    initialize(*engine.device(),engine.backendType(),slots);
    _engine = &engine;
    if (!available())
    {
        return;
    }
    try
    {
        gpu::PipelineLayoutDescription layout;
        layout.resourceBindings.push_back({0,{0,0},gpu::ResourceType::SampledImage,gpu::ShaderStage::Fragment});
        layout.resourceBindings.push_back({0,{1,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Fragment});
        _copy.initialize(engine,_configuration.shaderRoot,_configuration.shaderPrefix+"/copy",engine.device()->createPipelineLayout(layout),format,slots);
        _copyReady = true;
    }
    catch (const std::exception& error)
    {
        _copy.cleanup();
        _ready = false;
        _status = std::string("SMAA final copy unavailable: ")+error.what();
    }
}

SMAAResult SMAAProcessor::process(const SMAAInput& input)
{
    if (!_device || input.command.hasActiveScope() || !input.color.isValid() || input.color.device() != _device ||
        input.color.size().isZero() || input.color.imageType() != gpu::ImageType::Image2D ||
        !gpu::hasFlag(input.color.usage(),gpu::ImageUsage::Sampled) || gpu::isDepthFormat(input.color.pixelFormat()) ||
        gpu::isUnsignedIntegerFormat(input.color.pixelFormat()) || gpu::isSignedIntegerFormat(input.color.pixelFormat()))
    {
        throw std::invalid_argument("SMAA requires a same-device sampled normalized/float color 2D signal outside scopes");
    }
    if (!enabled() || !available() || !_device->formatCapabilities(input.color.pixelFormat()).linearFiltering)
    {
        return {&input.color,{}};
    }
    if (!std::isfinite(threshold()) || threshold() < 0.001f || threshold() > 1 || !searchSteps() || searchSteps() > 32)
    {
        throw std::invalid_argument("Invalid effective SMAA threshold/search accessor values");
    }
    auto& slot = _slots.at(input.slot);
    if (slot.prepared && slot.serial == input.serial)
    {
        throw std::logic_error("SMAA processes one final color per completed slot/serial");
    }
    slot = {};
    slot.serial = input.serial;
    slot.prepared = true;
    gpu::ImageDescription image;
    image.size = input.color.size();
    image.usage = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc;
    const gpu::PixelFormat formats[] = {gpu::PixelFormat::R8G8_UNORM,gpu::PixelFormat::R8G8B8A8_UNORM,gpu::PixelFormat::R16G16B16A16_SFLOAT};
    for (uint32_t i = 0; i < 3; ++i)
    {
        image.format = formats[i];
        slot.images[i] = _device->createImage(image);
    }
    SMAAUniforms data;
    data.metrics = {1.0f/input.color.width(),1.0f/input.color.height(),float(input.color.width()),float(input.color.height())};
    data.parameters = {threshold(),float(searchSteps()),0,0};
    slot.uniforms = _device->createBuffer("SMAA frame uniforms");
    slot.uniforms->createUniformBuffer(data);
    for (uint32_t pass = 0; pass < 3; ++pass)
    {
        // Every binding is initialized; unused inputs retain valid sampled images.
        const std::array<gpu::Image*,3> inputs = pass == 0 ? std::array<gpu::Image*,3>{&input.color,_area.get(),_search.get()} :
            (pass == 1 ? std::array<gpu::Image*,3>{slot.images[0].get(),_area.get(),_search.get()} :
             std::array<gpu::Image*,3>{&input.color,slot.images[1].get(),_search.get()});
        slot.sets[pass] = _device->createResourceSet(_layout.get(),0);
        auto& set = slot.sets[pass];
        set->setUniformBuffer({0,1},slot.uniforms.get());
        for (uint32_t i = 0; i < 3; ++i)
        {
            input.command.transition(inputs[i],gpu::ImageLayout::ShaderReadOnly);
            set->setSampledImage({i+1,i},inputs[i]);
        }
        set->setSampler({4,0},_linear.get());
        set->setStorageImage({5,3},slot.images[pass].get());
        set->update();
        input.command.transition(slot.images[pass].get(),gpu::ImageLayout::General);
        input.command.beginCompute();
        input.command.bindPipeline(_pipelines[pass].get());
        input.command.bindResourceSet(_pipelines[pass].get(),0,set.get());
        input.command.dispatch((input.color.width()+15)/16,(input.color.height()+15)/16,1);
        input.command.endCompute();
        input.command.transition(slot.images[pass].get(),gpu::ImageLayout::ShaderReadOnly);
    }
    return {slot.images[2].get(),slot.images[2]};
}

bool SMAAProcessor::record(const FrameContext& frame)
{
    if (!_engine || &frame.engine != _engine || !_copyReady || !enabled() || !available())
    {
        return false;
    }
    auto result = process({frame.commandBuffer,frame.frameSlot,frame.frameNumber,frame.colorTarget});
    if (!result.owner)
    {
        return false;
    }
    _copy.draw(frame,frame.colorTarget,{result.image},0,{});
    return true;
}

void SMAAProcessor::cleanup()
{
    if (_device)
    {
        _device->waitIdleForResourceUpdate();
    }
    _copy.cleanup();
    _slots.clear();
    _pipelines = {};
    _shaders = {};
    _layout.reset();
    _linear.reset();
    _area.reset();
    _search.reset();
    _device = nullptr;
    _engine = nullptr;
    _ready = _copyReady = false;
    _status = "Not initialized";
}

bool SMAAProcessor::enabled() const
{
    return _enabled;
}

void SMAAProcessor::setEnabled(bool value)
{
    _enabled = value;
}

float SMAAProcessor::threshold() const
{
    return _threshold;
}

void SMAAProcessor::setThreshold(float value)
{
    if (!std::isfinite(value) || value < 0.001f || value > 1)
    {
        throw std::invalid_argument("SMAA threshold must be in [0.001,1]");
    }
    _threshold = value;
}

uint32_t SMAAProcessor::searchSteps() const
{
    return _searchSteps;
}

void SMAAProcessor::setSearchSteps(uint32_t value)
{
    if (!value || value > 32)
    {
        throw std::invalid_argument("SMAA search steps must be 1..32");
    }
    _searchSteps = value;
}

bool SMAAProcessor::available() const
{
    return _ready;
}

std::string SMAAProcessor::status() const
{
    return _status;
}

reflection::TypeInfo SMAAProcessor::settingsType() const
{
    reflection::TypeInfoBuilder<SMAAProcessor> type("bg2e.draw.smaa.v1");
    type.property("enabled", &SMAAProcessor::enabled, &SMAAProcessor::setEnabled).category("Spatial AA");
    type.property("threshold", &SMAAProcessor::threshold, &SMAAProcessor::setThreshold).category("Spatial AA").range(0.001f,1);
    type.property("searchSteps", &SMAAProcessor::searchSteps, &SMAAProcessor::setSearchSteps).category("Spatial AA").range(1,32);
    type.property("available", &SMAAProcessor::available).category("Capabilities");
    type.property("status", &SMAAProcessor::status).category("Capabilities");
    return type.build();
}

void* SMAAProcessor::settingsInstance()
{
    return this;
}
}
