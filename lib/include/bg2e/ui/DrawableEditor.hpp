//
//  DrawableEditor.hpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 31/10/25.
//

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/ui/SubmeshSelector.hpp>

#include <functional>

namespace bg2e {
namespace ui {

class BG2E_API DrawableEditor {
public:
    virtual ~DrawableEditor();
    
    inline void setEditDrawable(std::shared_ptr<scene::Drawable> drawable, uint32_t submeshIndex)
    {
        _submeshSelector.setEditDrawable(drawable);
        _submeshSelector.clearSelection();
        _submeshSelector.addSelectedItem(submeshIndex);
    }
    inline std::shared_ptr<scene::Drawable> editDrawable() { return _drawable; }
    inline const std::shared_ptr<scene::Drawable> editDrawable() const { return _drawable; }
    void clearDrawable();
    
    // Direct access to submesh selector
    inline int32_t selectedItem() const { return _submeshSelector.selectedItem(); }
    inline const std::vector<uint32_t>& selectedItems() const { return _submeshSelector.selectedItems(); }
    inline void clearSelection() { _submeshSelector.clearSelection(); }
    
    
    inline SubmeshSelector & submeshSelector() { return _submeshSelector; }
    inline const SubmeshSelector & submeshSelector() const { return _submeshSelector; }
    
    bool draw();
    
    void cleanup();

    inline void onChanged(std::function<void()> cb) { _onChange = cb; }

protected:
    std::shared_ptr<scene::Drawable> _drawable;
    
    SubmeshSelector _submeshSelector;

    std::function<void()> _onChange;

    inline void notifyOnChange()
    {
        if (_onChange)
        {
            _onChange();
        }
    }
};

}
}
