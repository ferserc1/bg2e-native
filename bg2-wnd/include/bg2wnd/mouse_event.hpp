
#ifndef _bg2_wnd_mouse_event_hpp_
#define _bg2_wnd_mouse_event_hpp_

namespace bg2wnd {

    class MouseEvent {
    public:
        MouseEvent(int x, int y, bool b1, bool b2, bool b3, bool b4, bool b5, float wheelDeltaX, float wheelDeltaY);
        
        inline int posX() const { return _posX; }
        inline int posY() const { return _posY; }
        inline bool button(int b) const { return b>=1 && b<=5 ? _buttons[b - 1] : false; }
        inline float wheelDeltaX() const { return _wheelDeltaX; }
        inline float wheelDeltaY() const { return _wheelDeltaY; }
        
    protected:
        int _posX;
        int _posY;
        bool _buttons[5];
        float _wheelDeltaX;
        float _wheelDeltaY;
    };

}

#endif
