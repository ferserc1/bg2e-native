
#include <bg2wnd/glfw_window.hpp>
#include <bg2wnd/application.hpp>

#include <iostream>

#if BG2_PLATFORM_LINUX


namespace bg2wnd {
    static void key_callback(GLFWwindow * window, int key, int scancode, int action, int mods) {
        // TODO: Implement key events
        bg2wnd::Application * app = bg2wnd::Application::Get();
        auto win = app->getWindow(window);
        if (win && win->windowDelegate()) {
            bg2wnd::KeyboardEvent evt(KeyCode::KeyESCAPE, "", false, false, false, false);
            if (action == GLFW_PRESS) {
                win->windowDelegate()->keyDown(evt);
            }
            else if (action == GLFW_RELEASE) {
                win->windowDelegate()->keyUp(evt);
            }        
        }
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            std::cout << "Escape" << std::endl;
        }
    }

    GlfwWindow::~GlfwWindow() {
        
    }
        
    void GlfwWindow::build() {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        _windowHandler = glfwCreateWindow(_size.x(), _size.y(), _title.c_str(), nullptr, nullptr);
        glfwSetKeyCallback(_windowHandler, key_callback);
    }
        
    void GlfwWindow::close() {
        glfwSetWindowShouldClose(_windowHandler, GLFW_TRUE);
    }
        
    bool GlfwWindow::shouldClose() {
        return glfwWindowShouldClose(bg2base::native_cast<GLFWwindow*>(_windowHandler));
    }
        
    void GlfwWindow::destroy() {
        glfwDestroyWindow(_windowHandler);
    }
        
    bg2base::plain_ptr GlfwWindow::glfWindowPtr() {
        return _windowHandler;
    }
}

#else

namespace bg2wnd {
    
    GlfwWindow::~GlfwWindow() {}
    void GlfwWindow::build() {}
    void GlfwWindow::close() {}
    bool GlfwWindow::shouldClose() { return true; }
    void GlfwWindow::destroy() {}
    bg2base::plain_ptr glfWindowPtr() { return nullptr; }
        
}

#endif

