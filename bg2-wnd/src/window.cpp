
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <bg2wnd/window.hpp>

#include <ios>
#include <iostream>

namespace bg2wnd {

    static std::map<void *, Window *> s_windowMap;
    
    void keyCallback(GLFWwindow * window, int key, int scancode, int action, int mods) {
        if (s_windowMap.find(window) != s_windowMap.end()) {
            auto target = s_windowMap[window];
            if (target->windowDelegate()) {
                switch (action) {
                    case GLFW_PRESS:
                        target->windowDelegate()->keyDown(KeyboardEvent(key,scancode,mods));
                        break;
                    case GLFW_REPEAT:
                        break;
                    case GLFW_RELEASE:
                        target->windowDelegate()->keyUp(KeyboardEvent(key,scancode,mods));
                        break;
                }
                
            }
        }
    }
    
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
    
    void cursorPosCallback(GLFWwindow * window, double xpos, double ypos) {
        if (s_windowMap.find(window) != s_windowMap.end()) {
            auto target = s_windowMap[window];
            if (target->windowDelegate()) {
                target->windowDelegate()->mouseMove(getMouseEvent(window));
            }
        }
    }
    
    void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods) {
        if (s_windowMap.find(window) != s_windowMap.end()) {
            auto target = s_windowMap[window];
            if (target->windowDelegate()) {
                if (action == GLFW_PRESS) {
                    target->windowDelegate()->mouseDown(getMouseEvent(window));
                }
                else if (action == GLFW_RELEASE) {
                    target->windowDelegate()->mouseUp(getMouseEvent(window));
                }
            }
        }
    }
    
    void scrollCallback(GLFWwindow * window, double xOffset, double yOffset) {
        if (s_windowMap.find(window) != s_windowMap.end()) {
            auto target = s_windowMap[window];
            if (target->windowDelegate()) {
                target->windowDelegate()->mouseWheel(getMouseEvent(window,xOffset,yOffset));
            }
        }
    }
    
    
    
    Window * Window::Create() {
        return new Window();
    }
    
    Window::Window() {
        
    }
    
    Window::~Window() {
        if (_windowPtr) {
            GLFWwindow * window = reinterpret_cast<GLFWwindow*>(_windowPtr);
            glfwDestroyWindow(window);
        }
    }
    
    bool Window::shouldClose() {
        if (_windowPtr) {
            bool close = glfwWindowShouldClose(reinterpret_cast<GLFWwindow*>(_windowPtr));
            if (close) {
                // Remove from window map
                auto it = s_windowMap.find(_windowPtr);
                if (it!=s_windowMap.end()) {
                    s_windowMap.erase(it);
                }
            }
            return close;
        }
        return false;
    }
    
    void Window::build() {
        if (!_windowPtr) {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            GLFWwindow * window = glfwCreateWindow(_size.x(), _size.y(), _title.c_str(), nullptr, nullptr);
            if (window) {
                glfwSetKeyCallback(window, keyCallback);
                glfwSetCursorPosCallback(window, cursorPosCallback);
                glfwSetMouseButtonCallback(window, mouseButtonCallback);
                glfwSetScrollCallback(window, scrollCallback);
                _windowPtr = window;
				if (_windowDelegate.get()) {
					_windowDelegate->init();
				}
            }
            else {
                throw std::ios_base::failure("Error building GLFW window.");
            }
            s_windowMap[_windowPtr] = this;
        }
        else {
            std::cout << "Warning: trying to build the window more than one time." << std::endl;
        }
    }
    
    bool Window::updateWindowSize() {
        if (_windowPtr) {
            auto win = reinterpret_cast<GLFWwindow*>(_windowPtr);
            int w, h;
            glfwGetFramebufferSize(win, &w, &h);
            if (_size.x() != w || _size.y() != h) {
                _size.x() = w;
                _size.y() = h;
                if (_windowDelegate.get() != nullptr) {
                    _windowDelegate->resize(_size);
                }
                return true;
            }
        }
        return false;
    }
    
    void Window::frame() {
        if (_windowDelegate.get() != nullptr) {
            float elapsed = static_cast<float>(glfwGetTime());
            float delta = (elapsed - _lastFrameTime) * 1000.0f;  // Elapsed time, converted to milliseconds
            _lastFrameTime = elapsed;
            _windowDelegate->update(delta);
            _windowDelegate->draw();
        }
    }

	void Window::getVulkanRequiredInstanceExtensions(std::vector<const char *>& extensions) {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
			extensions.push_back(glfwExtensions[i]);
		}
	}
}
