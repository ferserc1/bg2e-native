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

#include "NodeCommands.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bg2e_composer::command::scene {
using namespace bg2e::app;

RenameNode::RenameNode(std::weak_ptr<bg2e::scene::Node> target, std::string name)
    : _target(std::move(target)), _after(std::move(name))
{
}

const CommandInfo& RenameNode::info() const noexcept
{
    return _info;
}

CommandMemory RenameNode::memory() const
{
    const auto node = _target.lock();
    const auto beforeBytes = node ? node->name().capacity() : _before.capacity();
    return {sizeof(*this) + std::max(beforeBytes, _before.capacity()) + _after.capacity(), {}};
}

CommandResult RenameNode::execute() noexcept
{
    auto node = _target.lock();
    if (!node)
    {
        return {CommandStatus::Rejected, "Rename target no longer exists"};
    }
    try
    {
        // Capture only at execution, after the dispatch guard validated identity.
        _before = node->name();
        _captured = true;
    }
    catch (...)
    {
        return {CommandStatus::Failed, "Could not capture node name"};
    }
    return assign(_after);
}

CommandResult RenameNode::assign(const std::string& name) noexcept
{
    auto node = _target.lock();
    if (!node)
    {
        return {CommandStatus::Rejected, "Rename target no longer exists"};
    }
    if (node->name() == name)
    {
        return {};
    }
    try
    {
        // std::string assignment preserves the old value on allocation failure.
        node->setName(name);
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        return {CommandStatus::Failed, "Node name assignment failed"};
    }
}

CommandResult RenameNode::undo() noexcept
{
    return _captured ? assign(_before) : CommandResult{CommandStatus::Rejected, "Rename has not executed"};
}

CommandResult RenameNode::redo() noexcept
{
    return _captured ? assign(_after) : CommandResult{CommandStatus::Rejected, "Rename has not executed"};
}

SetNodeEnabled::SetNodeEnabled(std::weak_ptr<StageScene> stage,
    std::weak_ptr<bg2e::scene::Node> target, bool enabled)
    : _stage(std::move(stage)), _target(std::move(target)), _after(enabled)
{
}

const CommandInfo& SetNodeEnabled::info() const noexcept
{
    return _info;
}

CommandMemory SetNodeEnabled::memory() const
{
    auto stage = _stage.lock();
    const auto bytes = stage ? stage->editingStateBytes() : 0;
    if (bytes > (std::numeric_limits<size_t>::max() - sizeof(*this)) / 4)
    {
        throw std::length_error("Enabled-state snapshot size overflow");
    }
    return {sizeof(*this) + 4 * bytes, {}};
}

CommandResult SetNodeEnabled::execute() noexcept
{
    auto stage = _stage.lock();
    auto node = _target.lock();
    if (!stage || !node)
    {
        return {CommandStatus::Rejected, "Enabled-state target no longer exists"};
    }
    if (node->enabled() == _after)
    {
        return {};
    }
    try
    {
        _beforeState = stage->editingState();
        _before = node->enabled();
        _captured = true;
    }
    catch (...)
    {
        return {CommandStatus::Failed, "Could not capture scene references"};
    }
    node->setEnabled(_after);
    try
    {
        stage->validateEditingGraph(false);
        stage->refreshCamera(true); // Never replace retained controller identities.
        if (node->enabled() != _after)
        {
            throw std::invalid_argument("The requested enabled state exceeds the light limit");
        }
        _afterState = stage->editingState();
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        node->setEnabled(_before);
        try
        {
            stage->restoreEditingState(_beforeState);
        }
        catch (...)
        {
            return {CommandStatus::Faulted, "Enabled-state recovery failed"};
        }
        return {CommandStatus::Failed, "Enabled state violates scene constraints or preparation failed"};
    }
}

CommandResult SetNodeEnabled::restore(bool enabled, const StageScene::EditingState& state) noexcept
{
    auto stage = _stage.lock();
    auto node = _target.lock();
    if (!_captured || !stage || !node)
    {
        return {CommandStatus::Rejected, "Enabled-state snapshot is stale"};
    }
    const auto previous = node->enabled();
    StageScene::EditingState recovery;
    try
    {
        recovery = stage->editingState();
    }
    catch (...)
    {
        return {CommandStatus::Failed, "Could not prepare enabled-state restoration"};
    }
    node->setEnabled(enabled);
    try
    {
        stage->validateEditingGraph(false);
        stage->restoreEditingState(state);
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        node->setEnabled(previous);
        try
        {
            stage->restoreEditingState(recovery);
        }
        catch (...)
        {
            return {CommandStatus::Faulted, "Enabled-state replay recovery failed"};
        }
        return {CommandStatus::Failed, "Could not restore enabled state"};
    }
}

CommandResult SetNodeEnabled::undo() noexcept
{
    return restore(_before, _beforeState);
}

CommandResult SetNodeEnabled::redo() noexcept
{
    return restore(_after, _afterState);
}

}
