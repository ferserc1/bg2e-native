/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 */

#include <bg2e/render/deferred/DirectPostProcessor.hpp>

namespace bg2e::render::deferred {

bool DirectPostProcessor::build(
    Engine*, VkExtent2D renderExtent, VkExtent2D displayExtent, VkFormat)
{
    _renderExtent = renderExtent;
    _displayExtent = displayExtent;
    return renderExtent.width == displayExtent.width &&
           renderExtent.height == displayExtent.height;
}

void DirectPostProcessor::resize(VkExtent2D renderExtent, VkExtent2D displayExtent)
{
    _renderExtent = renderExtent;
    _displayExtent = displayExtent;
}

glm::mat4 DirectPostProcessor::prepare(
    const glm::mat4& projMatrix, uint32_t, VkExtent2D)
{
    return projMatrix;
}

void DirectPostProcessor::process(
    VkCommandBuffer cmd,
    uint32_t,
    const vulkan::Image* colorInput,
    const vulkan::Image*,
    const vulkan::Image*,
    const vulkan::Image* colorOutput,
    float,
    float,
    float,
    float
)
{
    vulkan::Image::cmdTransitionImage(
        cmd, colorInput->handle(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );
    vulkan::Image::cmdTransitionImage(
        cmd, colorOutput->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    VkImageCopy region{};
    region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    region.extent = {
        colorInput->extent2D().width,
        colorInput->extent2D().height,
        1
    };
    vkCmdCopyImage(
        cmd,
        colorInput->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        colorOutput->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &region
    );

    vulkan::Image::cmdTransitionImage(
        cmd, colorOutput->handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
}

}
