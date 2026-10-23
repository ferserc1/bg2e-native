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

#include <bg2e/draw/MetalFXUpscaler.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace bg2e::draw {
namespace {
float halton(uint32_t index, uint32_t base)
{
    float result = 0, weight = 1;
    while (index)
    {
        weight /= float(base);
        result += weight * float(index % base);
        index /= base;
    }
    return result;
}

bool finite(const glm::mat4& matrix)
{
    for (unsigned column = 0; column < 4; ++column)
    {
        for (unsigned row = 0; row < 4; ++row)
        {
            if (!std::isfinite(matrix[column][row]))
            {
                return false;
            }
        }
    }
    return true;
}
}

std::string MetalFXUpscaler::identifier() const
{
    return "bg2e.draw.metalfx.temporal.v1";
}

std::string MetalFXUpscaler::label() const
{
    return "Apple MetalFX Temporal Upscaling";
}

UpscalerRequirements MetalFXUpscaler::requirements() const
{
    // The bridge owns its storage output and copies into the renderer's target.
    return {true,true,true,false};
}

void MetalFXUpscaler::initialize(Engine& engine, const UpscalerConfiguration& configuration)
{
    if (_engine)
    {
        throw std::logic_error("Clean up MetalFX before initialization");
    }
    _engine = &engine;
    _backendSupported = engine.backendType() == gpu::BackendType::Metal &&
        gpu::metal::MetalFXBridge::supportsDevice(*engine.device());
    resize(configuration);
}

void MetalFXUpscaler::resize(const UpscalerConfiguration& configuration)
{
    if (!_engine)
    {
        throw std::logic_error("Initialize MetalFX before resizing");
    }
    // Renderer/delegate owns synchronization; never rebuild in prepare/record.
    _opaqueMotion.cleanup();
    _motion.cleanup();
    _depth.cleanup();
    _slots.clear();
    _bridge.cleanup();
    _configuration = configuration;
    _builtEnabled = _enabled;
    _failed = false;
    _error.clear();
    _opaqueMotionReady = false;
    _opaqueMotionStatus = "Disabled or unavailable";
    reset();
    if (!_enabled)
    {
        return;
    }
    try
    {
        if (!_bridge.initialize(*_engine->device(),configuration.internalExtent,configuration.outputExtent,configuration.frameSlots))
        {
            return;
        }
        gpu::PipelineLayoutDescription layout;
        layout.resourceBindings.push_back({0,{0,0},gpu::ResourceType::SampledImage,gpu::ShaderStage::Fragment,1});
        layout.resourceBindings.push_back({0,{1,0},gpu::ResourceType::Sampler,gpu::ShaderStage::Fragment,1});
        layout.pushConstants.push_back({0,128,gpu::ShaderStage::Fragment});
        _motion.initialize(*_engine,configuration.shaderRoot,"draw/upscale/motion",
            _engine->device()->createPipelineLayout(layout),gpu::PixelFormat::R16G16_SFLOAT,configuration.frameSlots);
        layout.pushConstants.clear();
        _depth.initialize(*_engine,configuration.shaderRoot,"draw/upscale/metalfx_depth",
            _engine->device()->createPipelineLayout(layout),gpu::PixelFormat::R32_SFLOAT,configuration.frameSlots);
        for (uint32_t index = 0; index < configuration.frameSlots; ++index)
        {
            Slot slot;
            gpu::ImageDescription image;
            image.size = configuration.internalExtent;
            image.format = gpu::PixelFormat::R16G16_SFLOAT;
            image.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc;
            image.debugName = "draw MetalFX complete opaque motion";
            slot.motion = _engine->device()->createImage(image);
            image.format = gpu::PixelFormat::R32_SFLOAT;
            image.debugName = "draw MetalFX normalized opaque depth";
            slot.depth = _engine->device()->createImage(image);
            _slots.push_back(std::move(slot));
        }
        try
        {
            gpu::PipelineLayoutDescription opaqueLayout;
            for (uint32_t index = 0; index < 3; ++index)
            {
                opaqueLayout.resourceBindings.push_back({0,{index,index},gpu::ResourceType::SampledImage,gpu::ShaderStage::Fragment,1});
                opaqueLayout.resourceBindings.push_back({0,{index+3,index},gpu::ResourceType::Sampler,gpu::ShaderStage::Fragment,1});
            }
            opaqueLayout.pushConstants.push_back({0,128,gpu::ShaderStage::Fragment});
            _opaqueMotion.initialize(*_engine,configuration.shaderRoot,"draw/upscale/motion_opaque",
                _engine->device()->createPipelineLayout(opaqueLayout),gpu::PixelFormat::R16G16_SFLOAT,configuration.frameSlots);
            _opaqueMotionReady = true;
            _opaqueMotionStatus = "Opaque object vectors with per-pixel camera/depth fallback";
        }
        catch (const std::exception& error)
        {
            _opaqueMotion.cleanup();
            _opaqueMotionStatus = std::string("Camera-only fallback: ") + error.what();
        }
    }
    catch (const std::exception& error)
    {
        _error = std::string("MetalFX initialization failed: ") + error.what() + "; bilinear reconstruction active";
        _opaqueMotion.cleanup();
        _motion.cleanup();
        _depth.cleanup();
        _slots.clear();
        _bridge.cleanup();
        _opaqueMotionReady = false;
    }
}

