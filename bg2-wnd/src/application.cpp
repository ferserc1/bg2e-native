
#include <bg2wnd/application.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

namespace bg2wnd {
    
    Application::Application()
    {
        glfwInit();
    }
    
    void Application::addWindow(Window * wnd) {
        wnd->build();
        _windows.push_back(std::shared_ptr<Window>(wnd));
    }

    int Application::run() {        
        while (_windows.size() > 0) {
            auto closingWindowIt = _windows.end();
            for (auto it = _windows.begin(); it != _windows.end(); ++it) {
                if ((*it)->shouldClose()) {
                    closingWindowIt = it;
                    break;
                }
            }
            if (closingWindowIt != _windows.end()) {
                _windows.erase(closingWindowIt);
            }
            glfwPollEvents();
        }
        
        glfwTerminate();
        return 0;
    }
    
}
