
#include <bg2e/wnd/window.hpp>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <stdexcept>

namespace bg2e {
    namespace wnd {
        
        bgfx::ViewId Window::s_viewIdCounter = 0;
    
        std::map<GLFWwindow *, Window *> Window::s_windowHandleMap;
    
        MouseEvent getMouseEvent(GLFWwindow * window, double scrollx=0.0, double scrolly=0.0) {
            int b1 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
            int b2 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2);
            int b3 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3);
            int b4 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_4);
            int b5 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_5);
            double cx, cy;
            glfwGetCursorPos(window, &cx, &cy);
            return MouseEvent(static_cast<int>(cx), static_cast<int>(cy), b1, b2, b3, b4, b5, static_cast<float>(scrollx), static_cast<float>(scrolly));
        }
    
        void Window::GlfwKeyCallback(GLFWwindow *wndHandle, int key, int scancode, int action, int mods) {
            if (s_windowHandleMap.find(wndHandle) != s_windowHandleMap.end()) {
                auto win = s_windowHandleMap[wndHandle];
                
                if (win->_eventHandler) {
                    KeyboardEvent evt(key,scancode,mods);
                    
                    if (action == GLFW_RELEASE) {
                        win->_eventHandler->keyUp(evt);
                    }
                    else if (action == GLFW_PRESS) {
                        win->_eventHandler->keyDown(evt);
                    }
                    else if (action == GLFW_REPEAT) {
                        win->_eventHandler->keyRepeat(evt);
                    }
                }
            }
        }
    
        void Window::GlfwMouseCursorPosCallback(GLFWwindow * wndHandle, double xpos, double ypos) {
            if (s_windowHandleMap.find(wndHandle) != s_windowHandleMap.end()) {
                auto win = s_windowHandleMap[wndHandle];
                
                if (win->_eventHandler) {
                    auto evt = getMouseEvent(wndHandle);
                    win->_eventHandler->mouseMove(evt);
                    
                    if (evt.anyButtonPressed()) {
                        win->_eventHandler->mouseDrag(evt);
                    }
                }
            }
        }
        
        void Window::GlfwMouseButtonCallback(GLFWwindow * wndHandle, int button, int action, int mods) {
            if (s_windowHandleMap.find(wndHandle) != s_windowHandleMap.end()) {
                auto win = s_windowHandleMap[wndHandle];
                
                if (win->_eventHandler) {
                    auto evt = getMouseEvent(wndHandle);
                    if (action == GLFW_PRESS) {
                        win->_eventHandler->mouseDown(evt);
                    }
                    else if (action == GLFW_RELEASE){
                        win->_eventHandler->mouseUp(evt);
                    }
                }
            }
        }
            
        void Window::GlfwMouseScrollCallback(GLFWwindow * wndHandle, double xOffset, double yOffset) {
            if (s_windowHandleMap.find(wndHandle) != s_windowHandleMap.end()) {
                auto win = s_windowHandleMap[wndHandle];
                
                if (win->_eventHandler) {
                    win->_eventHandler->mouseWheel(getMouseEvent(wndHandle,xOffset,yOffset));
                }
            }
        }
        
    
        Window::Window()
        {
            
        }
    
        Window::~Window() {
            if (_eventHandler != nullptr) {
                _eventHandler->destroy();
                delete _eventHandler;
            }
            
            // TODO: destroy glfw window handler
        
            auto handleIt = s_windowHandleMap.end();
            if (_wndHandle && (handleIt = s_windowHandleMap.find(_wndHandle)) != s_windowHandleMap.end()) {
                s_windowHandleMap.erase(handleIt);
            }
        }

        void Window::create(uint32_t width, uint32_t height, const std::string& title) {
            _width = width;
            _height = height;
            _title = title;
            
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            _wndHandle = glfwCreateWindow(_width, _height, _title.c_str(), nullptr, nullptr);
            s_windowHandleMap[_wndHandle] = this;
            
            if (!_wndHandle) {
                throw std::runtime_error("Fatal error creating window.");
            }
            
            // Input event callbacks
            glfwSetKeyCallback(_wndHandle, Window::GlfwKeyCallback);
            glfwSetCursorPosCallback(_wndHandle, Window::GlfwMouseCursorPosCallback);
            glfwSetMouseButtonCallback(_wndHandle, Window::GlfwMouseButtonCallback);
            glfwSetScrollCallback(_wndHandle, Window::GlfwMouseScrollCallback);
            
            // TODO: game device


            // Most graphics APIs must be used on the same thread that created the window.
            bgfx::renderFrame();
            
            bgfx::Init init;
            #if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
                init.platformData.ndt = glfwGetX11Display();
                init.platformData.nwh = (void*)(uintptr_t)glfwGetX11Window(window);
            #elif BX_PLATFORM_OSX
                init.platformData.nwh = glfwGetCocoaWindow(_wndHandle);
            #elif BX_PLATFORM_WINDOWS
                init.platformData.nwh = glfwGetWin32Window(window);
            #endif
            init.resolution.width = _width;
            init.resolution.height = _height;
            init.resolution.reset = BGFX_RESET_VSYNC;
            if (!bgfx::init(init)) {
                throw std::runtime_error("Fatal error in graphic API initialization.");
            }
            
            _viewId = GenerateViewId();
            bgfx::setViewClear(_viewId, BGFX_CLEAR_COLOR);
            bgfx::setViewRect(_viewId, 0, 0, bgfx::BackbufferRatio::Equal);
            
            _lastFrameTime = glfwGetTime();
            
            if (_eventHandler) {
                _eventHandler->init();
                _eventHandler->resize(_width, _height);
            }
        }
    
    
        void Window::registerEventHandler(EventHandler * eh) {
            if (_eventHandler != nullptr) {
                _eventHandler->destroy();
                delete _eventHandler;
            }
            _eventHandler = eh;
            
            // The create() method has been called before registerEventHandler(): call init callback
            if (_wndHandle != nullptr && _eventHandler) {
                _eventHandler->init();
                _eventHandler->resize(_width, _height);
            }
        }
        
        EventHandler * Window::unregisterEventHandler() {
            auto tmp = _eventHandler;
            _eventHandler = nullptr;
            return tmp;
        }
    
        bool Window::shouldClose() {
            return glfwWindowShouldClose(_wndHandle);
        }
    
        void Window::updateWindowData() {
            int oldWidth = _width;
            int oldHeight = _height;
            int newWidth, newHeight;
            glfwGetWindowSize(_wndHandle, &newWidth, &newHeight);
            if (newWidth != oldWidth || newHeight != oldHeight) {
                _width = static_cast<uint32_t>(newWidth);
                _height = static_cast<uint32_t>(newHeight);
                
                bgfx::reset(_width, _height, BGFX_RESET_VSYNC);
                bgfx::setViewRect(_viewId, 0, 0, bgfx::BackbufferRatio::Equal);
                
                if (_eventHandler) {
                    _eventHandler->resize(_width, _height);
                }
            }
        }
    
        void Window::frame() {
            // This dummy draw call is here to make sure that view is cleared if no other draw call are submitted
            bgfx::touch(_viewId);
            
            if (_eventHandler) {
                float elapsed = static_cast<float>(glfwGetTime());
                float delta = (elapsed - _lastFrameTime) * 1000.0f;
                _lastFrameTime = elapsed;
                _eventHandler->update(delta);
                _eventHandler->draw();
            }
            
            bgfx::frame();
        }
    
    }
}