bool MetalFXUpscaler::enabled() const
{
    return _enabled;
}

void MetalFXUpscaler::setEnabled(bool value)
{
    _enabled = value;
}

bool MetalFXUpscaler::needsRebuild() const
{
    return _enabled != _builtEnabled;
}

bool MetalFXUpscaler::available() const
{
    return _enabled && !_failed && _bridge.available() && !_slots.empty();
}

bool MetalFXUpscaler::sdkAvailable() const
{
    return gpu::metal::MetalFXBridge::sdkAvailable();
}

bool MetalFXUpscaler::backendSupported() const
{
    return _backendSupported;
}

bool MetalFXUpscaler::opaqueMotionAvailable() const
{
    return available() && _opaqueMotionReady;
}

const std::string& MetalFXUpscaler::opaqueMotionStatus() const
{
    return _opaqueMotionStatus;
}

std::string MetalFXUpscaler::status() const
{
    if (!_enabled)
    {
        return "Disabled; bilinear reconstruction active";
    }
    if (!_error.empty())
    {
        return _error;
    }
    if (!_cameraSupported)
    {
        return "MetalFX requires a finite conventional perspective camera; unjittered bilinear reconstruction active";
    }
    return _bridge.status();
}

glm::mat4 MetalFXUpscaler::prepare(const UpscalerCamera& camera, uint64_t frame)
{
    const bool supported = finite(camera.view) && finite(camera.projection) &&
        std::abs(glm::determinant(camera.view)) > 1e-8f && std::abs(glm::determinant(camera.projection)) > 1e-8f &&
        std::abs(camera.projection[2][3] + 1.0f) < 0.0001f && std::abs(camera.projection[3][3]) < 0.0001f &&
        std::isfinite(camera.nearPlane) && std::isfinite(camera.farPlane) &&
        camera.nearPlane > 0 && camera.farPlane > camera.nearPlane &&
        camera.projection[2][2] < 0 && camera.projection[3][2] < 0;
    if (supported != _cameraSupported || camera.projection != _previousProjection ||
        (!_reset && (frame <= _lastSceneFrame || frame - _lastSceneFrame != 1)))
    {
        reset();
    }
    _cameraSupported = supported;
    _previousProjection = camera.projection;
    if (!available() || !supported)
    {
        return camera.projection;
    }
    const double scale = std::max(double(_configuration.outputExtent.width) / _configuration.internalExtent.width,
        double(_configuration.outputExtent.height) / _configuration.internalExtent.height);
    const auto phases = uint32_t(std::max(8.0,std::ceil(8.0 * scale * scale)));
    const auto index = uint32_t(frame % phases) + 1;
    const float x = halton(index,2) - 0.5f;
    const float y = halton(index,3) - 0.5f;
    glm::mat4 offset{1};
    offset[3][0] = 2 * x / float(_configuration.internalExtent.width);
    offset[3][1] = -2 * y / float(_configuration.internalExtent.height);
    return offset * camera.projection;
}

