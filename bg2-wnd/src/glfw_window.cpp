
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

    MouseEvent getMouseEvent(GLFWwindow * window, const MouseButton & mouseButton, double scrollx=0.0, double scrolly=0.0) {
        int b1 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
        int b2 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2);
        int b3 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3);
        int b4 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_4);
        int b5 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_5);
        double cx, cy;
        glfwGetCursorPos(window, &cx, &cy);
        return MouseEvent(static_cast<int>(cx), static_cast<int>(cy), mouseButton, b1, b2, b3, b4, b5, static_cast<float>(scrollx), static_cast<float>(scrolly));
    }

    void cursorPosCallback(GLFWwindow * window, double xpos, double ypos) {
        bg2wnd::Application * app = bg2wnd::Application::Get();
        auto win = app->getWindow(window);
        if (win && win->windowDelegate()) {
            win->windowDelegate()->mouseMove(getMouseEvent(window, MouseButton::ButtonNone));
        }
    }

    void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods) {
        bg2wnd::Application * app = bg2wnd::Application::Get();
        auto win = app->getWindow(window);
        if (win && win->windowDelegate()) {
            MouseButton mouseButton = static_cast<MouseButton>(button + 1);
            if (action == GLFW_PRESS) {
                win->windowDelegate()->mouseUp(getMouseEvent(window, mouseButton));
            }
            else if (action == GLFW_RELEASE) {
                win->windowDelegate()->mouseUp(getMouseEvent(window, mouseButton));
            }
        }
    }

    void scrollCallback(GLFWwindow * window, double xOffset, double yOffset) {
        bg2wnd::Application * app = bg2wnd::Application::Get();
        auto win = app->getWindow(window);
        if (win && win->windowDelegate()) {
            win->windowDelegate()->mouseWheel(getMouseEvent(window, MouseButton::ButtonNone, xOffset, yOffset));
        }
    }

    GlfwWindow::~GlfwWindow() {
        
    }
        
    void GlfwWindow::build() {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        _windowHandler = glfwCreateWindow(_size.x(), _size.y(), _title.c_str(), nullptr, nullptr);
        glfwSetKeyCallback(_windowHandler, key_callback);
        glfwSetCursorPosCallback(_windowHandler, cursorPosCallback);
        glfwSetMouseButtonCallback(_windowHandler, mouseButtonCallback);
        glfwSetScrollCallback(_windowHandler, scrollCallback);

        if (windowDelegate()) {
			windowDelegate()->init();
			windowDelegate()->resize(_size);
		}
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

    void GlfwWindow::getVulkanRequiredInstanceExtensions(std::vector<const char*>& extensions) {
        uint32_t count;
        const char ** ext = glfwGetRequiredInstanceExtensions(&count);
        for (uint32_t i = 0; i < count; ++i) {
            extensions.push_back(ext[i]);
        }
	}

	VkSurfaceKHR GlfwWindow::createVulkanSurface(VkInstance instance, VkAllocationCallbacks* allocationCallbacks) {
		VkSurfaceKHR surface;
        VkResult err = glfwCreateWindowSurface(instance, _windowHandler, nullptr, &surface);
        if (err != VK_SUCCESS) {
            throw std::runtime_error("Unexpected error creating vulkan surface for window '" + _title + "'");
        }
		return surface;
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

