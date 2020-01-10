

#include <vulkan/vulkan.h>

#include <bg2wnd/window.hpp>

#include <ios>
#include <iostream>

namespace bg2wnd {
    
    Window * Window::Create() {
        return new Window();
    }
    
    Window::Window() {
        
    }
    
    Window::~Window() {
    }
    
    bool Window::shouldClose() {
        return true;
    }
    
    void Window::build() {
    }

	void Window::getVulkanRequiredInstanceExtensions(std::vector<const char *>& extensions) {
	}

    VkSurfaceKHR Window::createVulkanSurface(VkInstance instance, VkAllocationCallbacks * allocationCallbacks) {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        return surface;
    }
}
