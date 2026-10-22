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

#include <bg2e/draw/FsrUpscaler.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace bg2e::draw {
std::string FsrUpscaler::identifier() const
{
    return "bg2e.draw.fsr3.v1";
}

std::string FsrUpscaler::label() const
{
    return "AMD FidelityFX Super Resolution 3.1";
}

UpscalerRequirements FsrUpscaler::requirements() const
{
    return {true, true, true, true};
}

bool FsrUpscaler::sdkAvailable() const
{
    return gpu::vk::FidelityFXBridge::sdkAvailable();
}

bool FsrUpscaler::backendSupported() const
{
    return _engine && _engine->backendType() == gpu::BackendType::Vulkan;
}

bool FsrUpscaler::opaqueMotionAvailable() const
{
    return available() && _opaqueMotionReady;
}

const std::string& FsrUpscaler::opaqueMotionStatus() const
{
    return _opaqueMotionStatus;
}

std::string FsrUpscaler::hardwareStatus() const
{
    return _bridge.hardwareStatus();
}

bool FsrUpscaler::enabled() const
{
    return _enabled;
}

void FsrUpscaler::setEnabled(bool value)
{
    _enabled = value;
}

float FsrUpscaler::sharpness() const
{
    return _sharpness;
}

void FsrUpscaler::setSharpness(float value)
{
    if (!std::isfinite(value) || value < 0 || value > 1)
    {
        throw std::invalid_argument("FSR sharpness must be between zero and one");
    }
    _sharpness = value;
}

bool FsrUpscaler::needsRebuild() const
{
    return _builtEnabled != _enabled;
}

void FsrUpscaler::initialize(Engine& engine, const UpscalerConfiguration& configuration)
{
    if (_engine)
    {
        throw std::logic_error("Clean up FSR before initialization");
    }
    _engine = &engine;
    _configuration = configuration;
    resize(configuration);
}

