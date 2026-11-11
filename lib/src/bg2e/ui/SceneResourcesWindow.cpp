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

#include <bg2e/ui/SceneResourcesWindow.hpp>
#include <bg2e/app/MessageBox.hpp>
#include <imgui.h>
#include <utility>

namespace bg2e::ui {

SceneResourcesWindow::SceneResourcesWindow()
{
    setTitle("Scene Resources Manager");
    setSize(720, 520);
    close();
}

void SceneResourcesWindow::configure(std::function<const db::SceneResourceAnalysis*()> analysis,
    std::function<bool()> available, std::function<void()> analyze,
    std::function<void(bool)> cleanup, std::function<std::string()> status)
{
    _analysis = std::move(analysis);
    _available = std::move(available);
    _analyze = std::move(analyze);
    _cleanup = std::move(cleanup);
    _status = std::move(status);
}

void SceneResourcesWindow::draw()
{
    Window::draw([this]
    {
        drawContents();
    });
}

void SceneResourcesWindow::drawContents()
{
    const bool available = _available && _available();
    ImGui::TextWrapped("Analysis reads the saved scene file. Saving first is only needed for pending changes or a scene without a saved file.");
    ImGui::BeginDisabled(!available || !_analyze);
    if (ImGui::Button("Analyze scene resources"))
    {
        _ignoreWarnings = false;
        _analyze();
    }
    ImGui::EndDisabled();
    if (_status)
    {
        ImGui::TextWrapped("%s", _status().c_str());
    }
    const auto* analysis = _analysis ? _analysis() : nullptr;
    if (!analysis)
    {
        _ignoreWarnings = false;
        ImGui::TextWrapped("No current analysis. Click Analyze scene resources to scan or refresh the file list.");
        return;
    }
    ImGui::TextWrapped("%s", analysis->scenePath.string().c_str());
    uintmax_t bytes = 0;
    for (const auto& file : analysis->unusedFiles)
    {
        bytes += file.bytes;
    }
    ImGui::Text("%zu unused files, %.2f MiB", analysis->unusedFiles.size(),
        static_cast<double>(bytes) / (1024.0 * 1024.0));
    if (!analysis->warnings.empty())
    {
        if (ImGui::CollapsingHeader("Dependency warnings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (const auto& warning : analysis->warnings)
            {
                ImGui::TextWrapped("%s", warning.c_str());
            }
        }
        ImGui::Checkbox("Ignore dependency warnings and allow cleanup", &_ignoreWarnings);
        ImGui::TextWrapped("Ignoring warnings can delete resources whose dependencies could not be identified.");
    }
    if (!analysis->externalResources.empty() && ImGui::CollapsingHeader("External resources (never deleted)"))
    {
        for (const auto& path : analysis->externalResources)
        {
            ImGui::TextWrapped("%s", path.string().c_str());
        }
    }
    ImGui::BeginDisabled(!available || !_cleanup || analysis->unusedFiles.empty() ||
        (!analysis->warnings.empty() && !_ignoreWarnings));
    const bool remove = ImGui::Button("Delete unused files...");
    ImGui::EndDisabled();
    if (remove)
    {
        using Box = app::MessageBox;
        const auto answer = Box::showWarning("Delete unused scene files",
            "Permanently delete the listed files? The saved scene is reloaded first, clearing selection, undo/redo history and the clipboard.",
            {{0, "Cancel", Box::Esc}, {1, "Delete files"}});
        if (answer == 1)
        {
            _cleanup(_ignoreWarnings);
        }
    }
    ImGui::Separator();
    ImGui::BeginChild("Unused scene files", ImVec2(0, 0));
    for (const auto& file : analysis->unusedFiles)
    {
        const auto relative = file.path.lexically_relative(analysis->scenePath.parent_path());
        ImGui::TextWrapped("%s (%llu bytes)", relative.generic_string().c_str(),
            static_cast<unsigned long long>(file.bytes));
    }
    ImGui::EndChild();
}

}
