//
//  Window.cpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 17/2/23.
//

#include <bg2e/app/Window.hpp>
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace bg2e {
namespace app {

Window::Window(const std::string & title, uint32_t width, uint32_t height)
    :_title(title)
    ,_width(width)
    ,_height(height)
{
    
}

void Window::create()
{
    if (_wnd)
    {
        throw std::runtime_error("Failed to create window: the window is already created");
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(_width, _height, _title.c_str(), nullptr, nullptr);
    _wnd = window;
}

void Window::destroy()
{
    if (_wnd)
    {
        auto window = reinterpret_cast<GLFWwindow*>(_wnd);
        glfwDestroyWindow(window);
    }
    _wnd = nullptr;
}


}
}


