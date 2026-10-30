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

namespace bg2e_composer::command::geometry {

enum class Operation { ZUpToYUp, Center, PutOnFloor, CentimetersToMeters, MetersToCentimeters };

// Whole-mesh edits affect every document instance sharing the CPU Mesh. Each
// shared CPU payload is transformed once and every live raster owner is rebuilt.
class GeometryEdit : public bg2e::app::Command {
  public:
    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;
    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  protected:
    using Mesh = bg2e::draw::Drawable::Mesh;
    GeometryEdit(const std::shared_ptr<StageScene>& stage,
        const std::vector<bg2e::manipulation::SelectionItem>& targets, Operation operation, std::function<void()> stopProducers = {});
    GeometryEdit(const std::shared_ptr<StageScene>& stage,
        const bg2e::manipulation::SelectionItem& target, const Mesh& prepared, std::function<void()> stopProducers = {}, std::function<bool()> sourceValid = {});

  private:
    struct Group {
        std::weak_ptr<Mesh> mesh;
        size_t capacityBound = 0;
        glm::mat4 forward{1}, inverse{1};
        glm::mat3 directions{1}, inverseDirections{1};
        std::shared_ptr<const Mesh> before, after;
    };
    struct Target {
        std::weak_ptr<bg2e::scene::Node> node;
        std::weak_ptr<bg2e::draw::Drawable> drawable;
        size_t group = 0;
    };
    std::weak_ptr<StageScene> _stage;
    std::function<void()> _stopProducers;
    std::function<bool()> _sourceValid;
    std::vector<Group> _groups;
    std::vector<Target> _targets;
    Operation _operation = Operation::Center;
    bool _replacement = false;
    bool _captured = false;
    bool _applied = false;
    bg2e::app::CommandInfo _info;

    void captureTargets(const std::shared_ptr<StageScene>& stage,
        const std::vector<bg2e::manipulation::SelectionItem>& targets);
    void validate() const;
    bg2e::app::CommandResult apply(bool forward, bool initial) noexcept;
};

class TransformGeometry final : public GeometryEdit {
  public:
    TransformGeometry(const std::shared_ptr<StageScene>& stage,
        const std::vector<bg2e::manipulation::SelectionItem>& targets, Operation operation, std::function<void()> stopProducers = {});
};

// Prepared CPU result adapter for information-losing edits. It retains old/new
// data; vertex/index counts may change, but submesh count/identity stays fixed.
// Import/UV2 producers opt in separately; generating their results stays outside.
class ReplaceGeometry final : public GeometryEdit {
  public:
    ReplaceGeometry(const std::shared_ptr<StageScene>& stage,
        const bg2e::manipulation::SelectionItem& target, const bg2e::draw::Drawable::Mesh& prepared, std::function<void()> stopProducers = {}, std::function<bool()> sourceValid = {});
};

}
