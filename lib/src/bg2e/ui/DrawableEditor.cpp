//
//  DrawableEditor.cpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 31/10/25.
//

#include <bg2e/ui/DrawableEditor.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Input.hpp>

namespace bg2e::ui {

DrawableEditor::~DrawableEditor()
{

}

void DrawableEditor::clearDrawable()
{
    _submeshSelector.clearDrawable();
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
            }
            if (BasicWidgets::checkBox("Visibility", &visible))
            {
                for (auto item : _submeshSelector.selectedItems())
                {
                    drawable->setSubmeshVisibility(visible, item);
                }
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
