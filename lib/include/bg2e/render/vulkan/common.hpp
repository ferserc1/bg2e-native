#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <bg2e/base/PlatformTools.hpp>

#ifdef BG2E_IS_MAC
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-deprecated-sync"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wunreachable-code-fallthrough"
#endif

#ifdef BG2E_IS_LINUX
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#include <vma/vk_mem_alloc.h>

#ifdef BG2E_IS_MAC
#pragma clang diagnostic pop
#endif
#ifdef BG2E_IS_LINUX
#pragma GCC diagnostic pop
#endif

namespace bg2e {
namespace render {
namespace vulkan {

void* getMappedData(VmaAllocation a);

}
}
}

#include <iostream>
#include <stdexcept>

#include <bg2e/math/base.hpp>

#include <bg2e/render/vulkan/extensions.hpp>

#define VK_ASSERT(x)                                                                         \
    do {                                                                                     \
        VkResult err = x;                                                                    \
        if (err) {                                                                           \
            std::string errorString = std::string("Vulkan error: ") + string_VkResult(err);  \
            std::cerr << errorString << std::endl;                                           \
            throw std::runtime_error(errorString);                                           \
        }                                                                                    \
    } while(0)
