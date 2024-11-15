#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace macros {

extern BG2E_API void cmdSetDefaultViewportAndScissor(VkCommandBuffer cmd, VkExtent2D extent);

}
}
}
}
