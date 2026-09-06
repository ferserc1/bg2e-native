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

#if !defined(__APPLE__)

#include <bg2e/render/deferred/FSRPostProcessor.hpp>
#include <bg2e/base/Log.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace bg2e::render::deferred {

constexpr FfxFsr3UpscalerQualityMode FSRPostProcessor::kQualityModes[];

FSRPostProcessor::~FSRPostProcessor()
{
    destroyContext();
    if (_scratchBuffer) { free(_scratchBuffer); _scratchBuffer = nullptr; }
}

// ---------------------------------------------------------------------------
// build / resize
// ---------------------------------------------------------------------------

bool FSRPostProcessor::build(
    Engine* engine,
    VkExtent2D renderExtent,
    VkExtent2D displayExtent,
    VkFormat colorFormat
)
{
    _engine        = engine;
    _renderExtent  = renderExtent;
    _displayExtent = displayExtent;
    _colorFormat   = colorFormat;

    return createContext();
}

void FSRPostProcessor::resize(VkExtent2D renderExtent, VkExtent2D displayExtent)
{
    _engine->device().waitIdle();

    _renderExtent  = renderExtent;
    _displayExtent = displayExtent;

    destroyContext();
    createContext();
}

// ---------------------------------------------------------------------------
// Internal context management
// ---------------------------------------------------------------------------

bool FSRPostProcessor::createContext()
{
    auto vkDevice   = _engine->device().handle();
    auto vkPhysical = _engine->physicalDevice().handle();

    size_t scratchSize = ffxGetScratchMemorySizeVK(vkPhysical, FFX_FSR3UPSCALER_CONTEXT_COUNT);
    _scratchBuffer = realloc(_scratchBuffer, scratchSize);

    VkDeviceContext vkCtx{};
    vkCtx.vkDevice         = vkDevice;
    vkCtx.vkPhysicalDevice = vkPhysical;
    vkCtx.vkDeviceProcAddr = vkGetDeviceProcAddr;

    FfxDevice ffxDevice = ffxGetDeviceVK(&vkCtx);
    FfxErrorCode err = ffxGetInterfaceVK(
        &_fsrBackend, ffxDevice, _scratchBuffer, scratchSize,
        FFX_FSR3UPSCALER_CONTEXT_COUNT
    );
    if (err != FFX_OK)
    {
        bg2e_log_error << "FSRPostProcessor: ffxGetInterfaceVK failed (" << err << ")" << bg2e_log_end;
        return false;
    }

    FfxFsr3UpscalerContextDescription desc{};
    desc.flags          = FFX_FSR3UPSCALER_ENABLE_HIGH_DYNAMIC_RANGE;
    desc.maxRenderSize  = { _renderExtent.width,  _renderExtent.height };
    desc.maxUpscaleSize = { _displayExtent.width, _displayExtent.height };
    desc.backendInterface = _fsrBackend;
    desc.fpMessage = nullptr;

    err = ffxFsr3UpscalerContextCreate(&_fsrContext, &desc);
    if (err != FFX_OK)
    {
        bg2e_log_error << "FSRPostProcessor: ffxFsr3UpscalerContextCreate failed (" << err << ")" << bg2e_log_end;
        return false;
    }

    _contextValid = true;

    createSharedResources();
    return true;
}

void FSRPostProcessor::destroyContext()
{
    destroySharedResources();

    if (_contextValid)
    {
        ffxFsr3UpscalerContextDestroy(&_fsrContext);
        _contextValid = false;
    }
}

// SRGB formats don't support VK_IMAGE_USAGE_STORAGE_BIT on most GPUs.
// Map to the equivalent UNORM with the same component layout.
static VkFormat toStorageCompatibleFormat(VkFormat fmt)
{
    switch (fmt)
    {
        case VK_FORMAT_R8G8B8A8_SRGB:        return VK_FORMAT_R8G8B8A8_UNORM;
        case VK_FORMAT_B8G8R8A8_SRGB:        return VK_FORMAT_B8G8R8A8_UNORM;
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
        default: return fmt;
    }
}

