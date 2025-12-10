#pragma once

#include <bg2e/app/KeyEvent.hpp>

#include <filesystem>

namespace bg2e {
namespace app {

class InputDelegate {
public:
    virtual void keyDown([[maybe_unused]] const KeyEvent& keyEvent) {}

    virtual void keyUp([[maybe_unused]] const KeyEvent& keyEvent) {}

    virtual void mouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) {}

    virtual void mouseButtonDown([[maybe_unused]] int button, [[maybe_unused]] int x, [[maybe_unused]] int y) {}

    virtual void mouseButtonUp([[maybe_unused]] int button, [[maybe_unused]] int x, [[maybe_unused]] int y) {}

    virtual void mouseWheel([[maybe_unused]] int deltaX, [[maybe_unused]] int deltaY) {}

    virtual void fileDropped([[maybe_unused]] const std::filesystem::path &) {}
};

}
}
