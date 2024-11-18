
#include <bg2e/render/RenderLoop.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/render/vulkan/Info.hpp>

namespace bg2e {
namespace render {

void RenderLoop::init(Vulkan * vulkan)
{
	_vulkan = vulkan;

    if (_renderDelegate)
    {
        _renderDelegate->init(vulkan);
    }

	_vulkan->iterateFrameResources([&](vulkan::FrameResources& frameResources) {
        frameResources.descriptorAllocator->init(_vulkan);
		initFrameResources(frameResources.descriptorAllocator);
	});
}

void RenderLoop::acquireAndPresent()
{
	if (!_vulkan) {
        throw new std::runtime_error("RenderLoop::render(): The frame cannot be rendered because Vulkan object has not been set.");
	}

    VkDevice dev = _vulkan->device().handle();
    auto swapchainData = _vulkan->swapchain();
    auto swapchain = swapchainData.handle();
    auto graphicsQueue = _vulkan->device().graphicsQueue();

    auto& frameResources = _vulkan->currentFrameResources();
	auto cmd = frameResources.commandBuffer;
    auto frameFence = frameResources.frameFence;
    auto swapchainSemaphore = frameResources.swapchainSemaphore;
	auto renderSemaphore = frameResources.renderSemaphore;

    VK_ASSERT(vkWaitForFences(dev, 1, &frameFence, true, 10000000000));
	VK_ASSERT(vkResetFences(dev, 1, &frameFence));

    frameResources.flushFrameData();

    uint32_t swapchainImageIndex;
	auto acquireResult = vulkan::acquireNextImage(dev, swapchain, 10000000000, swapchainSemaphore, nullptr, &swapchainImageIndex);
    if (acquireResult == VK_SUBOPTIMAL_KHR)
    {
        _vulkan->updateSwapchainSize();
    }
    else if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        _vulkan->updateSwapchainSize();
        return;
    }

    if (_renderDelegate.get())
    {
        _renderDelegate->update(_vulkan->currentFrame(), frameResources);
    }

    auto swapchainImage = swapchainData.colorImage(swapchainImageIndex);
    auto depthImage = swapchainData.depthImage();

    auto cmdBeginInfo = vulkan::Info::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_ASSERT(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    vulkan::Image::cmdTransitionImage(
        cmd,
        depthImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    );

    auto lastSwapchainLayout = render(
        cmd,
        swapchainImage,
        depthImage,
        frameResources
    );

    if (lastSwapchainLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        vulkan::Image::cmdTransitionImage(
            cmd,
            swapchainImage->handle(),
            lastSwapchainLayout,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
    }

    // TODO: Instead of using the swapchain image to render the user interface, we could use another image
    // and combine it with the swap chain here
	if (_renderUICallback)
	{
		_renderUICallback(cmd, swapchainImage->imageView());
	}

    vulkan::Image::cmdTransitionImage(
        cmd,
        swapchainImage->handle(),
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
        _vulkan->updateSwapchainSize();
    }

    // Next frame
    _vulkan->nextFrame();
}

void RenderLoop::swapchainResized()
{
    if (_renderDelegate)
    {
        auto newExtent = _vulkan->swapchain().extent();
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
    vulkan::FrameResources& frameResources
) {
    if (_renderDelegate)
    {
        return _renderDelegate->render(cmd, _vulkan->currentFrame(), colorImage, depthImage, frameResources);
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
