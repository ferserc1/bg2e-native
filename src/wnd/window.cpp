
#include <bg2e/wnd/window.hpp>

namespace bg2e {
    namespace wnd {
        
        Window::Window()
        {

        }

        void Window::create() {
            _wndHandle = glfwCreateWindow(1024, 720, "helloworld", nullptr, nullptr);
        }
    }
}

