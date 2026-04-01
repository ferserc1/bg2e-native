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
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/ui/SelectableList.hpp>
#include <bg2e/manipulation/SelectionManager.hpp>
#include <bg2e/manipulation/PickSelectionVisitor.hpp>

#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace bg2e {
namespace ui {

// Manage submeshes of the first selected item that contains a Drawable component
// using the SelectionManager as data source.
class BG2E_API SubmeshSelector {
public:
    virtual ~SubmeshSelector();

    void init(std::shared_ptr<manipulation::SelectionManager> sm);

    // Returns the first drawable item available in the SelectionManager
    std::shared_ptr<scene::Drawable> editDrawable();
    std::shared_ptr<scene::Drawable> editDrawable() const;

    // Returns the index of the first submesh selected or -1 if any
    int32_t selectedItem() const;
    
    // Returns all the index of the selected submeshes
    std::vector<uint32_t> selectedItems() const;

    // Add a submesh to the SelectionManager
    void addSelectedItem(uint32_t index) const;

    bool draw();

    void cleanup();

protected:

    void initWidgets();

    void clearWidgets();

    std::shared_ptr<manipulation::SelectionManager> _selectionMgr;
    manipulation::SelectionItem _currentSelection = {};
};

}
}
