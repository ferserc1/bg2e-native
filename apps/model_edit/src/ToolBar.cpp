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
            { .code = 1, .label = "Quit" },
            { .code = 2, .label = "Cancel", .key = bg2e::app::MessageBox::Esc }
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
            if (Menu::menuItem("Import GLTF"))
            {
                bg2e::app::FileDialog fd;
                fd.setFilters({
                    { "gltf file", "glb,gltf" }
                });
                auto filePath = fd.openFile();

                if (!filePath.empty())
                {
                    _appDelegate->stage()->importGltf(filePath);
                }
            }
            if (Menu::menuItem("Import OBJ"))
            {
                bg2e::app::FileDialog fd;
                fd.setFilters({
                    { "OBJ file", "obj" }
                });
                auto filePath = fd.openFile();

                if (!filePath.empty())
                {
                    _appDelegate->stage()->importObj(filePath);
                }
            }
            if (Menu::menuItem("Save", "Cmd+S"))
            {
                auto filePath = _appDelegate->stage()->filePath();
                if (filePath.empty())
                {
                    bg2e::app::FileDialog fd;
                    fd.setFilters({
                        { "bg2e 3D model", "bg2,vwglb" }
                    });
                    filePath = fd.saveFile();
                }
                
                if (!filePath.empty())
                {
                    _appDelegate->stage()->saveModel(filePath);
                }
            }
            if (Menu::menuItem("Save As..."))
            {
                bg2e::app::FileDialog fd;
                fd.setFilters({
                    { "bg2e 3D model", "bg2,vwglb" }
                });
                auto filePath = fd.saveFile();
                
                if (!filePath.empty())
                {
                    _appDelegate->stage()->saveModel(filePath);
                }
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
