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

#include <bg2e/common.hpp>
#include <bg2e/app/KeyEvent.hpp>

#include <functional>
#include <vector>
#include <cstdint>

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
        KeyEvent::Key key = KeyEvent::KeyUnknown;
        ShortcutHandler handler;

        std::string getShortcutString() const;
    };

    void keyDown(const KeyEvent& evt);
    void keyUp(const KeyEvent& evt);

    void addShortcutMapper(const ShortcutData & shortcut);

protected:
    std::vector<ShortcutData> _shortcutData;

    bool _altModifier = false;
    bool _ctrlModifier = false;
    bool _shiftModifier = false;

    bool matchShortcut(KeyEvent::Key currentKey, const ShortcutData & sc) const;
};

}
