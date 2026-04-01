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
