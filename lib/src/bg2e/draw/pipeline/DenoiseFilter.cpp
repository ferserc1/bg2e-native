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

#include <bg2e/draw/pipeline/DenoiseFilter.hpp>
#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace bg2e::draw::pipeline {
namespace {
struct alignas(16) DenoiseUniforms {
    glm::mat4 inverseViewProjection{1}, worldToView{1};
    glm::vec4 thresholds{0}; // depth threshold, normal cosine, depth/normal sigma.
    glm::vec4 parameters{0}; // spatial sigma, maximum roughness.
    glm::uvec4 extent{0}; // Surface and signal dimensions.
    glm::uvec4 counts{0}; // Signal kind, radius, reserved.
};
static_assert(sizeof(DenoiseUniforms) == 192 && offsetof(DenoiseUniforms, counts) == 176);
void positive(float value)
{
    if (!std::isfinite(value) || value < 1e-6f || value > 1e6f)
    {
        throw std::invalid_argument("Denoise sigma must be finite and in [1e-6,1e6]");
    }
}
}

DenoiseFilter::DenoiseFilter(DenoiseConfiguration configuration)
    : _configuration(std::move(configuration))
{
    if (_configuration.shaderRoot.empty())
    {
        _configuration.shaderRoot = base::PlatformTools::shaderPath();
    }
}

