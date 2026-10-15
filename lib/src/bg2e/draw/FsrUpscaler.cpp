#include <bg2e/draw/FsrUpscaler.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <cmath>
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
    _engine->device()->waitIdle();
    _motion.cleanup();
    _motionImages.clear();
    _bridge.cleanup();
    _configuration = configuration;
    _builtEnabled = _enabled;
    _error.clear();
    _failed = false;
    reset();
    if (!_enabled || !_bridge.initialize(*_engine->device(), configuration.internalExtent, configuration.outputExtent, configuration.frameSlots))
    {
        return;
    }
    try
    {
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
            image.debugName = "draw FSR camera motion";
            _motionImages.push_back(_engine->device()->createImage(image));
        }
    }
    catch (const std::exception& error)
    {
        _error = error.what();
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
    return _error.empty() ? _bridge.status() : _error;
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
    if (!available() || !_cameraSupported)
    {
        return false;
    }
    auto& motion = *_motionImages.at(input.frame.frameSlot);
    struct Push {
        glm::mat4 inverseCurrent;
        glm::mat4 previousFromCurrent;
    } push{glm::inverse(input.jitteredProjection * input.camera.view),
        (_reset ? input.camera.projection * input.camera.view : _previousViewProjection)};
    // Shader reconstructs world position with inverseCurrent and subtracts the
    // current unjittered UV (derived from known jitter) using the matrix below.
    // previousFromCurrent maps current jittered clip directly to previous clip.
    push.previousFromCurrent = push.previousFromCurrent * push.inverseCurrent;
    // Reuse inverseCurrent to map current jittered clip to current unjittered clip.
    push.inverseCurrent = input.camera.projection * input.camera.view * push.inverseCurrent;
    _motion.draw(input.frame, motion, {&input.opaqueDepth}, 0, {}, &push, sizeof(push));
    const bool result = _bridge.record({input.frame.commandBuffer, input.color, input.opaqueDepth, motion, input.output,
        _jitterX, _jitterY, input.frame.sceneDeltaSeconds * 1000, input.camera.nearPlane, input.camera.farPlane,
        input.camera.verticalFov, _sharpness, _reset});
    if (!result)
    {
        _failed = true;
        _reset = true;
        _error = _bridge.status();
        return false;
    }
    _previousViewProjection = input.camera.projection * input.camera.view;
    _lastSceneFrame = input.frame.sceneFrameNumber;
    _reset = false;
    return true;
}
void FsrUpscaler::reset()
{
    _reset = true;
    _jitterX = _jitterY = 0;
}
void FsrUpscaler::cleanup()
{
    if (_engine)
    {
        _engine->device()->waitIdle();
    }
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
    type.property("hardwareStatus", &FsrUpscaler::hardwareStatus).category("Capabilities");
    type.property("status", &FsrUpscaler::status).category("Capabilities");
    return type.build();
}
}
