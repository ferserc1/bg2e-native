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

#include <bg2e/ui/NodeEditor.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Value.hpp>

namespace bg2e::ui {

void NodeEditor::setNode(scene::Node * node)
{
    _node = node;
    _selectionCount = node ? 1 : 0;
    _componentInspector.setNode(node);
}

void NodeEditor::setNodes(const std::vector<scene::Node*>& nodes)
{
    if (nodes.size() == 1)
    {
        _node = nodes.front();
        _selectionCount = 1;
    }
    else
    {
        _node = nullptr;
        _selectionCount = nodes.size();
    }
    _componentInspector.setNode(_node);
}

void NodeEditor::draw()
{
    if (_selectionCount > 1)
    {
        Text::text("<multiple_selection>");
        return;
    }

    if (!_node)
    {
        Text::text("No selection");
        return;
    }

    auto name = _node->name();
    auto enabled = _node->enabled();
    if (Value::text("Node name", name))
    {
        _node->setName(name);
    }
    if (Button::checkBox("Enabled", &enabled))
    {
        _node->setEnabled(enabled);
    }

    _componentInspector.draw();
}

void NodeEditor::notifyChanged() const
{
    if (_onChanged)
    {
        _onChanged();
    }
}

}
