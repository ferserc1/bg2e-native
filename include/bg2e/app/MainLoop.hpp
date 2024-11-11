#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Vulkan.hpp>
#include <bg2e/render/RenderLoop.hpp>
#include <bg2e/app/Application.hpp>

namespace bg2e {
namespace app {

class BG2E_API MainLoop {
public:
    inline void initWindowSize(uint32_t width, uint32_t height) { _windowWidth = width; _windowHeight = height; }
    inline void initWindowTitle(const std::string& title) { _windowTitle = title; }
    
    int32_t run(Application* application);

protected:
    uint32_t _windowWidth = 1440;
    uint32_t _windowHeight = 700;
    std::string _windowTitle = "bg2 engine - native";
    
    render::Vulkan _vulkan;
	render::RenderLoop _renderLoop;
};

}
}
