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

#pragma once

#include <bg2e/render/deferred/FinalPostProcessor.hpp>

#if !defined(__APPLE__)

#include <FidelityFX/host/ffx_fsr3upscaler.h>
#include <FidelityFX/host/backends/vk/ffx_vk.h>
#include <bg2e/render/vulkan/Image.hpp>

namespace bg2e {
namespace render {
namespace deferred {

// FinalPostProcessor implementation using AMD FidelityFX Super Resolution 3.1.
// Available on Windows and Linux. Applies per-frame Halton jitter and upscales
// from render resolution to display resolution with temporal anti-aliasing.
class BG2E_API FSRPostProcessor : public FinalPostProcessor {
public:
    FSRPostProcessor() = default;
    ~FSRPostProcessor() override;

    bool build(
        Engine* engine,
        VkExtent2D renderExtent,
        VkExtent2D displayExtent,
        VkFormat colorFormat
    ) override;

    void resize(VkExtent2D renderExtent, VkExtent2D displayExtent) override;

    // Computes Halton jitter for this frame, stores it internally, and returns
    // the projection matrix with the jitter offset applied.
    glm::mat4 prepare(
        const glm::mat4& projMatrix,
        uint32_t frameCounter,
        VkExtent2D renderExtent
    ) override;

    void process(
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
    ) override;

    void cleanup() override;

    std::string processorName() const override { return "FSR3 Scale"; }
    std::vector<std::string> scaleOptions() const override;
    void setScaleOption(uint32_t index) override;
    uint32_t scaleOption() const override { return _scaleOptionIndex; }
    float renderScalePercent() const override;

private:
    Engine*    _engine        = nullptr;
    VkExtent2D _renderExtent  = {};
    VkExtent2D _displayExtent = {};
    VkFormat   _colorFormat   = VK_FORMAT_UNDEFINED;

    // FSR3 context and backend
    FfxFsr3UpscalerContext _fsrContext  = {};
    FfxInterface           _fsrBackend  = {};
    void*                  _scratchBuffer = nullptr;
    bool                   _contextValid  = false;

    // Jitter state — computed by prepare(), consumed by process()
    float _jitterX = 0.0f;
    float _jitterY = 0.0f;

    uint32_t _scaleOptionIndex = 2; // default: Performance (50%)

    // Ordered to match scaleOptions()
    static constexpr FfxFsr3UpscalerQualityMode kQualityModes[] = {
        FFX_FSR3UPSCALER_QUALITY_MODE_QUALITY,            // 0: Quality   (67%)
        FFX_FSR3UPSCALER_QUALITY_MODE_BALANCED,           // 1: Balanced  (59%)
        FFX_FSR3UPSCALER_QUALITY_MODE_PERFORMANCE,        // 2: Perf.     (50%)
        FFX_FSR3UPSCALER_QUALITY_MODE_ULTRA_PERFORMANCE,  // 3: Ultra     (33%)
    };

    bool createContext();
    void destroyContext();
    void createSharedResources();
    void destroySharedResources();

    FfxResource makeResource(const vulkan::Image* image, FfxResourceStates state,
                             const wchar_t* name = L"") const;
    FfxResource makeSharedResource(const vulkan::Image* image, FfxResourceUsage usage,
                                   FfxResourceStates state, const wchar_t* name) const;

    // Shared intermediate textures required by FfxFsr3UpscalerDispatchDescription.
    // Allocated at render resolution, recreated on resize.
    std::unique_ptr<vulkan::Image> _dilatedDepth;
    std::unique_ptr<vulkan::Image> _dilatedMotionVectors;
    std::unique_ptr<vulkan::Image> _reconstructedPrevNearestDepth;

    // Per-frame-in-flight FSR output images (storage + transfer src).
    // FSR cannot write directly to the swapchain/colorOutput (no STORAGE_BIT).
    // After dispatch we blit these to colorOutput.
    std::vector<std::unique_ptr<vulkan::Image>> _fsrOutputImages;

    // True after the shared resources have been transitioned to VK_IMAGE_LAYOUT_GENERAL
    // for the first time (they start as UNDEFINED after createSharedResources).
    bool _sharedResourcesInitialized = false;
};

}
}
}

#endif // !defined(__APPLE__)
