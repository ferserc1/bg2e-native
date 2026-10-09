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
#include <bg2e/app/Application.hpp>
#include <bg2e/app/InputManager.hpp>
#include <bg2e/app/Shortcuts.hpp>
#include <bg2e/draw/EngineConfig.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/ui/Loader.hpp>
#include <bg2e/base/Timeout.hpp>

#include <functional>
#include <cstdint>
#include <memory>
#include <atomic>
#include <vector>
#include <utility>
#include <thread>
#include <mutex>
#include <queue>
#include <exception>

#include <glm/glm.hpp>

namespace bg2e {
namespace app {

namespace detail {
class GraphicsExecution;
}

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


struct SafeUpdateToken {
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>>(true);
    ~SafeUpdateToken() { *alive = false; }
};

class BG2E_API MainLoop {
    struct SafeUpdateSceneEntry {
        std::function<void()> function;
        std::weak_ptr<SafeUpdateToken> token;
        bool hasToken = false;
    };

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

    // Experimental draw execution path. Always requires an explicit
    // configuration; there is no default EngineConfig argument.
    int32_t run(Application* application, const draw::EngineConfig& config);

    void exit();
    
    inline void setOnExitFunction(std::function<bool()> fn) { _onExitFunction = fn; }

    bg2e::base::Timeout& timeout() { return _timeout; }

    void setBackgroundFrameRateLimitEnabled(bool enabled);
    inline bool backgroundFrameRateLimitEnabled() const { return _backgroundFrameRateLimitEnabled.load(); }

    void setBackgroundMaxFrameRate(double fps);
    inline double backgroundMaxFrameRate() const { return _backgroundMaxFrameRate.load(); }

    // Thread-safe. The next background polling iteration will render a frame
    // even if the configured frame deadline has not been reached yet.
    void requestFrame();

    // Main-thread only, during an active run. In draw, scene invalidation is
    // independent of presentation; while paused, refresh remains pending.
    // Production scenes already update every frame.
    void requestSceneFrame();
    void pauseScene(const glm::vec4& clearColor = {0.f, 0.f, 0.f, 1.f});
    void resumeScene();

    // A non-null token is observed weakly; the caller must retain it until the
    // queued function should run. Releasing the last reference cancels the work.
    void safeUpdateScene(std::function<void()> fn, std::shared_ptr<SafeUpdateToken> token = nullptr)
    {
        {
            std::lock_guard lock(_safeUpdateSceneMutex);
            _safeUpdateScene.push_back({ std::move(fn), token, token != nullptr });
        }
        requestFrame();
    }

    void requestResizeEvent();

    void asyncLoad(
        std::function<void(ui::Loader*)> loadFn,
        glm::vec4 clearColor = {0.f, 0.f, 0.f, 1.f},
        std::function<void(std::exception_ptr)> onComplete = nullptr
    );

protected:
    WindowConfig _windowConfig;

    bool _quit = false;
    
    std::string _appId;
    
    static MainLoop * _mainLoopInstance;
    
    std::unique_ptr<detail::GraphicsExecution> _execution;
	app::InputManager _inputManager;
	ui::UserInterface _userInterface;
 
    std::function<bool()> _onExitFunction = nullptr;

    std::vector<SafeUpdateSceneEntry> _safeUpdateScene;
    std::mutex _safeUpdateSceneMutex;

    ui::Loader _loader;

    std::mutex                         _mainThreadQueueMutex;
    std::queue<std::function<void()>>  _mainThreadQueue;

    void drainMainThreadQueue();

    Shortcuts _shortcuts;

    bool _resizeRequested = false;

    std::atomic<bool> _backgroundFrameRateLimitEnabled { false };
    std::atomic<double> _backgroundMaxFrameRate { 1.0 };
    std::atomic<bool> _frameRequested { false };
    
    bg2e::base::Timeout _timeout;

    void initMainLoopInstance();

    int32_t runInternal(Application* application, std::unique_ptr<detail::GraphicsExecution> execution);

    void executeSafeUpdateScene();
};

}
}
