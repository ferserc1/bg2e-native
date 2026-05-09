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

#include <bg2e/render/RenderLoop.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/render/vulkan/Info.hpp>

namespace bg2e {
namespace render {

void RenderLoop::init(Engine * engine)
{
	_engine = engine;

    if (_renderDelegate)
    {
        _renderDelegate->init(engine);
    }
    
	_engine->iterateFrameResources([&](vulkan::FrameResources& frameResources) {
        frameResources.descriptorAllocator->init(_engine);
		initFrameResources(frameResources.descriptorAllocator);
	});
}

void RenderLoop::initScene()
{
    if (_renderDelegate)
    {
        _renderDelegate->initScene();
    }
}

void RenderLoop::acquireAndPresent()
{
    static VkFence skipFence = VK_NULL_HANDLE;
	if (!_engine) {
        throw new std::runtime_error("RenderLoop::render(): The frame cannot be rendered because Vulkan object has not been set.");
	}

    VkDevice dev = _engine->device().handle();
    auto swapchainData = _engine->swapchain();
    auto swapchain = swapchainData.handle();
    auto graphicsQueue = _engine->device().graphicsQueue();

    auto& frameResources = _engine->currentFrameResources();
	auto cmd = frameResources.commandBuffer;
    auto frameFence = frameResources.frameFence;
    auto swapchainSemaphore = frameResources.swapchainSemaphore;

    if (frameFence != skipFence)
    {
        VK_ASSERT(vkWaitForFences(dev, 1, &frameFence, true, 10000000000));
        VK_ASSERT(vkResetFences(dev, 1, &frameFence));
    }
    else
    {
        skipFence = VK_NULL_HANDLE;
    }

    frameResources.flushFrameData();

    uint32_t swapchainImageIndex;
	auto acquireResult = vulkan::acquireNextImage(dev, swapchain, 10000000000, swapchainSemaphore, nullptr, &swapchainImageIndex);
    if (acquireResult == VK_SUBOPTIMAL_KHR)
    {
        std::cout << "VK_SUBOPTIMAL_KHR" << std::endl;
        _engine->updateSwapchainSize();
    }
    else if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        _engine->device().waitIdle();
        _engine->updateSwapchainSize();
        std::cout << "VK_ERROR_OUT_OF_DATE_KHR" << std::endl;
        skipFence = frameFence;
        _engine->nextFrame();
        return;
    }

    auto renderSemaphore = swapchainData.renderSemaphore(swapchainImageIndex);

    if (_renderDelegate.get())
    {
        _renderDelegate->update(_engine->currentFrame(), frameResources);
        _renderDelegate->setDelta(this->_delta);
    }

    auto useMsaa = _renderDelegate->supportsMsaa();
    auto msaaImage = swapchainData.colorImage(swapchainImageIndex);
    auto resolveImage = swapchainData.msaaResolveImage(swapchainImageIndex);
    auto depthImage = swapchainData.depthImage();
    auto msaaDepthImage = swapchainData.msaaDepthImage();

    auto cmdBeginInfo = vulkan::Info::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_ASSERT(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    if (useMsaa)
    {
        vulkan::Image::cmdTransitionImage(
            cmd,
            msaaImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL
        );

        vulkan::Image::cmdTransitionImage(
            cmd,
            resolveImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );

        auto lastSwapchainLayout = render(
            cmd,
            msaaImage,
            depthImage,
            msaaDepthImage,
            frameResources
        );

        if (lastSwapchainLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
            vulkan::Image::cmdTransitionImage(
                cmd,
                msaaImage->handle(),
                lastSwapchainLayout,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            );
        }


        vulkan::Image::cmdTransitionImage(
            cmd,
            resolveImage->handle(),
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );


        // Resolve the MSAA image to the swapchain image
        VkImageResolve region = {};
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.srcOffset = { 0, 0, 0 };
        region.dstSubresource = region.srcSubresource;
        region.dstOffset = { 0, 0, 0 };
        region.extent = { msaaImage->extent() };
        vkCmdResolveImage(
            cmd,
            msaaImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            resolveImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region
        );

        vulkan::Image::cmdTransitionImage(
            cmd,
            resolveImage->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
    }
    else
    {

        vulkan::Image::cmdTransitionImage(
            cmd,
            resolveImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL
        );

        auto lastSwapchainLayout = render(
            cmd,
            resolveImage,
            depthImage,
            depthImage,
            frameResources
        );

        if (lastSwapchainLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            vulkan::Image::cmdTransitionImage(
                cmd,
                resolveImage->handle(),
                lastSwapchainLayout,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            );
        }
    }

    
    // TODO: Instead of using the swapchain image to render the user interface, we could use another image
    // and combine it with the swap chain here
	if (_renderUICallback)
	{
		_renderUICallback(cmd, resolveImage->imageView());
	}
    
    vulkan::Image::cmdTransitionImage(
        cmd,
        resolveImage->handle(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );

    // End command buffer
    VK_ASSERT(vkEndCommandBuffer(cmd));

    // Submit command buffer
    auto cmdInfo = vulkan::Info::commandBufferSubmitInfo(cmd);
    auto waitSemaphoreInfo = vulkan::Info::semaphoreSubmitInfo(
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        swapchainSemaphore
    );
    auto signalSemaphoreInfo = vulkan::Info::semaphoreSubmitInfo(
        VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        renderSemaphore
    );
    auto submitInfo = vulkan::Info::submitInfo(&cmdInfo, &signalSemaphoreInfo, &waitSemaphoreInfo);
    VK_ASSERT(vulkan::queueSubmit2(graphicsQueue, 1, &submitInfo, frameFence));

    // Present frame
    auto presentInfo = vulkan::Info::presentInfo(swapchain, renderSemaphore, swapchainImageIndex);
    auto presentResult = vulkan::queuePresent(graphicsQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
    {
        _engine->updateSwapchainSize();
        std::cout << "VK_ERROR_OUT_OF_DATE_KHR || VK_SUBOPTIMAL_KHR" << std::endl;
    }

    // Next frame
    _engine->nextFrame();
}

void RenderLoop::swapchainResized()
{
    if (_renderDelegate)
    {
        std::cout << "RenderLoop::swapchainResized()" << std::endl;
        auto newExtent = _engine->swapchain().extent();
		_renderDelegate->swapchainResized(newExtent);
    }
}

void RenderLoop::initFrameResources(vulkan::DescriptorSetAllocator* allocator)
{
    if (_renderDelegate)
    {
        _renderDelegate->initFrameResources(allocator);
    }
}

VkImageLayout RenderLoop::render(
    VkCommandBuffer cmd,
    const vulkan::Image* colorImage,
    const vulkan::Image* depthImage,
    const vulkan::Image* msaaDepthImage,
    vulkan::FrameResources& frameResources
) {
    if (_renderDelegate)
    {
        return _renderDelegate->render(
            cmd,
            _engine->currentFrame(),
            colorImage,
            depthImage,
            msaaDepthImage,
            frameResources
        );
    }
    else
    {
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

void RenderLoop::cleanup()
{
    if (_renderDelegate)
    {
        _renderDelegate->cleanup();
    }
}

}
}
