#pragma once

#include <bg2e/render/Vulkan.hpp>

namespace bg2e {
namespace ui {

class UserInterface;

class BG2E_API UserInterfaceDelegate {
public:
    virtual void init(bg2e::render::Vulkan*, UserInterface*) {}
    virtual void drawUI();
};

}
}