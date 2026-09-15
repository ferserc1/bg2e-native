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
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/reflection/Registry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Component.hpp>

#include <memory>

namespace bg2e {
namespace ui {

void ComponentInspector::draw()
{
    if (!_node)
    {
        BasicWidgets::text("No selection");
        return;
    }

    // Header: node name + "Add Component" stub (extension point, no
    // implementation yet)
    BasicWidgets::text(_node->name());
    BasicWidgets::button("Add Component");
    BasicWidgets::separator("Components");

    std::shared_ptr<scene::Component> pendingRemove;
    bool changed = false;
    int id = 0;

    for (auto & comp : _node->orderedComponents())
    {
        BasicWidgets::pushId(id++);

        const auto * typeInfo = reflection::TypeRegistry::get().type(comp->typeName());
        const auto headerTitle = typeInfo && !typeInfo->displayName.empty()
            ? typeInfo->displayName
            : comp->typeName();

        if (BasicWidgets::collapsingHeader(headerTitle))
        {
            // Right-aligned remove button on the header row
            auto removeWidth = static_cast<int32_t>(
                BasicWidgets::calcButtonWidth("Remove") + BasicWidgets::getItemHorizontalSpacing()
            );
            BasicWidgets::sameLine(-removeWidth);
            if (BasicWidgets::button("Remove"))
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
                BasicWidgets::text("No reflection data for '" + comp->typeName() + "'");
            }
        }

        BasicWidgets::popId();
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
