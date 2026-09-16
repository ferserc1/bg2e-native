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

#include <bg2e/ui/DragDrop.hpp>

#include "imgui.h"

#include <cstring>

namespace bg2e {
namespace ui {

bool DragDrop::beginSource(const std::string & payloadType,
                           const void * payloadData, size_t payloadSize,
                           bool disabled)
{
    if (disabled)
    {
        return false;
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        ImGui::SetDragDropPayload(payloadType.c_str(), payloadData, payloadSize);
        return true;
    }
    return false;
}

void DragDrop::endSource()
{
    ImGui::EndDragDropSource();
}

bool DragDrop::acceptPayload(const std::string & payloadType,
                             void * payloadData, size_t payloadSize)
{
    bool accepted = false;
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload(payloadType.c_str()))
        {
            if (payload->Data && static_cast<size_t>(payload->DataSize) == payloadSize)
            {
                std::memcpy(payloadData, payload->Data, payloadSize);
                accepted = true;
            }
        }
        ImGui::EndDragDropTarget();
    }
    return accepted;
}

void DragDrop::setPreviewText(const std::string & text)
{
    ImGui::TextUnformatted(text.c_str());
}

}
}