void DenoiseFilter::initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots)
{
    if (_device || !device.isValid() || !slots || !_configuration.maxInvocations || _configuration.shaderInterface != "draw.denoise.v1")
    {
        throw std::invalid_argument("Denoise initialization requires a clean lifecycle, slots, capacity and draw.denoise.v1");
    }
    _device = &device;
    try
    {
        gpu::PipelineLayoutDescription description;
        description.resourceBindings.push_back({0,{0,1},gpu::ResourceType::UniformBuffer,gpu::ShaderStage::Compute});
        for (uint32_t i = 1; i <= 5; ++i)
        {
            description.resourceBindings.push_back({0,{i,i-1},gpu::ResourceType::SampledImage,gpu::ShaderStage::Compute});
        }
        description.resourceBindings.push_back({0,{6,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Compute});
        description.resourceBindings.push_back({0,{7,5},gpu::ResourceType::StorageImage,gpu::ShaderStage::Compute});
        _layout = device.createPipelineLayout(description);
        gpu::SamplerDescription nearest;
        nearest.minFilter = nearest.magFilter = gpu::Filter::Nearest;
        nearest.addressModeU = nearest.addressModeV = gpu::AddressMode::ClampToEdge;
        nearest.maxLod = 0;
        _nearest = device.createSampler(nearest);
        _slots.resize(slots);
        _status.clear();
        const auto usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
        gpu::ShaderLib library(_configuration.shaderRoot,backend);
        for (uint32_t i = 0; i < 4; ++i)
        {
            const auto format = i%2 ? gpu::PixelFormat::R16G16B16A16_SFLOAT : gpu::PixelFormat::R16_SFLOAT;
            if (!device.formatCapabilities(format).supports(usage))
            {
                _status += "Unsupported scalar/HDR storage profile. ";
                continue;
            }
            try
            {
                const auto name = _configuration.shaderPrefix+"/"+(i >= 2 ? "world_" : "camera_")+(i%2 ? "hdr" : "scalar");
                _shaders[i] = library.compute(name,&device);
                _pipelines[i] = device.createComputePipeline({_shaders[i].get(),_layout.get()});
            }
            catch (const std::exception& error)
            {
                _shaders[i].reset();
                _status += error.what();
                _status += ". ";
            }
        }
        _status = (available() ? "Denoise ready. " : "Denoise unavailable. ")+_status;
    }
    catch (const std::exception& error)
    {
        _pipelines = {};
        _shaders = {};
        _layout.reset();
        _nearest.reset();
        _slots.clear();
        _status = std::string("Denoise unavailable: ")+error.what();
    }
}

void DenoiseFilter::beginFrame(uint32_t index, uint64_t serial)
{
    if (!available())
    {
        return;
    }
    auto& slot = _slots.at(index);
    if (slot.prepared && slot.serial == serial)
    {
        throw std::logic_error("Begin the completed denoise slot only once per serial");
    }
    slot = {};
    if (_pool)
    {
        _pool->collect();
    }
    slot.prepared = true;
    slot.serial = serial;
}

DenoiseResult DenoiseFilter::record(const DenoiseInput& input)
{
    base::CpuProfileScope profile("effect.denoise.record");
    if (!_device || input.command.hasActiveScope() || !input.signal.isValid() || input.signal.device() != _device ||
        input.signal.size().isZero() || input.signal.imageType() != gpu::ImageType::Image2D ||
        !gpu::hasFlag(input.signal.usage(),gpu::ImageUsage::Sampled) ||
        uint32_t(input.key.signal) > uint32_t(DenoiseSignal::ReflectedRadiance) ||
        uint32_t(input.surface.profile) > uint32_t(SurfaceProfile::WorldPositionCoverage))
    {
        throw std::invalid_argument("Denoise requires a matching sampled signal/profile and inactive scope");
    }
    const bool scalar = input.key.signal == DenoiseSignal::Visibility;
    if (input.signal.pixelFormat() != (scalar ? gpu::PixelFormat::R16_SFLOAT : gpu::PixelFormat::R16G16B16A16_SFLOAT))
    {
        throw std::invalid_argument("Denoise scalar/HDR signal format mismatch");
    }
    if (!enabled() || !kernelRadius() || !available(input.key.signal,input.surface.profile))
    {
        return {&input.signal,{}};
    }
    if (kernelRadius() > 8 || !std::isfinite(depthThreshold()) || depthThreshold() < 0 ||
        !std::isfinite(normalThreshold()) || normalThreshold() < -1 || normalThreshold() > 1)
    {
        throw std::invalid_argument("Invalid effective denoise radius/threshold accessors");
    }
    positive(depthSigma());
    positive(normalSigma());
    positive(spatialSigma());
    auto& slot = _slots.at(input.slot);
    if (!slot.prepared || slot.serial != input.serial || slot.keys.contains(input.key))
    {
        throw std::logic_error("Prepare a completed slot and filter each effect/view/layer once per serial");
    }
    if (slot.invocations.size() >= _configuration.maxInvocations)
    {
        throw std::length_error("Denoise acquired-slot invocation capacity exceeded");
    }
    input.surface.validate(*_device);
    for (uint32_t c = 0; c < 4; ++c)
    {
        for (uint32_t r = 0; r < 4; ++r)
        {
            if (!std::isfinite(input.worldToView[c][r]))
            {
                throw std::invalid_argument("Denoise world-to-view matrix must be finite");
            }
        }
    }
    if (input.surface.profile == SurfaceProfile::CameraDepth)
    {
        const auto& view = input.worldToView;
        const float determinant = glm::determinant(view);
        if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f ||
            std::abs(view[0][3]) > 1e-6f || std::abs(view[1][3]) > 1e-6f ||
            std::abs(view[2][3]) > 1e-6f || std::abs(view[3][3]-1.0f) > 1e-6f)
        {
            throw std::invalid_argument("Camera denoise requires an invertible affine world-to-view matrix");
        }
    }
    if (input.key.signal == DenoiseSignal::ReflectedRadiance && (!input.material || !input.material->isValid() ||
        input.material->device() != _device || input.material->imageType() != gpu::ImageType::Image2D ||
        input.material->size() != input.surface.extent() || !gpu::hasFlag(input.material->usage(),gpu::ImageUsage::Sampled) ||
        (input.material->pixelFormat() != gpu::PixelFormat::R8G8B8A8_UNORM && input.material->pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT) ||
        !std::isfinite(input.maxRoughness) || input.maxRoughness < 0 || input.maxRoughness > 1))
    {
        throw std::invalid_argument("Denoise reflections require matching receiver roughness and threshold");
    }
    if (!_pool)
    {
        _pool = std::make_shared<TemporaryResourcePool>(*_device, _configuration.poolLimits);
    }
    _pool->setEnabled(_poolingEnabled);
    Invocation invocation;
    gpu::ImageDescription image;
    image.size = input.signal.size();
    image.format = input.signal.pixelFormat();
    image.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc;
    image.debugName = "draw per-invocation bilateral signal";
    invocation.output = _pool->image(input.command, image);
    DenoiseUniforms data;
    data.inverseViewProjection = input.surface.inverseViewProjection;
    data.worldToView = input.worldToView;
    data.thresholds = {depthThreshold(),normalThreshold(),depthSigma(),normalSigma()};
    data.parameters = {spatialSigma(),input.maxRoughness,0,0};
    data.extent = {input.surface.extent().width,input.surface.extent().height,input.signal.width(),input.signal.height()};
    data.counts = {uint32_t(input.key.signal),kernelRadius(),0,0};
    invocation.pooled = _pool->bindings(input.command, _layout, 0, sizeof(data),
        {input.slot, uint64_t(input.key.signal), input.key.view, input.key.layer});
    invocation.pooled->writeUniforms(&data, sizeof(data));
    invocation.uniforms = invocation.pooled->uniforms();
    invocation.resources = invocation.pooled->resources();
    invocation.pooled->setUniformBuffer({0,1},invocation.uniforms.get());
    auto* coverage = input.surface.profile == SurfaceProfile::CameraDepth ? input.surface.normal : input.surface.coverage;
    const std::array<gpu::Image*,5> sources{input.surface.normal,input.surface.depthOrPosition,coverage,&input.signal,
        input.key.signal == DenoiseSignal::ReflectedRadiance ? input.material : input.surface.normal};
    for (uint32_t i = 0; i < sources.size(); ++i)
    {
        input.command.transition(sources[i],gpu::ImageLayout::ShaderReadOnly);
        invocation.pooled->setSampledImage({i+1,i},sources[i]);
    }
    invocation.pooled->setSampler({6,0},_nearest.get());
    input.command.transition(invocation.output.get(),gpu::ImageLayout::General);
    invocation.pooled->setStorageImage({7,5},invocation.output.get());
    invocation.pooled->update();
    slot.invocations.push_back(std::move(invocation));
    slot.keys.insert(input.key);
    const auto& retained = slot.invocations.back();
    const uint32_t index = (input.surface.profile == SurfaceProfile::WorldPositionCoverage ? 2u : 0u)+(scalar ? 0u : 1u);
    input.command.beginCompute();
    input.command.bindPipeline(_pipelines[index].get());
    input.command.bindResourceSet(_pipelines[index].get(),0,retained.resources.get());
    input.command.dispatch((input.signal.width()+15)/16,(input.signal.height()+15)/16,1);
    input.command.endCompute();
    input.command.transition(retained.output.get(),gpu::ImageLayout::ShaderReadOnly);
    return {retained.output.get(),retained.output};
}