void FSRPostProcessor::createSharedResources()
{
    // Query FSR3 for the required sizes/formats of the three shared intermediate textures.
    FfxFsr3UpscalerSharedResourceDescriptions sharedDescs{};
    ffxFsr3UpscalerGetSharedResourceDescriptions(&_fsrContext, &sharedDescs);

    // dilatedDepth: R32_FLOAT, UAV + color attachment
    _dilatedDepth.reset(vulkan::Image::createAllocatedImage(
        _engine, "FSR_DilatedDepth",
        VK_FORMAT_R32_SFLOAT,
        { sharedDescs.dilatedDepth.resourceDescription.width,
          sharedDescs.dilatedDepth.resourceDescription.height },
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1, false, 0, VK_SAMPLE_COUNT_1_BIT
    ));

    // dilatedMotionVectors: R16G16_FLOAT, UAV + color attachment
    _dilatedMotionVectors.reset(vulkan::Image::createAllocatedImage(
        _engine, "FSR_DilatedMotionVectors",
        VK_FORMAT_R16G16_SFLOAT,
        { sharedDescs.dilatedMotionVectors.resourceDescription.width,
          sharedDescs.dilatedMotionVectors.resourceDescription.height },
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1, false, 0, VK_SAMPLE_COUNT_1_BIT
    ));

    // reconstructedPrevNearestDepth: R32_UINT, UAV only
    _reconstructedPrevNearestDepth.reset(vulkan::Image::createAllocatedImage(
        _engine, "FSR_ReconstructedPrevNearestDepth",
        VK_FORMAT_R32_UINT,
        { sharedDescs.reconstructedPrevNearestDepth.resourceDescription.width,
          sharedDescs.reconstructedPrevNearestDepth.resourceDescription.height },
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1, false, 0, VK_SAMPLE_COUNT_1_BIT
    ));

    // Per-frame-in-flight FSR output images: swapchain/colorOutput images typically lack
    // STORAGE_BIT so FSR cannot write to them directly. FSR writes here; we then blit to colorOutput.
    // vkCmdBlitImage applies sRGB OETF encoding when the destination format is sRGB, so we use
    // the UNORM equivalent here and blit to whatever format colorOutput has.
    const VkFormat outputFmt = toStorageCompatibleFormat(_colorFormat);
    const uint32_t numFrames = _engine->numImages();
    _fsrOutputImages.resize(numFrames);
    for (uint32_t i = 0; i < numFrames; ++i)
    {
        _fsrOutputImages[i].reset(vulkan::Image::createAllocatedImage(
            _engine, "FSR_Output_" + std::to_string(i),
            outputFmt,
            _displayExtent,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1, false, 0, VK_SAMPLE_COUNT_1_BIT
        ));
    }
}

void FSRPostProcessor::destroySharedResources()
{
    for (auto& img : _fsrOutputImages) { if (img) { img->cleanup(); img.reset(); } }
    _fsrOutputImages.clear();

    if (_dilatedDepth)                  { _dilatedDepth->cleanup();                  _dilatedDepth.reset(); }
    if (_dilatedMotionVectors)          { _dilatedMotionVectors->cleanup();          _dilatedMotionVectors.reset(); }
    if (_reconstructedPrevNearestDepth) { _reconstructedPrevNearestDepth->cleanup(); _reconstructedPrevNearestDepth.reset(); }

    _sharedResourcesInitialized = false;
}

// ---------------------------------------------------------------------------
// prepare: jitter
// ---------------------------------------------------------------------------

glm::mat4 FSRPostProcessor::prepare(
    const glm::mat4& projMatrix,
    uint32_t frameCounter,
    VkExtent2D renderExtent
)
{
    int32_t phaseCount = ffxFsr3UpscalerGetJitterPhaseCount(
        static_cast<int32_t>(renderExtent.width),
        static_cast<int32_t>(_displayExtent.width)
    );

    ffxFsr3UpscalerGetJitterOffset(
        &_jitterX, &_jitterY,
        static_cast<int32_t>(frameCounter % static_cast<uint32_t>(phaseCount)),
        phaseCount
    );

    // Apply jitter to the projection matrix.
    // FSR returns pixel-space offsets; convert to NDC and add to proj[2] column.
    glm::mat4 jittered = projMatrix;
    jittered[2][0] += _jitterX * 2.0f / static_cast<float>(renderExtent.width);
    // Vulkan Y is positive downward in clip space; FSR jitter Y is positive upward.
    jittered[2][1] -= _jitterY * 2.0f / static_cast<float>(renderExtent.height);

    return jittered;
}

// ---------------------------------------------------------------------------
// process: FSR3 dispatch
// ---------------------------------------------------------------------------

static bool isDepthFormat(VkFormat fmt)
{
    switch (fmt)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return true;
        default:
            return false;
    }
}

