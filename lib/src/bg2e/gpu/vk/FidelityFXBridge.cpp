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

#include <bg2e/gpu/vk/FidelityFXBridge.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <cstdlib>
#include <mutex>
#include <unordered_map>

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
struct VulkanAdmission {
    Device::ShaderCapabilities shaders;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    FfxGetDeviceCapabilitiesFunc getCapabilities = nullptr;
    FfxCreateResourceFunc createResource = nullptr;
    FfxCreatePipelineFunc createPipeline = nullptr;
    std::string error;
};

// SDK interfaces are copied into contexts. scratchBuffer remains their stable,
// per-bridge identity, including callbacks during dispatch and destruction.
std::mutex admissionMutex;
std::unordered_map<void*, std::shared_ptr<VulkanAdmission>> admissions;

std::shared_ptr<VulkanAdmission> admission(FfxInterface* backend)
{
    std::lock_guard lock(admissionMutex);
    const auto found = admissions.find(backend->scratchBuffer);
    return found == admissions.end() ? nullptr : found->second;
}

FfxErrorCode getVulkanCapabilities(FfxInterface* backend, FfxDeviceCapabilities* capabilities)
{
    const auto state = admission(backend);
    if (!state)
    {
        return FFX_ERROR_BACKEND_API_ERROR;
    }
    const auto error = state->getCapabilities(backend, capabilities);
    if (error == FFX_OK)
    {
        capabilities->fp16Supported = state->shaders.float16 && state->shaders.int16;
        capabilities->waveLaneCountMin = state->shaders.subgroupSize;
        capabilities->waveLaneCountMax = state->shaders.subgroupSize;
        // Vulkan has no Shader Model. Preserve the SDK sentinel; in particular,
        // do not advertise SM 6.6 and accidentally request forced wave64.
    }
    return error;
}

std::string shaderIncompatibility(const Device::ShaderCapabilities& shaders, Size2D output)
{
    if (shaders.apiVersion < VK_API_VERSION_1_2)
    {
        return "Vulkan 1.2 is required by the FSR SPIR-V shaders";
    }
    if (!shaders.graphicsQueueCompute)
    {
        return "The graphics queue does not support compute";
    }
    if (!(shaders.subgroupStages & VK_SHADER_STAGE_COMPUTE_BIT) || shaders.subgroupSize == 0)
    {
        return "Compute subgroups are unavailable";
    }
    // Capabilities declared by the vendored GLSL/SPIR-V permutations, rather
    // than every extension mentioned by their shared GLSL headers.
    constexpr auto operations = VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_QUAD_BIT;
    if ((shaders.subgroupOperations & operations) != operations)
    {
        return "Compute subgroup basic and quad operations are required";
    }
    if (!shaders.storageImageExtendedFormats)
    {
        return "shaderStorageImageExtendedFormats is not enabled";
    }
    if (!shaders.storageImageWriteWithoutFormat)
    {
        return "shaderStorageImageWriteWithoutFormat is not enabled";
    }
    const auto& limits = shaders.limits;
    // SPD uses 256x1x1, other passes 64x1x1 or 8x8x1, and up to
    // four 16x16 float arrays plus a uint counter in shared memory.
    if (limits.maxComputeWorkGroupInvocations < 256 || limits.maxComputeWorkGroupSize[0] < 256 ||
        limits.maxComputeWorkGroupSize[1] < 8 || limits.maxComputeWorkGroupSize[2] < 1 ||
        limits.maxComputeSharedMemorySize < 4 * 16 * 16 * sizeof(float) + sizeof(uint32_t))
    {
        return "FSR compute workgroup or shared memory limits are unsupported";
    }
    if ((uint64_t(output.width) + 7) / 8 > limits.maxComputeWorkGroupCount[0] ||
        (uint64_t(output.height) + 7) / 8 > limits.maxComputeWorkGroupCount[1])
    {
        return "FSR output exceeds compute dispatch limits";
    }
    return {};
}

