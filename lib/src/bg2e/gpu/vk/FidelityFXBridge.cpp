#include <bg2e/gpu/vk/FidelityFXBridge.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <cstdlib>

// The existing non-Apple build links the SDK. Apple builds have no SDK link.
// Header absence is a separate compile-time availability condition.
#if !defined(__APPLE__) && __has_include(<FidelityFX/host/ffx_fsr3upscaler.h>)
#define BG2E_DRAW_HAS_FFX 1
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/Image.hpp>
#include <bg2e/gpu/vk/CommandBuffer.hpp>
#include <FidelityFX/host/ffx_fsr3upscaler.h>
#include <FidelityFX/host/backends/vk/ffx_vk.h>
#endif

namespace bg2e::gpu::vk {
#ifdef BG2E_DRAW_HAS_FFX
namespace {
// Observe backend creation so a later SDK allocation/pipeline failure can still
// release its partially initialized context. The SDK callbacks are synchronous.
thread_local FfxCreateBackendContextFunc originalCreate = nullptr;
thread_local bool* observedCreation = nullptr;
FfxErrorCode observeCreate(FfxInterface* backend, FfxEffect effect,
    FfxEffectBindlessConfig* bindless, FfxUInt32* id)
{
    const auto create = originalCreate;
    backend->fpCreateBackendContext = create;
    const auto error = create(backend, effect, bindless, id);
    if (error == FFX_OK && observedCreation)
    {
        *observedCreation = true;
    }
    return error;
}
}
#endif
struct FidelityFXBridge::State {
    std::string status = "Not initialized";
    bool valid = false;
    bool contextCreated = false;
    std::string hardwareStatus = "Not assessed";
    Size2D internal, output;
#ifdef BG2E_DRAW_HAS_FFX
    gpu::Device* device = nullptr;
    FfxFsr3UpscalerContext context{};
    FfxInterface backend{};
    void* scratch = nullptr;
    std::shared_ptr<gpu::Image> dilatedDepth, dilatedMotion, previousDepth;
#endif
};
FidelityFXBridge::FidelityFXBridge() : _state(std::make_unique<State>())
{
}
FidelityFXBridge::~FidelityFXBridge()
{
    cleanup();
}
bool FidelityFXBridge::sdkAvailable()
{
#ifdef BG2E_DRAW_HAS_FFX
    return true;
#else
    return false;
#endif
}
const std::string& FidelityFXBridge::status() const
{
    return _state->status;
}
const std::string& FidelityFXBridge::hardwareStatus() const
{
    return _state->hardwareStatus;
}
bool FidelityFXBridge::available() const
{
    return _state->valid;
}
void FidelityFXBridge::cleanup()
{
#ifdef BG2E_DRAW_HAS_FFX
    if (_state->device)
    {
        _state->device->waitIdle();
    }
    if (_state->contextCreated)
    {
        ffxFsr3UpscalerContextDestroy(&_state->context);
    }
    _state->context = {};
    _state->backend = {};
    _state->dilatedDepth.reset();
    _state->dilatedMotion.reset();
    _state->previousDepth.reset();
    std::free(_state->scratch);
    _state->scratch = nullptr;
    _state->device = nullptr;
#endif
    _state->valid = false;
    _state->contextCreated = false;
}
bool FidelityFXBridge::initialize(gpu::Device& device, Size2D internal, Size2D output, uint32_t frameSlots)
{
    cleanup();
    _state->internal = internal;
    _state->output = output;
    _state->status = "FidelityFX SDK is unavailable in this build";
    _state->hardwareStatus = "Not assessed";
#ifdef BG2E_DRAW_HAS_FFX
    auto* native = dynamic_cast<vk::Device*>(&device);
    if (!native)
    {
        _state->status = "FSR requires the Vulkan backend; using bilinear reconstruction";
        return false;
    }
    if (internal.isZero() || output.isZero() || internal.width > output.width || internal.height > output.height)
    {
        _state->status = "FSR requires nonzero render size no larger than output";
        return false;
    }
    if (frameSlots == 0 || frameSlots > FFX_MAX_QUEUED_FRAMES)
    {
        _state->status = "FSR supports at most four frames in flight; bilinear reconstruction active";
        return false;
    }
    _state->device = &device;
    try
    {
        const auto bytes = ffxGetScratchMemorySizeVK(native->physicalDeviceHandle(), FFX_FSR3UPSCALER_CONTEXT_COUNT);
        _state->scratch = std::calloc(1, bytes);
        if (!_state->scratch)
        {
            throw std::runtime_error("FSR backend scratch allocation failed");
        }
        VkDeviceContext context{};
        context.vkDevice = native->handle();
        context.vkPhysicalDevice = native->physicalDeviceHandle();
        context.vkDeviceProcAddr = vkGetDeviceProcAddr;
        auto error = ffxGetInterfaceVK(&_state->backend, ffxGetDeviceVK(&context), _state->scratch, bytes,
            FFX_FSR3UPSCALER_CONTEXT_COUNT);
        if (error != FFX_OK)
        {
            throw std::runtime_error("FSR Vulkan interface failed: " + std::to_string(error));
        }
        FfxDeviceCapabilities capabilities{};
        error = _state->backend.fpGetDeviceCapabilities(&_state->backend, &capabilities);
        if (error != FFX_OK || capabilities.maximumSupportedShaderModel < FFX_SHADER_MODEL_6_0)
        {
            _state->hardwareStatus = "FSR device shader capabilities unsupported";
            throw std::runtime_error(_state->hardwareStatus);
        }
        if (!device.formatCapabilities(PixelFormat::R16G16B16A16_SFLOAT).supports(ImageUsage::Storage | ImageUsage::Sampled))
        {
            _state->hardwareStatus = "Required HDR storage format unsupported";
            throw std::runtime_error(_state->hardwareStatus);
        }
        _state->hardwareStatus = "Vulkan shader model and HDR storage supported";
        FfxFsr3UpscalerContextDescription description{};
        description.flags = FFX_FSR3UPSCALER_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FSR3UPSCALER_ENABLE_AUTO_EXPOSURE;
        description.maxRenderSize = {internal.width, internal.height};
        description.maxUpscaleSize = {output.width, output.height};
        description.backendInterface = _state->backend;
        const auto previousCreate = originalCreate;
        const auto previousObservation = observedCreation;
        originalCreate = description.backendInterface.fpCreateBackendContext;
        observedCreation = &_state->contextCreated;
        description.backendInterface.fpCreateBackendContext = observeCreate;
        error = ffxFsr3UpscalerContextCreate(&_state->context, &description);
        originalCreate = previousCreate;
        observedCreation = previousObservation;
        if (error != FFX_OK)
        {
            throw std::runtime_error("FSR context creation failed: " + std::to_string(error));
        }
        _state->contextCreated = true;
        _state->valid = true;
        FfxFsr3UpscalerSharedResourceDescriptions shared{};
        error = ffxFsr3UpscalerGetSharedResourceDescriptions(&_state->context, &shared);
        if (error != FFX_OK)
        {
            throw std::runtime_error("FSR shared resource description failed");
        }
        auto image = [&](const FfxResourceDescription& source, PixelFormat format)
        {
            ImageDescription desc;
            desc.size = {source.width, source.height};
            desc.format = format;
            desc.usage = ImageUsage::Storage | ImageUsage::Sampled;
            if (format != PixelFormat::R32_UINT)
            {
                desc.usage = desc.usage | ImageUsage::ColorAttachment;
            }
            desc.debugName = "draw FSR SDK shared resource";
            return device.createImage(desc);
        };
        _state->dilatedDepth = image(shared.dilatedDepth.resourceDescription, PixelFormat::R32_SFLOAT);
        _state->dilatedMotion = image(shared.dilatedMotionVectors.resourceDescription, PixelFormat::R16G16_SFLOAT);
        _state->previousDepth = image(shared.reconstructedPrevNearestDepth.resourceDescription, PixelFormat::R32_UINT);
        _state->status = "FSR 3.1 available (camera motion only)";
        return true;
    }
    catch (const std::exception& error)
    {
        const std::string message = error.what();
        cleanup();
        _state->status = message;
    }
#endif
    return false;
}
void FidelityFXBridge::jitter(uint64_t frame, float& x, float& y) const
{
    x = y = 0;
#ifdef BG2E_DRAW_HAS_FFX
    if (_state->valid)
    {
        const int32_t phases = ffxFsr3UpscalerGetJitterPhaseCount(int32_t(_state->internal.width), int32_t(_state->output.width));
        if (phases > 0)
        {
            ffxFsr3UpscalerGetJitterOffset(&x, &y, int32_t(frame % uint64_t(phases)), phases);
        }
    }
#endif
}
bool FidelityFXBridge::record(const FidelityFXDispatch& input)
{
#ifdef BG2E_DRAW_HAS_FFX
    if (!_state->valid)
    {
        return false;
    }
    auto* command = dynamic_cast<vk::CommandBuffer*>(&input.command);
    if (!command || input.command.hasActiveScope())
    {
        throw std::invalid_argument("FSR requires a Vulkan command buffer outside encoder scopes");
    }
    if (&input.output == &input.color || &input.output == &input.depth || &input.output == &input.motion)
    {
        throw std::invalid_argument("FSR output must not alias input images");
    }
    auto validate = [&](gpu::Image& image, Size2D extent, PixelFormat format, ImageUsage usage)
    {
        if (image.device() != _state->device || !image.isValid() || image.size() != extent ||
            image.pixelFormat() != format || !hasFlag(image.usage(), usage) || image.mipLevels() != 1)
        {
            throw std::invalid_argument("FSR image context/extent/format/usage mismatch");
        }
    };
    validate(input.color, _state->internal, PixelFormat::R16G16B16A16_SFLOAT, ImageUsage::Sampled);
    validate(input.motion, _state->internal, PixelFormat::R16G16_SFLOAT, ImageUsage::Sampled);
    validate(input.output, _state->output, PixelFormat::R16G16B16A16_SFLOAT, ImageUsage::Storage);
    if (input.depth.device() != _state->device || input.depth.size() != _state->internal ||
        !input.depth.isValid() || !hasFlag(input.depth.usage(), ImageUsage::Sampled) ||
        input.depth.mipLevels() != 1 || input.depth.arrayLayers() != 1 ||
        (input.depth.pixelFormat() != PixelFormat::D16_UNORM && input.depth.pixelFormat() != PixelFormat::D32_SFLOAT))
    {
        throw std::invalid_argument("FSR opaque depth mismatch");
    }
    auto resource = [](gpu::Image& image, FfxResourceUsage usage, FfxResourceStates state)
    {
        auto* native = dynamic_cast<vk::Image*>(&image);
        if (!native)
        {
            throw std::invalid_argument("FSR resource is not Vulkan");
        }
        FfxResourceDescription description{};
        description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
        description.format = ffxGetSurfaceFormatVK(native->vkFormat());
        description.width = image.size().width;
        description.height = image.size().height;
        description.depth = description.mipCount = 1;
        description.usage = usage;
        return ffxGetResourceVK(reinterpret_cast<void*>(native->handle()), description, nullptr, state);
    };
    input.command.transition(&input.color, ImageLayout::ShaderReadOnly);
    input.command.transition(&input.depth, ImageLayout::ShaderReadOnly);
    input.command.transition(&input.motion, ImageLayout::ShaderReadOnly);
    input.command.transition(&input.output, ImageLayout::General);
    for (const auto& image : {_state->dilatedDepth, _state->dilatedMotion, _state->previousDepth})
    {
        // Also emits a same-layout barrier between shared SDK submissions.
        input.command.transition(image.get(), ImageLayout::General);
    }
    FfxFsr3UpscalerDispatchDescription dispatch{};
    dispatch.commandList = ffxGetCommandListVK(command->handle());
    dispatch.color = resource(input.color, FFX_RESOURCE_USAGE_READ_ONLY, FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    dispatch.depth = resource(input.depth, FfxResourceUsage(FFX_RESOURCE_USAGE_READ_ONLY | FFX_RESOURCE_USAGE_DEPTHTARGET), FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    dispatch.motionVectors = resource(input.motion, FFX_RESOURCE_USAGE_READ_ONLY, FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    dispatch.output = resource(input.output, FFX_RESOURCE_USAGE_UAV, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
    const auto usage = FfxResourceUsage(FFX_RESOURCE_USAGE_UAV | FFX_RESOURCE_USAGE_RENDERTARGET);
    dispatch.dilatedDepth = resource(*_state->dilatedDepth, usage, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
    dispatch.dilatedMotionVectors = resource(*_state->dilatedMotion, usage, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
    dispatch.reconstructedPrevNearestDepth = resource(*_state->previousDepth, FFX_RESOURCE_USAGE_UAV, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
    dispatch.jitterOffset = {input.jitterX, input.jitterY};
    dispatch.motionVectorScale = {1, 1}; // Motion texture is previous minus current in pixels.
    dispatch.renderSize = {_state->internal.width, _state->internal.height};
    dispatch.upscaleSize = {_state->output.width, _state->output.height};
    dispatch.enableSharpening = input.sharpness > 0;
    dispatch.sharpness = input.sharpness;
    dispatch.frameTimeDelta = std::max(0.01f, input.deltaMilliseconds);
    dispatch.preExposure = 1;
    dispatch.reset = input.reset;
    dispatch.cameraNear = input.nearPlane;
    dispatch.cameraFar = input.farPlane;
    dispatch.cameraFovAngleVertical = input.verticalFov;
    dispatch.viewSpaceToMetersFactor = 1;
    const auto error = ffxFsr3UpscalerContextDispatch(&_state->context, &dispatch);
    // SDK unregister restores external resources to their registered states.
    // Generic tracking therefore remains ShaderReadOnly/General respectively.
    input.command.transition(&input.output, ImageLayout::ShaderReadOnly);
    if (error != FFX_OK)
    {
        _state->status = "FSR dispatch failed: " + std::to_string(error) + "; bilinear fallback active";
        return false;
    }
    return true;
#else
    return false;
#endif
}
}
