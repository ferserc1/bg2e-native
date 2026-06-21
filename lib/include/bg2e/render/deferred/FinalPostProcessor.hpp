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

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/Image.hpp>

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace bg2e {
namespace render {
namespace deferred {

// Abstract interface for the final post-processing stage that takes the
// rendered scene at render-resolution and outputs it at display-resolution,
// performing anti-aliasing and/or upscaling.
//
// Concrete implementations:
//   SMAAPostProcessor  — available on all platforms
//   FSRPostProcessor   — Windows and Linux only
class BG2E_API FinalPostProcessor {
public:
    virtual ~FinalPostProcessor() = default;

    virtual void build(
        Engine* engine,
        VkExtent2D renderExtent,
        VkExtent2D displayExtent,
        VkFormat colorFormat
    ) = 0;

    // Recreate size-dependent resources. Call when viewport or render scale changes.
    virtual void resize(VkExtent2D renderExtent, VkExtent2D displayExtent) = 0;

    // Compute and store the jitter for this frame.  Returns the projection
    // matrix with sub-pixel jitter applied, ready to be used by all render
    // layers.  Implementations that do not need jitter (SMAA) return projMatrix
    // unmodified.
    virtual glm::mat4 prepare(
        const glm::mat4& projMatrix,
        uint32_t frameCounter,
        VkExtent2D renderExtent
    ) = 0;

    // Execute the post-processing pass.
    //   colorInput   — scene color at render resolution (SHADER_READ_ONLY_OPTIMAL on entry)
    //   depthInput   — scene depth at render resolution (SHADER_READ_ONLY_OPTIMAL on entry)
    //   motionVectors— RG16F motion vectors at render resolution (GENERAL on entry)
    //   colorOutput  — swapchain/display image that will receive the upscaled result
    //
    // After process() returns, colorOutput is in COLOR_ATTACHMENT_OPTIMAL.
    virtual void process(
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
    ) = 0;

    virtual void cleanup() = 0;

    // --- Scale UI API ---

    // Human-readable label for the scale control, e.g. "Render Scale" or "FSR3 Scale".
    virtual std::string processorName() const = 0;

    // Available scale options in display order.
    virtual std::vector<std::string> scaleOptions() const = 0;

    // Select an option by index into scaleOptions().
    virtual void setScaleOption(uint32_t index) = 0;

    // Currently selected index.
    virtual uint32_t scaleOption() const = 0;

    // Render scale as a percentage of the display size for the current option.
    // RendererDeferred reads this to drive its resize path.
    virtual float renderScalePercent() const = 0;
};

}
}
}
