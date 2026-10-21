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

#include <bg2e/draw/pipeline/RTReflections.hpp>
#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>

namespace bg2e::draw::pipeline {
namespace {
struct alignas(16) ReflectionUniforms {
    glm::mat4 inverseViewProjection{1};
    glm::vec4 parameters{0}; // max distance, bias, shadow samples, shadows enabled
    glm::uvec4 counts{0}; // samples, reserved, instances, rendered-sample key
    glm::uvec4 noise{0}; // BlueNoise atlas metadata
    glm::uvec4 extent{0}; // surface width/height, output width/height
    glm::vec4 cameraRoughness{0};
    glm::vec4 spread{0};
    LightUniforms lights;
};
static_assert(sizeof(ReflectionUniforms) == 816 && offsetof(ReflectionUniforms, counts) == 80 && offsetof(ReflectionUniforms, cameraRoughness) == 128 && offsetof(ReflectionUniforms, lights) == 160);
void positive(float value)
{
    if (!std::isfinite(value) || value <= 0)
    {
        throw std::invalid_argument("Reflection parameter must be finite and positive");
    }
}
}

gpu::PipelineLayoutDescription rtReflectionBindingLayout(uint32_t capacity, bool blueNoise, gpu::BackendType backend)
{
    const auto stage = backend == gpu::BackendType::Vulkan ? gpu::ShaderStage::RayGeneration : gpu::ShaderStage::Compute;
    auto result = tracingBindingLayout(capacity,stage,backend == gpu::BackendType::Vulkan ?
        std::vector<gpu::ShaderStage>{gpu::ShaderStage::AnyHit} : std::vector<gpu::ShaderStage>{});
    result.resourceBindings.push_back({0,{0,1},gpu::ResourceType::UniformBuffer,stage});
    for (uint32_t i = 0; i < 3; ++i)
    {
        result.resourceBindings.push_back({0,{1+i,i},gpu::ResourceType::SampledImage,stage});
    }
    result.resourceBindings.push_back({0,{4,9},gpu::ResourceType::Sampler,stage});
    result.resourceBindings.push_back({0,{5,4},gpu::ResourceType::StorageImage,stage});
    if (blueNoise)
    {
        result.resourceBindings.push_back({0,{6,3},gpu::ResourceType::SampledImage,stage});
        result.resourceBindings.push_back({0,{7,10},gpu::ResourceType::Sampler,stage});
    }
    result.resourceBindings.push_back({0,{8,5},gpu::ResourceType::SampledImage,stage});
    result.resourceBindings.push_back({0,{9,11},gpu::ResourceType::Sampler,stage});
    result.resourceBindings.push_back({0,{10,6},gpu::ResourceType::SampledImage,stage});
    return result;
}

RTReflections::RTReflections(RTReflectionConfiguration configuration)
    : _configuration(std::move(configuration))
{
    if (_configuration.shaderRoot.empty())
    {
        _configuration.shaderRoot = base::PlatformTools::shaderPath();
    }
}

void RTReflections::initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots)
{
    if (_device || !device.isValid() || !slots || _configuration.shaderInterface != "draw.rt_reflections.v3")
    {
        throw std::invalid_argument("Reflection initialization requires a clean instance, valid device/slots and draw.rt_reflections.v3 shaders");
    }
    _device = &device;
    _backend = backend;
    const auto usage = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment;
    _capacity = device.resourceArrayCapabilities().profileCapacity(6,3,16);
    if (!(backend == gpu::BackendType::Vulkan ? device.rayTracingCapabilities().pipelineSupported() : device.rayTracingCapabilities().querySupported()) || !_capacity ||
        !device.formatCapabilities(gpu::PixelFormat::R16G16B16A16_SFLOAT).supports(usage))
    {
        _capacity = 0;
        _status = "Reflection unavailable: native tracing, material arrays or HDR target support is missing";
        return;
    }
    if (_configuration.noise && &_configuration.noise->device() != &device)
    {
        throw std::invalid_argument("Reflection noise belongs to another device");
    }
    try
    {
        for (uint32_t index = 0; index < 4; ++index)
        {
            const bool world = index >= 2, blue = index % 2 != 0;
            _layouts[index] = device.createPipelineLayout(rtReflectionBindingLayout(_capacity,blue,backend));
            gpu::ShaderLib library(_configuration.shaderRoot,backend);
            const auto name = _configuration.shaderPrefix+"/"+(world ? "world_" : "camera_")+
                (blue ? "blue_" : "hash_")+"256";
            if (backend == gpu::BackendType::Vulkan)
            {
                _shaders[index] = library.specialized(name,gpu::ShaderStage::RayGeneration,&device,{{0,_capacity}});
                _stages[index*3] = library.specialized(name,gpu::ShaderStage::Miss,&device,{{0,_capacity}});
                _stages[index*3+1] = library.specialized(name,gpu::ShaderStage::ClosestHit,&device,{{0,_capacity}});
                _stages[index*3+2] = library.specialized(name,gpu::ShaderStage::AnyHit,&device,{{0,_capacity}});
                gpu::RayTracingPipelineDescription description;
                description.raygenShader = _shaders[index].get();
                description.missShader = _stages[index*3].get();
                description.closestHitShader = _stages[index*3+1].get();
                description.anyHitShader = _stages[index*3+2].get();
                description.layout = _layouts[index].get();
                description.maxRecursionDepth = 1;
                _pipelines[index] = device.createRayTracingPipeline(description);
            }
            else
            {
                _shaders[index] = library.compute(name,&device);
                _compute[index] = device.createComputePipeline({_shaders[index].get(),_layouts[index].get()});
            }
        }
        gpu::SamplerDescription nearest;
        nearest.minFilter = nearest.magFilter = gpu::Filter::Nearest;
        nearest.addressModeU = nearest.addressModeV = gpu::AddressMode::ClampToEdge;
        nearest.maxLod = 0;
        _nearest = device.createSampler(nearest);
        _tracing = std::make_shared<TracingSceneResources>(device,_capacity);
        _slots.resize(slots);
        _status = "Reflection native transport ready";
    }
    catch (const std::exception& error)
    {
        _pipelines = {};
        _compute = {};
        _stages = {};
        _shaders = {};
        _layouts = {};
        _nearest.reset();
        _tracing.reset();
        _slots.clear();
        _capacity = 0;
        _status = std::string("Reflection unavailable: ")+error.what();
    }
}

