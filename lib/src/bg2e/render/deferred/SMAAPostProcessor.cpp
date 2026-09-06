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

#include <bg2e/render/deferred/SMAAPostProcessor.hpp>

namespace bg2e::render::deferred {

constexpr float SMAAPostProcessor::kScaleValues[];

bool SMAAPostProcessor::build(
    Engine* engine,
    VkExtent2D renderExtent,
    VkExtent2D displayExtent,
    VkFormat colorFormat
)
{
    _engine        = engine;
    _renderExtent  = renderExtent;
    _displayExtent = displayExtent;

    _smaa = std::make_unique<SMAAProcessor>(engine);
    _smaa->build(renderExtent, colorFormat);

    return true;
}

void SMAAPostProcessor::resize(VkExtent2D renderExtent, VkExtent2D displayExtent)
{
    _renderExtent  = renderExtent;
    _displayExtent = displayExtent;
    if (_smaa) _smaa->resize(renderExtent);
}

glm::mat4 SMAAPostProcessor::prepare(
    const glm::mat4& projMatrix,
    [[maybe_unused]] uint32_t frameCounter,
    [[maybe_unused]] VkExtent2D renderExtent
)
{
    return projMatrix;
}

void SMAAPostProcessor::process(
    VkCommandBuffer cmd,
    uint32_t frameIndex,
    const vulkan::Image* colorInput,
    [[maybe_unused]] const vulkan::Image* depthInput,
    [[maybe_unused]] const vulkan::Image* motionVectors,
    const vulkan::Image* colorOutput,
    [[maybe_unused]] float deltaMs,
    [[maybe_unused]] float cameraNear,
    [[maybe_unused]] float cameraFar,
    [[maybe_unused]] float cameraFovVertical
)
{
    // colorInput must be in SHADER_READ_ONLY_OPTIMAL on entry (caller's responsibility)
    const vulkan::Image* smaaOutput = _smaa->process(cmd, frameIndex, colorInput);

    bool needsBlit = _renderExtent.width  != _displayExtent.width ||
                     _renderExtent.height != _displayExtent.height;

    if (needsBlit)
    {
        vulkan::Image::cmdTransitionImage(
            cmd, smaaOutput->handle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        );
        vulkan::Image::cmdTransitionImage(
            cmd, colorOutput->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        VkImageBlit blitRegion = {};
        blitRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        blitRegion.srcOffsets[0]  = { 0, 0, 0 };
        blitRegion.srcOffsets[1]  = {
            static_cast<int32_t>(smaaOutput->extent2D().width),
            static_cast<int32_t>(smaaOutput->extent2D().height), 1
        };
        blitRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        blitRegion.dstOffsets[0]  = { 0, 0, 0 };
        blitRegion.dstOffsets[1]  = {
            static_cast<int32_t>(colorOutput->extent2D().width),
            static_cast<int32_t>(colorOutput->extent2D().height), 1
        };

        vkCmdBlitImage(
            cmd,
            smaaOutput->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            colorOutput->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blitRegion, VK_FILTER_LINEAR
        );

        vulkan::Image::cmdTransitionImage(
            cmd, smaaOutput->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        vulkan::Image::cmdTransitionImage(
            cmd, colorOutput->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
    }
    else
    {
        vulkan::Image::cmdCopy(
            cmd,
            smaaOutput->handle(), smaaOutput->extent2D(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            colorOutput->handle(), colorOutput->extent2D(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
    }
}

void SMAAPostProcessor::cleanup()
{
    if (_smaa) { _smaa->cleanup(); _smaa.reset(); }
}

std::vector<std::string> SMAAPostProcessor::scaleOptions() const
{
    return { "25%", "50%", "75%", "100%", "150%" };
}

void SMAAPostProcessor::setScaleOption(uint32_t index)
{
    constexpr uint32_t kCount = sizeof(kScaleValues) / sizeof(kScaleValues[0]);
    if (index < kCount) _scaleOptionIndex = index;
}

float SMAAPostProcessor::renderScalePercent() const
{
    return kScaleValues[_scaleOptionIndex];
}

}
