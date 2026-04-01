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

#include <bg2e/ui/DrawableEditor.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Input.hpp>

namespace bg2e::ui {

DrawableEditor::~DrawableEditor()
{

}

void DrawableEditor::init(const std::shared_ptr<manipulation::SelectionManager> & selectionManager)
{
    _submeshSelector.init(selectionManager);
}

bool DrawableEditor::draw()
{
    std::shared_ptr<scene::Drawable> drawable = _submeshSelector.editDrawable();
    auto drawableName = drawable.get() && !drawable->name().empty() ?
        drawable->name() : "Drawable";
    auto changed = false;
        
    if (drawable.get() && BasicWidgets::collapsingHeader(drawableName)) {
        if (_submeshSelector.draw())
        {
            changed = true;
        }
        
        auto selectedPlist = _submeshSelector.selectedItem();
        
        if (selectedPlist != -1)
        {
            std::string nameLabel = "Name";
            std::string separatorLabel = "Submesh Properties";
            auto plName = drawable->submeshName(selectedPlist);
            auto grpName = drawable->submeshGroupName(selectedPlist);
            auto visible = drawable->submeshVisibility(selectedPlist);
            
            if (_submeshSelector.selectedItems().size() > 1)
            {
                separatorLabel += " (" + std::to_string(_submeshSelector.selectedItems().size()) + " items selected)";
            }
            BasicWidgets::separator(separatorLabel);
            
            
            if (Input::text(nameLabel, plName))
            {
                drawable->setSubmeshName(plName, selectedPlist);
                notifyOnChange();
            }
            if (_submeshSelector.selectedItems().size() > 1)
            {
                BasicWidgets::text("(" + plName + ")", true);
            }
            if (Input::text("Group Name", grpName))
            {
                for (auto item : _submeshSelector.selectedItems())
                {
                    drawable->setSubmeshGroupName(grpName, item);
                }
                notifyOnChange();
            }
            if (BasicWidgets::checkBox("Visibility", &visible))
            {
                for (auto item : _submeshSelector.selectedItems())
                {
                    drawable->setSubmeshVisibility(visible, item);
                }
                notifyOnChange();
            }
        }
    }
     
    return changed;
}

void DrawableEditor::cleanup()
{
    _submeshSelector.cleanup();
}

}
