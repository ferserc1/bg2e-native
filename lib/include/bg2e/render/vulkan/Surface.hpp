#pragma once

#include <bg2e/common.hpp>
#include <bg2e/base/PlatformTools.hpp>  
#include <bg2e/render/vulkan/common.hpp>
#include <SDL2/SDL.h>

namespace bg2e {
namespace render {
namespace vulkan {

class Instance;

class BG2E_API Surface {
public:
    void create(const Instance& instance, SDL_Window* window);
    void cleanup();

    VkExtent2D getExtent() const;

    inline VkSurfaceKHR handle() const { return _surface; }
    inline bool isValid() { return _surface != VK_NULL_HANDLE; }

protected:
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkInstance _instance = VK_NULL_HANDLE;
    SDL_Window* _window = nullptr;
};

}
}
}
