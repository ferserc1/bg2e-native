
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
        if (_windowPtr) {
            GLFWwindow * window = reinterpret_cast<GLFWwindow*>(_windowPtr);
            glfwDestroyWindow(window);
        }
    }
    
    bool Window::shouldClose() {
        if (_windowPtr) {
            return glfwWindowShouldClose(reinterpret_cast<GLFWwindow*>(_windowPtr));
        }
        return false;
    }
    
    void Window::build() {
        if (!_windowPtr) {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            GLFWwindow * window = glfwCreateWindow(_size.x(), _size.y(), _title.c_str(), nullptr, nullptr);
            if (window) {
                _windowPtr = window;
            }
            else {
                throw std::ios_base::failure("Error building GLFW window.");
            }
        }
        else {
            std::cout << "Warning: trying to build the window more than one time." << std::endl;
        }
    }
}
