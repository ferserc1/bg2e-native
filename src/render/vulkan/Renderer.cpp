
#include <bg2e/render/vulkan/Renderer.hpp>

#include <bg2e/app/Window.hpp>

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <iostream>

namespace bg2e {
namespace render {
namespace vulkan {

Renderer::Renderer()
{
    
}

void Renderer::init(const std::string& appName)
{
    _vulkanApi = std::make_unique<VulkanAPI>();
    _appName = appName;
}

void Renderer::bindWindow(app::Window& window)
{
    // TODO: Enable validation layers only if debug
    bool validationLayers = true;

    _vulkanApi->init(validationLayers, _appName, window);
}

void Renderer::resize(uint32_t w, uint32_t h)
{
    _vulkanApi->setResized();
}

void Renderer::update(float delta)
{
    
}

void Renderer::drawFrame()
{
    int32_t imageIndex = _vulkanApi->beginFrame();
    if (imageIndex >= 0)
    {
        // TODO: Wrap commands in higher level tool to begin render pass and record command buffers
        auto commandBuffer = _vulkanApi->commandBuffer();
        auto scExtent = _vulkanApi->swapchainResources().extent;
        
        commandBuffer.begin(vk::CommandBufferBeginInfo{});
        
        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = _vulkanApi->mainRenderPass();
        renderPassInfo.framebuffer = _vulkanApi->framebuffer(imageIndex);
        renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
        renderPassInfo.renderArea.extent = scExtent;
        
        std::array<vk::ClearValue, 2> clearValues;
        clearValues[0].color = vk::ClearColorValue(_clearColor.r, _clearColor.g, _clearColor.b, 1.0f);
        clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        
        vk::Viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(scExtent.width);
        viewport.height = static_cast<float>(scExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        commandBuffer.setViewport(0, 1, &viewport);
        
        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = scExtent;
        commandBuffer.setScissor(0, 1, &scissor);
        
        // Draw here: pass a command buffer to record
        
        commandBuffer.endRenderPass();
        
        commandBuffer.end();
        
        _vulkanApi->endFrame(imageIndex);
    }
}

void Renderer::destroy()
{
    _vulkanApi->destroy();
}

}
}
}
