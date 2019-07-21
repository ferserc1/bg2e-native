

#ifndef _bg2_wnd_window_hpp_
#define _bg2_wnd_window_hpp_

#include <bg2math/vector.hpp>
#include <bg2wnd/window_delegate.hpp>

#include <string>
#include <memory>
#include <map>
#include <vector>

#include <vulkan/vulkan.h>

namespace bg2wnd {
    class Application;
    
    class Window {
        friend class Application;
    public:
        static Window * Create();
        
        inline void setSize(const bg2math::int2 & size) { _size = size; }
        inline void setTitle(const std::string & title) { _title = title; }
        inline const bg2math::int2 & size() const { return _size; }
        inline const std::string & title() const { return _title; }
        inline void setWindowDelegate(WindowDelegate * del) { _windowDelegate = std::shared_ptr<WindowDelegate>(del); del->_window = this; }
        inline const WindowDelegate * windowDelegate() const { return _windowDelegate.get(); }
        inline WindowDelegate * windowDelegate() { return _windowDelegate.get(); }
        
        void build();
        
        bool shouldClose();
        
        virtual ~Window();

		void getVulkanRequiredInstanceExtensions(std::vector<const char *>& extensions);
        
        VkSurfaceKHR createVulkanSurface(VkInstance instance, VkAllocationCallbacks * allocationCallbacks = nullptr);

    protected:
        Window();
        
        void * _windowPtr = nullptr;
        bg2math::int2 _size = bg2math::int2(800,600);
        std::string _title = "Window";
        float _lastFrameTime = 0.0f;
        VkSurfaceKHR _surface = VK_NULL_HANDLE;
        
        std::shared_ptr<WindowDelegate> _windowDelegate;
        
        bool updateWindowSize();
        void frame();
    };
}
#endif