void RTReflections::prepare(uint32_t slot, uint64_t serial, const RenderQueue& queue)
{
    if (available())
    {
        const auto& previous = _slots.at(slot);
        if (previous.prepared && previous.serial == serial)
        {
            throw std::logic_error("Prepare a reflection slot once per acquired presentation/job sample");
        }
    }
    auto snapshot = available() && enabled() ? _tracing->prepare(queue) : nullptr;
    prepareSnapshot(slot,serial,std::move(snapshot));
}

void RTReflections::prepareSnapshot(uint32_t index, uint64_t serial, std::shared_ptr<TracingSnapshot> snapshot)
{
    if (!available())
    {
        return;
    }
    auto& slot = _slots.at(index);
    if (slot.prepared && slot.serial == serial)
    {
        throw std::logic_error("Prepare a reflection slot once per acquired presentation/job sample");
    }
    if (snapshot && (snapshot->device != _device || snapshot->capacity > _capacity))
    {
        throw std::invalid_argument("Reflection snapshot device/profile mismatch");
    }
    Slot next;
    next.prepared = true;
    next.serial = serial;
    next.snapshot = std::move(snapshot);
    slot = std::move(next); // Only the caller's retired slot may be replaced.
    if (_pool)
    {
        _pool->collect();
    }
}

