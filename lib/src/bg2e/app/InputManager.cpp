/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/app/InputManager.hpp>
#include <bg2e/base/PlatformTools.hpp>

#include <SDL2/SDL.h>

#include "imgui.h"

namespace bg2e {
namespace app {

InputManager::MouseButtonsStatus InputManager::getMouseStatus()
{
    MouseButtonsStatus status;
    int x, y;
    Uint32 buttons { SDL_GetMouseState(&x, &y) };
    
    status.left = buttons & SDL_BUTTON_LMASK;
    status.middle = buttons & SDL_BUTTON_MMASK;
    status.rigth = buttons & SDL_BUTTON_RMASK;
    
    return status;
}

glm::vec2 InputManager::normalizedCursorPosition(uint32_t viewportWidth, uint32_t viewportHeight)
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    
    float nx = static_cast<float>(x) / static_cast<float>(viewportWidth);
    float ny = static_cast<float>(y) / static_cast<float>(viewportHeight);
    
    return glm::vec2{ nx * 2.0f - 1.0f, ny * 2.0f - 1.0f };
}

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
        auto factor = base::PlatformTools::currentPlatform() == base::Platform::macOS
            ? 0.7f
            : 1.0f;

        _delegate->mouseWheel(
            static_cast<int>(static_cast<float>(deltaX) * factor),
            static_cast<int>(static_cast<float>(deltaY) * factor)
        );
    }
}


}
}
