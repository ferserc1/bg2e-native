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

#include <bg2e/draw/pipeline/DeferredLayer.hpp>
#include <bg2e/draw/pipeline/GBuffer.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <cmath>

namespace bg2e::draw::pipeline {
namespace {
void imageBindings(gpu::PipelineLayoutDescription& layout, uint32_t set, uint32_t count, bool shadows = false)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        const uint32_t metal = shadows && i >= 8 ? i + 4 : i;
        layout.resourceBindings.push_back({set, {i, metal}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        layout.resourceBindings.push_back({set, {count + i, metal}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
    }
}

uint32_t slots(Engine& engine, uint32_t count)
{
    if (count == 0)
    {
        count = engine.frameSlotCount();
    }
    if (count == 0)
    {
        throw std::invalid_argument("Raster passes require an explicit frame slot count without a surface");
    }
    return count;
}
}

PassContract deferredContract(gpu::PixelFormat depth, bool shadows, bool ambientOcclusion, bool globalIllumination, bool reflections)
{
    auto result = gbufferContract(depth);
    result.shaderInterface = shadows ? "draw.raster.deferred.shadows.v1" : "draw.raster.deferred.v1";
    if (ambientOcclusion)
    {
        result.shaderInterface = shadows ? "draw.raster.deferred.shadows.ao.v1" : "draw.raster.deferred.ao.v1";
    }
    if (globalIllumination)
    {
        result.shaderInterface.insert(result.shaderInterface.size()-3,".gi");
    }
    if (reflections)
    {
        result.shaderInterface.insert(result.shaderInterface.size()-3,".reflections");
    }
    result.inputs = std::move(result.outputs);
    for (auto& slot : result.inputs)
    {
        slot.usage = gpu::ImageUsage::Sampled;
    }
    result.inputs.push_back({"background", gpu::PixelFormat::R16G16B16A16_SFLOAT});
    if (shadows)
    {
        result.inputs.push_back({"shadow0", gpu::PixelFormat::R16G16B16A16_SFLOAT});
        result.inputs.push_back({"shadow1", gpu::PixelFormat::R16G16B16A16_SFLOAT});
    }
    if (ambientOcclusion)
    {
        result.inputs.push_back({"ambientOcclusion",gpu::PixelFormat::R16_SFLOAT,ExtentDomain::Scaled});
    }
    if (globalIllumination)
    {
        result.inputs.push_back({"globalIllumination",gpu::PixelFormat::R16G16B16A16_SFLOAT,ExtentDomain::Scaled});
    }
    if (reflections)
    {
        result.inputs.push_back({"reflections",gpu::PixelFormat::R16G16B16A16_SFLOAT,ExtentDomain::Scaled});
    }
    result.outputs = {{"color", gpu::PixelFormat::R16G16B16A16_SFLOAT, ExtentDomain::Internal,
        gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Sampled}};
    return result;
}

gpu::PipelineLayoutDescription deferredBindingLayout(bool shadows, bool ambientOcclusion, bool globalIllumination, bool reflections)
{
    auto layout = drawBindingLayout();
    // Set 1 is unused by lighting: reserve textures 0..7 for G-buffer/background;
    // environment occupies native textures 8..11 in this fullscreen ABI.
    layout.resourceBindings.clear();
    layout.resourceBindings.push_back({0, {0, 2}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1});
    layout.resourceBindings.push_back({0, {1, 6}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment, 1});
    layout.resourceBindings.push_back({2, {0, 4}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment, 1});
    for (uint32_t i = 0; i < 4; ++i)
    {
        layout.resourceBindings.push_back({3, {i, 8 + i}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        layout.resourceBindings.push_back({3, {4 + i, 8 + i}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
    }
    layout.resourceBindings.push_back({3, {8, 5}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment, 1});
    const uint32_t count = (shadows ? 10u : 8u)+(ambientOcclusion ? 1u : 0u)+(globalIllumination ? 1u : 0u)+(reflections ? 1u : 0u);
    for (uint32_t i = 0; i < count; ++i)
    {
        const uint32_t metal = i < 8 ? i : (shadows && i < 10 ? i+4 :
            (ambientOcclusion && i == (shadows ? 10u : 8u) ? 14u :
             (globalIllumination && i == (shadows ? 10u : 8u)+(ambientOcclusion ? 1u : 0u) ? 15u : 16u)));
        layout.resourceBindings.push_back({1,{i,metal},gpu::ResourceType::SampledImage,gpu::ShaderStage::Fragment});
        layout.resourceBindings.push_back({1,{count+i,metal == 16 ? 0u : metal},gpu::ResourceType::Sampler,gpu::ShaderStage::Fragment});
    }
    layout.pushConstants.push_back({0, sizeof(glm::uvec4), gpu::ShaderStage::Fragment});
    return layout;
}

DeferredLayer::DeferredLayer(std::filesystem::path root, uint32_t count, gpu::PixelFormat depth, bool shadows, bool ambientOcclusion, bool globalIllumination, bool reflections)
    : _shaderRoot(root.empty() ? base::PlatformTools::shaderPath() : std::move(root)), _slots(count), _contract(deferredContract(depth, shadows, ambientOcclusion, globalIllumination, reflections))
{
}

LightingDebugView DeferredLayer::debugView() const
{
    return _debugView;
}

void DeferredLayer::setDebugView(LightingDebugView value)
{
    if (uint32_t(value) > uint32_t(LightingDebugView::ReflectionConfidence))
    {
        throw std::invalid_argument("Unknown lighting debug view");
    }
    _debugView = value;
}

const PassContract& DeferredLayer::contract() const
{
    return _contract;
}

void DeferredLayer::initialize(Engine& engine, const LayerConfiguration& configuration)
{
    const bool shadows = configuration.contract.shaderInterface.find(".shadows.") != std::string::npos;
    const bool ao = configuration.contract.shaderInterface.find(".ao.") != std::string::npos;
    const bool gi = configuration.contract.shaderInterface.find(".gi.") != std::string::npos;
    const bool reflections = configuration.contract.shaderInterface.find(".reflections.") != std::string::npos;
    _contract = deferredContract(configuration.contract.inputs.at(6).format, shadows, ao, gi, reflections);
    const std::string shader = std::string(shadows ? "draw/raster/deferred_shadow" : "draw/raster/deferred")+(ao ? "_ao" : "")+(gi ? "_gi" : "")+(reflections ? "_reflections" : "");
    _pass.initialize(engine, _shaderRoot, shader,
        engine.device()->createPipelineLayout(deferredBindingLayout(shadows,ao,gi,reflections)),
        gpu::PixelFormat::R16G16B16A16_SFLOAT, slots(engine, _slots));
}

void DeferredLayer::attachScene(std::shared_ptr<scene::Node>)
{
}

void DeferredLayer::resize(const LayerConfiguration&)
{
}

void DeferredLayer::update(const FrameContext& frame)
{
    _pass.beginFrame(frame);
}

void DeferredLayer::record(const PassContext& context)
{
    const auto* raster = context.raster;
    if (!raster || !raster->frame || !raster->lights || !raster->environment)
    {
        throw std::invalid_argument("Deferred layer requires prepared frame/light/environment bindings");
    }
    std::vector<gpu::Image*> inputs;
    for (const auto& input : context.inputs)
    {
        inputs.push_back(input.image);
    }
    const bool shadows = _contract.shaderInterface.find(".shadows.") != std::string::npos;
    const bool ao = _contract.shaderInterface.find(".ao.") != std::string::npos;
    const bool gi = _contract.shaderInterface.find(".gi.") != std::string::npos;
    std::vector<uint32_t> indices, samplers;
    for (uint32_t i = 0; i < inputs.size(); ++i)
    {
        indices.push_back(i < 8 ? i : (shadows && i < 10 ? i+4 :
            (ao && i == (shadows ? 10u : 8u) ? 14u :
             (gi && i == (shadows ? 10u : 8u)+(ao ? 1u : 0u) ? 15u : 16u))));
        samplers.push_back(indices.back() == 16 ? 0u : indices.back());
    }
    const glm::uvec4 debug{uint32_t(_debugView), 0, 0, 0};
    _pass.draw(context.frame, *context.outputs.at(0).image, inputs, 1,
        {{0, raster->frame->set(context.frame)}, {2, raster->lights->set(context.frame)}, {3, raster->environment->set(context.frame)}},
        &debug, sizeof(debug), indices, samplers, raster->region);
}

void DeferredLayer::cleanup()
{
    _pass.cleanup();
}

PassContract finalContract(gpu::PixelFormat output, gpu::PixelFormat depth, bool reconstructedColor)
{
    PassContract result{"draw.raster.final.v1", {{"color", gpu::PixelFormat::R16G16B16A16_SFLOAT}},
        {{"output", output, ExtentDomain::Output, gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Sampled}}};
    if (reconstructedColor)
    {
        result.shaderInterface = "draw.raster.final.reconstructed.v1";
        result.inputs[0].extent = ExtentDomain::Output;
    }
    auto gbuffer = gbufferContract(depth);
    for (auto slot : gbuffer.outputs)
    {
        slot.usage = gpu::ImageUsage::Sampled;
        result.inputs.push_back(slot);
    }
    return result;
}

FinalLayer::FinalLayer(std::filesystem::path root, uint32_t count, gpu::PixelFormat output, gpu::PixelFormat depth, bool reconstructedColor)
    : _shaderRoot(root.empty() ? base::PlatformTools::shaderPath() : std::move(root)), _slots(count), _contract(finalContract(output, depth, reconstructedColor))
{
}

float FinalLayer::exposure() const
{
    return settings.color.x;
}

void FinalLayer::setExposure(float value)
{
    if (!std::isfinite(value) || value < -32.0f || value > 32.0f)
    {
        throw std::invalid_argument("Final exposure is outside its supported range");
    }
    settings.color.x = value;
}

float FinalLayer::contrast() const
{
    return settings.color.y;
}

void FinalLayer::setContrast(float value)
{
    if (!std::isfinite(value) || value < 0.0f || value > 10.0f)
    {
        throw std::invalid_argument("Final contrast is outside its supported range");
    }
    settings.color.y = value;
}

float FinalLayer::brightness() const
{
    return settings.color.z;
}

void FinalLayer::setBrightness(float value)
{
    if (!std::isfinite(value) || value < -1.0f || value > 1.0f)
    {
        throw std::invalid_argument("Final brightness is outside its supported range");
    }
    settings.color.z = value;
}

float FinalLayer::gamma() const
{
    return settings.color.w;
}

void FinalLayer::setGamma(float value)
{
    if (!std::isfinite(value) || value < 0.1f || value > 8.0f)
    {
        throw std::invalid_argument("Final gamma is outside its supported range");
    }
    settings.color.w = value;
}

FinalDebugView FinalLayer::debugView() const
{
    return static_cast<FinalDebugView>(settings.display.x);
}

void FinalLayer::setDebugView(FinalDebugView value)
{
    if (static_cast<uint32_t>(value) > uint32_t(FinalDebugView::RawHDR))
    {
        throw std::invalid_argument("Unknown final debug view identity");
    }
    settings.display.x = static_cast<uint32_t>(value);
}

void FinalLayer::suppressBloom(bool value)
{
    _suppressBloom = value;
}

bool FinalLayer::bloomAvailable() const
{
    return _bloomAvailable;
}

const std::string& FinalLayer::bloomStatus() const
{
    return _bloomStatus;
}

const PassContract& FinalLayer::contract() const
{
    return _contract;
}

void FinalLayer::initialize(Engine& engine, const LayerConfiguration& configuration)
{
    const auto format = configuration.contract.outputs.at(0).format;
    _contract = finalContract(format, configuration.contract.inputs.at(7).format,
        configuration.contract.inputs[0].extent == ExtentDomain::Output);
    settings.display.y = format == gpu::PixelFormat::R8G8B8A8_SRGB || format == gpu::PixelFormat::B8G8R8A8_SRGB;
    gpu::PipelineLayoutDescription layout;
    imageBindings(layout, 0, 9);
    layout.pushConstants.push_back({0, sizeof(FinalSettings), gpu::ShaderStage::Fragment});
    const auto count = slots(engine, _slots);
    _pass.initialize(engine, _shaderRoot, "draw/raster/final", engine.device()->createPipelineLayout(layout), format, count);
    try
    {
        _bloom.initialize(engine, _shaderRoot, count);
        _bloomAvailable = true;
        _bloomStatus = "Dual filtering; one RGBA16F pyramid per frame slot";
    }
    catch (const std::runtime_error& error)
    {
        _bloom.cleanup();
        _bloomAvailable = false;
        _bloomStatus = std::string("Bloom unavailable: ") + error.what();
    }
}

void FinalLayer::attachScene(std::shared_ptr<scene::Node>)
{
}

void FinalLayer::resize(const LayerConfiguration&)
{
}

void FinalLayer::update(const FrameContext& frame)
{
    _pass.beginFrame(frame);
}

void FinalLayer::record(const PassContext& context)
{
    for (uint32_t i = 0; i < 4; ++i)
    {
        if (!std::isfinite(settings.color[i]))
        {
            throw std::invalid_argument("Final color settings must be finite");
        }
    }
    if (settings.color.w <= 0 || std::abs(settings.color.x) > 32 || settings.display.x > uint32_t(FinalDebugView::RawHDR))
    {
        throw std::invalid_argument("Invalid final gamma/exposure/debug view");
    }
    std::vector<gpu::Image*> inputs;
    for (const auto& input : context.inputs)
    {
        inputs.push_back(input.image);
    }
    settings.display.y = context.outputs.at(0).image->pixelFormat() == gpu::PixelFormat::R8G8B8A8_SRGB ||
        context.outputs.at(0).image->pixelFormat() == gpu::PixelFormat::B8G8R8A8_SRGB;
    bloom.validate();
    auto effectiveBloom = bloom;
    effectiveBloom.enabled = bloom.enabled && !_suppressBloom && settings.display.x == uint32_t(FinalDebugView::Final);
    gpu::Image* glow = _bloomAvailable ? _bloom.record(context.frame, *inputs.at(0), effectiveBloom) : nullptr;
    // A valid fallback descriptor is required even when the shader branch is off.
    inputs.push_back(glow ? glow : inputs.at(0));
    settings.outputExtent = {float(context.frame.extent.width), float(context.frame.extent.height), glow ? bloom.intensity : 0.0f, 0};
    _pass.draw(context.frame, *context.outputs.at(0).image, inputs, 0, {}, &settings, sizeof(settings));
}

void FinalLayer::cleanup()
{
    _pass.cleanup(); // Drains GPU users before releasing bloom attachments/bindings.
    _bloom.cleanup();
    _bloomAvailable = false;
    _bloomStatus = "Not initialized";
}
}
