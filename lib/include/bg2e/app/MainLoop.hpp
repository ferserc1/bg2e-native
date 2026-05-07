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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/RenderLoop.hpp>
#include <bg2e/app/Application.hpp>
#include <bg2e/app/InputManager.hpp>
#include <bg2e/app/Shortcuts.hpp>
#include <bg2e/ui/UserInterface.hpp>

#include <functional>
#include <cstdint>

namespace bg2e {
namespace app {

struct WindowConfig {
    std::string title = "";

    // Initial position. Negative values → default system position.
    int32_t x = -1;
    int32_t y = -1;

    // Initial size when not maximised or in full screen mode.
    uint32_t width = 1280;
    uint32_t height = 720;

    // Window states
    bool isMaximized = false;
    bool isFullscreen = false;

    // Common options
    bool resizable = true;
    bool decorated = true;       // With border and title bar
    bool alwaysOnTop = false;

    // Preferences
    bool persistentSize = false;

    // ------------------------------------------------
    // Factory methods
    // ------------------------------------------------

    static WindowConfig withSize(
        const std::string& title,
        uint32_t w,
        uint32_t h,
        bool persistentSize = false
    ) {
        WindowConfig c;
        c.title = title;
        c.width = w;
        c.height = h;
        c.persistentSize = persistentSize;
        return c;
    }

    static WindowConfig withPositionAndSize(
        const std::string& title,
        int32_t x, int32_t y,
        uint32_t w, uint32_t h,
        bool persistentSize = false
    ) {
        WindowConfig c;
        c.title = title;
        c.x = x;
        c.y = y;
        c.width = w;
        c.height = h;
        c.persistentSize = persistentSize;
        return c;
    }

    static WindowConfig maximized(const std::string& title, bool persistentSize = false)
    {
        WindowConfig c;
        c.title = title;
        c.isMaximized = true;
        c.persistentSize = persistentSize;
        return c;
    }

    static WindowConfig fullscreen(const std::string& title)
    {
        WindowConfig c;
        c.title = title;
        c.isFullscreen = true;
        return c;
    }
};


class BG2E_API MainLoop {
public:
    MainLoop(const std::string& appId);
    MainLoop(std::string && appId);
    ~MainLoop();
    
    inline const std::string & appId() const { return _appId; }
    
    inline void initWindowConfig(const WindowConfig& config) { _windowConfig = config; }
    inline void initWindowConfig(WindowConfig&& config) { _windowConfig = std::move(config); }
    
    static MainLoop * current() { return _mainLoopInstance; }

    static Shortcuts & shortcuts() { return MainLoop::current()->_shortcuts; }
    
    int32_t run(Application* application);

    void exit();
    
    inline void setOnExitFunction(std::function<bool()> fn) { _onExitFunction = fn; }
    
protected:
    WindowConfig _windowConfig;

    bool _quit = false;
    
    std::string _appId;
    
    static MainLoop * _mainLoopInstance;
    
    render::Engine _engine;
	render::RenderLoop _renderLoop;
	app::InputManager _inputManager;
	ui::UserInterface _userInterface;
 
    std::function<bool()> _onExitFunction = nullptr;

    Shortcuts _shortcuts;
    
    void initMainLoopInstance();
};

}
}
