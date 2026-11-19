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

#include <bg2e/ui/Window.hpp>
#include <bg2e/draw/DrawableUsage.hpp>
#include <bg2e/math/glm.hpp>
#include <filesystem>
#include <functional>
#include <vector>

namespace bg2e::ui {

// Read-only host snapshots: no native objects, owning cache references or usage
// leases are retained merely because the browser is open.
struct DrawableBrowserEntry {
    draw::DrawableResourceHandle handle;
    std::string label;
    std::filesystem::path source, exportPath;
    draw::DrawableUsage usage;
    uint64_t revision = 0;
    bool unloadAvailable = false;
    std::string unloadReason;
    std::string scope;
};

struct DrawableSubmeshInspection {
    std::string name, group, material;
    std::vector<std::string> images;
    glm::mat4 transform{1};
    bool visible = true;
};

struct DrawableInstanceInspection {
    std::string nodeName;
    draw::DrawableResourceHandle handle;
    bool owner = false, instance = false, removable = false, tracing = false;
    glm::mat4 transform{1};
    std::vector<DrawableSubmeshInspection> submeshes;
};

struct DrawableBrowserSnapshot {
    uint64_t generation = 0;
    std::vector<DrawableBrowserEntry> entries;
    draw::DrawableResourceHandle preferred;
    DrawableInstanceInspection selection;
};

class BG2E_API DrawableInstancesWindow : public Window {
  public:
    DrawableInstancesWindow();

    // UI callbacks enqueue host IO/commands. The host performs all cache and
    // resource mutations at its stopped resource-update boundary.
    void configure(std::function<const DrawableBrowserSnapshot*()> snapshot,
        std::function<bool()> available, std::function<void()> load,
        std::function<void(draw::DrawableResourceHandle)> unload,
        std::function<void(draw::DrawableResourceHandle)> create,
        std::function<void()> createFromOwner,
        std::function<void(draw::DrawableResourceHandle)> rebind,
        std::function<void()> remove, std::function<std::string()> status);
    void draw() override;

  private:
    std::function<const DrawableBrowserSnapshot*()> _snapshot;
    std::function<bool()> _available;
    std::function<void()> _load, _createFromOwner, _remove;
    std::function<void(draw::DrawableResourceHandle)> _unload, _create, _rebind;
    std::function<std::string()> _status;
    draw::DrawableResourceHandle _selected, _preferred;
    uint64_t _generation = UINT64_MAX;

    void drawContents();
};

}
