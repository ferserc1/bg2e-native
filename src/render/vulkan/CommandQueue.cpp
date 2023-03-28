
#include <bg2e/render/vulkan/CommandQueue.hpp>

#include <bg2e/render/CommandQueue.hpp>
#include <bg2e/render/vulkan/Renderer.hpp>

#include <vulkan/vulkan.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

void CommandQueue::beginFrame(const glm::vec4& color)
{
    auto renderer = this->renderer<vulkan::Renderer>();
    auto vulkanApi = renderer->vulkanApi();
    
    auto scExtent = vulkanApi->swapchainResources().extent;
    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.renderPass = vulkanApi->mainRenderPass();
    renderPassInfo.framebuffer = vulkanApi->framebuffer();
    renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
    renderPassInfo.renderArea.extent = scExtent;
    
    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color = vk::ClearColorValue(color.r, color.g, color.b, color.a);
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    renderer->vulkanApi()->commandBuffer().beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void CommandQueue::endFrame()
{
    auto renderer = this->renderer<vulkan::Renderer>();
    
    renderer->vulkanApi()->commandBuffer().endRenderPass();
}

void CommandQueue::setViewport(const Viewport& viewport)
{
    auto renderer = this->renderer<vulkan::Renderer>();
    
    vk::Viewport vp;
    vp.x = static_cast<float>(viewport.x);
    vp.y = static_cast<float>(viewport.y);
    vp.width = static_cast<float>(viewport.width);
    vp.height = static_cast<float>(viewport.height);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    
    renderer->vulkanApi()->commandBuffer().setViewport(0, 1, &vp);
}

void CommandQueue::setScissor(int32_t x, int32_t y, const Size& size)
{
    auto renderer = this->renderer<vulkan::Renderer>();
    
    vk::Rect2D scissor{};
    scissor.offset.x = x;
    scissor.offset.y = y;
    scissor.extent.width = size.width;
    scissor.extent.height = size.height;
    renderer->vulkanApi()->commandBuffer().setScissor(0, 1, &scissor);
}

}
}
}
