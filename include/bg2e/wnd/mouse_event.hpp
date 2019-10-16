#ifndef _bg2e_wnd_mouse_event_hpp_
#define _bg2e_wnd_mouse_event_hpp_

#include <bg2e/wnd/glfw_defines.hpp>

namespace bg2e {
    namespace wnd {
    
        class MouseEvent {
        public:
            MouseEvent(int x, int y, bool b1, bool b2, bool b3, bool b4, bool b5, float wheelDeltaX, float wheelDeltaY)
                :_posX(x)
                ,_posY(y)
                ,_wheelDeltaX(wheelDeltaX)
                ,_wheelDeltaY(wheelDeltaY)
            {
                _buttons[0] = b1;
                _buttons[1] = b2;
                _buttons[2] = b3;
                _buttons[3] = b4;
                _buttons[4] = b5;
            }
            
            inline int posX() const { return _posX; }
            inline int posY() const { return _posY; }
            inline bool button(int b) const { return b>=1 && b<=5 ? _buttons[b - 1] : false; }
            inline float wheelDeltaX() const { return _wheelDeltaX; }
            inline float wheelDeltaY() const { return _wheelDeltaY; }
            inline bool anyButtonPressed() const { return _buttons[0] || _buttons[1] || _buttons[2] || _buttons[3] || _buttons[4]; }
            
        protected:
            int _posX;
            int _posY;
            bool _buttons[5];
            float _wheelDeltaX;
            float _wheelDeltaY;
        };


    }
}

#endif
