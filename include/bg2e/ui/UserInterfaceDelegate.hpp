#pragma once

#include <bg2e/render/Vulkan.hpp>

namespace bg2e {
namespace ui {

class UserInterface;

class UserInterfaceDelegate {
public:
    virtual void init(bg2e::render::Vulkan*, UserInterface*) {}
    virtual void drawUI() {}
};

}
}