FfxResource FSRPostProcessor::makeSharedResource(
    const vulkan::Image* image,
    FfxResourceUsage usage,
    FfxResourceStates state,
    const wchar_t* name
) const
{
    FfxResourceDescription resDesc{};
    resDesc.type     = FFX_RESOURCE_TYPE_TEXTURE2D;
    resDesc.format   = ffxGetSurfaceFormatVK(image->format());
    resDesc.width    = image->extent2D().width;
    resDesc.height   = image->extent2D().height;
    resDesc.depth    = 1;
    resDesc.mipCount = 1;
    resDesc.flags    = FFX_RESOURCE_FLAGS_NONE;
    resDesc.usage    = usage;

    return ffxGetResourceVK(
        reinterpret_cast<void*>(image->handle()),
        resDesc,
        const_cast<wchar_t*>(name),
        state
    );
}

FfxResource FSRPostProcessor::makeResource(
    const vulkan::Image* image,
    FfxResourceStates state,
    const wchar_t* name
) const
{
    FfxResourceDescription resDesc{};
    resDesc.type     = FFX_RESOURCE_TYPE_TEXTURE2D;
    resDesc.format   = ffxGetSurfaceFormatVK(image->format());
    resDesc.width    = image->extent2D().width;
    resDesc.height   = image->extent2D().height;
    resDesc.depth    = 1;
    resDesc.mipCount = 1;
    resDesc.flags    = FFX_RESOURCE_FLAGS_NONE;
    // Without FFX_RESOURCE_USAGE_DEPTHTARGET on a depth image, the backend creates
    // the internal VkImageView with R32_SFLOAT instead of D32_SFLOAT (ffx_vk.cpp:2764).
    resDesc.usage    = isDepthFormat(image->format())
        ? (FfxResourceUsage)(FFX_RESOURCE_USAGE_READ_ONLY | FFX_RESOURCE_USAGE_DEPTHTARGET)
        : FFX_RESOURCE_USAGE_READ_ONLY;

    return ffxGetResourceVK(
        reinterpret_cast<void*>(image->handle()),
        resDesc,
        const_cast<wchar_t*>(name),
        state
    );
}

