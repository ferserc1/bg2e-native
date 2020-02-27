
#include <bg2wnd/glfw_window.hpp>
#include <bg2wnd/application.hpp>

#include <iostream>

#if BG2_PLATFORM_LINUX


namespace bg2wnd {

    bg2wnd::KeyboardEvent getKeyboardEvent(int key, int scancode, int mods) {
        const char * byteCharacter = glfwGetKeyName(key, scancode);
        std::string character = byteCharacter ? byteCharacter : "";
        return bg2wnd::KeyboardEvent(
            static_cast<KeyCode>(key),
            character,
            mods & GLFW_MOD_SHIFT,
            mods & GLFW_MOD_CONTROL,
            mods & GLFW_MOD_ALT,
            mods & GLFW_MOD_CAPS_LOCK);
    }

    static void key_callback(GLFWwindow * window, int key, int scancode, int action, int mods) {
        bg2wnd::Application * app = bg2wnd::Application::Get();
        auto win = app->getWindow(window);
        if (win && win->windowDelegate()) {
            if (action == GLFW_PRESS) {
                win->windowDelegate()->keyDown(getKeyboardEvent(key,scancode,mods));
            }
            else if (action == GLFW_RELEASE) {
                win->windowDelegate()->keyUp(getKeyboardEvent(key,scancode,mods));
            }        
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