void RTReflections::record(const RTReflectionContext& context)
{
    base::CpuProfileScope profile("effect.reflections.record");
    const auto usage = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment;
    if (!_device || context.command.hasActiveScope() || context.output.device() != _device || !context.output.isValid() ||
        context.output.pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT || context.output.size().isZero() ||
        context.output.imageType() != gpu::ImageType::Image2D ||
        !gpu::hasFlag(context.output.usage(),gpu::ImageUsage::Storage) ||
        !gpu::hasFlag(context.output.usage(),gpu::ImageUsage::Sampled) ||
        !gpu::hasFlag(context.output.usage(),gpu::ImageUsage::ColorAttachment) ||
        !_device->formatCapabilities(gpu::PixelFormat::R16G16B16A16_SFLOAT).supports(usage))
    {
        throw std::invalid_argument("Reflection output requires compatible same-device RGBA16 HDR image and inactive scope");
    }
    if (!available())
    {
        RenderTarget::clear(context.command,context.output,{0,0,0,0});
        return;
    }
    auto& slot = _slots.at(context.slot);
    if (!slot.prepared || slot.serial != context.serial)
    {
        throw std::logic_error("Prepare the acquired Reflection slot before recording");
    }
    if (!enabled() || !slot.snapshot || slot.snapshot->empty())
    {
        RenderTarget::clear(context.command,context.output,{0,0,0,0});
        return;
    }
    context.surface.validate(*_device);
    if (context.output.size() != outputExtent(context.displayExtent) || context.output.imageType() != gpu::ImageType::Image2D ||
        &context.output == context.surface.normal || &context.output == context.surface.depthOrPosition || &context.output == context.surface.coverage || &context.output == &context.material)
    {
        throw std::invalid_argument("Reflection output extent/profile aliases or differs from the declared surface quality");
    }
    if (!context.material.isValid() || context.material.device() != _device ||
        context.material.imageType() != gpu::ImageType::Image2D || context.material.size() != context.surface.extent() ||
        !gpu::hasFlag(context.material.usage(),gpu::ImageUsage::Sampled) ||
        (context.material.pixelFormat() != gpu::PixelFormat::R8G8B8A8_UNORM &&
         context.material.pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT))
    {
        throw std::invalid_argument("Reflection receiver material requires matching same-device sampled RGBA8/16 roughness");
    }
    for (uint32_t c = 0; c < 3; ++c)
    {
        if (!std::isfinite(context.cameraPosition[c]))
        {
            throw std::invalid_argument("Reflection camera position must be finite");
        }
    }
    const bool blue = _settings.useBlueNoise && _configuration.noise && _configuration.noise->isReady();
    const uint32_t index = (context.surface.profile == SurfaceProfile::WorldPositionCoverage ? 2u : 0u)+(blue ? 1u : 0u);
    Invocation invocation;
    ReflectionUniforms data;
    data.inverseViewProjection = context.surface.inverseViewProjection;
    data.parameters = {_settings.maxDistance,_settings.bias,float(_settings.shadowSamples),_settings.useShadows ? 1.0f : 0.0f};
    if (_settings.maxDistance <= _settings.bias*2)
    {
        throw std::invalid_argument("Reflection maximum distance must exceed twice its ray bias");
    }
    if (!context.environment)
    {
        throw std::invalid_argument("Reflection requires retained environment resources");
    }
    context.environment->validate(_device);
    if (context.lights.count.x > MaxLights)
    {
        throw std::invalid_argument("Reflection light count exceeds the shared light ABI");
    }
    for (uint32_t i = 0; i < context.lights.count.x; ++i)
    {
        const auto& light = context.lights.lights[i];
        for (auto vector : {light.positionIntensity,light.color,light.directionAngle,light.parameters})
        {
            for (uint32_t c = 0; c < 4; ++c)
            {
                if (!std::isfinite(vector[c]))
                {
                    throw std::invalid_argument("Reflection light values must be finite");
                }
            }
        }
    }
    invocation.environment = context.environment;
    data.lights = context.lights;
    data.cameraRoughness = glm::vec4(context.cameraPosition,_settings.maxRoughness);
    data.spread.x = _settings.roughnessSpread;
    data.counts = {_settings.sampleCount,1u,uint32_t(slot.snapshot->instances.size()),context.renderedSample};
    data.extent = {context.surface.extent().width,context.surface.extent().height,context.output.width(),context.output.height()};
    if (blue)
    {
        invocation.noiseImage = _configuration.noise->image();
        invocation.noiseSampler = _configuration.noise->sampler();
        data.noise = _configuration.noise->shaderMetadata();
        if (data.noise.z < 2)
        {
            throw std::invalid_argument("Reflection BlueNoise sampling requires two source channels");
        }
    }
    if (!_pool)
    {
        _pool = std::make_shared<TemporaryResourcePool>(*_device);
    }
    _pool->setEnabled(_poolingEnabled);
    invocation.pooled = _pool->bindings(context.command, _layouts[index], 0, sizeof(data),
        {context.slot, slot.invocations.size(), 0, 0});
    invocation.pooled->writeUniforms(&data, sizeof(data));
    invocation.uniforms = invocation.pooled->uniforms();
    slot.snapshot->build(context.command);
    invocation.sceneSet = slot.snapshot->resourceSet(*_layouts[index], 1, _capacity);
    invocation.tableSet = slot.snapshot->materialResourceSet(*_layouts[index], _capacity);
    invocation.surfaceSet = invocation.pooled->resources();
    invocation.pooled->setUniformBuffer({0,1},invocation.uniforms.get());
    invocation.pooled->setSampledImage({1,0},context.surface.normal);
    invocation.pooled->setSampledImage({2,1},context.surface.depthOrPosition);
    auto* coverage = context.surface.profile == SurfaceProfile::CameraDepth ? context.surface.normal : context.surface.coverage;
    invocation.pooled->setSampledImage({3,2},coverage);
    invocation.pooled->setSampler({4,9},_nearest.get());
    invocation.pooled->setStorageImage({5,4},&context.output);
    invocation.pooled->setSampledImage({10,6},&context.material);
    context.command.transition(&context.material,gpu::ImageLayout::ShaderReadOnly);
    for (auto* image : {context.surface.normal,context.surface.depthOrPosition,coverage})
    {
        context.command.transition(image,gpu::ImageLayout::ShaderReadOnly);
    }
    if (blue)
    {
        invocation.pooled->setSampledImage({6,3},invocation.noiseImage.get());
        invocation.pooled->setSampler({7,10},invocation.noiseSampler.get());
        context.command.transition(invocation.noiseImage.get(),gpu::ImageLayout::ShaderReadOnly);
    }
    invocation.pooled->setSampledImage({8,5},invocation.environment->irradiance.get());
    invocation.pooled->setSampler({9,11},invocation.environment->sampler.get());
    context.command.transition(invocation.environment->irradiance.get(),gpu::ImageLayout::ShaderReadOnly);
    invocation.pooled->update();
    slot.invocations.push_back(std::move(invocation));
    auto& retained = slot.invocations.back();
    context.command.transition(&context.output,gpu::ImageLayout::General);
    if (_backend == gpu::BackendType::Vulkan)
    {
        context.command.bindPipeline(_pipelines[index].get());
        context.command.bindResourceSet(_pipelines[index].get(),0,retained.surfaceSet.get());
        context.command.bindResourceSet(_pipelines[index].get(),1,retained.sceneSet.get());
        context.command.bindResourceSet(_pipelines[index].get(),2,retained.tableSet.get());
        context.command.traceRays(context.output.width(),context.output.height(),1);
    }
    else
    {
        context.command.beginCompute();
        context.command.bindPipeline(_compute[index].get());
        context.command.bindResourceSet(_compute[index].get(),0,retained.surfaceSet.get());
        context.command.bindResourceSet(_compute[index].get(),1,retained.sceneSet.get());
        context.command.bindResourceSet(_compute[index].get(),2,retained.tableSet.get());
        context.command.dispatch((context.output.width()+15)/16,(context.output.height()+15)/16,1);
        context.command.endCompute();
    }
    context.command.transition(&context.output,gpu::ImageLayout::ShaderReadOnly);
}

