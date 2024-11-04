#pragma once

#include <bg2e/common.hpp>

namespace bg2e {
namespace app {

class BG2E_API MainLoop {
public:
inline void initWindowSize(uint32_t width, uint32_t height) { _windowWidth = width; _windowHeight = height; }
    inline void initWindowTitle(const std::string& title) { _windowTitle = title; }
    
    int32_t run();

protected:
    uint32_t _windowWidth = 1440;
    uint32_t _windowHeight = 700;
    std::string _windowTitle = "bg2 engine - native";
};

}
}
