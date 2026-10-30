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

#include "SceneSettingsCommand.hpp"

#include <limits>
#include <stdexcept>

namespace bg2e_composer::command {
namespace {
class SettingsChild final : public bg2e::app::Command {
  public:
    explicit SettingsChild(std::unique_ptr<bg2e::app::Command> child) : _child(std::move(child)), _info(_child->info())
    {
        if (!_info.undoable || _info.history != bg2e::app::CommandHistoryPolicy::Always ||
            _info.impact != bg2e::app::CommandDocumentImpact::Document)
        {
            throw std::invalid_argument("Scene settings require reversible authored adapters");
        }
        _info.category = "scene.settings";
    }

    const bg2e::app::CommandInfo& info() const noexcept override
    {
        return _info;
    }
    bg2e::app::CommandMemory memory() const override
    {
        auto result = _child->memory();
        if (result.localBytes > std::numeric_limits<size_t>::max() - 4096)
        {
            throw std::length_error("Scene settings memory overflow");
        }
        result.localBytes += 4096;
        return result;
    }

    bg2e::app::CommandResult execute() noexcept override
    {
        return _child->execute();
    }
    bg2e::app::CommandResult undo() noexcept override
    {
        return _child->undo();
    }
    bg2e::app::CommandResult redo() noexcept override
    {
        return _child->redo();
    }

  private:
    std::unique_ptr<bg2e::app::Command> _child;
    bg2e::app::CommandInfo _info;
};
}

SceneSettingsCommand::SceneSettingsCommand(std::vector<std::unique_ptr<bg2e::app::Command>> children, std::function<bool()> valid)
    : _valid(std::move(valid))
{
    std::vector<std::unique_ptr<bg2e::app::Command>> adapters;
    for (auto& child : children)
    {
        adapters.push_back(std::make_unique<SettingsChild>(std::move(child)));
    }
    _batch = std::make_unique<bg2e::app::CompositeCommand>(
        bg2e::app::CommandInfo{"Edit scene graphics settings", "scene.settings"}, std::move(adapters));
}

const bg2e::app::CommandInfo& SceneSettingsCommand::info() const noexcept
{
    return _batch->info();
}

bg2e::app::CommandMemory SceneSettingsCommand::memory() const
{
    auto result = _batch->memory();
    if (result.localBytes > std::numeric_limits<size_t>::max() - 4096)
    {
        throw std::length_error("Scene settings memory overflow");
    }
    result.localBytes += 4096;
    return result;
}

bg2e::app::CommandResult SceneSettingsCommand::execute() noexcept
{
    try
    {
        if (!_valid())
        {
            return {bg2e::app::CommandStatus::Rejected, "Scene settings source or catalog changed"};
        }
        return _batch->execute();
    }
    catch (...)
    {
        return {bg2e::app::CommandStatus::Rejected, "Scene settings validation failed"};
    }
}

bg2e::app::CommandResult SceneSettingsCommand::undo() noexcept
{
    return _batch->undo();
}

bg2e::app::CommandResult SceneSettingsCommand::redo() noexcept
{
    return _batch->redo();
}

}
