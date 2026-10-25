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

#include <bg2e/plugin/PluginComponentsVisitor.hpp>
#include <bg2e/scene/Node.hpp>

#include <algorithm>
#include <utility>

namespace bg2e::plugin {

PluginComponentsVisitor::PluginComponentsVisitor(std::vector<std::string> types)
    : _types(std::move(types))
{
    setIgnoreDisabled(false);
}

void PluginComponentsVisitor::visit(scene::Node* node)
{
    for (const auto& [key, component] : node->components())
    {
        const auto type = component->typeName();
        if (std::find(_types.begin(), _types.end(), type) != _types.end())
        {
            _entries.push_back({node->shared_from_this(), component, type});
        }
    }
}

const std::vector<PluginComponentsVisitor::Entry>& PluginComponentsVisitor::entries() const
{
    return _entries;
}

size_t PluginComponentsVisitor::removeComponents()
{
    size_t removed = 0;
    for (auto& entry : _entries)
    {
        if (entry.component->ownerNode() == entry.node.get())
        {
            entry.node->removeComponent(entry.component);
            ++removed;
        }
    }
    _entries.clear();
    return removed;
}

}
