#ifndef bg2e_app_mouseevent_hpp
#define bg2e_app_mouseevent_hpp

namespace bg2e {
namespace app {

typedef enum {
    ButtonNone = 0,
    ButtonLeft,
    ButtonMiddle,
    ButtonRight
} MouseButton;

struct MouseStatus {
    bool leftButton;
    bool middleButton;
    bool rightButton;
    uint32_t posX;
    uint32_t posY;
    float deltaX;
    float deltaY;
};

class MouseEvent {
public:
    

    MouseEvent(MouseStatus status, MouseButton button = MouseButton::ButtonNone)
        :_mouseStatus{ status }, _button{ button }
    {
    }


    inline const MouseStatus& mouseStatus() const { return _mouseStatus; }
    inline MouseButton button() const { return _button; }

protected:
    MouseStatus _mouseStatus;
    MouseButton _button;
};

}
}

#endif
