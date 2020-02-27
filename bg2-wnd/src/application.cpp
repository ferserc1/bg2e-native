
#include <bg2wnd/application.hpp>

#include <bg2base/platform.hpp>
#include <bg2wnd/win32_application.hpp>
#include <bg2wnd/glfw_application.hpp>

#include <iostream>

namespace bg2wnd {
    
    Application * Application::s_Application = nullptr;
    static bool s_destroyCalled = false;

    Application * Application::Get() {
        if (!s_Application) {
            if (s_destroyCalled) {
                throw std::runtime_error("Attempting to create application singleton after the end of the main event loop.");
            }

    #if BG2_PLATFORM_WINDOWS
            s_Application = new Win32Application();
    #elif BG2_PLATFORM_MACOS
            throw std::runtime_error("This platform is not implemented.");
    #elif BG2_PLATFORM_LINUX
            s_Application = new GlfwApplication();
    #else
            throw std::runtime_error("Unrecognized platform.");
    #endif
    
            s_Application->build();
        }
        return s_Application;
    }

    void Application::Destroy() {
        s_destroyCalled = true;
        delete s_Application;
    }

    Application::Application()
    {

    }

    Application::~Application() {

    }
    
    void Application::addWindow(Window * wnd) {
        wnd->build();
        _windows.push_back(std::shared_ptr<Window>(wnd));
    }

    
}
