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

#include <bg2e/app/CompositeCommand.hpp>

#include <limits>
#include <stdexcept>
#include <utility>

namespace bg2e::app {

CompositeCommand::CompositeCommand(CommandInfo info, std::vector<std::unique_ptr<Command>> children)
    : _info(std::move(info)), _children(std::move(children)), _effective(_children.size(), 0)
{
    if (!_info.undoable)
    {
        throw std::invalid_argument("Atomic composites require undo support");
    }
    for (const auto& child : _children)
    {
        if (!child || !child->info().undoable || child->info().history != _info.history ||
            child->info().category != _info.category ||
            (child->info().impact == CommandDocumentImpact::Document &&
                _info.impact != CommandDocumentImpact::Document))
        {
            throw std::invalid_argument("Composite children have incompatible policies");
        }
    }
}

CompositeCommand::~CompositeCommand() = default;

const CommandInfo& CompositeCommand::info() const noexcept
{
    return _info;
}

CommandMemory CompositeCommand::memory() const
{
    CommandMemory result;
    const auto add = [&result](size_t bytes)
    {
        if (bytes > std::numeric_limits<size_t>::max() - result.localBytes)
        {
            throw std::length_error("Composite retained size overflow");
        }
        result.localBytes += bytes;
    };
    add(sizeof(*this));
    add(_info.name.capacity());
    add(_info.category.capacity());
    add(_effective.capacity());
    add(_children.capacity() * sizeof(std::unique_ptr<Command>));
    for (const auto& child : _children)
    {
        auto memory = child->memory();
        add(memory.localBytes);
        result.resources.insert(result.resources.end(), memory.resources.begin(), memory.resources.end());
    }
    return result;
}

CommandResult CompositeCommand::apply(bool replay) noexcept
{
    bool changed = false;
    for (size_t i = 0; i < _children.size(); ++i)
    {
        if (replay && !_effective[i])
        {
            continue;
        }
        const auto result = replay ? _children[i]->redo() : _children[i]->execute();
        if (result.status == CommandStatus::Applied)
        {
            if (!replay)
            {
                _effective[i] = 1;
            }
            changed = true;
            continue;
        }
        if (!replay && result.status == CommandStatus::NoOp)
        {
            _effective[i] = 0;
            continue;
        }
        bool fault = result.status == CommandStatus::Faulted;
        for (size_t j = i; j > 0; --j)
        {
            if (_effective[j - 1])
            {
                const auto restored = _children[j - 1]->undo();
                if (restored.status != CommandStatus::Applied)
                {
                    fault = true;
                }
                if (!replay)
                {
                    _effective[j - 1] = 0;
                }
            }
        }
        if (fault)
        {
            return {CommandStatus::Faulted, "Composite apply recovery failed"};
        }
        return result.status == CommandStatus::NoOp
            ? CommandResult{CommandStatus::Failed, "Effective composite child did not replay"} : result;
    }
    _applied = changed;
    return {changed ? CommandStatus::Applied : CommandStatus::NoOp, {}};
}

CommandResult CompositeCommand::execute() noexcept
{
    if (_applied)
    {
        return {CommandStatus::Rejected, "Composite is already applied"};
    }
    return apply(false);
}

CommandResult CompositeCommand::undo() noexcept
{
    if (!_applied)
    {
        return {CommandStatus::Rejected, "Composite is not applied"};
    }
    for (size_t i = _children.size(); i > 0; --i)
    {
        if (!_effective[i - 1])
        {
            continue;
        }
        const auto result = _children[i - 1]->undo();
        if (result.status == CommandStatus::Applied)
        {
            continue;
        }
        bool fault = result.status == CommandStatus::Faulted;
        // Children undone so far form a suffix. Recover in forward order.
        for (size_t j = i; j < _children.size(); ++j)
        {
            if (_effective[j] && _children[j]->redo().status != CommandStatus::Applied)
            {
                fault = true;
            }
        }
        if (fault)
        {
            return {CommandStatus::Faulted, "Composite undo recovery failed"};
        }
        return result.status == CommandStatus::NoOp
            ? CommandResult{CommandStatus::Failed, "Effective composite child did not undo"} : result;
    }
    _applied = false;
    return {CommandStatus::Applied, {}};
}

CommandResult CompositeCommand::redo() noexcept
{
    if (_applied)
    {
        return {CommandStatus::Rejected, "Composite is already applied"};
    }
    return apply(true);
}

}
