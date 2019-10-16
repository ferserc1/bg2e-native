
#ifndef _bg2e_wnd_window_hpp_
#define _bg2e_wnd_window_hpp_

#include <bg2e/wnd/glfw_defines.hpp>
#include <bg2e/wnd/event_handler.hpp>
#include <bgfx/bgfx.h>

#include <string>
#include <map>

namespace bg2e {
    namespace wnd {

        class Window {
        public:
            Window();
            virtual ~Window();

            void create(uint32_t width, uint32_t height, const std::string& title);
            
            void registerEventHandler(EventHandler * eh);
        
            EventHandler * unregisterEventHandler();
            
            inline GLFWwindow * wndHandle() { return _wndHandle; }
            inline const GLFWwindow * wndHandle() const { return _wndHandle; }

            inline uint32_t width() const { return _width; }
            inline uint32_t height() const { return _height; }
            inline const std::string & title() const { return _title; }
            inline bgfx::ViewId viewId() const { return _viewId; }
            
            bool shouldClose();
            void updateWindowData();
            void frame();
            
        protected:
            GLFWwindow * _wndHandle = nullptr;
            
            uint32_t _width;
            uint32_t _height;
            std::string _title;
            bgfx::ViewId _viewId;
            float _lastFrameTime;
            
            EventHandler * _eventHandler = nullptr;
            
            static bgfx::ViewId s_viewIdCounter;
            
            static bgfx::ViewId GenerateViewId() {
                return s_viewIdCounter++;
            }
            
            static void GlfwKeyCallback(GLFWwindow * wndHandle, int key, int scancode, int action, int mods);
            static void GlfwMouseCursorPosCallback(GLFWwindow * window, double xpos, double ypos);
            static void GlfwMouseButtonCallback(GLFWwindow * window, int button, int action, int mods);
            static void GlfwMouseScrollCallback(GLFWwindow * window, double xOffset, double yOffset);
            static std::map<GLFWwindow *, Window *> s_windowHandleMap;
        };

    }
}

#endif