void FSRPostProcessor::process(
    VkCommandBuffer cmd,
    uint32_t frameIndex,
    const vulkan::Image* colorInput,
    const vulkan::Image* depthInput,
    const vulkan::Image* motionVectors,
    const vulkan::Image* colorOutput,
    float deltaMs,
    float cameraNear,
    float cameraFar,
    float cameraFovVertical
)
{
    if (!_contextValid) return;

    // Shared resources start as VK_IMAGE_LAYOUT_UNDEFINED after creation.
    // Transition them to GENERAL once before the first dispatch so FSR can use them as UAV.
    if (!_sharedResourcesInitialized)
    {
        vulkan::Image::cmdTransitionImage(cmd, _dilatedDepth->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        vulkan::Image::cmdTransitionImage(cmd, _dilatedMotionVectors->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        vulkan::Image::cmdTransitionImage(cmd, _reconstructedPrevNearestDepth->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        _sharedResourcesInitialized = true;
    }

    // Transition motion vectors from GENERAL (storage write) to read state
    vulkan::Image::cmdTransitionImage(cmd, motionVectors->handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Pick the per-frame intermediate output image and bring it to GENERAL for FSR write.
    // Using UNDEFINED as old layout discards previous content (FSR overwrites entirely).
    auto* fsrOut = _fsrOutputImages[frameIndex % static_cast<uint32_t>(_fsrOutputImages.size())].get();
    vulkan::Image::cmdTransitionImage(cmd, fsrOut->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    FfxFsr3UpscalerDispatchDescription dispatch{};
    dispatch.commandList = ffxGetCommandListVK(cmd);

    dispatch.color         = makeResource(colorInput,    FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ, L"FSR_Color");
    dispatch.depth         = makeResource(depthInput,    FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ, L"FSR_Depth");
    dispatch.motionVectors = makeResource(motionVectors, FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ, L"FSR_MotionVectors");
    dispatch.exposure                   = {};
    dispatch.reactive                   = {};
    dispatch.transparencyAndComposition = {};

    // Write to our intermediate image which has VK_IMAGE_USAGE_STORAGE_BIT.
    {
        FfxResourceDescription outDesc{};
        outDesc.type     = FFX_RESOURCE_TYPE_TEXTURE2D;
        outDesc.format   = ffxGetSurfaceFormatVK(fsrOut->format());
        outDesc.width    = fsrOut->extent2D().width;
        outDesc.height   = fsrOut->extent2D().height;
        outDesc.depth    = 1;
        outDesc.mipCount = 1;
        outDesc.flags    = FFX_RESOURCE_FLAGS_NONE;
        outDesc.usage    = FFX_RESOURCE_USAGE_UAV;
        dispatch.output  = ffxGetResourceVK(
            reinterpret_cast<void*>(fsrOut->handle()),
            outDesc, const_cast<wchar_t*>(L"FSR_Output"),
            FFX_RESOURCE_STATE_UNORDERED_ACCESS
        );
    }

    const FfxResourceUsage kUavRt = (FfxResourceUsage)(FFX_RESOURCE_USAGE_UAV | FFX_RESOURCE_USAGE_RENDERTARGET);
    dispatch.dilatedDepth = makeSharedResource(
        _dilatedDepth.get(), kUavRt, FFX_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR_DilatedDepth");
    dispatch.dilatedMotionVectors = makeSharedResource(
        _dilatedMotionVectors.get(), kUavRt, FFX_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR_DilatedMotionVectors");
    dispatch.reconstructedPrevNearestDepth = makeSharedResource(
        _reconstructedPrevNearestDepth.get(), FFX_RESOURCE_USAGE_UAV,
        FFX_RESOURCE_STATE_UNORDERED_ACCESS, L"FSR_ReconstructedPrevNearestDepth");

    dispatch.jitterOffset            = { _jitterX, _jitterY };
    dispatch.motionVectorScale       = { 1.0f, 1.0f };
    dispatch.renderSize              = { _renderExtent.width,  _renderExtent.height };
    dispatch.upscaleSize             = { _displayExtent.width, _displayExtent.height };
    dispatch.enableSharpening        = true;
    dispatch.sharpness               = 0.5f;
    dispatch.frameTimeDelta          = deltaMs;
    dispatch.preExposure             = 1.0f;
    dispatch.reset                   = false;
    dispatch.cameraNear              = cameraNear;
    dispatch.cameraFar               = cameraFar;
    dispatch.cameraFovAngleVertical  = cameraFovVertical;
    dispatch.viewSpaceToMetersFactor = 1.0f;
    dispatch.flags                   = 0;

    FfxErrorCode err = ffxFsr3UpscalerContextDispatch(&_fsrContext, &dispatch);
    if (err != FFX_OK)
        bg2e_log_error << "FSRPostProcessor: ffxFsr3UpscalerContextDispatch failed (" << err << ")" << bg2e_log_end;

    // Blit FSR output → colorOutput.
    // vkCmdBlitImage applies sRGB OETF encoding when the destination format is sRGB,
    // so a UNORM intermediate blitting to an sRGB target is gamma-correct.
    vulkan::Image::cmdTransitionImage(cmd, fsrOut->handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    // Use UNDEFINED as old layout — colorOutput content is fully replaced by the blit.
    vulkan::Image::cmdTransitionImage(cmd, colorOutput->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkImageBlit blitRegion{};
    blitRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    blitRegion.srcOffsets[1]  = { static_cast<int32_t>(fsrOut->extent2D().width),
                                   static_cast<int32_t>(fsrOut->extent2D().height), 1 };
    blitRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    blitRegion.dstOffsets[1]  = { static_cast<int32_t>(colorOutput->extent2D().width),
                                   static_cast<int32_t>(colorOutput->extent2D().height), 1 };
    vkCmdBlitImage(
        cmd,
        fsrOut->handle(),      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        colorOutput->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blitRegion, VK_FILTER_NEAREST
    );

    vulkan::Image::cmdTransitionImage(cmd, colorOutput->handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

// ---------------------------------------------------------------------------
// Scale UI
// ---------------------------------------------------------------------------

void FSRPostProcessor::cleanup()
{
    destroyContext();
}

std::vector<std::string> FSRPostProcessor::scaleOptions() const
{
    return {
        "Quality (67%)",
        "Balanced (59%)",
        "Performance (50%)",
        "Ultra Performance (33%)"
    };
}

void FSRPostProcessor::setScaleOption(uint32_t index)
{
    constexpr uint32_t kCount = sizeof(kQualityModes) / sizeof(kQualityModes[0]);
    if (index < kCount) _scaleOptionIndex = index;
}

float FSRPostProcessor::renderScalePercent() const
{
    float ratio = ffxFsr3UpscalerGetUpscaleRatioFromQualityMode(
        kQualityModes[_scaleOptionIndex]
    );
    // ratio is the upscale ratio (e.g., 2.0 for Performance means render at 50%)
    return (ratio > 0.0f) ? (100.0f / ratio) : 50.0f;
}

}

#endif // !defined(__APPLE__)
