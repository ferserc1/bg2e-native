
#include <bg2wnd/application.hpp>

#include <bg2base/platform.hpp>
#include <bg2wnd/win32_application.hpp>

#include <iostream>

namespace bg2wnd {
    
    Application * Application::Create() {
#if BG2_PLATFORM_WINDOWS
        return new Win32Application();
#elif BG2_PLATFORM_MACOS
        return nullptr;
#elif BG2_PLATFORM_LINUX
        return nullptr;
#else
        return nullptr;
#endif
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
