
#include <bg2e/render/vulkan/macros/graphics.hpp>

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

}
