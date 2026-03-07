
#include <bg2e/app/Shortcuts.hpp>
#include <bg2e/base/PlatformTools.hpp>

#include <iostream>

namespace bg2e::app
{

void Shortcuts::keyDown(const KeyEvent& evt)
{
    if (evt.isModifier())
    {
        switch (evt.key())
        {
        case KeyEvent::KeyLeftShift:
        case KeyEvent::KeyRightShift:
            _shiftModifier = true;
            break;
        case KeyEvent::KeyRightAlt:
        case KeyEvent::KeyLeftAlt:
            _altModifier = true;
            break;
        case KeyEvent::KeyRightControl:
        case KeyEvent::KeyLeftControl:
            _ctrlModifier = true;
        case KeyEvent::KeyLeftSuper:
        case KeyEvent::KeyRightSuper:
            _cmdModifier = true;
        default:
            break;
        }
    }

}

void Shortcuts::keyUp(const KeyEvent& evt)
{
    if (evt.isModifier())
    {
        switch (evt.key())
        {
        case KeyEvent::KeyLeftShift:
        case KeyEvent::KeyRightShift:
            _shiftModifier = false;
        case KeyEvent::KeyRightAlt:
        case KeyEvent::KeyLeftAlt:
            _altModifier = false;
            break;
        case KeyEvent::KeyRightControl:
        case KeyEvent::KeyLeftControl:
            _ctrlModifier = false;
        case KeyEvent::KeyLeftSuper:
        case KeyEvent::KeyRightSuper:
            _cmdModifier = false;
        default:
            break;
        }
    }
    for (const auto & sc : _shortcutData)
    {
        if (matchShortcut(evt.key(), sc))
        {
            sc.handler();
        }
    }
}

void Shortcuts::addShortcutMapper(const ShortcutData & shortcut)
{
    _shortcutData.push_back(shortcut);
}

bool Shortcuts::matchShortcut(const KeyEvent::Key currentKey, const ShortcutData & sc) const
{
    return currentKey == sc.key &&
        _altModifier == sc.altModifier &&
        _shiftModifier == sc.shiftModifier &&
#ifdef BG2E_IS_MAC
            (sc.cmdOrCtrlModifier == _cmdModifier || sc.cmdModifier == _cmdModifier) &&
#endif
        (sc.cmdOrCtrlModifier == _ctrlModifier || _ctrlModifier == sc.ctrlModifier);
}

}
