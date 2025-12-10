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

#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace bg2e {
namespace ui {

class BG2E_API SubmeshSelector {
public:
    virtual ~SubmeshSelector();

    void setEditDrawable(std::shared_ptr<scene::Drawable> drawable);
    inline std::shared_ptr<scene::Drawable> editDrawable() { return _drawable; }
    inline const std::shared_ptr<scene::Drawable> editDrawable() const { return _drawable; }
    void clearDrawable();
    
    // Returns the index of the first submesh selected
    inline int32_t selectedItem() const { return _selectedItems.size() > 0 ? _selectedItems[0] : -1; }
    
    // Returns all the index of the selected submeshes
    inline const std::vector<uint32_t>& selectedItems() const { return _selectedItems; }
    
    void clearSelection();

    void addSelectedItem(uint32_t index);
    
    bool draw();

    void cleanup();

protected:

    void initWidgets();

    void clearWidgets();

    std::shared_ptr<scene::Drawable> _drawable;
    std::vector<bool> _selectedSubmeshes;
    std::vector<uint32_t> _selectedItems;

};

}
}
