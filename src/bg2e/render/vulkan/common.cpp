
#include <bg2e/render/vulkan/common.hpp>

#ifndef _WIN32
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-deprecated-sync"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wunreachable-code-fallthrough"
#endif


#define VMA_IMPLEMENTATION
#ifdef __linux__
#include <vma/vk_mem_alloc.h>
#else
#include "vk_mem_alloc.h"
#endif

#ifndef _WIN32
#pragma clang diagnostic pop
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

