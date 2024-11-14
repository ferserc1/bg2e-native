#include <bg2e/app/InputManager.hpp>


#include "imgui.h"

namespace bg2e {
namespace app {

void InputManager::keyDown(const KeyEvent& event)
{
    ImGuiIO& io = ImGui::GetIO();
    if (_delegate.get() && !io.WantCaptureKeyboard)
    {
        _delegate->keyDown(event);
    }
}

void InputManager::keyUp(const KeyEvent& event)
{
    ImGuiIO& io = ImGui::GetIO();
    if (_delegate.get() && !io.WantCaptureKeyboard)
    {
        _delegate->keyUp(event);
    }
}

void InputManager::mouseMove(int x, int y)
{
	ImGuiIO& io = ImGui::GetIO();
    if (_delegate.get() && !io.WantCaptureMouse)
    {
        _delegate->mouseMove(x, y);
    }
}

void InputManager::mouseButtonDown(int button, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    if (_delegate.get() && !io.WantCaptureMouse)
    {
        _delegate->mouseButtonDown(button, x, y);
    }
}

void InputManager::mouseButtonUp(int button, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    if (_delegate.get() && !io.WantCaptureMouse)
    {
        _delegate->mouseButtonUp(button, x, y);
    }
}

void InputManager::mouseWheel(int deltaX, int deltaY)
{
    ImGuiIO& io = ImGui::GetIO();
    if (_delegate.get() && !io.WantCaptureMouse)
    {
        _delegate->mouseWheel(deltaX, deltaY);
    }
}


}
}
