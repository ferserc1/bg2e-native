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

#include <bg2e/ui/ActionDefinition.hpp>
#include <bg2e/ui/UserInterface.hpp>

namespace bg2e::ui {

std::string ActionDefinition::displayLabel() const
{
    return dynamicLabel ? dynamicLabel() : label;
}

bool ActionDefinition::isEnabled() const
{
    return execute && (!enabled || enabled());
}

bool ActionDefinition::isChecked() const
{
    return checked && checked();
}

void ActionDefinition::invoke() const
{
    if (isEnabled())
    {
        execute();
    }
}

void ActionDefinition::registerShortcut(app::Shortcuts& shortcuts,
    const std::shared_ptr<const ActionDefinition>& action)
{
    if (!action || action->shortcut.key == app::KeyEvent::KeyUnknown)
    {
        return;
    }

    auto shortcut = action->shortcut;
    shortcut.handler = [weakAction = std::weak_ptr<const ActionDefinition>(action),
                           allowText = shortcut.allowInTextInput]
    {
        if (const auto definition = weakAction.lock(); definition &&
            (allowText || !UserInterface::textInputActive()))
        {
            definition->invoke();
        }
    };
    shortcuts.addShortcutMapper(shortcut);
}

}
