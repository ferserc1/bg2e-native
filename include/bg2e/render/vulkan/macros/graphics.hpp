#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Image.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace macros {

extern BG2E_API void cmdSetDefaultViewportAndScissor(VkCommandBuffer cmd, VkExtent2D extent);

extern BG2E_API void cmdClearImageAndBeginRendering(
    VkCommandBuffer cmd,
    const Image * colorImage,
    VkClearColorValue clearValue,
    VkImageLayout colorImageInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    const Image * depthImage = nullptr,
    float depthValue = 1.0f
);

}
}
}
}
