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
#include <utility>

namespace bg2e {
namespace scene {
    class Node;
}
namespace manipulation {
    class SelectionManager;
}
namespace ui {

// SceneTree renders the node hierarchy. Selection is fully delegated to a
// SelectionManager (the single source of truth). When no SelectionManager is
// set, the tree is read-only: it displays the hierarchy but clicks do not
// change any selection and no row is highlighted.
class BG2E_API SceneTree {
public:
    void setRootNode(scene::Node * root) { _root = root; }
    scene::Node * rootNode() const { return _root; }

    void setSelectionManager(manipulation::SelectionManager * selectionManager) { _selectionManager = selectionManager; }
    manipulation::SelectionManager * selectionManager() const { return _selectionManager; }

    void draw();

    // Called after a drag & drop reparent operation modifies the hierarchy.
    using ChangedCallback = std::function<void()>;
    void onChanged(ChangedCallback cb) { _onChanged = std::move(cb); }

protected:
    scene::Node * _root = nullptr;
    manipulation::SelectionManager * _selectionManager = nullptr;

    ChangedCallback _onChanged;

    void drawNode(scene::Node * node);
    void handleClick(scene::Node * node);
    void handleDragSource(scene::Node * node);
    void handleDropTarget(scene::Node * target);
    void reparentNode(scene::Node * node, scene::Node * newParent);
    void notifyChanged() const;

    static bool isAncestor(scene::Node * ancestor, scene::Node * node);
};

}
}
