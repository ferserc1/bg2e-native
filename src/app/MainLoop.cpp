
#include <bg2e/app/MainLoop.hpp>

#include <iostream>

#include <GLFW/glfw3.h>

namespace bg2e {
namespace app {

MainLoop::MainLoop(Window* window)
    :_window(window)
{
    
}

int MainLoop::run()
{
    glfwInit();
    
    if (!_window->created())
    {
        _window->create();
    }
    
    auto window = reinterpret_cast<GLFWwindow*>(_window->impl_ptr());
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
    
    _window->destroy();
    
    glfwTerminate();
    
    return 0;
}


}
}
