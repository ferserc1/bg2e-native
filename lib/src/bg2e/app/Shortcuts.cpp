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
