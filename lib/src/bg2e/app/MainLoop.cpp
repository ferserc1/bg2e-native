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

#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/PreferencesStore.hpp>

#ifdef BG2E_LINUX

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_syswm.h>
#include <wayland-client.h>
#include <cstdlib>
#include <cstring>

#else

#include <SDL.h>
#include <SDL_vulkan.h>

#endif

#include <iostream>

#include <thread>

namespace bg2e {
namespace app {

static const char preferencesContext[] = "app";

#ifdef BG2E_LINUX
static bool HasSdlWaylandDriver()
{
    const int numDrivers = SDL_GetNumVideoDrivers();
    for (int i = 0; i < numDrivers; ++i)
    {
        const char * driver = SDL_GetVideoDriver(i);
        if (driver && std::strcmp(driver, "wayland") == 0)
        {
            return true;
        }
    }
    return false;
}

static bool CanConnectToWayland()
{
    wl_display * display = wl_display_connect(nullptr);
    if (!display)
    {
        return false;
    }
    wl_display_disconnect(display);
    return true;
}

void InitSdlVideoDriver()
{
    if (HasSdlWaylandDriver() && CanConnectToWayland())
    {
        setenv("SDL_VIDEODRIVER", "wayland", 1);
    }
}
#else

void InitSdlVideoDriver() {}

#endif


SDL_Window* createSDLWindow(const WindowConfig& cfg) {
    // Position
    int xpos = (cfg.x < 0) ? SDL_WINDOWPOS_CENTERED : cfg.x;
    int ypos = (cfg.y < 0) ? SDL_WINDOWPOS_CENTERED : cfg.y;

    // SDL flags derived from WindowConfig
    Uint32 flags = SDL_WINDOW_VULKAN;

    if (cfg.resizable)       flags |= SDL_WINDOW_RESIZABLE;
    if (!cfg.decorated)      flags |= SDL_WINDOW_BORDERLESS;
    if (cfg.alwaysOnTop)     flags |= SDL_WINDOW_ALWAYS_ON_TOP;

    // States
    if (cfg.isFullscreen)    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (cfg.isMaximized)     flags |= SDL_WINDOW_MAXIMIZED;

    auto width = cfg.width;
    auto height = cfg.height;
    if (cfg.persistentSize)
    {
        auto appPreferences = PreferencesStore::instance().preferences(preferencesContext);
        auto windowSize = appPreferences.get("windowSize", glm::vec2(
            static_cast<float>(width),
            static_cast<float>(height)
        ));
        width = static_cast<int>(windowSize.x);
        height = static_cast<int>(windowSize.y);
    }

    SDL_Window* window = SDL_CreateWindow(
        cfg.title.c_str(),
        xpos,
        ypos,
        width,
        height,
        flags
    );

    if (!window) {
        throw std::runtime_error(
            std::string("Failed to create SDL window: ") + SDL_GetError()
        );
    }

    return window;
}


MainLoop * MainLoop::_mainLoopInstance = nullptr;

void MainLoop::initMainLoopInstance()
{
    if (MainLoop::_mainLoopInstance)
    {
        throw std::runtime_error("Fatal Error: Multiple instances of MainLoop created. MainLoop instance can only be created once. You can access existing MainLoop instance through the static MainLoop::current() method.");
    }
    MainLoop::_mainLoopInstance = this;
}

MainLoop::MainLoop(const std::string& appId)
    :_appId { appId }
{
    initMainLoopInstance();
}

MainLoop::MainLoop(std::string && appId)
    :_appId { appId }
{
    initMainLoopInstance();
}

MainLoop::~MainLoop()
{
    if (MainLoop::_mainLoopInstance == this)
    {
        _mainLoopInstance = nullptr;
    }
}

int32_t MainLoop::run(app::Application * application) {
#ifdef BG2E_IS_MAC
    SDL_SetHint(SDL_HINT_MAC_BACKGROUND_APP, "0");
#endif

    InitSdlVideoDriver();
	SDL_Init(SDL_INIT_VIDEO);

    auto window = createSDLWindow(_windowConfig);

    _engine.init(window);

    int viewportW = 0;
    int viewportH = 0;

    SDL_GetWindowSize(window, &viewportW, &viewportH);

    application->uiDelegate()->_viewportWidth = viewportW;
    application->uiDelegate()->_viewportHeight = viewportH;
	_userInterface.setDelegate(application->uiDelegate());

	_userInterface.init(&_engine);
    _renderLoop.setDelegate(application->renderDelegate());
	_renderLoop.init(&_engine);

	_inputManager.setDelegate(application->inputDelegate());

	_renderLoop.renderUICallback([&](VkCommandBuffer cmd, VkImageView targetImageView) {
		_userInterface.draw(cmd, targetImageView);
	});
    
    SDL_Event event;
    bool quit = false;
    bool stopRendering = false;
    bool resizing = false;
    auto minResizeInterval = 500;
    std::chrono::steady_clock::time_point lastResizeEventTime;
    
    
    // Initialize the main descriptor set allocator before executing the first frame
    _engine.descriptorSetAllocator().initPool();
    _renderLoop.initScene();
    auto start = std::chrono::high_resolution_clock::now();
    int storeWidth = _windowConfig.width;
    int storeHeight = _windowConfig.height;
    
    while (!quit)
    {
        while (SDL_PollEvent(&event) != 0)
        {
            
            if (event.type == SDL_QUIT)
            {
                if (_onExitFunction)
                {
                    quit = _onExitFunction();
                }
                else {
                    quit = true;
                }
            }
            else if (event.type == SDL_DROPFILE)
            {
                std::string pathStr(event.drop.file);
                SDL_free(event.drop.file);

                application->inputDelegate()->fileDropped(std::filesystem::path(pathStr));
            }
            else if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_MINIMIZED)
            {
                stopRendering = true;
            }
            else if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESTORED)
            {
                stopRendering = false;
            }
            else if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                int w, h;
                SDL_GetWindowSize(window, &w, &h);
                storeWidth = w;
                storeHeight = h;
                application->uiDelegate()->_viewportWidth = static_cast<uint32_t>(w);
                application->uiDelegate()->_viewportHeight = static_cast<uint32_t>(h);
                
                lastResizeEventTime = std::chrono::steady_clock::now();
                if (!resizing) {
                    _engine.updateSwapchainSize();
                    resizing = true;
                }
            }
			else if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN)
			{
			    const auto bg2eEvent = KeyEvent::fromSDLEvent(event);
                if (event.key.state == SDL_PRESSED)
                {
					_inputManager.keyDown(bg2eEvent);
                    _shortcuts.keyDown(bg2eEvent);
                }
                else if (event.key.state == SDL_RELEASED)
                {
                    _inputManager.keyUp(bg2eEvent);
                    _shortcuts.keyUp(bg2eEvent);
                }
			}
            else if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
            {
                int x, y;
                SDL_GetMouseState(&x, &y);
                
                if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
                {
                    auto button = event.button.button == SDL_BUTTON_LEFT ?
                        0 : event.button.button == SDL_BUTTON_MIDDLE ?
                        1 : 2;
                    if (event.button.state == SDL_PRESSED)
                    {
                        _inputManager.mouseButtonDown(button, x, y);
                    }
                    else
                    {
                        _inputManager.mouseButtonUp(button, x, y);
                    }
                }
                else
                {
                    _inputManager.mouseMove(x, y);
                }
            }
            else if (event.type == SDL_MOUSEWHEEL)
            {
                _inputManager.mouseWheel(event.wheel.x, event.wheel.y);
            }
            
            _userInterface.processEvent(&event);
        }

        if (resizing)
        {
            auto now = std::chrono::steady_clock::now();
            if (now - lastResizeEventTime > std::chrono::milliseconds(minResizeInterval)) {
                resizing = false;
            }
        }
        
        if (stopRendering)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        if (!resizing && _engine.newFrame())
        {
            _renderLoop.swapchainResized();
        }
        
        _userInterface.newFrame();
        
        if (!resizing) {
            _renderLoop.acquireAndPresent();
        }
        auto end = std::chrono::high_resolution_clock::now();
    
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
        //std::cout << millis.count() << std::endl;
        _renderLoop.setDelta(static_cast<float>(millis.count()));
        
        start = std::chrono::high_resolution_clock::now();
    }

    if (_windowConfig.persistentSize)
    {
        auto appPreferences = PreferencesStore::instance().preferences(preferencesContext);
        appPreferences.set("windowSize", glm::vec2{ storeWidth, storeHeight });
    }

    _engine.device().waitIdle();
	_renderLoop.cleanup();
	_userInterface.cleanup();
    _engine.cleanup();
    SDL_DestroyWindow(window);

    return 0;
}

void MainLoop::exit()
{
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

}
}
