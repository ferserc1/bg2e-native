#pragma once

#include <bg2e/common.hpp>
#include <bg2e/app/KeyEvent.hpp>

#include <functional>
#include <unordered_map>

namespace bg2e::app
{

class BG2E_API Shortcuts {
public:
    typedef std::function<void(const KeyEvent&)> ShortcutHandler;

    struct ShortcutData
    {
        bool altModifier = false;
        bool ctrlModifier = false;
        bool shiftModifier = false;
    };

    void keyDown(const KeyEvent& evt) const;
    void keyUp(const KeyEvent& evt) const;

    void addShortcutMapper(const KeyEvent::Key key, const ShortcutHandler& handler);

protected:
    std::unordered_map<KeyEvent::Key, ShortcutHandler> _keyMap;
};

}