void DenoiseFilter::cleanup()
{
    if (_device)
    {
        _device->waitIdle();
    }
    _slots.clear();
    _pool.reset();
    _pipelines = {};
    _shaders = {};
    _layout.reset();
    _nearest.reset();
    _device = nullptr;
    _status = "Not initialized";
}

bool DenoiseFilter::available() const
{
    return !_slots.empty() && std::any_of(_pipelines.begin(),_pipelines.end(),[](const auto& pipeline)
    {
        return bool(pipeline);
    });
}

bool DenoiseFilter::available(DenoiseSignal signal, SurfaceProfile surface) const
{
    if (uint32_t(signal) > uint32_t(DenoiseSignal::ReflectedRadiance) || uint32_t(surface) > uint32_t(SurfaceProfile::WorldPositionCoverage))
    {
        return false;
    }
    const auto index = (surface == SurfaceProfile::WorldPositionCoverage ? 2u : 0u)+(signal == DenoiseSignal::Visibility ? 0u : 1u);
    return !_slots.empty() && bool(_pipelines[index]);
}

const std::string& DenoiseFilter::status() const
{
    return _status;
}

bool DenoiseFilter::enabled() const
{
    return _settings.enabled;
}

void DenoiseFilter::setEnabled(bool value)
{
    _settings.enabled = value;
}

