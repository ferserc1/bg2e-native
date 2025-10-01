//
//  ToolBar.cpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#include "ToolBar.hpp"
#include "AppDelegate.hpp"
#include <bg2e/app/MainLoop.hpp>

void ToolBar::init(AppDelegate * delegate)
{
    using namespace bg2e::ui;
    _appDelegate = delegate;
    
    bg2e::app::MainLoop::current()->setOnExitFunction([]() -> bool {
        return bg2e::app::MessageBox::showWarning("Quit Model Edit", "Any unsaved changes will be lost. Are you sure you want to quit?", {
            { .label = "Quit", .code = 1 },
            { .label = "Cancel", .code = 2, .key = bg2e::app::MessageBox::Esc }
        }) == 1;
    });
    
    setMenuFunction([&]() {
        if (Menu::beginMenu("File"))
        {
            if (Menu::menuItem("Open", "Cmd+O"))
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
            if (Menu::menuItem("Save", "Cmd+S"))
            {
                // TODO: Implement this
            }
            if (Menu::menuItem("Save As..."))
            {
                // TODO: Implement this
            }
            Menu::separator();
            if (Menu::menuItem("Quit"))
            {
                bg2e::app::MainLoop::current()->exit();
            }
            Menu::endMenu();
        }
        if (Menu::beginMenu("View"))
        {
            if (Menu::menuItem("Center Camera", "Ctrl+A"))
            {
                _appDelegate->stage()->orbitCamera()->reset();
            }
            Menu::endMenu();
        }
    });
    
    addButton({
        .label = "Open",
        .action = [&]() {
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
    });
    
    addButton({
        .label = "Submesh Editor",
        .action = [&]() {
            _appDelegate->workspace().toggleLeftPanel();
        }
    }, AlignRight);
    
    addButton({
        .label = "Environment",
        .action = [&]() {
            _appDelegate->workspace().toggleRightPanel();
        }
    }, AlignRight);
}
