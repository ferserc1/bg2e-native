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

#include <bg2e/ui/Toolbar.hpp>
#include <bg2e/ui/DragDrop.hpp>
#include "detail/ToolbarConfiguration.hpp"

#include <imgui.h>

#include <algorithm>

namespace bg2e::ui {

namespace {

const std::string toolPayloadType = "BG2E_TOOLBAR_ACTION";

enum class ToolList
{
    Available,
    Left,
    Right
};

struct ToolPayload {
    const Toolbar* toolbar;
    ToolList source;
    std::size_t index;
    const ActionDefinition* action;
};

struct ToolDrop {
    ToolPayload payload;
    ToolList destination;
    std::size_t index;
};

}

void Toolbar::enableCustomization()
{
    if (_customizationEnabled)
    {
        return;
    }

    auto settings = configuration();
    settings.iconSize = std::clamp(settings.iconSize, detail::minimumToolbarIconSize, detail::maximumToolbarIconSize);
    setConfiguration(settings);
    _settingsWindow.options.noMenu = true;
    _settingsWindow.options.noCollapse = true;
    _settingsWindow.setSize(680, 450);
    _settingsWindow.close();
    _customizationEnabled = true;
}

void Toolbar::enableCustomization(std::filesystem::path preferencesPath)
{
    enableCustomization();
    if (_customizationPath != preferencesPath)
    {
        _customizationPath = std::move(preferencesPath);
        loadCustomization();
    }
}

const ToolbarLayout& Toolbar::activeLayout() const
{
    return _customLayout ? *_customLayout : _defaultLayout;
}

void Toolbar::drawContextMenu()
{
    if (ImGui::BeginPopupContextWindow("ToolbarContext", ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::MenuItem("Toolbar Settings..."))
        {
            _settingsWindow.setTitle("Toolbar Settings###ToolbarSettings/" + title());
            _settingsWindow.open();
            ImGui::SetNextWindowFocus();
        }
        ImGui::EndPopup();
    }
}

void Toolbar::drawSettings()
{
    auto settings = configuration();
    if (detail::editToolbarConfiguration(settings))
    {
        setConfiguration(settings);
        saveCustomization();
    }

    if (ImGui::Button("Restore Defaults"))
    {
        setConfiguration(ToolbarConfiguration{});
        _customLayout.reset();
        saveCustomization();
    }

    ImGui::Separator();
    ImGui::TextWrapped("Drag items between lists. Drop on a row to insert before it.");

    const auto& layout = activeLayout();
    std::vector<const ActionDefinition*> available;
    for (const auto& [id, action] : _actions)
    {
        if (std::find(layout.left.begin(), layout.left.end(), id) == layout.left.end() &&
            std::find(layout.right.begin(), layout.right.end(), id) == layout.right.end())
        {
            available.push_back(action.get());
        }
    }
    std::sort(available.begin(), available.end(), [](const ActionDefinition* a, const ActionDefinition* b)
    {
        return a->label == b->label ? a->id < b->id : a->label < b->label;
    });

    std::optional<ToolDrop> drop;
    const auto accept = [&](ToolList destination, std::size_t index)
    {
        ToolPayload payload{};
        if (DragDrop::acceptPayload(toolPayloadType, &payload, sizeof(payload)) && payload.toolbar == this)
        {
            drop = ToolDrop{payload, destination, index};
        }
    };

    const auto drawRow = [&](const std::string& label, const std::string& widgetId, const ToolPayload& payload)
    {
        const auto widgetLabel = label + "###" + widgetId;
        const bool clipped = ImGui::CalcTextSize(label.c_str()).x > ImGui::GetContentRegionAvail().x;
        ImGui::Selectable(widgetLabel.c_str());
        if (clipped)
        {
            ImGui::SetItemTooltip("%s", label.c_str());
        }
        if (DragDrop::beginSource(toolPayloadType, &payload, sizeof(payload)))
        {
            DragDrop::setPreviewText(label);
            DragDrop::endSource();
        }
        accept(payload.source, payload.index);
    };

    const float listHeight = 12.0f * ImGui::GetTextLineHeightWithSpacing();
    const auto drawSide = [&](const char* name, const std::vector<std::string>& ids, ToolList list)
    {
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(name);
        if (ImGui::BeginChild(name, ImVec2(0, listHeight), ImGuiChildFlags_Borders))
        {
            for (std::size_t index = 0; index < ids.size(); ++index)
            {
                const auto& id = ids[index];
                const auto widgetId = "slot:" + std::to_string(index);
                if (id == ToolbarLayout::Separator)
                {
                    drawRow("Separator", widgetId, {this, list, index, nullptr});
                }
                else if (const auto found = _actions.find(id); found != _actions.end())
                {
                    drawRow(found->second->label, widgetId, {this, list, index, found->second.get()});
                }
            }
            ImGui::Selectable("Drop to append");
            accept(list, ids.size());
        }
        ImGui::EndChild();
    };

    if (ImGui::BeginTable("ToolLists", 3, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Available Tools", ImGuiTableColumnFlags_WidthStretch, 1.3f);
        ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch, 1.0f);

        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Available Tools");
        if (ImGui::BeginChild("Available", ImVec2(0, listHeight), ImGuiChildFlags_Borders))
        {
            ImGui::Selectable("Drop to remove");
            accept(ToolList::Available, 0);
            drawRow("Separator", ToolbarLayout::Separator, {this, ToolList::Available, 0, nullptr});
            for (const auto* action : available)
            {
                drawRow(action->label, action->id, {this, ToolList::Available, 0, action});
            }
        }
        ImGui::EndChild();

        drawSide("Left", layout.left, ToolList::Left);
        drawSide("Right", layout.right, ToolList::Right);
        ImGui::EndTable();
    }

    if (!drop)
    {
        return;
    }
    const auto& payload = drop->payload;
    std::string id = ToolbarLayout::Separator;
    if (payload.action)
    {
        const auto found = std::find_if(_actions.begin(), _actions.end(), [&](const auto& entry)
        {
            return entry.second.get() == payload.action;
        });
        if (found == _actions.end())
        {
            return;
        }
        id = found->first;
    }

    auto updated = layout;
    const auto side = [&](ToolList list) -> std::vector<std::string>&
    {
        return list == ToolList::Left ? updated.left : updated.right;
    };
    auto insertionIndex = drop->index;
    if (payload.source != ToolList::Available)
    {
        auto& source = side(payload.source);
        if (payload.index >= source.size() || source[payload.index] != id)
        {
            return;
        }
        source.erase(source.begin() + payload.index);
        if (payload.source == drop->destination && payload.index < insertionIndex)
        {
            --insertionIndex;
        }
    }
    else if (payload.action &&
        (std::ranges::find(updated.left, id) != updated.left.end() ||
            std::ranges::find(updated.right, id) != updated.right.end()))
    {
        return;
    }

    if (drop->destination != ToolList::Available)
    {
        auto& destination = side(drop->destination);
        if (insertionIndex > destination.size())
        {
            return;
        }
        destination.insert(destination.begin() + insertionIndex, std::move(id));
    }
    if (updated.left != layout.left || updated.right != layout.right)
    {
        _customLayout = std::move(updated);
        saveCustomization();
    }
}

}
