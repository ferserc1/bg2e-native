

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
        
        inline void setPosition(const bg2math::int2 & pos) {
            windowPositionWillChange(pos);
            _position = pos;
            windowPositionDidChange(pos);
        }
        inline void setSize(const bg2math::int2 & size) {
            windowSizeWillChange(size);
            _size = size;
            windowSizeDidChange(size);
        }
        inline void setTitle(const std::string & title) {
            windowTitleWillChange(title);
            _title = title;
            windowTitleDidChange(title);
        }
        inline void setWindowDelegate(WindowDelegate * del) {
            windowDelegateWillChange(del);
            _windowDelegate = std::shared_ptr<WindowDelegate>(del);
            del->_window = this;
            windowDelegateDidChange(del);
        }
        
        inline const bg2math::int2 & size() const { return _size; }
        inline const std::string & title() const { return _title; }
        inline const WindowDelegate * windowDelegate() const { return _windowDelegate.get(); }
        inline WindowDelegate * windowDelegate() { return _windowDelegate.get(); }
        
        virtual void build() = 0;
        
        virtual bool shouldClose() = 0;
        
        virtual ~Window();

		virtual void getVulkanRequiredInstanceExtensions(std::vector<const char *>& extensions);
        
        virtual VkSurfaceKHR createVulkanSurface(VkInstance instance, VkAllocationCallbacks * allocationCallbacks = nullptr);

    protected:
        Window();
        
        bg2math::int2 _position = bg2math::int2(100, 100);
        bg2math::int2 _size = bg2math::int2(800,600);
        std::string _title = "Window";
        float _lastFrameTime = 0.0f;
        VkSurfaceKHR _surface = VK_NULL_HANDLE;
        
        std::shared_ptr<WindowDelegate> _windowDelegate;

        virtual void windowPositionWillChange(const bg2math::int2 & newpos) {}
        virtual void windowPositionDidChange(const bg2math::int2 & newpos) {}
        virtual void windowSizeWillChange(const bg2math::int2 & newSize) {}
        virtual void windowSizeDidChange(const bg2math::int2 & newSize) {}
        virtual void windowTitleWillChange(const std::string & newTitle) {}
        virtual void windowTitleDidChange(const std::string & title) {}
        virtual void windowDelegateWillChange(WindowDelegate * del) {}
        virtual void windowDelegateDidChange(WindowDelegate * del) {}
    };
}
#endif
