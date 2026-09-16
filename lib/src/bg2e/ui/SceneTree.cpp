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

#include <bg2e/ui/SceneTree.hpp>
#include <bg2e/ui/DragDrop.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/manipulation/SelectionManager.hpp>
#include "imgui.h"

namespace bg2e::ui {

static const std::string kNodePayloadType = "BG2E_SCENE_NODE";

void SceneTree::draw()
{
    if (!_root)
    {
        return;
    }

    drawNode(_root);
}

void SceneTree::drawNode(scene::Node * node)
{
    if (!node)
    {
        return;
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (node == _root)
    {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (_selectionManager && _selectionManager->isSelected(node))
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (node->children().empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    ImGui::PushID(node);

    const auto & nodeName = node->name();
    const char * label = nodeName.empty() ? "(unnamed)" : nodeName.c_str();

    bool open = ImGui::TreeNodeEx((void*)node, flags, "%s", label);

    if (ImGui::IsItemClicked())
    {
        handleClick(node);
    }

    handleDragSource(node);
    handleDropTarget(node);

    if (open)
    {
        if (!(flags & ImGuiTreeNodeFlags_Leaf))
        {
            for (auto & child : node->children())
            {
                drawNode(child.get());
            }
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void SceneTree::handleClick(scene::Node * node)
{
    if (!node || !_selectionManager)
    {
        return;
    }

    const bool additive = _selectionManager->multiSelection() && ImGui::GetIO().KeyCtrl;

    if (additive)
    {
        // Toggle the clicked node, keeping the rest of the selection
        if (_selectionManager->isSelected(node))
        {
            _selectionManager->removeFromSelectedItems(node);
        }
        else
        {
            _selectionManager->addToSelectedItems(node);
        }
    }
    else
    {
        // Replace the whole selection with the clicked node
        _selectionManager->deselect();
        _selectionManager->addToSelectedItems(node);
    }
}

void SceneTree::handleDragSource(scene::Node * node)
{
    // The root node can never be the source of a drag & drop operation
    if (node == _root)
    {
        return;
    }

    if (DragDrop::beginSource(kNodePayloadType, &node, sizeof(node)))
    {
        const auto & nodeName = node->name();
        DragDrop::setPreviewText(nodeName.empty() ? "(unnamed)" : nodeName);
        DragDrop::endSource();
    }
}

void SceneTree::handleDropTarget(scene::Node * target)
{
    scene::Node * dragged = nullptr;
    if (!DragDrop::acceptPayload(kNodePayloadType, &dragged, sizeof(dragged)))
    {
        return;
    }

    if (!dragged ||
        dragged == target ||                    // drop on itself
        dragged->parent() == target ||          // already a child of the target
        isAncestor(dragged, target))            // would create a cycle
    {
        return;
    }

    reparentNode(dragged, target);
}

void SceneTree::reparentNode(scene::Node * node, scene::Node * newParent)
{
    auto * oldParent = node->parent();
    if (!oldParent || !newParent)
    {
        return;
    }

    // Keep the shared_ptr alive across the remove/add pair
    std::shared_ptr<scene::Node> nodeShared;
    for (auto & child : oldParent->children())
    {
        if (child.get() == node)
        {
            nodeShared = child;
            break;
        }
    }

    if (!nodeShared)
    {
        return;
    }

    oldParent->removeChild(nodeShared);
    newParent->addChild(nodeShared);

    // The reparent operation ignores the current selection: only the dropped
    // node is moved, and it becomes the new selection
    if (_selectionManager)
    {
        _selectionManager->deselect();
        _selectionManager->addToSelectedItems(node);
    }

    notifyChanged();
}

bool SceneTree::isAncestor(scene::Node * ancestor, scene::Node * node)
{
    for (auto * p = node->parent(); p; p = p->parent())
    {
        if (p == ancestor)
        {
            return true;
        }
    }
    return false;
}

void SceneTree::notifyChanged() const
{
    if (_onChanged)
    {
        _onChanged();
    }
}

}
