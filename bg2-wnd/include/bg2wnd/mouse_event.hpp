
#ifndef _bg2_wnd_mouse_event_hpp_
#define _bg2_wnd_mouse_event_hpp_

namespace bg2wnd {

    enum class MouseButton {
        ButtonNone = 0,
        ButtonLeft = 1,
        ButtonMiddle = 3,
        ButtonRight = 2,
        Button4 = 4,
        Button5 = 5
    };

    class MouseEvent {
    public:
        MouseEvent(int x, int y, MouseButton eventButton, bool b1, bool b2, bool b3, bool b4, bool b5, float wheelDeltaX, float wheelDeltaY);
        
        inline int posX() const { return _posX; }
        inline int posY() const { return _posY; }
        inline float wheelDeltaX() const { return _wheelDeltaX; }
        inline float wheelDeltaY() const { return _wheelDeltaY; }
        
        // eventButton is the mouse button that triggered the event
        inline MouseButton eventButton() const { return _eventButton; }

        // button(buttonCode) returns the current status of the indicated mouse button
        inline bool button(int b) const { return b>=1 && b<=5 ? _buttons[b - 1] : false; }
        
    protected:
        int _posX;
        int _posY;
        bool _buttons[5];
        float _wheelDeltaX;
        float _wheelDeltaY;
        MouseButton _eventButton;
    };

}

#endif
