
#include <bg2e/render/vulkan/common.hpp>
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
#elif defined(BG2E_IS_LINUX)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif


#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#ifdef BG2E_IS_MAC
#pragma clang diagnostic pop
#elif defined(BG2E_IS_LINUX)
#pragma GCC diagnostic pop
#endif 


namespace bg2e {
namespace render {
namespace vulkan {

void* getMappedData(VmaAllocation a)
{
    return a->GetMappedData();
}

}
}
}