VkFormat fsrFormat(FfxSurfaceFormat format)
{
    switch (format)
    {
    case FFX_SURFACE_FORMAT_R8_UNORM: return VK_FORMAT_R8_UNORM;
    case FFX_SURFACE_FORMAT_R16_FLOAT: return VK_FORMAT_R16_SFLOAT;
    case FFX_SURFACE_FORMAT_R16_SNORM: return VK_FORMAT_R16_SNORM;
    case FFX_SURFACE_FORMAT_R16G16_FLOAT: return VK_FORMAT_R16G16_SFLOAT;
    case FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case FFX_SURFACE_FORMAT_R32_UINT: return VK_FORMAT_R32_UINT;
    case FFX_SURFACE_FORMAT_R32_FLOAT: return VK_FORMAT_R32_SFLOAT;
    case FFX_SURFACE_FORMAT_R32G32_FLOAT: return VK_FORMAT_R32G32_SFLOAT;
    case FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case FFX_SURFACE_FORMAT_R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
    default: return VK_FORMAT_UNDEFINED;
    }
}

FfxErrorCode createVulkanResource(FfxInterface* backend, const FfxCreateResourceDescription* description,
    FfxUInt32 context, FfxResourceInternal* resource)
{
    if (!description || !description->name || std::wstring(description->name).find_first_not_of(L" \t\r\n") == std::wstring::npos)
    {
        return FFX_ERROR_INVALID_ARGUMENT;
    }

    const auto state = admission(backend);
    if (!state)
    {
        return FFX_ERROR_BACKEND_API_ERROR;
    }
    const auto& image = description->resourceDescription;
    if (image.type == FFX_RESOURCE_TYPE_TEXTURE2D)
    {
        const auto format = fsrFormat(image.format);
        // Match the usage flags added by the vendored Vulkan backend.
        VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (image.usage & FFX_RESOURCE_USAGE_UAV)
        {
            usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (image.usage & FFX_RESOURCE_USAGE_RENDERTARGET)
        {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        VkImageFormatProperties properties{};
        const auto result = format == VK_FORMAT_UNDEFINED ? VK_ERROR_FORMAT_NOT_SUPPORTED :
            vkGetPhysicalDeviceImageFormatProperties(state->physical, format, VK_IMAGE_TYPE_2D,
                VK_IMAGE_TILING_OPTIMAL, usage, 0, &properties);
        VkFormatProperties features{};
        if (format != VK_FORMAT_UNDEFINED)
        {
            vkGetPhysicalDeviceFormatProperties(state->physical, format, &features);
        }
        const bool atomic = format == VK_FORMAT_R32_UINT && (image.usage & FFX_RESOURCE_USAGE_UAV);
        const bool linear = format == VK_FORMAT_R8_UNORM || format == VK_FORMAT_R16_SFLOAT ||
            format == VK_FORMAT_R16_SNORM || format == VK_FORMAT_R16G16_SFLOAT ||
            format == VK_FORMAT_R16G16B16A16_SFLOAT || format == VK_FORMAT_R32_SFLOAT ||
            format == VK_FORMAT_R8G8B8A8_UNORM;
        if (result != VK_SUCCESS || image.width > properties.maxExtent.width ||
            image.height > properties.maxExtent.height || image.depth > properties.maxArrayLayers ||
            (image.mipCount && image.mipCount > properties.maxMipLevels) ||
            (atomic && !(features.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT)) ||
            (linear && !(features.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)))
        {
            state->error = "Unsupported FSR image format/usage/extent: format=" +
                std::to_string(format) + ", usage=" + std::to_string(usage);
            return FFX_ERROR_BACKEND_API_ERROR;
        }
    }
    return state->createResource(backend, description, context, resource);
}

FfxErrorCode createVulkanPipeline(FfxInterface* backend, FfxEffect effect, FfxPass pass,
    uint32_t permutation, const FfxPipelineDescription* description, FfxUInt32 context, FfxPipelineState* pipeline)
{
    if (!description || std::wstring(description->name).find_first_not_of(L" \t\r\n") == std::wstring::npos)
    {
        return FFX_ERROR_INVALID_ARGUMENT;
    }

    const auto state = admission(backend);
    if (!state)
    {
        return FFX_ERROR_BACKEND_API_ERROR;
    }
    FfxShaderBlob blob{};
    const auto error = backend->fpGetPermutationBlobByIndex(effect, pass, FFX_BIND_COMPUTE_SHADER_STAGE,
        permutation, &blob);
    if (error != FFX_OK)
    {
        return error;
    }
    const auto count = [](uint32_t size, const uint32_t* counts)
    {
        uint64_t result = 0;
        for (uint32_t i = 0; i < size; ++i)
        {
            result += counts[i];
        }
        return result;
    };
    const auto sampled = count(blob.srvTextureCount, blob.boundSRVTextureCounts);
    const auto storage = count(blob.uavTextureCount, blob.boundUAVTextureCounts);
    const auto buffers = count(blob.srvBufferCount, blob.boundSRVBufferCounts) +
        count(blob.uavBufferCount, blob.boundUAVBufferCounts);
    const auto constants = count(blob.cbvCount, blob.boundConstantBufferCounts);
    // The SDK puts both immutable samplers in the layout, including unused ones.
    const auto samplers = description->samplerCount;
    const auto& limits = state->shaders.limits;
    if (sampled > std::min(limits.maxPerStageDescriptorSampledImages, limits.maxDescriptorSetSampledImages) ||
        storage > std::min(limits.maxPerStageDescriptorStorageImages, limits.maxDescriptorSetStorageImages) ||
        buffers > std::min(limits.maxPerStageDescriptorStorageBuffers, limits.maxDescriptorSetStorageBuffers) ||
        constants > std::min(limits.maxPerStageDescriptorUniformBuffers, limits.maxDescriptorSetUniformBuffers) ||
        samplers > std::min(limits.maxPerStageDescriptorSamplers, limits.maxDescriptorSetSamplers) ||
        sampled + storage + buffers + constants > limits.maxPerStageResources)
    {
        state->error = "FSR shader descriptors exceed Vulkan device limits (pass " + std::to_string(pass) + ")";
        return FFX_ERROR_BACKEND_API_ERROR;
    }
    for (uint32_t i = 0; i < description->rootConstantBufferCount; ++i)
    {
        if (uint64_t(description->rootConstants[i].size) * sizeof(uint32_t) > limits.maxUniformBufferRange)
        {
            state->error = "FSR constant buffer exceeds maxUniformBufferRange";
            return FFX_ERROR_BACKEND_API_ERROR;
        }
    }
    return state->createPipeline(backend, effect, pass, permutation, description, context, pipeline);
}

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
    std::shared_ptr<VulkanAdmission> admission;
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
    {
        std::lock_guard lock(admissionMutex);
        admissions.erase(_state->scratch);
    }
    _state->admission.reset();
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
        const auto incompatibility = shaderIncompatibility(native->shaderCapabilities(), output);
        if (!incompatibility.empty())
        {
            _state->hardwareStatus = "Unsupported: " + incompatibility;
            throw std::runtime_error(_state->hardwareStatus);
        }
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
        _state->admission = std::make_shared<VulkanAdmission>();
        _state->admission->shaders = native->shaderCapabilities();
        _state->admission->physical = native->physicalDeviceHandle();
        _state->admission->getCapabilities = _state->backend.fpGetDeviceCapabilities;
        _state->admission->createResource = _state->backend.fpCreateResource;
        _state->admission->createPipeline = _state->backend.fpCreatePipeline;
        {
            std::lock_guard lock(admissionMutex);
            admissions.emplace(_state->scratch, _state->admission);
        }
        _state->backend.fpGetDeviceCapabilities = getVulkanCapabilities;
        _state->backend.fpCreateResource = createVulkanResource;
        _state->backend.fpCreatePipeline = createVulkanPipeline;
        FfxDeviceCapabilities capabilities{};
        error = _state->backend.fpGetDeviceCapabilities(&_state->backend, &capabilities);
        if (error != FFX_OK)
        {
            _state->hardwareStatus = "FSR Vulkan capability query failed: " + std::to_string(error);
            throw std::runtime_error(_state->hardwareStatus);
        }
        if (!device.formatCapabilities(PixelFormat::R16G16B16A16_SFLOAT).supports(ImageUsage::Storage | ImageUsage::Sampled))
        {
            _state->hardwareStatus = "Required HDR storage format unsupported";
            throw std::runtime_error(_state->hardwareStatus);
        }
        for (const auto format : {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16_SFLOAT})
        {
            VkFormatProperties properties{};
            vkGetPhysicalDeviceFormatProperties(native->physicalDeviceHandle(), format, &properties);
            if (!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            {
                _state->hardwareStatus = "FSR input format does not support linear filtering: format=" +
                    std::to_string(format);
                throw std::runtime_error(_state->hardwareStatus);
            }
        }
        _state->hardwareStatus = "Vulkan compute supported; native subgroup size " +
            std::to_string(capabilities.waveLaneCountMin) +
            (capabilities.fp16Supported ? "; FP16" : "; FP32 fallback (FP16/Int16 not enabled)");
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
            if (!_state->admission->error.empty())
            {
                _state->hardwareStatus = _state->admission->error;
                throw std::runtime_error(_state->hardwareStatus);
            }
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
        auto image = [&](const FfxResourceDescription& source, PixelFormat format, const char* debugName)
        {
            VkFormatProperties properties{};
            const auto vkFormat = fsrFormat(source.format);
            vkGetPhysicalDeviceFormatProperties(native->physicalDeviceHandle(), vkFormat, &properties);
            const auto required = format == PixelFormat::R32_UINT ? VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT :
                VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
            if (!(properties.optimalTilingFeatures & required))
            {
                _state->hardwareStatus = "Unsupported FSR shared image filtering/atomics: format=" +
                    std::to_string(vkFormat);
                throw std::runtime_error(_state->hardwareStatus);
            }
            ImageDescription desc;
            desc.size = {source.width, source.height};
            desc.format = format;
            desc.usage = ImageUsage::Storage | ImageUsage::Sampled;
            if (format == PixelFormat::R32_UINT)
            {
                // FSR clears reconstructed previous depth with vkCmdClearColorImage.
                // Storage usage alone does not permit that transfer operation.
                desc.usage = desc.usage | ImageUsage::TransferDst;
            }
            else
            {
                desc.usage = desc.usage | ImageUsage::ColorAttachment;
            }
            desc.debugName = debugName;
            return device.createImage(desc);
        };
        _state->dilatedDepth = image(shared.dilatedDepth.resourceDescription, PixelFormat::R32_SFLOAT, "fsr-dilated-depth-image");
        _state->dilatedMotion = image(shared.dilatedMotionVectors.resourceDescription, PixelFormat::R16G16_SFLOAT, "fsr-dilated-motion-vectors-image");
        _state->previousDepth = image(shared.reconstructedPrevNearestDepth.resourceDescription, PixelFormat::R32_UINT, "fsr-reconstructed-previous-depth-image");
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
    auto resource = [](gpu::Image& image, FfxResourceUsage usage, FfxResourceStates state, const wchar_t* debugName)
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
        return ffxGetResourceVK(reinterpret_cast<void*>(native->handle()), description, debugName, state);
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
    dispatch.color = resource(input.color, FFX_RESOURCE_USAGE_READ_ONLY, FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ, L"fsr-input-color-image");
    dispatch.depth = resource(input.depth, FfxResourceUsage(FFX_RESOURCE_USAGE_READ_ONLY | FFX_RESOURCE_USAGE_DEPTHTARGET), FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ, L"fsr-input-depth-image");
    dispatch.motionVectors = resource(input.motion, FFX_RESOURCE_USAGE_READ_ONLY, FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ, L"fsr-input-motion-image");
    dispatch.output = resource(input.output, FFX_RESOURCE_USAGE_UAV, FFX_RESOURCE_STATE_UNORDERED_ACCESS, L"fsr-output-image");
    const auto usage = FfxResourceUsage(FFX_RESOURCE_USAGE_UAV | FFX_RESOURCE_USAGE_RENDERTARGET);
    dispatch.dilatedDepth = resource(*_state->dilatedDepth, usage, FFX_RESOURCE_STATE_UNORDERED_ACCESS, L"fsr-dilated-depth-image");
    dispatch.dilatedMotionVectors = resource(*_state->dilatedMotion, usage, FFX_RESOURCE_STATE_UNORDERED_ACCESS, L"fsr-dilated-motion-vectors-image");
    dispatch.reconstructedPrevNearestDepth = resource(*_state->previousDepth, FFX_RESOURCE_USAGE_UAV, FFX_RESOURCE_STATE_UNORDERED_ACCESS, L"fsr-reconstructed-previous-depth-image");
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
