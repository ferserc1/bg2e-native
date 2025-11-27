#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/RenderLoop.hpp>
#include <bg2e/app/Application.hpp>
#include <bg2e/app/InputManager.hpp>
#include <bg2e/ui/UserInterface.hpp>

#include <functional>

namespace bg2e {
namespace app {

class BG2E_API MainLoop {
public:
    MainLoop(const std::string& appId);
    MainLoop(std::string && appId);
    ~MainLoop();
    
    inline const std::string & appId() const { return _appId; }
    
    inline void initWindowSize(uint32_t width, uint32_t height) { _windowWidth = width; _windowHeight = height; }
    inline void initWindowTitle(const std::string& title) { _windowTitle = title; }
    
    static MainLoop * current() { return _mainLoopInstance; }
    
    int32_t run(Application* application);

    void exit();
    
    inline void setOnExitFunction(std::function<bool()> fn) { _onExitFunction = fn; }
    
protected:
    uint32_t _windowWidth = 1440;
    uint32_t _windowHeight = 700;
    std::string _windowTitle = "bg2 engine - native";
    bool _quit = false;
    
    std::string _appId;
    
    static MainLoop * _mainLoopInstance;
    
    render::Engine _engine;
	render::RenderLoop _renderLoop;
	app::InputManager _inputManager;
	ui::UserInterface _userInterface;
 
    std::function<bool()> _onExitFunction = nullptr;
    
    void initMainLoopInstance();
};

}
}
