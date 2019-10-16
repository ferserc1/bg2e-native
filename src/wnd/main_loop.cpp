
#include <bg2e/wnd/main_loop.hpp>

#include <bg2e/wnd/glfw_defines.hpp>

#include <bx/bx.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <stdexcept>


namespace bg2e {
    namespace wnd {

        MainLoop::MainLoop() {
            // GLFW initialization
            if (!glfwInit()) {
                throw std::runtime_error("Window system initialization error.");
            }
        }
    
        MainLoop::~MainLoop() {
            for (auto * wnd : _windows) {
                delete wnd;
            }
            _windows.clear();
        }
    
        void MainLoop::registerWindow(Window * wnd) {
            _windows.push_back(wnd);
        }
    
        int MainLoop::run() {
            bool running = true;
            
            while (running) {
                glfwPollEvents();
                
                Window * windowToClose = nullptr;
                for (auto * wnd : _windows) {
                    if (wnd->shouldClose()) {
                        windowToClose = wnd;
                        break;
                    }
                    
                    wnd->updateWindowData();
                    wnd->frame();                
                }
                
                if (windowToClose) {
                    auto it = std::find(_windows.begin(), _windows.end(), windowToClose);
                    if (it != _windows.end()) {
                        _windows.erase(it);
                        delete windowToClose;
                    }
                    
                    running = _windows.size() > 0;
                }
            }
            
            bgfx::shutdown();
            glfwTerminate();
            return 0;
        }

    }
}
