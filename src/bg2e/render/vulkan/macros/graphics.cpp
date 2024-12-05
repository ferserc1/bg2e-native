
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/extensions.hpp>

namespace bg2e::render::vulkan::macros {

void cmdSetDefaultViewportAndScissor(VkCommandBuffer cmd, VkExtent2D extent) {
    VkViewport viewport = {};
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void cmdClearImageAndBeginRendering(
    VkCommandBuffer cmd,
    const Image * colorImage,
    VkClearColorValue clearValue,
    VkImageLayout colorImageInitialLayout,
    const Image * depthImage,
    float depthValue
) {
    Image::cmdTransitionImage(
        cmd,
        colorImage->handle(),
        colorImageInitialLayout,
        VK_IMAGE_LAYOUT_GENERAL
    );

    auto clearRange = Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(
        cmd,
        colorImage->handle(),
        VK_IMAGE_LAYOUT_GENERAL,
        &clearValue, 1, &clearRange
    );

    Image::cmdTransitionImage(
        cmd, colorImage->handle(),
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    if (depthImage)
    {
        auto colorAttachment = Info::attachmentInfo(colorImage->imageView(), nullptr);
        auto depthAttachment = Info::depthAttachmentInfo(depthImage->imageView(), depthValue);
        auto renderInfo = Info::renderingInfo(colorImage->extent2D(), &colorAttachment, &depthAttachment);
        cmdBeginRendering(cmd, &renderInfo);
    }
    else
    {
        auto colorAttachment = Info::attachmentInfo(colorImage->imageView(), nullptr);
        auto renderInfo = Info::renderingInfo(colorImage->extent2D(), &colorAttachment, nullptr);
        cmdBeginRendering(cmd, &renderInfo);
    }
}

}
