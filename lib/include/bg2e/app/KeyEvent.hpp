/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

    inline bool isModifier() const
    {
        switch (_key) {
        case KeyLeftShift:
        case KeyRightShift:
        case KeyLeftControl:
        case KeyRightControl:
        case KeyLeftAlt:
        case KeyRightAlt:
        case KeyLeftSuper:
        case KeyRightSuper:
            return true;
        default:
            return false;
        }
    }

    static const char* keyName(const KeyEvent& e) {
        switch (e.key()) {
            case KeyUnknown: return "KeyUnknown";
            case KeySpace: return "Space";
            case KeyApostrophe: return "Apostrophe";
            case KeyComma: return "Comma";
            case KeyMinus: return "Minus";
            case KeyPeriod: return "Period";
            case KeySlash: return "Slash";
            case Key0: return "0";
            case Key1: return "1";
            case Key2: return "2";
            case Key3: return "3";
            case Key4: return "4";
            case Key5: return "5";
            case Key6: return "6";
            case Key7: return "7";
            case Key8: return "8";
            case Key9: return "9";
            case KeySemicolon: return "Semicolon";
            case KeyEqual: return "Equal";
            case KeyA: return "A";
            case KeyB: return "B";
            case KeyC: return "C";
            case KeyD: return "D";
            case KeyE: return "E";
            case KeyF: return "F";
            case KeyG: return "G";
            case KeyH: return "H";
            case KeyI: return "I";
            case KeyJ: return "J";
            case KeyK: return "K";
            case KeyL: return "L";
            case KeyM: return "M";
            case KeyN: return "N";
            case KeyO: return "O";
            case KeyP: return "P";
            case KeyQ: return "Q";
            case KeyR: return "R";
            case KeyS: return "S";
            case KeyT: return "T";
            case KeyU: return "U";
            case KeyV: return "V";
            case KeyW: return "W";
            case KeyX: return "X";
            case KeyY: return "Y";
            case KeyZ: return "Z";
            case KeyLeftBracket: return "LeftBracket";
            case KeyBackslash: return "Backslash";
            case KeyRightBracket: return "RightBracket";
            case KeyGraveAccent: return "GraveAccent";
            case KeyWorld1: return "KeyWorld1";
            case KeyWorld2: return "KeyWorld2";
            case KeyEscape: return "Escape";
            case KeyEnter: return "Enter";
            case KeyTab: return "Tab";
            case KeyBackspace: return "Backspace";
            case KeyInsert: return "Insert";
            case KeyDelete: return "Delete";
            case KeyRight: return "Right";
            case KeyLeft: return "Left";
            case KeyDown: return "Down";
            case KeyUp: return "Up";
            case KeyPageUp: return "PageUp";
            case KeyPageDown: return "PageDown";
            case KeyHome: return "Home";
            case KeyEnd: return "End";
            case KeyCapsLock: return "CapsLock";
            case KeyScrollLock: return "ScrollLock";
            case KeyNumLock: return "NumLock";
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