void FsrUpscaler::resize(const UpscalerConfiguration& configuration)
{
    if (!_engine)
    {
        throw std::logic_error("Initialize FSR before resizing");
    }
    {
        base::CpuProfileScope profile("upscaler.waitIdle.resize");
        _engine->device()->waitIdle();
    }
    _opaqueMotion.cleanup();
    _opaqueMotionReady = false;
    _opaqueMotionStatus = "Not initialized";
    _motionSlots.clear();
    _motion.cleanup();
    _motionImages.clear();
    _bridge.cleanup();
    _configuration = configuration;
    _builtEnabled = _enabled;
    _error.clear();
    _failed = false;
    reset();
    if (!_enabled)
    {
        _opaqueMotionStatus = "Disabled";
        return;
    }
    try
    {
        if (!_bridge.initialize(*_engine->device(), configuration.internalExtent, configuration.outputExtent, configuration.frameSlots))
        {
            _opaqueMotionStatus = "Unavailable: "+_bridge.status();
            return;
        }
        gpu::PipelineLayoutDescription layout;
        layout.resourceBindings.push_back({0, {0, 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
        layout.resourceBindings.push_back({0, {1, 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
        layout.pushConstants.push_back({0, 128, gpu::ShaderStage::Fragment});
        _motion.initialize(*_engine, configuration.shaderRoot, "draw/upscale/motion",
            _engine->device()->createPipelineLayout(layout), gpu::PixelFormat::R16G16_SFLOAT, configuration.frameSlots);
        for (uint32_t i = 0; i < configuration.frameSlots; ++i)
        {
            gpu::ImageDescription image;
            image.size = configuration.internalExtent;
            image.format = gpu::PixelFormat::R16G16_SFLOAT;
            image.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment;
            image.debugName = "draw FSR complete motion";
            _motionImages.push_back(_engine->device()->createImage(image));
        }
        _motionSlots.resize(configuration.frameSlots);
        // Optional selection shader failure keeps the original camera generator usable.
        try
        {
            if (!_engine->device()->formatCapabilities(gpu::PixelFormat::R32_UINT).supports(gpu::ImageUsage::Sampled))
            {
                throw std::runtime_error("Integer opaque token sampling unsupported");
            }
            gpu::PipelineLayoutDescription opaqueLayout;
            for (uint32_t i = 0; i < 3; ++i)
            {
                opaqueLayout.resourceBindings.push_back({0,{i,i},gpu::ResourceType::SampledImage,gpu::ShaderStage::Fragment,1});
                opaqueLayout.resourceBindings.push_back({0,{i+3,i},gpu::ResourceType::Sampler,gpu::ShaderStage::Fragment,1});
            }
            opaqueLayout.pushConstants.push_back({0,128,gpu::ShaderStage::Fragment});
            _opaqueMotion.initialize(*_engine,configuration.shaderRoot,"draw/upscale/motion_opaque",
                _engine->device()->createPipelineLayout(opaqueLayout),gpu::PixelFormat::R16G16_SFLOAT,configuration.frameSlots);
            _opaqueMotionReady = true;
            _opaqueMotionStatus = "Ready: opaque object vectors with per-pixel camera/depth fallback";
        }
        catch (const std::exception& error)
        {
            _opaqueMotion.cleanup();
            _opaqueMotionReady = false;
            _opaqueMotionStatus = std::string("Camera-only fallback: ")+error.what();
        }
    }
    catch (const std::exception& error)
    {
        _error = error.what();
        _opaqueMotion.cleanup();
        _opaqueMotionReady = false;
        _opaqueMotionStatus = "Unavailable: "+_error;
        _motionSlots.clear();
        _motion.cleanup();
        _motionImages.clear();
        _bridge.cleanup();
    }
}

bool FsrUpscaler::available() const
{
    return _enabled && !_failed && _bridge.available() && !_motionImages.empty();
}

std::string FsrUpscaler::status() const
{
    if (!_enabled)
    {
        return "Disabled; bilinear reconstruction active";
    }
    if (!_cameraSupported)
    {
        return "FSR requires a finite conventional perspective camera; bilinear reconstruction active";
    }
    if (!_error.empty())
    {
        return _error;
    }
    if (opaqueMotionAvailable())
    {
        return "FSR 3.1 available; optional opaque object motion and camera fallback";
    }
    return _bridge.available() ? _bridge.status()+"; "+_opaqueMotionStatus : _bridge.status();
}

glm::mat4 FsrUpscaler::prepare(const UpscalerCamera& camera, uint64_t frame)
{
    const bool supported = std::abs(camera.projection[2][3] + 1.0f) < 0.0001f &&
        std::abs(camera.projection[3][3]) < 0.0001f && std::isfinite(camera.nearPlane) &&
        std::isfinite(camera.farPlane) && camera.nearPlane > 0 && camera.farPlane > camera.nearPlane &&
        std::isfinite(camera.verticalFov) && camera.verticalFov > 0 && camera.verticalFov < 3.14159265f;
    if (supported != _cameraSupported || camera.projection != _previousProjection)
    {
        reset();
    }
    _cameraSupported = supported;
    _previousProjection = camera.projection;
    if (!available() || !_cameraSupported)
    {
        return camera.projection;
    }
    if (!_reset && frame != _lastSceneFrame + 1)
    {
        reset();
    }
    _bridge.jitter(frame, _jitterX, _jitterY);
    // Pixel jitter maps to clip translation; framebuffer Y opposes NDC Y.
    glm::mat4 offset{1};
    offset[3][0] = 2 * _jitterX / float(_configuration.internalExtent.width);
    offset[3][1] = -2 * _jitterY / float(_configuration.internalExtent.height);
    return offset * camera.projection;
}

bool FsrUpscaler::record(const UpscalerInput& input)
{
    if (!_engine)
    {
        return false;
    }
    if (&input.frame.engine != _engine)
    {
        throw std::invalid_argument("FSR input must belong to its initialized engine");
    }
    input.opaqueMotion.validate(*_engine->device(),_configuration.internalExtent);
    if (!available() || !_cameraSupported)
    {
        return false;
    }
    const auto recording = input.frame.commandBuffer.recordingCompletion();
    if (!recording || input.frame.commandBuffer.hasActiveScope())
    {
        throw std::invalid_argument("FSR requires an active command recording outside encoder scopes");
    }
    if (!_reset && (!_previousRecording ||
        (!_previousRecording->wasSubmitted() && _previousRecording != recording) ||
        input.camera.projection != _recordedProjection ||
        input.frame.sceneFrameNumber <= _lastSceneFrame || input.frame.sceneFrameNumber-_lastSceneFrame != 1))
    {
        // prepare() may precede submission of the preceding scene. Resolve admission
        // here, without waiting, and preserve the jitter already recorded in GBuffer.
        _reset = true;
    }
    // Recover the SDK pixel jitter from this recorded projection, rather than the
    // latest prepare() call (which may already belong to a different scene/slot).
    const auto recordedJitter = input.jitteredProjection*glm::inverse(input.camera.projection);
    const float jitterX = recordedJitter[3][0]*float(_configuration.internalExtent.width)*0.5f;
    const float jitterY = -recordedJitter[3][1]*float(_configuration.internalExtent.height)*0.5f;
    if (!std::isfinite(jitterX) || !std::isfinite(jitterY))
    {
        throw std::invalid_argument("FSR recorded projection must encode finite pixel jitter");
    }
    auto& retained = _motionSlots.at(input.frame.frameSlot);
    if (!retained.prepared || retained.serial != input.frame.frameNumber)
    {
        retained.inputs.clear(); // Caller has retired this acquired slot.
        retained.prepared = true;
        retained.serial = input.frame.frameNumber;
    }
    auto& motion = *_motionImages.at(input.frame.frameSlot);
    struct Push {
        glm::mat4 inverseCurrent;
        glm::mat4 previousFromCurrent;
    } push{glm::inverse(input.jitteredProjection * input.camera.view),
        (_reset ? input.camera.projection * input.camera.view : _previousViewProjection)};
    static_assert(sizeof(Push) == 128 && offsetof(Push, previousFromCurrent) == 64);
    // Shader reconstructs world position with inverseCurrent and subtracts the
    // current unjittered UV (derived from known jitter) using the matrix below.
    // previousFromCurrent maps current jittered clip directly to previous clip.
    push.previousFromCurrent = push.previousFromCurrent * push.inverseCurrent;
    // Reuse inverseCurrent to map current jittered clip to current unjittered clip.
    push.inverseCurrent = input.camera.projection * input.camera.view * push.inverseCurrent;
    if (input.opaqueMotion.motion && opaqueMotionAvailable() && !_reset)
    {
        // Retain supplied strong owners before recording any reads, including on failure.
        retained.inputs.push_back(input.opaqueMotion);
        _opaqueMotion.draw(input.frame,motion,{&input.opaqueDepth,input.opaqueMotion.token,input.opaqueMotion.motion},
            0,{},&push,sizeof(push));
    }
    else
    {
        _motion.draw(input.frame, motion, {&input.opaqueDepth}, 0, {}, &push, sizeof(push));
    }
    const bool result = _bridge.record({input.frame.commandBuffer, input.color, input.opaqueDepth, motion, input.output,
        jitterX, jitterY, input.frame.sceneDeltaSeconds * 1000, input.camera.nearPlane, input.camera.farPlane,
        input.camera.verticalFov, _sharpness, _reset});
    if (!result)
    {
        _failed = true;
        _reset = true;
        _error = _bridge.status();
        return false;
    }
    _previousRecording = recording;
    _recordedProjection = input.camera.projection;
    _previousViewProjection = input.camera.projection * input.camera.view;
    _lastSceneFrame = input.frame.sceneFrameNumber;
    _reset = false;
    return true;
}

void FsrUpscaler::reset()
{
    _reset = true;
    _previousRecording.reset();
    _jitterX = _jitterY = 0;
}

void FsrUpscaler::cleanup()
{
    if (_engine)
    {
        base::CpuProfileScope profile("upscaler.waitIdle.cleanup");
        _engine->device()->waitIdle();
    }
    _opaqueMotion.cleanup();
    _opaqueMotionReady = false;
    _opaqueMotionStatus = "Not initialized";
    _motionSlots.clear();
    _motion.cleanup();
    _motionImages.clear();
    _bridge.cleanup();
    _engine = nullptr;
    reset();
}

reflection::TypeInfo FsrUpscaler::settingsType() const
{
    reflection::TypeInfoBuilder<FsrUpscaler> type(identifier());
    type.property("enabled", &FsrUpscaler::enabled, &FsrUpscaler::setEnabled).category("Upscaling");
    type.property("sharpness", &FsrUpscaler::sharpness, &FsrUpscaler::setSharpness).category("Upscaling").range(0, 1);
    type.property("available", &FsrUpscaler::available).category("Capabilities");
    type.property("sdkAvailable", &FsrUpscaler::sdkAvailable).category("Capabilities");
    type.property("backendSupported", &FsrUpscaler::backendSupported).category("Capabilities");
    type.property("opaqueMotionAvailable", &FsrUpscaler::opaqueMotionAvailable).category("Capabilities");
    type.property("opaqueMotionStatus", &FsrUpscaler::opaqueMotionStatus).category("Capabilities");
    type.property("hardwareStatus", &FsrUpscaler::hardwareStatus).category("Capabilities");
    type.property("status", &FsrUpscaler::status).category("Capabilities");
    return type.build();
}
}
