

#include <vulkan/vulkan.h>

#include <bg2base/platform.hpp>

#include <bg2wnd/window.hpp>

#include <bg2wnd/win32_window.hpp>
#include <bg2wnd/glfw_window.hpp>

#include <ios>
#include <iostream>

namespace bg2wnd {
    
    Window * Window::New() {
#if BG2_PLATFORM_WINDOWS
        return new Win32Window();
#elif BG2_PLATFORM_MACOS
        return nullptr;
#elif BG2_PLATFORM_LINUX
        return new GlfwWindow();
#else
        return nullptr;
#endif
    }
    
    Window::Window()
        :_t0Clock(std::clock())
        ,_t1Clock(std::clock())
    {
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
