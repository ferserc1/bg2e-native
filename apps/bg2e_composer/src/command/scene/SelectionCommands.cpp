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

#include "SelectionCommands.hpp"
#include <algorithm>

namespace bg2e_composer::command::scene {
using namespace bg2e::app;

SetSelection::SetSelection(std::weak_ptr<StageScene> stage,
    std::weak_ptr<bg2e::manipulation::SelectionManager> selection,
    std::weak_ptr<bg2e::scene::Node> node, bool additive,
    std::optional<bg2e::manipulation::SelectionItem> item, bool clear)
    : _stage(std::move(stage)), _selection(std::move(selection)), _node(std::move(node)),
      _additive(additive), _item(std::move(item))
{
    _clear = clear;
}

const CommandInfo& SetSelection::info() const noexcept
{
    return _info;
}

CommandMemory SetSelection::memory() const
{
    size_t entries = _before.items.capacity() + _before.nodes.capacity() +
        _after.items.capacity() + _after.nodes.capacity() + 4;
    if (auto selection = _selection.lock())
    {
        entries += 4 * (selection->selectedItems().size() + selection->selectedNodes().size());
    }
    return {sizeof(*this) + 1024 + entries * (sizeof(bg2e::manipulation::SelectionItem) +
        sizeof(std::weak_ptr<bg2e::scene::Node>)), {}};
}

CommandResult SetSelection::execute() noexcept
{
    try
    {
        auto stage = _stage.lock();
        auto selection = _selection.lock();
        auto node = _node.lock();
        if (!stage || !selection || (!_clear && !node) || (node && !stage->contains(node.get()) && node != stage->fillLightNode()) || (_item && !_item->lockMesh()))
        {
            return {CommandStatus::Rejected, "Selection target is stale"};
        }
        _before = selection->captureSelection();
        _after = _additive ? _before : bg2e::manipulation::SelectionState{};
        if (node)
        {
            bool selected = false;
            if (_item)
            {
                selected = std::any_of(_after.items.begin(), _after.items.end(), [&](const auto& item)
                {
                    return item.node.lock() == node && item.mesh.lock() == _item->mesh.lock() && item.submesh == _item->submesh;
                });
                if (selected)
                {
                    std::erase_if(_after.items, [&](const auto& item)
                    {
                        return item.node.lock() == node && item.mesh.lock() == _item->mesh.lock() && item.submesh == _item->submesh;
                    });
                }
                else
                {
                    _after.items.push_back(*_item);
                }
            }
            else
            {
                selected = std::any_of(_after.nodes.begin(), _after.nodes.end(), [&](const auto& weak)
                {
                    return weak.lock() == node;
                });
                if (selected)
                {
                    std::erase_if(_after.items, [&](const auto& item)
                    {
                        return item.node.lock() == node;
                    });
                }
            }
            const bool otherItems = std::any_of(_after.items.begin(), _after.items.end(), [&](const auto& item)
            {
                return item.node.lock() == node;
            });
            std::erase_if(_after.nodes, [&](const auto& weak)
            {
                return weak.lock() == node;
            });
            if (!selected || otherItems)
            {
                _after.nodes.push_back(node);
            }
        }
        bool same = _before.nodes.size() == _after.nodes.size() && _before.items.size() == _after.items.size();
        for (size_t i = 0; same && i < _before.nodes.size(); ++i)
        {
            same = _before.nodes[i].lock() == _after.nodes[i].lock();
        }
        for (size_t i = 0; same && i < _before.items.size(); ++i)
        {
            const auto& a = _before.items[i];
            const auto& b = _after.items[i];
            same = a.node.lock() == b.node.lock() && a.mesh.lock() == b.mesh.lock() && a.submesh == b.submesh;
        }
        if (same)
        {
            return {CommandStatus::NoOp, {}};
        }
        const auto result = assign(_after);
        _captured = result.accepted();
        return result;
    }
    catch (...)
    {
        return {CommandStatus::Failed, "Could not prepare selection"};
    }
}

CommandResult SetSelection::assign(const bg2e::manipulation::SelectionState& state) noexcept
{
    auto stage = _stage.lock();
    auto selection = _selection.lock();
    if (!stage || !selection)
    {
        return {CommandStatus::Rejected, "Selection owner is stale"};
    }
    bg2e::manipulation::SelectionState previous;
    try
    {
        previous = selection->captureSelection();
    }
    catch (...)
    {
        return {CommandStatus::Failed, "Cannot reserve selection recovery"};
    }
    try
    {
        selection->restoreSelection(state, [stage](auto* node)
        {
            return stage->contains(node) || node == stage->fillLightNode().get();
        });
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        try
        {
            selection->restoreSelection(previous, [stage](auto* node)
            {
                return stage->contains(node) || node == stage->fillLightNode().get();
            });
        }
        catch (...)
        {
            return {CommandStatus::Faulted, "Selection recovery failed"};
        }
        return {CommandStatus::Failed, "Selection restored after a failed notification"};
    }
}

CommandResult SetSelection::undo() noexcept
{
    return _captured ? assign(_before) : CommandResult{CommandStatus::Rejected, "No selection snapshot"};
}

CommandResult SetSelection::redo() noexcept
{
    return _captured ? assign(_after) : CommandResult{CommandStatus::Rejected, "No selection snapshot"};
}

}
