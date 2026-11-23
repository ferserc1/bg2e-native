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

#include <bg2e/ui/DrawableInstancesWindow.hpp>
#include <imgui.h>
#include <algorithm>
#include <utility>

namespace bg2e::ui {
namespace {

void matrix(const glm::mat4& value)
{
    for (int row = 0; row < 4; ++row)
    {
        ImGui::Text("% .4f  % .4f  % .4f  % .4f", value[0][row], value[1][row], value[2][row], value[3][row]);
    }
}

}

DrawableInstancesWindow::DrawableInstancesWindow()
{
    setTitle("Drawable Instances");
    setSize(780, 720);
    close();
}


void DrawableInstancesWindow::configure(std::function<const DrawableBrowserSnapshot*()> snapshot,
    std::function<bool()> available, std::function<void()> load,
    std::function<void(draw::DrawableResourceHandle)> unload,
    std::function<void(draw::DrawableResourceHandle)> create, std::function<void()> createFromOwner,
    std::function<void(draw::DrawableResourceHandle)> rebind, std::function<void()> remove,
    std::function<std::string()> status)
{
    _snapshot = std::move(snapshot);
    _available = std::move(available);
    _load = std::move(load);
    _unload = std::move(unload);
    _create = std::move(create);
    _createFromOwner = std::move(createFromOwner);
    _rebind = std::move(rebind);
    _remove = std::move(remove);
    _status = std::move(status);
}


void DrawableInstancesWindow::draw()
{
    Window::draw([this]
    {
        drawContents();
    });
}


void DrawableInstancesWindow::drawContents()
{
    const auto* snapshot = _snapshot ? _snapshot() : nullptr;
    const bool available = _available && _available();
    ImGui::TextWrapped("Load a model once, then create instances with shared geometry and independent appearance. Loading or unloading the cache does not modify the scene or its undo history.");
    ImGui::BeginDisabled(!available || !_load);
    if (ImGui::Button("Load BG2 into cache..."))
    {
        _load();
    }
    ImGui::EndDisabled();
    if (_status)
    {
        ImGui::TextWrapped("%s", _status().c_str());
    }
    if (!snapshot)
    {
        return;
    }
    if (_generation != snapshot->generation)
    {
        _generation = snapshot->generation;
        _selected = {};
        _preferred = {};
    }
    if (snapshot->preferred.valid() && !(_preferred == snapshot->preferred))
    {
        _preferred = snapshot->preferred;
        _selected = _preferred;
    }
    if (std::none_of(snapshot->entries.begin(), snapshot->entries.end(), [&](const auto& entry)
    {
        return entry.handle == _selected;
    }))
    {
        _selected = snapshot->entries.empty() ? draw::DrawableResourceHandle{} : snapshot->entries.front().handle;
    }
    ImGui::Text("%zu resident resources", snapshot->entries.size());
    ImGui::BeginChild("Resident drawables", ImVec2(0, 180), true);
    for (size_t i = 0; i < snapshot->entries.size(); ++i)
    {
        const auto& entry = snapshot->entries[i];
        ImGui::PushID(static_cast<int>(i));
        const auto label = entry.label + " (" + entry.scope + ")";
        if (ImGui::Selectable(label.c_str(), entry.handle == _selected))
        {
            _selected = entry.handle;
        }
        ImGui::TextDisabled("Owners %zu | Instances %zu | History %zu | Jobs %zu | Frames %zu",
            entry.usage.sceneOwners, entry.usage.sceneInstances, entry.usage.history, entry.usage.jobs, entry.usage.frames);
        ImGui::PopID();
    }
    ImGui::EndChild();
    const auto found = std::find_if(snapshot->entries.begin(), snapshot->entries.end(), [&](const auto& entry)
    {
        return entry.handle == _selected;
    });
    if (found != snapshot->entries.end())
    {
        ImGui::TextWrapped("Identity: %s", found->handle.identifier().c_str());
        ImGui::TextWrapped("Scope: %s", found->scope.c_str());
        ImGui::TextWrapped("Source: %s", found->source.empty() ? "Procedural geometry" : found->source.string().c_str());
        if (!found->exportPath.empty())
        {
            ImGui::TextWrapped("Export destination: %s", found->exportPath.string().c_str());
        }
        ImGui::Text("Geometry revision: %llu", static_cast<unsigned long long>(found->revision));
        ImGui::BeginDisabled(!available || !_create || !found->handle.valid());
        if (ImGui::Button("Create instance"))
        {
            _create(found->handle);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(!available || !_unload || !found->unloadAvailable);
        if (ImGui::Button("Unload resource"))
        {
            _unload(found->handle);
        }
        ImGui::EndDisabled();
        if (!found->unloadAvailable)
        {
            ImGui::TextWrapped("Cannot unload: %s", found->unloadReason.c_str());
        }
    }
    const auto& selection = snapshot->selection;
    ImGui::Separator();
    ImGui::TextWrapped("Scene selection: %s", selection.nodeName.empty() ? "Select an owner or instance in the scene" : selection.nodeName.c_str());
    ImGui::BeginDisabled(!available || !_createFromOwner || !selection.owner || !selection.handle.valid());
    if (ImGui::Button("Create instance from selected owner"))
    {
        _createFromOwner();
    }
    ImGui::EndDisabled();
    if (selection.instance)
    {
        ImGui::BeginDisabled(!available || !_rebind || found == snapshot->entries.end() || found->handle == selection.handle);
        if (ImGui::Button("Use selected resource (reset appearance)"))
        {
            _rebind(found->handle);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(!available || !_remove || !selection.removable);
        if (ImGui::Button("Delete selected instance"))
        {
            _remove();
        }
        ImGui::EndDisabled();
    }
    if (selection.owner || selection.instance)
    {
        ImGui::TextWrapped("Resource identity: %s", selection.handle.identifier().c_str());
        ImGui::Text("Ray tracing: %s", selection.tracing ? "Enabled" : "Disabled");
        if (selection.owner && ImGui::CollapsingHeader("Drawable transform"))
        {
            matrix(selection.transform);
        }
        for (size_t sub = 0; sub < selection.submeshes.size(); ++sub)
        {
            const auto& item = selection.submeshes[sub];
            ImGui::PushID(static_cast<int>(sub));
            const auto label = std::to_string(sub) + ": " + item.name;
            if (ImGui::TreeNode(label.c_str()))
            {
                ImGui::TextWrapped("Group: %s | %s", item.group.c_str(), item.visible ? "Visible" : "Hidden");
                ImGui::TextWrapped("%s", item.material.c_str());
                for (const auto& image : item.images)
                {
                    ImGui::TextWrapped("%s", image.c_str());
                }
                ImGui::TextUnformatted("Local submesh transform");
                matrix(item.transform);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
}

}
