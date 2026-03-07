//
//  DrawableEditor.hpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 31/10/25.
//

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/ui/SubmeshSelector.hpp>
#include <bg2e/manipulation/SelectionManager.hpp>

#include <functional>

namespace bg2e {
namespace ui {

class BG2E_API DrawableEditor {
public:
    virtual ~DrawableEditor();

    void init(const std::shared_ptr<manipulation::SelectionManager>& selectionManager);

    // Direct access to submesh selector
    inline int32_t selectedItem() const { return _submeshSelector.selectedItem(); }
    inline std::vector<uint32_t> selectedItems() const { return _submeshSelector.selectedItems(); }
    
    inline SubmeshSelector & submeshSelector() { return _submeshSelector; }
    inline const SubmeshSelector & submeshSelector() const { return _submeshSelector; }
    
    bool draw();
    
    void cleanup();

    inline void onChanged(std::function<void()> cb) { _onChange = cb; }

protected:
    SubmeshSelector _submeshSelector;

    std::function<void()> _onChange;

    inline void notifyOnChange() const
    {
        if (_onChange)
        {
            _onChange();
        }
    }
};

}
}
