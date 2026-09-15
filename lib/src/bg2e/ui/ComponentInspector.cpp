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

#include <bg2e/ui/ComponentInspector.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/reflection/Registry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Component.hpp>
#include <bg2e/ui/Layout.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Group.hpp>
#include <bg2e/ui/Button.hpp>

#include <memory>

namespace bg2e {
namespace ui {

void ComponentInspector::draw()
{
    if (!_node)
    {
        Text::text("No selection");
        return;
    }

    // Header: node name + "Add Component" stub (extension point, no
    // implementation yet)
    Text::text(_node->name());
    Button::button("Add Component");
    Text::separator("Components");

    std::shared_ptr<scene::Component> pendingRemove;
    bool changed = false;
    int id = 0;

    for (auto & comp : _node->orderedComponents())
    {
        Group::pushId(id++);

        const auto * typeInfo = reflection::TypeRegistry::get().type(comp->typeName());
        const auto headerTitle = typeInfo && !typeInfo->displayName.empty()
            ? typeInfo->displayName
            : comp->typeName();

        if (Group::collapsingHeader(headerTitle))
        {
            // Right-aligned remove button on the header row
            auto removeWidth = static_cast<int32_t>(
                Layout::calcButtonWidth("Remove") + Layout::getItemHorizontalSpacing()
            );
            Layout::sameLine(-removeWidth);
            if (Button::button("Remove"))
            {
                // Defer the removal until the iteration is finished
                pendingRemove = comp;
            }

            if (typeInfo)
            {
                if (ReflectionWidget::drawProperties(comp.get(), *typeInfo))
                {
                    changed = true;
                }
                ReflectionWidget::drawActions(comp.get(), *typeInfo);
            }
            else
            {
                Text::text("No reflection data for '" + comp->typeName() + "'");
            }
        }

        Group::popId();
    }

    if (pendingRemove)
    {
        _node->removeComponent(pendingRemove);
        changed = true;
    }

    if (changed)
    {
        notifyChanged();
    }
}

void ComponentInspector::notifyChanged() const
{
    if (_onChanged)
    {
        _onChanged();
    }
}

}
}
