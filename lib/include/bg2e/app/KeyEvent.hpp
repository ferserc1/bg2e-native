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
    };

    KeyEvent(Key key) :_key(key) {}

    static KeyEvent fromSDLEvent(SDL_Event & e) {
		Key key = KeyUnknown;
        if (e.key.keysym.sym >= 'a' && e.key.keysym.sym <= 'z')
        {
			key = static_cast<Key>(e.key.keysym.sym - 'a' + KeyA);
        }
        else {
			key = static_cast<Key>(e.key.keysym.sym);
        }
        return KeyEvent(key);
    }

    inline Key key() const { return _key; }

protected:
    Key _key;
};

}
}
