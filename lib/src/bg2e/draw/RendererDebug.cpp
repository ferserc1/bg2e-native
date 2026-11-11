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

#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/gpu/Device.hpp>
#include <cmath>
#include <stdexcept>

namespace bg2e::draw {
struct RendererDeferred::DebugState {
    struct Slot {
        std::shared_ptr<gpu::Image> image, lighting;
        bool captured = false;
    };
    pipeline::FullscreenPass color, depth, display;
    std::vector<Slot> slots;
};

namespace {
struct DebugPush {
    glm::mat4 inverseProjection{1};
    glm::vec4 display{0,0,0,1};
    glm::vec4 rangeMode{0,10,0,0};
};
static_assert(sizeof(DebugPush) == 96);
}

const RendererDebugOptions& RendererDeferred::debugOptions() const
{
    return _debugOptions;
}

const std::string& RendererDeferred::debugStatus() const
{
    return _debugStatus;
}

void RendererDeferred::setDebugOptions(const RendererDebugOptions& options)
{
    if (uint32_t(options.view) > uint32_t(RendererDebugView::SheenRefraction) ||
        uint32_t(options.stage) > uint32_t(RendererDebugStage::Filtered) || options.layer > 1 ||
        options.channel > 4 || options.shadowIndex > 7 || options.temporalSignal > 2 || !std::isfinite(options.exposure) || std::abs(options.exposure) > 32 ||
        !std::isfinite(options.zoom) || options.zoom < 1 || options.zoom > 16 ||
        !std::isfinite(options.depthMinimum) || !std::isfinite(options.depthMaximum) ||
        options.depthMinimum < 0 || options.depthMaximum <= options.depthMinimum)
    {
        throw std::invalid_argument("Invalid renderer debug selection or display range");
    }
    const bool visibilityChanged = (_debugOptions.enabled && _debugOptions.disableTransparent) !=
        (options.enabled && options.disableTransparent);
    _debugOptions = options;
    _debugFailed = false;
    if (!options.enabled || options.view == RendererDebugView::Final)
    {
        cleanupDebug();
    }
    _debugStatus = !options.enabled ? "Disabled; no inspection resources" :
        (options.view == RendererDebugView::Final ? "Final scene; no buffer snapshots retained" : "Waiting for scene rendering");
    if (visibilityChanged)
    {
        invalidateHistories();
    }
}

void RendererDeferred::cleanupDebug()
{
    if (_debug)
    {
        _debug->color.cleanup();
        _debug->depth.cleanup();
        _debug->display.cleanup();
        _debug.reset();
    }
    _debugFailed = false;
    if (_debugUsedTemporal && _configuration.temporal)
    {
        _configuration.temporal->releaseDiagnosticCaptures();
        for (auto& target : _targets)
        {
            target.diagnostic.reset();
        }
        _debugUsedTemporal = false;
    }
}

void RendererDeferred::beginDebugFrame(const FrameContext& frame)
{
    if (!_debugOptions.enabled || _debugOptions.view == RendererDebugView::Final || _debugFailed)
    {
        return;
    }
    _debugUsedTemporal = _debugUsedTemporal || debugTemporalView() != pipeline::TemporalDiagnosticView::Off;
    if (!_debug)
    {
        _debug = std::make_shared<DebugState>();
        try
        {
            gpu::PipelineLayoutDescription description;
            description.debugName = "renderer-debug-display-layout";
            description.resourceBindings.push_back({0,{0,0},gpu::ResourceType::SampledImage,gpu::ShaderStage::Fragment,1});
            description.resourceBindings.push_back({0,{1,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Fragment,1});
            description.pushConstants.push_back({0,96,gpu::ShaderStage::Fragment});
            auto layout = _engine->device()->createPipelineLayout(description);
            _debug->color.initialize(*_engine,_shaderRoot,"draw/buffer_inspection/color",layout,
                gpu::PixelFormat::R16G16B16A16_SFLOAT,uint32_t(_targets.size()));
            _debug->depth.initialize(*_engine,_shaderRoot,"draw/buffer_inspection/depth",layout,
                gpu::PixelFormat::R16G16B16A16_SFLOAT,uint32_t(_targets.size()));
            _debug->display.initialize(*_engine,_shaderRoot,"draw/buffer_inspection/color",layout,
                _configuration.outputFormat,uint32_t(_targets.size()));
            _debug->slots.resize(_targets.size());
        }
        catch (const std::exception& error)
        {
            const std::string message = error.what();
            cleanupDebug();
            _debugFailed = true;
            _debugStatus = "Debug shaders unavailable: " + message;
            return;
        }
    }
    auto& slot = _debug->slots.at(frame.frameSlot);
    slot.captured = false;
    _debugStatus = "Selected buffer unavailable in this frame/layer; check Graphics Settings";
    if (!slot.image)
    {
        slot.image = pipeline::RenderTarget::create(*_engine,_configuration.outputExtent,
            gpu::PixelFormat::R16G16B16A16_SFLOAT,"renderer-debug-selected-buffer-slot-" + std::to_string(frame.frameSlot));
    }
    pipeline::RenderTarget::clear(frame.commandBuffer,*slot.image,{0.3f,0,0.3f,1});
}

void RendererDeferred::captureDebug(const FrameContext& frame, gpu::Image& image, uint32_t mode)
{
    if (!_debug || !_debugOptions.enabled || _debugOptions.view == RendererDebugView::Final)
    {
        return;
    }
    auto& slot = _debug->slots.at(frame.frameSlot);
    DebugPush push;
    push.inverseProjection = glm::inverse(_targets.at(frame.frameSlot).jitteredProjection);
    push.display = {float(slot.image->width()),float(slot.image->height()),0,1};
    push.rangeMode = {_debugOptions.depthMinimum,_debugOptions.depthMaximum,float(mode),-1};
    auto& pass = gpu::isDepthFormat(image.pixelFormat()) ? _debug->depth : _debug->color;
    pass.draw(frame,*slot.image,{&image},0,{},&push,sizeof(push));
    slot.captured = true;
    _debugStatus = "Source: " + std::to_string(image.width()) + " x " + std::to_string(image.height()) +
        "; jitter and normal rendering remain active";
}

void RendererDeferred::captureDebugSignal(const FrameContext& frame, pipeline::TemporalSignal kind,
    RendererDebugStage stage, gpu::Image& image, uint64_t layer)
{
    if (!_debugOptions.enabled || _debugOptions.layer != layer || _debugOptions.stage != stage)
    {
        return;
    }
    const auto view = _debugOptions.view;
    const bool selected = (kind == pipeline::TemporalSignal::DiffuseRadiance && view == RendererDebugView::GlobalIllumination) ||
        (kind == pipeline::TemporalSignal::Visibility && view == RendererDebugView::AmbientOcclusion) ||
        (kind == pipeline::TemporalSignal::ReflectedRadiance &&
            (view == RendererDebugView::Reflections || view == RendererDebugView::ReflectionConfidence));
    if (selected)
    {
        captureDebug(frame,image,view == RendererDebugView::ReflectionConfidence ? 4u :
            (kind == pipeline::TemporalSignal::Visibility ? 1u : 0u));
    }
}

void RendererDeferred::captureDebugGBuffer(const FrameContext& frame, uint64_t layer)
{
    if (!_debugOptions.enabled || _debugOptions.layer != layer)
    {
        return;
    }
    auto& slot = _targets.at(frame.frameSlot);
    size_t index = 0;
    uint32_t mode = 0;
    switch (_debugOptions.view)
    {
    case RendererDebugView::Albedo: break;
    case RendererDebugView::Normals: index = 1; break;
    case RendererDebugView::Roughness: index = 2; mode = 2; break;
    case RendererDebugView::Metallic: index = 2; mode = 1; break;
    case RendererDebugView::Material: index = 2; break;
    case RendererDebugView::FresnelFlags: index = 3; break;
    case RendererDebugView::SheenRefraction: index = 4; break;
    case RendererDebugView::Baked: index = 5; break;
    case RendererDebugView::Depth: index = 6; mode = 5; break;
    default: return;
    }
    captureDebug(frame,*slot.gbuffer.at(index),mode);
}

void RendererDeferred::captureDebugLighting(const FrameContext& frame,
    const std::vector<pipeline::ImageBinding>& inputs, const pipeline::RasterFrame& raster, uint64_t layer)
{
    if (!_debug || !_debugOptions.enabled || _debugOptions.layer != layer)
    {
        return;
    }
    pipeline::LightingDebugView view;
    switch (_debugOptions.view)
    {
    case RendererDebugView::Direct: view = pipeline::LightingDebugView::Direct; break;
    case RendererDebugView::Indirect: view = pipeline::LightingDebugView::Indirect; break;
    case RendererDebugView::Emission: view = pipeline::LightingDebugView::Emission; break;
    case RendererDebugView::Irradiance: view = pipeline::LightingDebugView::Irradiance; break;
    case RendererDebugView::Prefiltered: view = pipeline::LightingDebugView::Prefiltered; break;
    default: return;
    }
    auto lighting = std::dynamic_pointer_cast<pipeline::DeferredLayer>(_configuration.layers.at(1).layer);
    if (!lighting)
    {
        _debugStatus = "Debug lighting requires the standard deferred lighting layer";
        return;
    }
    auto& slot = _debug->slots.at(frame.frameSlot);
    if (!slot.lighting)
    {
        slot.lighting = pipeline::RenderTarget::create(*_engine,_configuration.internalExtent,
            gpu::PixelFormat::R16G16B16A16_SFLOAT,"renderer-debug-lighting-component-slot-" + std::to_string(frame.frameSlot));
    }
    const auto previous = lighting->debugView();
    struct Restore {
        pipeline::DeferredLayer& layer;
        pipeline::LightingDebugView view;
        ~Restore()
        {
            layer.setDebugView(view);
        }
    } restore{*lighting,previous};
    lighting->setDebugView(view);
    dispatch(1,frame,inputs,{{"color",slot.lighting.get()}},&raster);
    captureDebug(frame,*slot.lighting);
}

pipeline::TemporalDiagnosticView RendererDeferred::debugTemporalView() const
{
    if (!_debugOptions.enabled)
    {
        return pipeline::TemporalDiagnosticView::Off;
    }
    switch (_debugOptions.view)
    {
    case RendererDebugView::TemporalAcceptance: return pipeline::TemporalDiagnosticView::LocalHistory;
    case RendererDebugView::TemporalMotion: return pipeline::TemporalDiagnosticView::Motion;
    case RendererDebugView::TemporalCurrentWeight: return pipeline::TemporalDiagnosticView::CurrentWeight;
    case RendererDebugView::TemporalCount: return pipeline::TemporalDiagnosticView::EffectiveCount;
    case RendererDebugView::TemporalReactivity: return pipeline::TemporalDiagnosticView::Reactivity;
    case RendererDebugView::TemporalVariance: return pipeline::TemporalDiagnosticView::Variance;
    default: return pipeline::TemporalDiagnosticView::Off;
    }
}

void RendererDeferred::recordDebugFinal(const FrameContext& frame)
{
    if (!_debugOptions.enabled || !_debug || _debugOptions.view == RendererDebugView::Final)
    {
        return;
    }
    auto& slot = _debug->slots.at(frame.frameSlot);
    DebugPush push;
    push.display = {float(frame.extent.width),float(frame.extent.height),_debugOptions.exposure,_debugOptions.zoom};
    push.rangeMode.z = float(_debugOptions.channel);
    const auto format = frame.colorTarget.pixelFormat();
    push.rangeMode.w = format == gpu::PixelFormat::R8G8B8A8_SRGB || format == gpu::PixelFormat::B8G8R8A8_SRGB ? 1 : 0;
    _debug->display.draw(frame,frame.colorTarget,{slot.image.get()},0,{},&push,sizeof(push));
}
}
