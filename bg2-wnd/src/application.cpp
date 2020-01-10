
#include <bg2wnd/application.hpp>


#include <iostream>

namespace bg2wnd {
    
    Application::Application()
    {

    }
    
    void Application::addWindow(Window * wnd) {
        wnd->build();
        _windows.push_back(std::shared_ptr<Window>(wnd));
    }

    int Application::run() {        

        return 0;
    }
    
}
