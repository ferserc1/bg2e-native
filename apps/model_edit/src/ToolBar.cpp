//
//  ToolBar.cpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#include "ToolBar.hpp"
#include "AppDelegate.hpp"

void ToolBar::init(AppDelegate * delegate)
{
    _appDelegate = delegate;
    setDrawFunction([&]() {
        // TODO: Build the menu bar
        using namespace bg2e::ui;
        
        if (BasicWidgets::button("Open", true))
        {
            bg2e::app::FileDialog fd;
            fd.setFilters({
                { "bg2e 3D model", "bg2,vwglb" }
            });
            auto filePath = fd.openFile();
            
            if (!filePath.empty())
            {
                _appDelegate->stage()->loadModel(filePath);
            }
        }
        if (BasicWidgets::button("Save", true))
        {
        
        }
        
        auto targetDrawable = _appDelegate->stage()->targetDrawable();
        if (targetDrawable && BasicWidgets::button("Submesh Editor", true))
        {
            _appDelegate->workspace().toggleLeftPanel();
        }
        
        if (BasicWidgets::button("Environment", true))
        {
            _appDelegate->workspace().toggleRightPanel();
        }
        
        if (BasicWidgets::button("Center Camera", true))
        {
            _appDelegate->stage()->orbitCamera()->reset();
        }
        
        
    });
}
