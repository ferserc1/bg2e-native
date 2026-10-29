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

#include "../../StageScene.hpp"
#include <bg2e/manipulation/SelectionManager.hpp>

namespace bg2e_composer::command::scene {

enum class MetadataField { Name, Group, Visibility };
struct MetadataRequest {
    std::vector<bg2e::manipulation::SelectionItem> targets;
    MetadataField field = MetadataField::Name;
    std::string text;
    bool visible = true;
};

class EditMetadata final : public bg2e::app::Command {
  public:
    EditMetadata(std::weak_ptr<StageScene> stage, MetadataRequest request);

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;
    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    struct Value { std::string text; bool visible = true; };
    std::weak_ptr<StageScene> _stage;
    MetadataRequest _request;
    std::vector<Value> _before, _after;
    bool _captured = false;
    bg2e::app::CommandInfo _info{"Edit submesh metadata", "scene.metadata"};

    Value read(const bg2e::manipulation::SelectionItem& item) const;
    void write(const bg2e::manipulation::SelectionItem& item, const Value& value) const;
    bg2e::app::CommandResult assign(const std::vector<Value>& values) noexcept;
};

}
