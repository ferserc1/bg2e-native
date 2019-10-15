
#ifndef _bg2e_wnd_window_hpp_
#define _bg2e_wnd_window_hpp_

#include <bg2e/wnd/glfw_defines.hpp>

namespace bg2e {
    namespace wnd {

        class Window {
        public:
            Window();

            void create();
            
            inline GLFWwindow * wndHandle() { return _wndHandle; }
            inline const GLFWwindow * wndHandle() const { return _wndHandle; }

        protected:
            GLFWwindow * _wndHandle = nullptr;
        };

    }
}

#endif
