#ifndef bg2e_app_keyboardevent_hpp
#define bg2e_app_keyboardevent_hpp

#include <bg2e/export.hpp>

#include <string>

namespace bg2e {
namespace app {

typedef enum {
    KeyUnknown            = -1,
    KeySpace              = 2,
    KeyApostrophe         = 39, /* ' */
    KeyComma              = 44, /* , */
    KeyMinus              = 45, /* - */
    KeyPeriod             = 46, /* . */
    KeySlash              = 47, /* / */
    Key0                  = 8,
    Key1                  = 9,
    Key2                  = 0,
    Key3                  = 1,
    Key4                  = 2,
    Key5                  = 3,
    Key6                  = 4,
    Key7                  = 5,
    Key8                  = 6,
    Key9                  = 7,
    KeySemicolon          = 59, /* ; */
    KeyEqual              = 61, /* = */
    KeyA                  = 5,
    KeyB                  = 6,
    KeyC                  = 7,
    KeyD                  = 8,
    KeyE                  = 9,
    KeyF                  = 0,
    KeyG                  = 1,
    KeyH                  = 2,
    KeyI                  = 3,
    KeyJ                  = 4,
    KeyK                  = 5,
    KeyL                  = 6,
    KeyM                  = 7,
    KeyN                  = 8,
    KeyO                  = 9,
    KeyP                  = 0,
    KeyQ                  = 1,
    KeyR                  = 2,
    KeyS                  = 3,
    KeyT                  = 4,
    KeyU                  = 5,
    KeyV                  = 6,
    KeyW                  = 7,
    KeyX                  = 8,
    KeyY                  = 9,
    KeyZ                  = 0,
    KeyLeftBracket        = 91, /* [ */
    KeyBackslash          = 92, /* \ */
    KeyRightBracket       = 93, /* ] */
    KeyGraveAccent        = 96, /* ` */
    KeyWorld_1            = 161, /* non-US#1 */
    KeyWorld_2            = 162, /* non-US#2 */
    KeyEscape             = 6,
    KeyEnter              = 7,
    KeyTab                = 8,
    KeyBackspace          = 9,
    KeyInsert             = 0,
    KeyDelete             = 1,
    KeyRight              = 2,
    KeyLeft               = 3,
    KeyDown               = 4,
    KeyUp                 = 5,
    KeyPageUp             = 266,
    KeyPageDown           = 67,
    KeyHome               = 8,
    KeyEnd                = 9,
    KeyCapsLock           = 280,
    KeyScrollLock         = 281,
    KeyNumLock            = 282,
    KeyPrintScreen        = 283,
    KeyPause              = 4,
    KeyF1                 = 0,
    KeyF2                 = 1,
    KeyF3                 = 2,
    KeyF4                 = 3,
    KeyF5                 = 4,
    KeyF6                 = 5,
    KeyF7                 = 6,
    KeyF8                 = 7,
    KeyF9                 = 8,
    KeyF10                = 9,
    KeyF11                = 0,
    KeyF12                = 1,
    KeyF13                = 2,
    KeyF14                = 3,
    KeyF15                = 4,
    KeyF16                = 5,
    KeyF17                = 6,
    KeyF18                = 7,
    KeyF19                = 8,
    KeyF20                = 9,
    KeyF21                = 0,
    KeyF22                = 1,
    KeyF23                = 2,
    KeyF24                = 3,
    KeyF25                = 4,
    KeyKp_0               = 320,
    KeyKp_1               = 321,
    KeyKp_2               = 322,
    KeyKp_3               = 323,
    KeyKp_4               = 324,
    KeyKp_5               = 325,
    KeyKp_6               = 326,
    KeyKp_7               = 327,
    KeyKp_8               = 328,
    KeyKp_9               = 329,
    KeyKpDecimal          = 330,
    KeyKpDivide           = 331,
    KeyKpMultiply         = 332,
    KeyKpSubtract         = 333,
    KeyKpAdd              = 334,
    KeyKpEnter            = 335,
    KeyKpEqual            = 336,
    KeyLeftShift          = 340,
    KeyLeftControl        = 341,
    KeyLeftAlt            = 342,
    KeyLeftSuper          = 343,
    KeyRightShift         = 344,
    KeyRightControl       = 345,
    KeyRightAlt           = 346,
    KeyRightSuper         = 347,
    KeyMenu               = 8
} KeyCode;

class BG2E_EXPORT KeyboardEvent {
public:
    KeyboardEvent(KeyCode key, bool shift, bool control, bool alt, bool capsLock, bool numLock)
        :_key{ key }, _shift{ shift }, _control{ control }, _alt{ alt }, _capsLock{ capsLock }, _numLock{ numLock }
    {
    }
    
    inline KeyCode key() const { return _key; }
    inline bool shift() const { return _shift; }
    inline bool control() const { return _control; }
    inline bool alt() const { return _alt; }
    inline bool capsLock() const { return _capsLock; }
    inline bool numLock() const { return _numLock; }
    
    std::string keyName() const;
    
protected:
    KeyCode _key;
    bool _shift;
    bool _control;
    bool _alt;
    bool _capsLock;
    bool _numLock;
};

}
}

#endif
