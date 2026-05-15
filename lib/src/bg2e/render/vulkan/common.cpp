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


#if BG2E_DEBUG_LOG_VMA_ALLOCATIONS == 1
#define VMA_DEBUG_LOG_FORMAT(format, ...) \
                        do { \
                            printf(format "\n", __VA_ARGS__); \
                        } while(false)
#endif

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

