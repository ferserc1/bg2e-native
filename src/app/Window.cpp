//
//  Window.cpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 17/2/23.
//

#include <bg2e/app/Window.hpp>
#include <bg2e/app/MouseEvent.hpp>

#include <bg2e/render/vulkan/Renderer.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <iostream>

namespace bg2e {
namespace app {

void resizeCallback(GLFWwindow* glfwWindow, int width, int height)
{
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    
    window->appController()->resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods)
{
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    
    KeyboardEvent event(
        static_cast<KeyCode>(key),
        mods & GLFW_MOD_SHIFT,
        mods & GLFW_MOD_CONTROL,
        mods & GLFW_MOD_ALT,
        mods & GLFW_MOD_CAPS_LOCK,
        mods & GLFW_MOD_NUM_LOCK);
    if (action == GLFW_PRESS)
    {
        window->appController()->keyDown(event);
    }
    else if (action == GLFW_RELEASE)
    {
        window->appController()->keyUp(event);
    }
}

MouseButton getMouseButton(int glfwButton)
{
    switch (glfwButton)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
        return MouseButton::ButtonLeft;
    case GLFW_MOUSE_BUTTON_MIDDLE:
        return MouseButton::ButtonMiddle;
    case GLFW_MOUSE_BUTTON_RIGHT:
        return MouseButton::ButtonRight;
    default:
        return MouseButton::ButtonNone;
    }
}

MouseEvent getMouseEvent(GLFWwindow* glfwWindow, int button = -1, double xOffset = 0.0, double yOffset = 0.0)
{
    double xpos, ypos;
    glfwGetCursorPos(glfwWindow, &xpos, &ypos);
    float deltaX = static_cast<float>(xOffset);
    float deltaY = static_cast<float>(yOffset);
    MouseEvent event({
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS,
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS,
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS,
        static_cast<uint32_t>(xpos),
        static_cast<uint32_t>(ypos),
        deltaX,
        deltaY
    }, getMouseButton(button));
    return event;
}

void cursorPositionCallback(GLFWwindow* glfwWindow, double xpos, double ypos)
{
    if (glfwGetWindowAttrib(glfwWindow, GLFW_HOVERED))
    {
        auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        MouseEvent event = getMouseEvent(glfwWindow);
        window->appController()->mouseMove(event);
        if (event.mouseStatus().leftButton ||
            event.mouseStatus().middleButton ||
            event.mouseStatus().rightButton)
        {
            window->appController()->mouseDrag(event);
        }
    }
}

void mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods)
{
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    auto event = getMouseEvent(glfwWindow, button);
    if (action == GLFW_PRESS)
    {
        window->appController()->mouseDown(event);
    }
    else if (action == GLFW_RELEASE)
    {
        window->appController()->mouseUp(event);
    }
}

void mouseScrollCallback(GLFWwindow* glfwWindow, double xOffset, double yOffset)
{
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    auto event = getMouseEvent(glfwWindow, -1, xOffset, yOffset);
    window->appController()->mouseWheel(event);
}

Window::Window(const std::string & title, uint32_t width, uint32_t height)
    :_title(title)
    ,_size{width,height}
{
    
}

void Window::setRenderer(std::unique_ptr<render::Renderer>&& renderer)
{
    _renderer = std::move(renderer);
}

void Window::setTitle(const std::string & title)
{
    _title = title;
    if (_wnd)
    {
        auto window = reinterpret_cast<GLFWwindow*>(_wnd);
        glfwSetWindowTitle(window, title.c_str());
    }
}

void Window::create()
{
    if (_wnd)
    {
        throw std::runtime_error("Failed to create window: the window is already created");
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(_size.width, _size.height, _title.c_str(), nullptr, nullptr);
    _wnd = window;
    
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, resizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window , mouseScrollCallback);
    
    if (!_renderer.get())
    {
        setRenderer(std::make_unique<render::vulkan::Renderer>());
    }
    
    // TODO: get application name from other parameter than window title
    _renderer->init(_title);
    _renderer->bindWindow(*this);
}

void Window::destroy()
{
    if (_wnd)
    {
        auto window = reinterpret_cast<GLFWwindow*>(_wnd);
        glfwDestroyWindow(window);
    }
    _wnd = nullptr;
}


}
}


