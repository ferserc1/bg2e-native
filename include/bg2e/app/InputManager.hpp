#pragma once

#include <bg2e/common.hpp>
#include <bg2e/app/InputDelegate.hpp>
#include <bg2e/app/KeyEvent.hpp>

#include <memory>

namespace bg2e {
namespace app {

class BG2E_API InputManager {
public:
    struct MouseButtonsStatus {
        bool left;
        bool middle;
        bool rigth;
        
        uint32_t x;
        uint32_t y;
    };
    static MouseButtonsStatus getMouseStatus();
    
    void keyDown(const KeyEvent& event);
    void keyUp(const KeyEvent& event);
    void mouseMove(int x, int y);
    void mouseButtonDown(int button, int x, int y);
    void mouseButtonUp(int button, int x, int y);
    void mouseWheel(int deltaX, int deltaY);

    inline void setDelegate(std::shared_ptr<InputDelegate> delegate) { _delegate = delegate; }

protected:
    std::shared_ptr<InputDelegate> _delegate;
};

}
}
