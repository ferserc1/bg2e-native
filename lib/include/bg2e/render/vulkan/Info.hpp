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
#include <bg2e/render/vulkan/common.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_API Info {
public:
    static VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo(
        PFN_vkDebugUtilsMessengerCallbackEXT callback,
        void * pUserData = nullptr
    );
    
    static VkCommandPoolCreateInfo commandPoolCreateInfo(
        uint32_t queueFamilyIndex,
        VkCommandPoolCreateFlags flags = 0
    );

    static VkCommandBufferAllocateInfo commandBufferAllocateInfo(
        VkCommandPool pool,
        uint32_t count = 1
    );

    static VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);

    static VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

    static VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);

    static VkSemaphoreSubmitInfo semaphoreSubmitInfo(
        VkPipelineStageFlags2 stageMask,
        VkSemaphore semaphore
    );

    static VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd);

    static VkSubmitInfo2 submitInfo(
        VkCommandBufferSubmitInfo* cmdInfo,
        VkSemaphoreSubmitInfo* signalSemaphoreInfo,
        VkSemaphoreSubmitInfo*
        waitSemaphoreInfo
    );

    static VkSubmitInfo2 submitInfo(
        VkCommandBuffer cmd,
        VkPipelineStageFlags2 waitSemaphoreStageFlags, VkSemaphore waitSemaphore,
        VkPipelineStageFlags2 signalSemaphoreStageFlags, VkSemaphore signalSemaphore
    );

    static VkPresentInfoKHR presentInfo(
        VkSwapchainKHR& swapchain,
        VkSemaphore& waitSemaphore,
        uint32_t& imageIndex
    );

    static VkImageCreateInfo imageCreateInfo(
        VkFormat format,
        VkImageUsageFlags usageFlags,
        VkExtent3D extent,
        uint32_t arrayLayers = 1,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT
    );

    static VkImageViewCreateInfo imageViewCreateInfo(
        VkFormat format,
        VkImage image,
        VkImageAspectFlags aspectFlags
    );

    static VkRenderingAttachmentInfo attachmentInfo(
        VkImageView view,
        VkClearValue* clearValue,
        VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    static VkRenderingAttachmentInfo depthAttachmentInfo(
        VkImageView view,
        float depthValue = 1.0f,
        VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VkAttachmentLoadOp depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR
    );

    static VkRenderingInfo renderingInfo(
        VkExtent2D renderExtent,
        VkRenderingAttachmentInfo* colorAttachment,
        VkRenderingAttachmentInfo* depthAttachment,
        uint32_t colorAttachmentsCount = 1
    );

    static VkPipelineLayoutCreateInfo pipelineLayoutInfo();
};

}
}
}

