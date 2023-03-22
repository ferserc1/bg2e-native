
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/Window.hpp>
#include <bg2e/app/AppController.hpp>

#include <iostream>
#include <chrono>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace bg2e {
namespace app {

MainLoop::MainLoop(std::unique_ptr<Window> window)
:_window{ std::move(window) }
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
    
    _window->appController()->init();
    
    float delta = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        glfwPollEvents();
        _window->appController()->frame(delta);
        _window->appController()->display();
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = endTime - startTime;
        delta = elapsed.count() * std::chrono::seconds::period::num / std::chrono::seconds::period::den;
    }
    
    _window->appController()->destroy();
    _window->destroy();
    
    glfwTerminate();
    
    return 0;
}


}
}
