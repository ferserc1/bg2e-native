
#include <bg2wnd/glfw_application.hpp>
#include <bg2wnd/glfw_window.hpp>

#include <iostream>

#if BG2_PLATFORM_LINUX

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace bg2wnd {

    void GlfwApplication::build() {
        std::cout << "Initializing GLFW..." << std::endl;
        glfwInit();
        
    }

    int GlfwApplication::run() {
        int exitCode = 0;
        while (_windows.size() > 0) {
            glfwPollEvents();
            std::vector<std::shared_ptr<Window>>::iterator closingWindow = _windows.end(), w;
            for (w = _windows.begin(); w != _windows.end(); ++w) {
                if ((*w)->shouldClose()) {
                    closingWindow = w;
                    break;
                }
                else if ((*w)->windowDelegate()) {
                    (*w)->setT1Clock(std::clock());
                    (*w)->windowDelegate()->update((*w)->deltaTime());
                    (*w)->windowDelegate()->draw();
                    (*w)->setT0Clock(std::clock());
                }
            }
            
            if (closingWindow != _windows.end()) {
                (*closingWindow)->destroy();
                _windows.erase(closingWindow);
            }
        }
        
        glfwTerminate();
        
        Destroy();
        
        return exitCode;
    }

    Window * GlfwApplication::getWindow(bg2base::plain_ptr nativeWindowHandler) {
        for (auto w : _windows) {
            GlfwWindow * window = dynamic_cast<GlfwWindow*>(w.get());
            if (window && window->glfWindowPtr() == nativeWindowHandler) {
                return w.get();
            }
        }
        return nullptr;
    }

}   

#else

namespace bg2wnd {
    void GlfwApplication::build() {}        
    int GlfwApplication::run() { return 0; }
    Window * GlfwApplication::getWindow(bg2base::plain_ptr nativeWindowHandler) { return nullptr; }
}        
#endif

