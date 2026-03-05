//
//  SubmeshSelector.hpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 31/10/25.
//

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
