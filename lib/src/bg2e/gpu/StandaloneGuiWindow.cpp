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

#include <bg2e/gpu/StandaloneGuiWindow.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <stdexcept>
#include <string>

namespace bg2e::gpu {
namespace {
constexpr uint32_t subsystems = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS;

void requireSDL(bool success, const char* operation)
{
    if (!success)
    {
        throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
    }
}
}

struct StandaloneGuiWindow::Impl {
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;
    SDL_Window* previousWindow = SDL_GL_GetCurrentWindow();
    SDL_GLContext previousContext = SDL_GL_GetCurrentContext();
    bool sdlStarted = false;
    bool platformStarted = false;
    bool rendererStarted = false;

    ~Impl()
    {
        if (context)
        {
            SDL_GL_MakeCurrent(window, context);
        }
        if (rendererStarted)
        {
            ImGui_ImplOpenGL3_Shutdown();
        }
        if (platformStarted)
        {
            ImGui_ImplSDL2_Shutdown();
        }
        if (context)
        {
            SDL_GL_DeleteContext(context);
        }
        if (window)
        {
            SDL_DestroyWindow(window);
        }
        if (previousContext)
        {
            SDL_GL_MakeCurrent(previousWindow, previousContext);
        }
        if (sdlStarted)
        {
            SDL_QuitSubSystem(subsystems);
        }
    }
};

StandaloneGuiWindow::StandaloneGuiWindow(const char* title, Size2D size)
    : _impl(std::make_unique<Impl>())
{
    if (!ImGui::GetCurrentContext() || size.isZero())
    {
        throw std::invalid_argument("Standalone GUI requires an ImGui context and a nonzero size.");
    }
    requireSDL(SDL_InitSubSystem(subsystems) == 0, "Initialize standalone SDL");
    _impl->sdlStarted = true;
#ifdef BG2E_IS_MAC
    const char* version = "#version 150";
    requireSDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG) == 0, "Set GL context flags");
    requireSDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) == 0, "Set GL major version");
    requireSDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2) == 0, "Set GL minor version");
    requireSDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) == 0, "Set GL core profile");
#else
    const char* version = "#version 130";
    requireSDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0) == 0, "Set GL context flags");
    requireSDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0) == 0, "Set GL profile");
    requireSDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) == 0, "Set GL major version");
    requireSDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0) == 0, "Set GL minor version");
#endif
    _impl->window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                     int(size.width), int(size.height), SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    requireSDL(_impl->window != nullptr, "Create standalone window");
    _impl->context = SDL_GL_CreateContext(_impl->window);
    requireSDL(_impl->context != nullptr, "Create standalone GL context");
    requireSDL(SDL_GL_MakeCurrent(_impl->window, _impl->context) == 0, "Activate standalone GL context");
    SDL_GL_SetSwapInterval(1); // Vsync is optional on some window systems.
    if (!ImGui_ImplSDL2_InitForOpenGL(_impl->window, _impl->context))
    {
        throw std::runtime_error("Initialize standalone GUI platform backend failed.");
    }
    _impl->platformStarted = true;
    if (!ImGui_ImplOpenGL3_Init(version))
    {
        throw std::runtime_error("Initialize standalone GUI renderer failed.");
    }
    _impl->rendererStarted = true;
}

StandaloneGuiWindow::~StandaloneGuiWindow() = default;

void StandaloneGuiWindow::processEvent(const SDL_Event& event)
{
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void StandaloneGuiWindow::prepareFrame()
{
    requireSDL(SDL_GL_MakeCurrent(_impl->window, _impl->context) == 0, "Activate standalone GL context");
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
}

void StandaloneGuiWindow::present()
{
    int width = 0, height = 0;
    SDL_GL_GetDrawableSize(_impl->window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(_impl->window);
}
}
