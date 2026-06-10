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

#include <bg2e/common.hpp>
#include <functional>
#include <vector>
#include <memory>
#include <string>

namespace bg2e {
namespace scene {
    class Node;
}
namespace ui {

class BG2E_API SceneTree {
public:
    using SelectionChangedCallback = std::function<void()>;

    void setRootNode(scene::Node * root);
    scene::Node * rootNode() const { return _root; }

    void setMultiSelection(bool enabled) { _multiSelection = enabled; }
    bool multiSelection() const { return _multiSelection; }

    void draw();

    std::vector<scene::Node*> selectedNodes() const;
    scene::Node * primarySelectedNode() const;

    void setSelectedNodes(const std::vector<scene::Node*>& nodes);
    void selectNode(scene::Node * node, bool additive = false);
    void clearSelection();
    bool isSelected(const scene::Node * node) const;

    void onSelectionChanged(SelectionChangedCallback cb) { _onSelectionChanged = cb; }

protected:
    scene::Node * _root = nullptr;
    bool _multiSelection = true;
    std::vector<std::weak_ptr<scene::Node>> _selected;
    std::weak_ptr<scene::Node> _primary;
    SelectionChangedCallback _onSelectionChanged;

    void drawNode(scene::Node * node);
    void handleClick(scene::Node * node);
    void notifyChanged() const;
};

}
}
