#pragma once

#include <SDL2/SDL.h>

namespace bg2e {
namespace app {

class KeyEvent {
public:
    enum Key {
        KeyUnknown = -1,
        KeySpace = 32,
        KeyApostrophe = 39,
        KeyComma = 44,
        KeyMinus = 45,
        KeyPeriod = 46,
        KeySlash = 47,
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
        KeySemicolon = 59,
        KeyEqual = 61,
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
        KeyLeftBracket = 91,
        KeyBackslash = 92,
        KeyRightBracket = 93,
        KeyGraveAccent = 96,
        KeyWorld1 = 161,
        KeyWorld2 = 162,
        KeyEscape = 27,
        KeyEnter = 257,
        KeyTab = 258,
        KeyBackspace = 259,
        KeyInsert = 260,
        KeyDelete = 261,
        KeyRight = 262,
        KeyLeft = 263,
        KeyDown = 264,
        KeyUp = 265,
        KeyPageUp = 266,
        KeyPageDown = 267,
        KeyHome = 268,
        KeyEnd = 269,
        KeyCapsLock = 280,
        KeyScrollLock = 281,
        KeyNumLock = 282,

        // Modifier keys
        KeyLeftShift = 340,
        KeyLeftControl = 341,
        KeyLeftAlt = 342,
        KeyLeftSuper = 343,
        KeyRightShift = 344,
        KeyRightControl = 345,
        KeyRightAlt = 346,
        KeyRightSuper = 347,
        KeyMenu = 348
    };

    KeyEvent(Key key) :_key(key) {}

    static KeyEvent fromSDLEvent(SDL_Event & e) {
        Key key = KeyUnknown;

        SDL_Keycode sym = e.key.keysym.sym;

        if (sym >= 'a' && sym <= 'z') {
            key = static_cast<Key>(sym - 'a' + KeyA);
        }
        else {
            switch (sym) {
            case SDLK_LSHIFT: key = KeyLeftShift; break;
            case SDLK_RSHIFT: key = KeyRightShift; break;
            case SDLK_LCTRL: key = KeyLeftControl; break;
            case SDLK_RCTRL: key = KeyRightControl; break;
            case SDLK_LALT: key = KeyLeftAlt; break;
            case SDLK_RALT: key = KeyRightAlt; break;
            case SDLK_LGUI: key = KeyLeftSuper; break;
            case SDLK_RGUI: key = KeyRightSuper; break;
            case SDLK_MENU: key = KeyMenu; break;
            case SDLK_LEFT: key = KeyLeft; break;
            case SDLK_RIGHT: key = KeyRight; break;
            case SDLK_UP: key = KeyUp; break;
            case SDLK_DOWN: key = KeyDown; break;
            case SDLK_RETURN: key = KeyEnter; break;
            case SDLK_KP_ENTER: key = KeyEnter; break;
            case SDLK_TAB: key = KeyTab; break;
            case SDLK_BACKSPACE: key = KeyBackspace; break;
            case SDLK_INSERT: key = KeyInsert; break;
            case SDLK_DELETE: key = KeyDelete; break;
            case SDLK_PAGEUP: key = KeyPageUp; break;
            case SDLK_PAGEDOWN: key = KeyPageDown; break;
            case SDLK_HOME: key = KeyHome; break;
            case SDLK_END: key = KeyEnd; break;
            case SDLK_CAPSLOCK: key = KeyCapsLock; break;
            case SDLK_SCROLLLOCK: key = KeyScrollLock; break;
            case SDLK_NUMLOCKCLEAR: key = KeyNumLock; break;
            default:
                key = static_cast<Key>(sym);
                break;
            }
        }

        return KeyEvent(key);
    }

    inline Key key() const { return _key; }

    static const char* keyName(const KeyEvent& e) {
        switch (e.key()) {
            case KeyUnknown: return "KeyUnknown";
            case KeySpace: return "KeySpace";
            case KeyApostrophe: return "KeyApostrophe";
            case KeyComma: return "KeyComma";
            case KeyMinus: return "KeyMinus";
            case KeyPeriod: return "KeyPeriod";
            case KeySlash: return "KeySlash";
            case Key0: return "Key0";
            case Key1: return "Key1";
            case Key2: return "Key2";
            case Key3: return "Key3";
            case Key4: return "Key4";
            case Key5: return "Key5";
            case Key6: return "Key6";
            case Key7: return "Key7";
            case Key8: return "Key8";
            case Key9: return "Key9";
            case KeySemicolon: return "KeySemicolon";
            case KeyEqual: return "KeyEqual";
            case KeyA: return "KeyA";
            case KeyB: return "KeyB";
            case KeyC: return "KeyC";
            case KeyD: return "KeyD";
            case KeyE: return "KeyE";
            case KeyF: return "KeyF";
            case KeyG: return "KeyG";
            case KeyH: return "KeyH";
            case KeyI: return "KeyI";
            case KeyJ: return "KeyJ";
            case KeyK: return "KeyK";
            case KeyL: return "KeyL";
            case KeyM: return "KeyM";
            case KeyN: return "KeyN";
            case KeyO: return "KeyO";
            case KeyP: return "KeyP";
            case KeyQ: return "KeyQ";
            case KeyR: return "KeyR";
            case KeyS: return "KeyS";
            case KeyT: return "KeyT";
            case KeyU: return "KeyU";
            case KeyV: return "KeyV";
            case KeyW: return "KeyW";
            case KeyX: return "KeyX";
            case KeyY: return "KeyY";
            case KeyZ: return "KeyZ";
            case KeyLeftBracket: return "KeyLeftBracket";
            case KeyBackslash: return "KeyBackslash";
            case KeyRightBracket: return "KeyRightBracket";
            case KeyGraveAccent: return "KeyGraveAccent";
            case KeyWorld1: return "KeyWorld1";
            case KeyWorld2: return "KeyWorld2";
            case KeyEscape: return "KeyEscape";
            case KeyEnter: return "KeyEnter";
            case KeyTab: return "KeyTab";
            case KeyBackspace: return "KeyBackspace";
            case KeyInsert: return "KeyInsert";
            case KeyDelete: return "KeyDelete";
            case KeyRight: return "KeyRight";
            case KeyLeft: return "KeyLeft";
            case KeyDown: return "KeyDown";
            case KeyUp: return "KeyUp";
            case KeyPageUp: return "KeyPageUp";
            case KeyPageDown: return "KeyPageDown";
            case KeyHome: return "KeyHome";
            case KeyEnd: return "KeyEnd";
            case KeyCapsLock: return "KeyCapsLock";
            case KeyScrollLock: return "KeyScrollLock";
            case KeyNumLock: return "KeyNumLock";
            case KeyLeftShift: return "KeyLeftShift";
            case KeyLeftControl: return "KeyLeftControl";
            case KeyLeftAlt: return "KeyLeftAlt";
            case KeyLeftSuper: return "KeyLeftSuper";
            case KeyRightShift: return "KeyRightShift";
            case KeyRightControl: return "KeyRightControl";
            case KeyRightAlt: return "KeyRightAlt";
            case KeyRightSuper: return "KeyRightSuper";
            case KeyMenu: return "KeyMenu";
            default: return "KeyUnknown";
        }
    }

protected:
    Key _key;
};

}
}
