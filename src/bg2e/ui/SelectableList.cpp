//
//  Selectable.cpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 31/10/25.
//

#include <bg2e/ui/SelectableList.hpp>
#include "imgui.h"

#include <iostream>


namespace bg2e::ui {

void SelectableList::beginList(int columns)
{
    ImGui::BeginTable("Selectable", columns, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders);
}

bool SelectableList::item(const std::string & title, bool & selected)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    return ImGui::Selectable(title.c_str(), &selected);
}

bool SelectableList::itemButton(const std::string & title)
{
    ImGui::TableNextColumn();
    return ImGui::Button(title.c_str());
}

void SelectableList::endList()
{
    ImGui::EndTable();
}

}