uint32_t DenoiseFilter::kernelRadius() const
{
    return _settings.kernelRadius;
}

void DenoiseFilter::setKernelRadius(uint32_t value)
{
    if (value > 8)
    {
        throw std::invalid_argument("Denoise radius must be 0..8");
    }
    _settings.kernelRadius = value;
}

float DenoiseFilter::depthThreshold() const
{
    return _settings.depthThreshold;
}

void DenoiseFilter::setDepthThreshold(float value)
{
    if (!std::isfinite(value) || value < 0)
    {
        throw std::invalid_argument("Denoise depth threshold must be finite and nonnegative");
    }
    _settings.depthThreshold = value;
}

float DenoiseFilter::normalThreshold() const
{
    return _settings.normalThreshold;
}

void DenoiseFilter::setNormalThreshold(float value)
{
    if (!std::isfinite(value) || value < -1 || value > 1)
    {
        throw std::invalid_argument("Denoise normal threshold must be in [-1,1]");
    }
    _settings.normalThreshold = value;
}

float DenoiseFilter::depthSigma() const
{
    return _settings.depthSigma;
}

void DenoiseFilter::setDepthSigma(float value)
{
    positive(value);
    _settings.depthSigma = value;
}

float DenoiseFilter::normalSigma() const
{
    return _settings.normalSigma;
}

void DenoiseFilter::setNormalSigma(float value)
{
    positive(value);
    _settings.normalSigma = value;
}

float DenoiseFilter::spatialSigma() const
{
    return _settings.spatialSigma;
}

void DenoiseFilter::setSpatialSigma(float value)
{
    positive(value);
    _settings.spatialSigma = value;
}

reflection::TypeInfo DenoiseFilter::settingsType() const
{
    reflection::TypeInfoBuilder<DenoiseFilter> type("bg2e.draw.denoise.v1");
    type.property("enabled", &DenoiseFilter::enabled, &DenoiseFilter::setEnabled).category("Denoise");
    type.property("kernelRadius", &DenoiseFilter::kernelRadius, &DenoiseFilter::setKernelRadius).category("Denoise").range(0,8);
    type.property("depthThreshold", &DenoiseFilter::depthThreshold, &DenoiseFilter::setDepthThreshold).category("Denoise").range(0,std::numeric_limits<float>::max());
    type.property("normalThreshold", &DenoiseFilter::normalThreshold, &DenoiseFilter::setNormalThreshold).category("Denoise").range(-1,1);
    type.property("depthSigma", &DenoiseFilter::depthSigma, &DenoiseFilter::setDepthSigma).category("Denoise").range(1e-6f,1e6f);
    type.property("normalSigma", &DenoiseFilter::normalSigma, &DenoiseFilter::setNormalSigma).category("Denoise").range(1e-6f,1e6f);
    type.property("spatialSigma", &DenoiseFilter::spatialSigma, &DenoiseFilter::setSpatialSigma).category("Denoise").range(1e-6f,1e6f);
    type.property("available", static_cast<bool (DenoiseFilter::*)() const>(&DenoiseFilter::available)).category("Capabilities");
    type.property("status", &DenoiseFilter::status).category("Capabilities");
    return type.build();
}

void* DenoiseFilter::settingsInstance()
{
    return this;
}

TemporaryPoolStatistics DenoiseFilter::poolStatistics() const
{
    return _pool ? _pool->statistics() : TemporaryPoolStatistics{};
}

}
