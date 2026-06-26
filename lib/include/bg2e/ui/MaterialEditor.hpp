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
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/ui/TextureWidgets.hpp>
#include <bg2e/manipulation/SelectionManager.hpp>

#include <memory>
#include <vector>
#include <functional>

namespace bg2e {
namespace ui {

class BG2E_API MaterialEditor {
public:
    virtual ~MaterialEditor();

    // These functions allow you to manage the editing of materials manually, setting direct pointers to the materials to be edited.
    void setEditMaterial(std::shared_ptr<render::MaterialBase>& mat);
    void addEditMaterial(std::shared_ptr<render::MaterialBase>& mat);
    void clearMaterial();

    // If a selection manager is set, the setEditMaterial(), addEditMaterial() and clearMaterial() functions
    // will be ignored, and the editMaterial() getter will get the editing material from the selection manager
    void setSelectionManager(const std::shared_ptr<manipulation::SelectionManager>& sm);

    // Return the main material: the first material selected in material list
    std::shared_ptr<render::MaterialBase> editMaterial();
    std::shared_ptr<render::MaterialBase> editMaterial() const;

    bool draw();

    void cleanup();

    inline void onChanged(std::function<void()> cb) { _onChangedFunction = cb; }

protected:
    std::shared_ptr<manipulation::SelectionManager> _selectionManager;

    std::shared_ptr<render::MaterialBase> _material;
    std::vector<std::shared_ptr<render::MaterialBase>> _editMaterialList;
    
    TextureWidgets _albedoWidget;
    TextureWidgets _normalWidget;
    TextureWidgets _metallicWidget;
    TextureWidgets _roughnessWidget;
    TextureWidgets _aoWidget;
    TextureWidgets _lightEmissionWidget;

    std::function<void()> _onChangedFunction;
    
    void initWidgets();
    void clearWidgets();

    inline void notifyOnChange() const
    {
        if (_onChangedFunction)
        {
            _onChangedFunction();
        }
    }
};

}
}
