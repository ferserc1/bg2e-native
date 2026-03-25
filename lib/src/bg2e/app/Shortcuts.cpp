
#include <bg2e/app/Shortcuts.hpp>
#include <bg2e/base/PlatformTools.hpp>

#include <iostream>
#include <sstream>

namespace bg2e::app
{
std::string Shortcuts::ShortcutData::getShortcutString() const
{
    std::stringstream ss;

    if (key != KeyEvent::KeyUnknown)
    {
        std::string sep;
        if (altModifier)
        {
            ss << "Alt";
            sep = "+";
        }
        if (shiftModifier)
        {
            ss << sep + "Shift";
            sep = "+";
        }
        if (ctrlModifier)
        {
            ss << sep << "Ctrl";
            sep = "+";
        }
        ss << sep << KeyEvent::keyName(key);
    }

    return ss.str();
}

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
            break;
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
            break;
        default:
            break;
        }
    }
    else
    {
        for (const auto & sc : _shortcutData)
        {
            if (matchShortcut(evt.key(), sc))
            {
                sc.handler();
            }
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
        sc.ctrlModifier == _ctrlModifier;
}

}
