#pragma once

#include <bg2e/common.hpp>
#include <bg2e/app/KeyEvent.hpp>

#include <functional>
#include <vector>

namespace bg2e::app
{

class BG2E_API Shortcuts {
public:
    typedef std::function<void()> ShortcutHandler;

    struct ShortcutData
    {
        bool altModifier = false;
        bool ctrlModifier = false; // In Windows/Linux is better to use cmdOrCtrlModifier instead.
        bool shiftModifier = false;
        bool cmdModifier = false; // macOS only
        bool cmdOrCtrlModifier = false; // ctrl in Windows/Linux or cmd in macOS
        KeyEvent::Key key = KeyEvent::KeyUnknown;
        ShortcutHandler handler;
    };

    void keyDown(const KeyEvent& evt);
    void keyUp(const KeyEvent& evt);

    void addShortcutMapper(const ShortcutData & shortcut);

protected:
    std::vector<ShortcutData> _shortcutData;

    bool _altModifier = false;
    bool _ctrlModifier = false;
    bool _shiftModifier = false;
    bool _cmdModifier = false;

    bool matchShortcut(KeyEvent::Key currentKey, const ShortcutData & sc) const;
};

}
