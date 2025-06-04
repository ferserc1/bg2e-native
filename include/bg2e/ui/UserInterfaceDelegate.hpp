#pragma once

#include <bg2e/render/Engine.hpp>

namespace bg2e {
namespace ui {

class UserInterface;

class BG2E_API UserInterfaceDelegate {
public:
    virtual void init(bg2e::render::Engine*, UserInterface*) {}
    virtual void drawUI();
};

}
}
