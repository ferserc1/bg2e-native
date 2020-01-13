
#include <bg2wnd/keyboard_event.hpp>


namespace bg2wnd {

KeyboardEvent::KeyboardEvent(KeyCode key, bool shift, bool ctrl, bool alt, bool capsLock)
    :_shift(shift)
    ,_control(ctrl)
    ,_alt(alt)
    ,_capsLock(capsLock)
    ,_key(key)
{
}
    
}