void RTReflections::reset()
{
    if (_device)
    {
        _device->waitIdle();
    }
    for (auto& slot : _slots)
    {
        slot = {};
    }
    if (_pool)
    {
        _pool->collect();
    }
    if (_tracing)
    {
        _tracing->clear();
    }
}

void RTReflections::cleanup()
{
    reset();
    _slots.clear();
    _pool.reset();
    _pipelines = {};
    _compute = {};
    _stages = {};
    _shaders = {};
    _layouts = {};
    _nearest.reset();
    _tracing.reset();
    _capacity = 0;
    _device = nullptr;
    _configuration.noise.reset();
    _status = "Not initialized";
}

const RTReflectionSettings& RTReflections::settings() const
{
    return _settings;
}

void RTReflections::setEnabled(bool value)
{
    _settings.enabled = value;
}

bool RTReflections::enabled() const
{
    return _settings.enabled;
}

void RTReflections::setQuality(RTReflectionQuality value)
{
    if (uint32_t(value) > uint32_t(RTReflectionQuality::Ultra))
    {
        throw std::invalid_argument("Unknown Reflection quality");
    }
    _settings.quality = value;
}

RTReflectionQuality RTReflections::quality() const
{
    return _settings.quality;
}

void RTReflections::setSampleCount(uint32_t value)
{
    if (!value || value > 64)
    {
        throw std::invalid_argument("Reflection sample count must be 1..64");
    }
    _settings.sampleCount = value;
}

