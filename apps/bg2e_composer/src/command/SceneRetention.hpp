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

#include <bg2e/app/Command.hpp>
#include <bg2e/scene/Node.hpp>

namespace bg2e_composer {
class StageScene;
namespace command {

// Stage-owned retirement lease. Commands keep the second reference until
// eviction/discard; only StageScene releases detached native data, after a drain.
class SceneRetention {
  private:
    std::shared_ptr<void> _registryLease; // Outlives all retained asset/control blocks.

  public:
    explicit SceneRetention(std::shared_ptr<bg2e::scene::Node> node);
    explicit SceneRetention(std::shared_ptr<bg2e::scene::Component> component);

    std::shared_ptr<bg2e::scene::Node> node;
    std::shared_ptr<bg2e::scene::Component> component;
    size_t bytes = 0;

    void refreshBudget();
    bool detached() const;
    void release(StageScene& stage);
};

bool supportedComponent(const bg2e::scene::Component& component);
void requireSupportedTree(const std::shared_ptr<bg2e::scene::Node>& node);

}
}
