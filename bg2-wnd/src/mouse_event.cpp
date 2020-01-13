
#include <bg2wnd/mouse_event.hpp>

namespace bg2wnd {

    MouseEvent::MouseEvent(int x, int y, MouseButton eventButton, bool b1, bool b2, bool b3, bool b4, bool b5, float wheelDeltaX, float wheelDeltaY)
        :_posX(x)
        ,_posY(y)
        ,_wheelDeltaX(wheelDeltaX)
        ,_wheelDeltaY(wheelDeltaY)
        ,_eventButton(eventButton)
    {
        _buttons[0] = b1;
        _buttons[1] = b2;
        _buttons[2] = b3;
        _buttons[3] = b4;
        _buttons[4] = b5;
    }
    
}
