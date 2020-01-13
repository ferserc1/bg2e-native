
#ifndef _bg2_wnd_keyboard_event_hpp_
#define _bg2_wnd_keyboard_event_hpp_

#include <string>

namespace bg2wnd {

    enum class KeyCode {
        KeyUNKNOWN = -1,
        KeySPACE = 32,
        KeyAPOSTROPHE = 39,
        KeyCOMMA = 44,
        KeyMINUS = 45,
        KeyPERIOD = 46,
        KeySLASH = 47,
        Key0 = 48,
        Key1 = 49,
        Key2 = 50,
        Key3 = 51,
        Key4 = 52,
        Key5 = 53,
        Key6 = 54,
        Key7 = 55,
        Key8 = 56,
        Key9 = 57,
        KeyNUMPAD0 = 2048,
        KeyNUMPAD1 = 2049,
        KeyNUMPAD2 = 2050,
        KeyNUMPAD3 = 2051,
        KeyNUMPAD4 = 2052,
        KeyNUMPAD5 = 2053,
        KeyNUMPAD6 = 2054,
        KeyNUMPAD7 = 2055,
        KeyNUMPAD8 = 2056,
        KeyNUMPAD9 = 2057,
        KeySEMICOLON = 59,
        KeyEQUAL = 61,
        KeyA = 65,
        KeyB = 66,
        KeyC = 67,
        KeyD = 68,
        KeyE = 69,
        KeyF = 70,
        KeyG = 71,
        KeyH = 72,
        KeyI = 73,
        KeyJ = 74,
        KeyK = 75,
        KeyL = 76,
        KeyM = 77,
        KeyN = 78,
        KeyO = 79,
        KeyP = 80,
        KeyQ = 81,
        KeyR = 82,
        KeyS = 83,
        KeyT = 84,
        KeyU = 85,
        KeyV = 86,
        KeyW = 87,
        KeyX = 88,
        KeyY = 89,
        KeyZ = 90,
        KeyLEFT_BRACKET = 91,
        KeyBACKSLASH = 92,
        KeyRIGHT_BRACKET = 93,
        KeyGRAVE_ACCENT = 96,
        KeyWORLD_1 = 161,
        KeyWORLD_2 = 162,
        KeyESCAPE = 256,
        KeyENTER = 257,
        KeyTAB = 258,
        KeyBACKSPACE = 259,
        KeyINSERT = 260,
        KeyDELETE = 261,
        KeyRIGHT = 262,
        KeyLEFT = 263,
        KeyDOWN = 264,
        KeyUP = 265,
        KeyPAGE_UP = 266,
        KeyPAGE_DOWN = 267,
        KeyHOME = 268,
        KeyEND = 269,
        KeyCAPS_LOCK = 280,
        KeySCROLL_LOCK = 281,
        KeyNUM_LOCK = 282,
        KeyPRINT_SCREEN = 283,
        KeyPAUSE = 284,
        KeyF1 = 290,
        KeyF2 = 291,
        KeyF3 = 292,
        KeyF4 = 293,
        KeyF5 = 294,
        KeyF6 = 295,
        KeyF7 = 296,
        KeyF8 = 297,
        KeyF9 = 298,
        KeyF10 = 299,
        KeyF11 = 300,
        KeyF12 = 301,
        KeyF13 = 302,
        KeyF14 = 303,
        KeyF15 = 304,
        KeyF16 = 305,
        KeyF17 = 306,
        KeyF18 = 307,
        KeyF19 = 308,
        KeyF20 = 309,
        KeyF21 = 310,
        KeyF22 = 311,
        KeyF23 = 312,
        KeyF24 = 313,
        KeyF25 = 314,
        KeyKP_DECIMAL = 330,
        KeyKP_DIVIDE = 331,
        KeyKP_MULTIPLY = 332,
        KeyKP_SUBTRACT = 333,
        KeyKP_ADD = 334,
        KeyKP_ENTER = 335,
        KeyKP_EQUAL = 336,
        KeyLEFT_SHIFT = 340,
        KeyLEFT_CONTROL = 341,
        KeyLEFT_ALT = 342,
        KeyLEFT_SUPER = 343,
        KeyRIGHT_SHIFT = 344,
        KeyRIGHT_CONTROL = 345,
        KeyRIGHT_ALT = 346,
        KeyRIGHT_SUPER = 347,
        KeyMENU = 348
    };

    class KeyboardEvent {
    public:
        KeyboardEvent(KeyCode key, const std::string & character, bool shift, bool ctrl, bool alt, bool capsLock);
        
        inline bool shiftEnabled() const { return _shift; }
        inline bool controlEnabled() const { return _control; }
        inline bool altEnabled() const { return _alt; } 
        inline bool capsLockEnabled() const { return _capsLock; }
        inline KeyCode keyCode() const { return _key; }
        inline const std::string & character() const { return _character; }

        protected:
            bool _shift;
            bool _control;
            bool _alt;
            bool _capsLock;

            KeyCode _key;
            
            std::string _character;
        };

   }
   
   #endif
