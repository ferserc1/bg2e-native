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

#include <bg2e/draw/pipeline/TemporalAccumulator.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace bg2e::draw::pipeline {
namespace {
struct alignas(16) TemporalUniforms {
    glm::mat4 inverseViewProjection{1}, previousViewProjection{1};
    glm::vec4 rejection{0}; // absolute/relative position, normal cosine, previous valid.
    glm::uvec4 counts{0}; // signal kind, frame limit, freeze, explicit world profile.
    glm::uvec4 extent{0}; // source and output dimensions.
    glm::vec4 cameraRoughness{0};
    glm::uvec4 flags{0}; // camera reprojection enabled.
};
static_assert(sizeof(TemporalUniforms) == 208 && offsetof(TemporalUniforms, flags) == 192);
}

TemporalAccumulator::TemporalAccumulator(TemporalConfiguration configuration)
    : _configuration(std::move(configuration))
{
    if (_configuration.shaderRoot.empty())
    {
        _configuration.shaderRoot = base::PlatformTools::shaderPath();
    }
}

void TemporalAccumulator::initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots)
{
    if (_device || !slots || !device.isValid() || !_configuration.maxHistories ||
        _configuration.shaderInterface != "draw.temporal.v1")
    {
        throw std::invalid_argument("Temporal initialization requires a clean lifecycle, valid slots/capacity/interface");
    }
    _device = &device;
    const auto usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
    for (auto format : {gpu::PixelFormat::R16_SFLOAT,gpu::PixelFormat::R16G16B16A16_SFLOAT,gpu::PixelFormat::R32G32B32A32_SFLOAT})
    {
        if (!device.formatCapabilities(format).supports(usage))
        {
            _status = "Temporal unavailable: required scalar/HDR/position storage formats are missing";
            return;
        }
    }
    try
    {
        gpu::PipelineLayoutDescription layout;
        layout.resourceBindings.push_back({0,{0,1},gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Compute});
        for (uint32_t i = 1; i <= 8; ++i)
        {
            layout.resourceBindings.push_back({0,{i,i-1},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
        }
        layout.resourceBindings.push_back({0,{9,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute});
        for (uint32_t i = 10; i <= 12; ++i)
        {
            layout.resourceBindings.push_back({0,{i,i-2},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
        }
        _layout = device.createPipelineLayout(layout);
        gpu::ShaderLib library(_configuration.shaderRoot,backend);
        for (uint32_t i = 0; i < 4; ++i)
        {
            const auto name = _configuration.shaderPrefix+"/"+(i >= 2 ? "world_" : "camera_")+(i%2 ? "hdr" : "scalar");
            _shaders[i] = library.compute(name,&device);
            _pipelines[i] = device.createComputePipeline({_shaders[i].get(),_layout.get()});
        }
        gpu::SamplerDescription nearest;
        nearest.minFilter = nearest.magFilter = gpu::Filter::Nearest;
        nearest.addressModeU = nearest.addressModeV = gpu::AddressMode::ClampToEdge;
        nearest.maxLod = 0;
        _nearest = device.createSampler(nearest);
        const float zero[4] = {};
        const uint16_t halfZero[4] = {};
        gpu::ImageDescription description;
        description.size = {1,1};
        description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
        description.format = gpu::PixelFormat::R32G32B32A32_SFLOAT;
        _zeroPosition = device.createImage(description);
        _zeroPosition->upload({zero,sizeof(zero),sizeof(zero),0,0});
        description.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
        _zeroNormal = device.createImage(description);
        _zeroNormal->upload({halfZero,sizeof(halfZero),sizeof(halfZero),0,0});
        _slots.resize(slots);
        _status = "Temporal accumulation ready";
    }
    catch (const std::exception& error)
    {
        _pipelines = {};
        _shaders = {};
        _layout.reset();
        _nearest.reset();
        _zeroPosition.reset();
        _zeroNormal.reset();
        _slots.clear();
        _status = std::string("Temporal unavailable: ")+error.what();
    }
}

void TemporalAccumulator::beginFrame(uint32_t slotIndex, uint64_t serial, uint64_t sequence)
{
    if (!available())
    {
        return;
    }
    if (_hasSequence && sequence < _latestSequence)
    {
        throw std::logic_error("Temporal scene sequence cannot move backwards within a lifecycle");
    }
    auto& slot = _slots.at(slotIndex);
    if (slot.prepared && slot.serial == serial)
    {
        throw std::logic_error("Begin temporal acquired slot only once per serial");
    }
    _latestSequence = sequence;
    _hasSequence = true;
    slot = {};
    slot.serial = serial;
    slot.sequence = sequence;
    slot.prepared = true;
    for (auto it = _histories.begin(); it != _histories.end();)
    {
        if (it->second->sequence < sequence && sequence-it->second->sequence > 1)
        {
            it = _histories.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

TemporalResult TemporalAccumulator::record(const TemporalInput& input)
{
    if (!_device || input.command.hasActiveScope() || input.signal.device() != _device || !input.signal.isValid() ||
        input.signal.size().isZero() || input.signal.imageType() != gpu::ImageType::Image2D ||
        !gpu::hasFlag(input.signal.usage(),gpu::ImageUsage::Sampled) ||
        uint32_t(input.key.signal) > uint32_t(TemporalSignal::ReflectedRadiance))
    {
        throw std::invalid_argument("Temporal input requires a valid same-device sampled signal and inactive scope");
    }
    const bool scalar = input.key.signal == TemporalSignal::Visibility;
    if (input.signal.pixelFormat() != (scalar ? gpu::PixelFormat::R16_SFLOAT : gpu::PixelFormat::R16G16B16A16_SFLOAT))
    {
        throw std::invalid_argument("Temporal signal kind does not match scalar/HDR format");
    }
    if (!available() || !_settings.enabled)
    {
        _histories.erase(input.key);
        return {&input.signal,{},1};
    }
    auto& slot = _slots.at(input.slot);
    if (!slot.prepared || slot.serial != input.serial || slot.sequence != input.sceneFrameNumber)
    {
        throw std::logic_error("Begin the acquired temporal scene slot before recording");
    }
    input.surface.validate(*_device);
    MotionVectorGenerator::validate(input.view);
    if (input.key.signal == TemporalSignal::ReflectedRadiance && (!input.material || !input.material->isValid() ||
        input.material->device() != _device || input.material->size() != input.surface.extent() ||
        input.material->imageType() != gpu::ImageType::Image2D || !gpu::hasFlag(input.material->usage(),gpu::ImageUsage::Sampled) ||
        (input.material->pixelFormat() != gpu::PixelFormat::R8G8B8A8_UNORM && input.material->pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT) ||
        !std::isfinite(input.maxRoughness) || input.maxRoughness < 0 || input.maxRoughness > 1))
    {
        throw std::invalid_argument("Temporal reflections require matching roughness and threshold");
    }
    auto found = _histories.find(input.key);
    auto previous = found != _histories.end() ? found->second : nullptr;
    if (previous && previous->sequence == input.sceneFrameNumber)
    {
        throw std::logic_error("Record each effect/view/layer once per rendered scene sequence");
    }
    const bool consecutive = previous && previous->sequence < input.sceneFrameNumber &&
        input.sceneFrameNumber-previous->sequence == 1;
    if (!consecutive || previous->generation != input.generation || previous->reset != _generation ||
        previous->signal->size() != input.signal.size() || previous->surfaceExtent != input.surface.extent() ||
        previous->profile != input.surface.profile || previous->view.reproject != input.view.reproject ||
        previous->view.projection != input.view.projection || previous->maxRoughness != input.maxRoughness)
    {
        previous.reset();
    }
    if (found == _histories.end() && _histories.size() >= _configuration.maxHistories)
    {
        throw std::length_error("Temporal effect/view/layer history capacity exceeded");
    }
    auto next = std::make_shared<History>();
    next->sequence = input.sceneFrameNumber;
    next->generation = input.generation;
    next->reset = _generation;
    next->view = input.view;
    next->profile = input.surface.profile;
    next->surfaceExtent = input.surface.extent();
    next->maxRoughness = input.maxRoughness;
    next->count = previous ? std::min(_settings.maxFrames,previous->count+1) : 1;
    gpu::ImageDescription description;
    description.size = input.signal.size();
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
    description.format = input.signal.pixelFormat();
    next->signal = _device->createImage(description);
    description.format = gpu::PixelFormat::R32G32B32A32_SFLOAT;
    next->position = _device->createImage(description);
    description.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
    next->normal = _device->createImage(description);
    Invocation invocation;
    invocation.previous = previous;
    invocation.next = next;
    TemporalUniforms data;
    data.inverseViewProjection = input.surface.inverseViewProjection;
    data.previousViewProjection = previous ? previous->view.jitteredProjection*previous->view.view : glm::mat4(1);
    data.rejection = {_settings.absolutePositionThreshold,_settings.relativePositionThreshold,_settings.normalThreshold,previous ? 1.0f : 0.0f};
    data.counts = {uint32_t(input.key.signal),_settings.maxFrames,_settings.freezeAtLimit ? 1u : 0u,
        input.surface.profile == SurfaceProfile::WorldPositionCoverage ? 1u : 0u};
    data.extent = {input.surface.extent().width,input.surface.extent().height,input.signal.width(),input.signal.height()};
    data.cameraRoughness = glm::vec4(glm::vec3(glm::inverse(input.view.view)[3]),input.maxRoughness);
    data.flags.x = input.view.reproject ? 1u : 0u;
    invocation.uniforms = _device->createBuffer("Temporal invocation uniforms");
    invocation.uniforms->createUniformBuffer(data);
    invocation.resources = _device->createResourceSet(_layout.get(),0);
    invocation.resources->setUniformBuffer({0,1},invocation.uniforms.get());
    auto* coverage = input.surface.profile == SurfaceProfile::CameraDepth ? input.surface.normal : input.surface.coverage;
    const std::array<gpu::Image*,8> sources{input.surface.normal,input.surface.depthOrPosition,coverage,&input.signal,
        previous ? previous->signal.get() : &input.signal,previous ? previous->position.get() : _zeroPosition.get(),
        previous ? previous->normal.get() : _zeroNormal.get(),input.key.signal == TemporalSignal::ReflectedRadiance ? input.material : input.surface.normal};
    for (uint32_t i = 0; i < sources.size(); ++i)
    {
        input.command.transition(sources[i],gpu::ImageLayout::ShaderReadOnly);
        invocation.resources->setSampledImage({i+1,i},sources[i]);
    }
    invocation.resources->setSampler({9,0},_nearest.get());
    const std::array<gpu::Image*,3> outputs{next->signal.get(),next->position.get(),next->normal.get()};
    for (uint32_t i = 0; i < outputs.size(); ++i)
    {
        input.command.transition(outputs[i],gpu::ImageLayout::General);
        invocation.resources->setStorageImage({i+10,i+8},outputs[i]);
    }
    invocation.resources->update();
    slot.invocations.push_back(std::move(invocation));
    const uint32_t index = (input.surface.profile == SurfaceProfile::WorldPositionCoverage ? 2u : 0u)+(scalar ? 0u : 1u);
    input.command.beginCompute();
    input.command.bindPipeline(_pipelines[index].get());
    input.command.bindResourceSet(_pipelines[index].get(),0,slot.invocations.back().resources.get());
    input.command.dispatch((input.signal.width()+15)/16,(input.signal.height()+15)/16,1);
    input.command.endCompute();
    for (auto* image : outputs)
    {
        input.command.transition(image,gpu::ImageLayout::ShaderReadOnly);
    }
    _histories[input.key] = next;
    return {next->signal.get(),next->signal,next->count};
}

void TemporalAccumulator::invalidate()
{
    if (_generation == std::numeric_limits<uint64_t>::max())
    {
        throw std::overflow_error("Temporal reset generation exhausted");
    }
    ++_generation;
    _histories.clear();
}

void TemporalAccumulator::cleanup()
{
    if (_device)
    {
        _device->waitIdle();
    }
    _histories.clear();
    _slots.clear();
    _pipelines = {};
    _shaders = {};
    _layout.reset();
    _nearest.reset();
    _zeroPosition.reset();
    _zeroNormal.reset();
    _device = nullptr;
    _hasSequence = false;
    _latestSequence = 0;
    _status = "Not initialized";
}

const TemporalSettings& TemporalAccumulator::settings() const
{
    return _settings;
}

void TemporalAccumulator::setEnabled(bool value)
{
    if (_settings.enabled != value)
    {
        _settings.enabled = value;
        invalidate();
    }
}

void TemporalAccumulator::setMaxFrames(uint32_t value)
{
    if (!value || value > 1024)
    {
        throw std::invalid_argument("Temporal frame bound must be 1..1024");
    }
    if (_settings.maxFrames != value)
    {
        _settings.maxFrames = value;
        invalidate();
    }
}

void TemporalAccumulator::setFreezeAtLimit(bool value)
{
    if (_settings.freezeAtLimit != value)
    {
        _settings.freezeAtLimit = value;
        invalidate();
    }
}

void TemporalAccumulator::setPositionThresholds(float absolute, float relative)
{
    if (!std::isfinite(absolute) || !std::isfinite(relative) || absolute < 0 || relative < 0)
    {
        throw std::invalid_argument("Temporal position thresholds must be finite and nonnegative");
    }
    _settings.absolutePositionThreshold = absolute;
    _settings.relativePositionThreshold = relative;
    invalidate();
}

void TemporalAccumulator::setNormalThreshold(float value)
{
    if (!std::isfinite(value) || value < -1 || value > 1)
    {
        throw std::invalid_argument("Temporal normal threshold must be in [-1,1]");
    }
    _settings.normalThreshold = value;
    invalidate();
}

bool TemporalAccumulator::available() const
{
    return bool(_pipelines[0]) && !_slots.empty();
}

const std::string& TemporalAccumulator::status() const
{
    return _status;
}

uint64_t TemporalAccumulator::generation() const
{
    return _generation;
}

uint32_t TemporalAccumulator::frameCount(const TemporalKey& key) const
{
    const auto found = _histories.find(key);
    return found != _histories.end() ? found->second->count : 0;
}
}
