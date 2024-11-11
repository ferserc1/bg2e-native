#pragma once

#include <bg2e/app/KeyEvent.hpp>

namespace bg2e {
namespace app {

class InputDelegate {
public:
    virtual void keyDown(const KeyEvent& event) {}
    virtual void keyUp(const KeyEvent& event) {}
    virtual void mouseMove(int x, int y) {}
    virtual void mouseButtonDown(int button, int x, int y) {}
    virtual void mouseButtonUp(int button, int x, int y) {}
    virtual void mouseWheel(int deltaX, int deltaY) {}
};

}
}
