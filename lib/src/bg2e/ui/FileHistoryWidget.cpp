/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation: either version 3 of the License, or
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

#include <bg2e/ui/FileHistoryWidget.hpp>
#include <bg2e/app/FileHistory.hpp>
#include <bg2e/app/FileDialog.hpp>
#include <bg2e/ui/Button.hpp>

#include "imgui.h"

namespace bg2e::ui {

FileHistoryWidget::FileHistoryWidget(const std::string& historyType)
    : _historyType{ historyType }
{
}

bool FileHistoryWidget::draw(const std::string& id, const std::string& buttonLabel)
{
    _selectedPath.clear();
    _popupId = "fileHistory##" + id;

    if (Button::button(buttonLabel + "##" + id))
    {
        openPicker();
    }

    return drawHistoryPopup();
}

bool FileHistoryWidget::drawImageButton(const std::string& id, TextureID image,
                                        uint32_t width, uint32_t height)
{
    _selectedPath.clear();
    _popupId = "fileHistory##" + id;

    bool clicked = false;
    if (image != 0)
    {
        clicked = ImGui::ImageButton(
            id.c_str(), static_cast<ImTextureID>(image),
            ImVec2(static_cast<float>(width), static_cast<float>(height)),
            ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
            ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    else
    {
        clicked = Button::button("...##" + id);
    }

    if (clicked)
    {
        openPicker();
    }

    return drawHistoryPopup();
}

void FileHistoryWidget::openPicker()
{
    auto & history = app::FileHistory::get();
    if (history.empty(_historyType))
    {
        auto filePath = app::FileDialog::getOpenFilePath(history.filtersFor(_historyType));
        if (!filePath.empty())
        {
            commitSelection(filePath);
        }
    }
    else
    {
        ImGui::OpenPopup(_popupId.c_str());
    }
}

bool FileHistoryWidget::drawHistoryPopup()
{
    bool committed = !_selectedPath.empty();

    if (ImGui::BeginPopup(_popupId.c_str()))
    {
        auto & history = app::FileHistory::get();

        if (Button::button("Open file...##" + _popupId))
        {
            auto filePath = app::FileDialog::getOpenFilePath(history.filtersFor(_historyType));
            if (!filePath.empty())
            {
                commitSelection(filePath);
                committed = true;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::Separator();

        if (ImGui::BeginChild(("##scroll" + _popupId).c_str(), ImVec2(360.0f, 240.0f), true))
        {
            bool entryPicked = false;
            for (const auto & entry : history.entries(_historyType))
            {
                ImGui::PushID(entry.string().c_str());

                auto thumbnail = _thumbnailProvider ? _thumbnailProvider(entry) : 0;
                if (thumbnail != 0)
                {
                    ImGui::Image(static_cast<ImTextureID>(thumbnail), ImVec2(32.0f, 32.0f));
                    ImGui::SameLine();
                }

                if (ImGui::Selectable(entry.filename().string().c_str()))
                {
                    commitSelection(entry);
                    committed = true;
                    entryPicked = true;
                    ImGui::CloseCurrentPopup();
                }
                // commitSelection() mutates the history list, so entry is no
                // longer valid afterwards: skip the tooltip and stop iterating
                if (!entryPicked && ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", entry.string().c_str());
                }

                ImGui::PopID();
                if (entryPicked)
                {
                    break;
                }
            }
        }
        ImGui::EndChild();

        ImGui::EndPopup();
    }

    return committed;
}

void FileHistoryWidget::commitSelection(const std::filesystem::path& path)
{
    // Copy before add(): path may reference an entry inside the history list,
    // which add() mutates (erase/insert), invalidating the reference
    _selectedPath = path;
    app::FileHistory::get().add(_historyType, _selectedPath);
}

}
