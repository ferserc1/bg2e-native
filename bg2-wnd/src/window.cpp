

#include <vulkan/vulkan.h>

#include <bg2base/platform.hpp>

#include <bg2wnd/window.hpp>

#include <bg2wnd/win32_window.hpp>

#include <ios>
#include <iostream>

namespace bg2wnd {
    
    Window * Window::Create() {
#if BG2_PLATFORM_WINDOWS
        return new Win32Window();
#elif BG2_PLATFORM_MACOS
        return nullptr;
#elif BG2_PLATFORM_LINUX
        return nullptr;
#else
        return nullptr;
#endif
    }
    
    Window::Window() {
        
    }
    
    Window::~Window() {
    }
    
	void Window::getVulkanRequiredInstanceExtensions(std::vector<const char *>& extensions) {
	}

    VkSurfaceKHR Window::createVulkanSurface(VkInstance instance, VkAllocationCallbacks * allocationCallbacks) {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        return surface;
    }
}
