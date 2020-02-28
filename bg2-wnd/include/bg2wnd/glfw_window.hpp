#ifndef _bg2_wnd_glfw_window_hpp_
#define _bg2_wnd_glfw_window_hpp_

#include <bg2wnd/window.hpp>
#include <bg2base/platform.hpp>
#include <bg2base/export.hpp>

#if BG2_PLATFORM_LINUX
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

namespace bg2wnd {
    
    class GlfwWindow : public Window {
    public:
        virtual ~GlfwWindow();
        
        virtual void build() override;
        
        virtual void close() override;
        
        virtual bool shouldClose() override;
        
        virtual void destroy() override;
        
        bg2base::plain_ptr glfWindowPtr();

        void getVulkanRequiredInstanceExtensions(std::vector<const char *>& extensions);

        VkSurfaceKHR createVulkanSurface(VkInstance instance, VkAllocationCallbacks * allocationCallbacks);
        
    protected:
    
    #if BG2_PLATFORM_LINUX
    
        GLFWwindow * _windowHandler = nullptr;

        virtual void windowPositionWillChange(const bg2math::int2 & newpos);
        virtual void windowPositionDidChange(const bg2math::int2 & newpos);
        virtual void windowSizeWillChange(const bg2math::int2 & newSize);
        virtual void windowSizeDidChange(const bg2math::int2 & newSize);
        virtual void windowTitleWillChange(const std::string & newTitle);
        virtual void windowTitleDidChange(const std::string & title);
        virtual void windowDelegateWillChange(WindowDelegate * del);
        virtual void windowDelegateDidChange(WindowDelegate * del);

        static void windowSizeCallback(GLFWwindow * window, int width, int height);

        inline void updateSize(const bg2math::int2 & s) { _size = s; }

    #endif
    
    };
}

#endif

