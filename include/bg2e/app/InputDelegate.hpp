#pragma once

#include <bg2e/app/KeyEvent.hpp>

namespace bg2e {
namespace app {

class InputDelegate {
public:
    virtual void keyDown(const KeyEvent&) {}

    /**
     * Input event: key up
     * @param keyEvent The key event data.
     */
    virtual void keyUp(const KeyEvent&) {}

    /**
     * Input event: mouse move
     * @param x The x position of the mouse in window coordinates.
     * @param y The y position of the mouse in window coordinates.
     */
    virtual void mouseMove(int, int) {}

    /**
     * Input event: mouse button down
     * @param button The mouse button index.
     * @param x The x position of the mouse in window coordinates.
     * @param y The y position of the mouse in window coordinates.
     */
    virtual void mouseButtonDown(int, int, int) {}

    /**
     * Input event: mouse button up
     * @param button The mouse button index.
     * @param x The x position of the mouse in window coordinates.
     * @param y The y position of the mouse in window coordinates.
     */
    virtual void mouseButtonUp(int, int, int) {}

    /**
     * Input event: mouse wheel
     * @param deltaX The scroll delta in the x direction.
     * @param deltaY The scroll delta in the y direction.
     */
    virtual void mouseWheel(int, int) {}
};

}
}
