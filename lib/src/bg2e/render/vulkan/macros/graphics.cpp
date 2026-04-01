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

#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/extensions.hpp>

namespace bg2e::render::vulkan::macros {

void cmdSetDefaultViewportAndScissor(VkCommandBuffer cmd, VkExtent2D extent) {
    VkViewport viewport = {};

    viewport.x = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    
    // Invert Y axis
    viewport.height = -static_cast<float>(extent.height);
    viewport.y = static_cast<float>(extent.height);

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

extern BG2E_API void cmdClearImagesAndBeginRendering(
    VkCommandBuffer cmd,
    const std::vector<const Image *>& colorImages,
    VkClearColorValue clearValue,
    VkImageLayout colorImageInitialLayout,
    const Image * depthImage,
    float depthValue
) {
    if (colorImages.size() == 0)
    {
        throw new std::runtime_error("cmdClearImagesAndBeginRender - parameter error: the colorImage vector is empty");
    }
    
    auto clearRange = Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    VkExtent2D imageExtent = colorImages[0]->extent2D();
    for (auto image : colorImages)
    {
        Image::cmdTransitionImage(
            cmd, image->handle(),
            colorImageInitialLayout,
            VK_IMAGE_LAYOUT_GENERAL
        );
        
        vkCmdClearColorImage(
            cmd,
            image->handle(),
            VK_IMAGE_LAYOUT_GENERAL,
            &clearValue, 1, &clearRange
        );
        
        Image::cmdTransitionImage(
            cmd, image->handle(),
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
        
        auto colorAttachment = Info::attachmentInfo(image->imageView(), nullptr);
        colorAttachments.push_back(colorAttachment);
    }
    
    
    if (depthImage)
    {
        auto depthAttachment = Info::depthAttachmentInfo(depthImage->imageView(), depthValue);
        auto renderInfo = Info::renderingInfo(
            imageExtent,
            colorAttachments.data(),
            &depthAttachment,
            static_cast<uint32_t>(colorAttachments.size())
        );
        cmdBeginRendering(cmd, &renderInfo);
    }
    else
    {
        auto renderInfo = Info::renderingInfo(
            imageExtent,
            colorAttachments.data(),
            nullptr,
            static_cast<uint32_t>(colorAttachments.size())
        );
        cmdBeginRendering(cmd, &renderInfo);
    }
}

void cmdClearImageAndSetLayout(
    VkCommandBuffer cmd,
    const Image* colorImage,
    VkClearColorValue clearValue,
    VkImageLayout initialLayout,
    VkImageLayout finalLayout
) {
    Image::cmdTransitionImage(
        cmd,
        colorImage->handle(),
        initialLayout,
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
        finalLayout
    );
}

}
