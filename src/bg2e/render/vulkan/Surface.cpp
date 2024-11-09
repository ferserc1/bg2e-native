
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Instance.hpp>
#include <bg2e/render/vulkan/Surface.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

namespace bg2e {
namespace render {
namespace vulkan {

void Surface::create(const Instance& instance, SDL_Window* window)
{
    _window = window;
	_instance = instance.handle();
    SDL_Vulkan_CreateSurface(window, instance.handle(), &_surface);
}

void Surface::cleanup()
{
    if (_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        _surface = VK_NULL_HANDLE;
    }
}

VkExtent2D Surface::getExtent() const
{
    int width, height;
    SDL_GetWindowSize(_window, &width, &height);
    return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

}
}
}

