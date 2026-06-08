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
#include <bg2e/scene/Node.hpp>
#include "imgui.h"
#include <algorithm>

namespace bg2e::ui {

void SceneTree::setRootNode(scene::Node * root)
{
    _root = root;
    _selected.clear();
    _primary.reset();
}

void SceneTree::draw()
{
    if (!_root)
    {
        return;
    }

    for (auto & child : _root->children())
    {
        drawNode(child.get());
    }
}

void SceneTree::drawNode(scene::Node * node)
{
    if (!node)
    {
        return;
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (isSelected(node))
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
    if (!node)
    {
        return;
    }

    try {
        auto sp = node->shared_from_this();

        if (_multiSelection && ImGui::GetIO().KeyCtrl)
        {
            auto it = std::find_if(_selected.begin(), _selected.end(),
                [&sp](const std::weak_ptr<scene::Node> & wp) {
                    auto locked = wp.lock();
                    return locked && locked.get() == sp.get();
                });

            if (it != _selected.end())
            {
                _selected.erase(it);
            }
            else
            {
                _selected.push_back(sp);
            }
        }
        else
        {
            _selected.clear();
            _selected.push_back(sp);
        }

        _primary = sp;
    }
    catch (const std::bad_weak_ptr &) {
        // Node is not managed by a shared_ptr; ignore
        return;
    }

    notifyChanged();
}

void SceneTree::notifyChanged() const
{
    if (_onSelectionChanged)
    {
        _onSelectionChanged();
    }
}

std::vector<scene::Node*> SceneTree::selectedNodes() const
{
    std::vector<scene::Node*> result;
    for (auto & wp : _selected)
    {
        auto sp = wp.lock();
        if (sp)
        {
            result.push_back(sp.get());
        }
    }
    return result;
}

scene::Node * SceneTree::primarySelectedNode() const
{
    auto sp = _primary.lock();
    return sp ? sp.get() : nullptr;
}

void SceneTree::setSelectedNodes(const std::vector<scene::Node*>& nodes)
{
    _selected.clear();
    for (auto * node : nodes)
    {
        if (node)
        {
            try {
                _selected.push_back(node->shared_from_this());
            }
            catch (const std::bad_weak_ptr &) {
                // Skip nodes not managed by shared_ptr
            }
        }
    }

    if (_selected.empty())
    {
        _primary.reset();
    }
    else
    {
        _primary = _selected.back();
    }
}

void SceneTree::selectNode(scene::Node * node, bool additive)
{
    if (!node)
    {
        return;
    }

    try {
        auto sp = node->shared_from_this();

        if (!additive)
        {
            _selected.clear();
        }

        auto it = std::find_if(_selected.begin(), _selected.end(),
            [&sp](const std::weak_ptr<scene::Node> & wp) {
                auto locked = wp.lock();
                return locked && locked.get() == sp.get();
            });

        if (it == _selected.end())
        {
            _selected.push_back(sp);
        }

        _primary = sp;
    }
    catch (const std::bad_weak_ptr &) {
        return;
    }
}

void SceneTree::clearSelection()
{
    _selected.clear();
    _primary.reset();
}

bool SceneTree::isSelected(const scene::Node * node) const
{
    for (auto & wp : _selected)
    {
        auto sp = wp.lock();
        if (sp && sp.get() == node)
        {
            return true;
        }
    }
    return false;
}

}