uint32_t RTReflections::sampleCount() const
{
    return _settings.sampleCount;
}

void RTReflections::setBias(float value)
{
    positive(value);
    _settings.bias = value;
}

float RTReflections::bias() const
{
    return _settings.bias;
}

void RTReflections::setUseBlueNoise(bool value)
{
    _settings.useBlueNoise = value;
}

bool RTReflections::useBlueNoise() const
{
    return _settings.useBlueNoise;
}

void RTReflections::setNoise(std::shared_ptr<utils::BlueNoise> noise)
{
    if (_device && noise && &noise->device() != _device)
    {
        throw std::invalid_argument("Reflection noise belongs to another device");
    }
    _configuration.noise = std::move(noise);
}

std::shared_ptr<utils::BlueNoise> RTReflections::noise() const
{
    return _configuration.noise;
}

bool RTReflections::available() const
{
    return bool(_pipelines[0]) || bool(_compute[0]);
}

const std::string& RTReflections::status() const
{
    return _status;
}

uint32_t RTReflections::tableCapacity() const
{
    return _capacity;
}

std::shared_ptr<TracingSceneResources> RTReflections::tracingResources() const
{
    return _tracing;
}

gpu::Size2D RTReflections::outputExtent(gpu::Size2D display) const
{
    if (display.isZero())
    {
        throw std::invalid_argument("Reflection display extent must be nonzero");
    }
    if (uint32_t(_settings.quality) > uint32_t(RTReflectionQuality::Ultra))
    {
        throw std::invalid_argument("Unknown Reflection quality");
    }
    const uint32_t numerator[] = {1,1,2,1}, denominator[] = {3,2,3,1};
    const auto n = numerator[uint32_t(_settings.quality)], d = denominator[uint32_t(_settings.quality)];
    return {std::max(1u,uint32_t((uint64_t(display.width)*n+d/2)/d)),
            std::max(1u,uint32_t((uint64_t(display.height)*n+d/2)/d))};
}

void RTReflections::setMaxDistance(float value)
{
    positive(value);
    _settings.maxDistance = value;
}

float RTReflections::maxDistance() const
{
    return _settings.maxDistance;
}

void RTReflections::setShadowSamples(uint32_t value)
{
    if (!value || value > 64)
    {
        throw std::invalid_argument("Reflection shadow samples must be 1..64");
    }
    _settings.shadowSamples = value;
}

uint32_t RTReflections::shadowSamples() const
{
    return _settings.shadowSamples;
}

void RTReflections::setUseShadows(bool value)
{
    _settings.useShadows = value;
}

bool RTReflections::useShadows() const
{
    return _settings.useShadows;
}

void RTReflections::setMaxRoughness(float value)
{
    if (!std::isfinite(value) || value < 0 || value > 1)
    {
        throw std::invalid_argument("Reflection roughness threshold must be in [0,1]");
    }
    _settings.maxRoughness = value;
}

float RTReflections::maxRoughness() const
{
    return _settings.maxRoughness;
}

void RTReflections::setRoughnessSpread(float value)
{
    if (!std::isfinite(value) || value < 0 || value > 16)
    {
        throw std::invalid_argument("Reflection roughness spread must be in [0,16]");
    }
    _settings.roughnessSpread = value;
}

float RTReflections::roughnessSpread() const
{
    return _settings.roughnessSpread;
}

TemporaryPoolStatistics RTReflections::poolStatistics() const
{
    return _pool ? _pool->statistics() : TemporaryPoolStatistics{};
}

}