bool MetalFXUpscaler::record(const UpscalerInput& input)
{
    if (!_engine)
    {
        return false;
    }
    if (&input.frame.engine != _engine)
    {
        throw std::invalid_argument("MetalFX input must belong to its initialized engine");
    }
    input.opaqueMotion.validate(*_engine->device(),_configuration.internalExtent);
    if (!available() || !_cameraSupported)
    {
        return false;
    }
    const auto recording = input.frame.commandBuffer.recordingCompletion();
    if (!recording || input.frame.commandBuffer.hasActiveScope())
    {
        throw std::invalid_argument("MetalFX requires an active command recording outside encoder scopes");
    }
    if (!_reset && (!_previousRecording ||
        (!_previousRecording->wasSubmitted() && _previousRecording != recording) ||
        input.camera.projection != _recordedProjection || input.frame.sceneFrameNumber <= _lastSceneFrame ||
        input.frame.sceneFrameNumber - _lastSceneFrame != 1))
    {
        _reset = true;
    }
    const auto recordedJitter = input.jitteredProjection * glm::inverse(input.camera.projection);
    const float jitterX = recordedJitter[3][0] * float(_configuration.internalExtent.width) * 0.5f;
    const float jitterY = -recordedJitter[3][1] * float(_configuration.internalExtent.height) * 0.5f;
    if (!std::isfinite(jitterX) || !std::isfinite(jitterY))
    {
        throw std::invalid_argument("MetalFX recorded projection must encode finite pixel jitter");
    }
    auto& slot = _slots.at(input.frame.frameSlot);
    if (slot.recording && slot.recording != recording && !slot.recording->retired())
    {
        throw std::logic_error("MetalFX motion/depth slot is still in flight");
    }
    if (!slot.prepared || slot.serial != input.frame.frameNumber)
    {
        slot.inputs.clear();
        slot.prepared = true;
        slot.serial = input.frame.frameNumber;
    }
    slot.recording = recording;
    struct Push {
        glm::mat4 currentFromJittered, previousFromJittered;
    };
    static_assert(sizeof(Push) == 128 && offsetof(Push,previousFromJittered) == 64);
    const auto viewProjection = input.camera.projection * input.camera.view;
    const auto inverseCurrent = glm::inverse(input.jitteredProjection * input.camera.view);
    const Push push{viewProjection * inverseCurrent,
        (_reset ? viewProjection : _previousViewProjection) * inverseCurrent};
    if (input.opaqueMotion.motion && opaqueMotionAvailable() && !_reset)
    {
        slot.inputs.push_back(input.opaqueMotion);
        _opaqueMotion.draw(input.frame,*slot.motion,{&input.opaqueDepth,input.opaqueMotion.token,input.opaqueMotion.motion},
            0,{},&push,sizeof(push));
    }
    else
    {
        _motion.draw(input.frame,*slot.motion,{&input.opaqueDepth},0,{},&push,sizeof(push));
    }
    // Convert D16/D32 opaque depth to the R32F contract required by the native scaler.
    _depth.draw(input.frame,*slot.depth,{&input.opaqueDepth},0,{});
    const bool result = _bridge.record({input.frame.commandBuffer,input.color,*slot.depth,*slot.motion,input.output,
        input.frame.frameSlot,jitterX,jitterY,_reset});
    if (!result)
    {
        _failed = true;
        _reset = true;
        _error = _bridge.status();
        return false;
    }
    _previousRecording = recording;
    _recordedProjection = input.camera.projection;
    _previousViewProjection = viewProjection;
    _lastSceneFrame = input.frame.sceneFrameNumber;
    _reset = false;
    return true;
}

void MetalFXUpscaler::reset()
{
    _reset = true;
    _previousRecording.reset();
}

void MetalFXUpscaler::cleanup()
{
    // Caller has drained this renderer's graphics stream.
    _opaqueMotion.cleanup();
    _motion.cleanup();
    _depth.cleanup();
    _slots.clear();
    _bridge.cleanup();
    _engine = nullptr;
    _backendSupported = false;
    _opaqueMotionReady = false;
    _opaqueMotionStatus = "Not initialized";
    reset();
}

reflection::TypeInfo MetalFXUpscaler::settingsType() const
{
    reflection::TypeInfoBuilder<MetalFXUpscaler> type(identifier());
    type.property("available",&MetalFXUpscaler::available).category("Capabilities");
    type.property("sdkAvailable",&MetalFXUpscaler::sdkAvailable).category("Capabilities");
    type.property("backendSupported",&MetalFXUpscaler::backendSupported).category("Capabilities");
    type.property("opaqueMotionAvailable",&MetalFXUpscaler::opaqueMotionAvailable).category("Capabilities");
    type.property("opaqueMotionStatus",&MetalFXUpscaler::opaqueMotionStatus).category("Capabilities");
    type.property("status",&MetalFXUpscaler::status).category("Capabilities");
    return type.build();
}
}
