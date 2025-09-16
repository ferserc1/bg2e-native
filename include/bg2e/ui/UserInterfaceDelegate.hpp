#pragma once

#include <bg2e/render/Engine.hpp>

namespace bg2e {

namespace app {

class MainLoop;

}

namespace ui {

class UserInterface;

class BG2E_API UserInterfaceDelegate {
    friend class app::MainLoop;
public:
    virtual void init(bg2e::render::Engine*, UserInterface*) {}
    virtual void drawUI();
    
    inline uint32_t uiWidth() const { return _viewportWidth; }
    inline uint32_t uiHeight() const { return _viewportHeight; }
    
protected:
    uint32_t _viewportWidth = 0;
    uint32_t _viewportHeight = 0;
};

}
}
