//
//  ToolBar.cpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#include "ToolBar.hpp"
#include "AppDelegate.hpp"
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/geo/modifiers.hpp>

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

    bg2e::ui::MenuItem file("File");
    file.addMenuItem({ "Open", {
        .cmdOrCtrlModifier = true,
        .key = bg2e::app::KeyEvent::KeyO,
        .handler = [&]() {
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
    }});
    addMenuItem(file);
    /*setMenuFunction([&]() {
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
                auto filePath = _appDelegate->stage()->document()->path();
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
            if (Menu::menuItem("Toggle Selection Highlight", "Shift+Space"))
            {
                _appDelegate->toggleSelectionHighlight();
            }
            if (Menu::menuItem("Center Camera", "Ctrl+A"))
            {
                _appDelegate->stage()->orbitCamera()->reset();
            }
            Menu::endMenu();
        }
    });
    */
    
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
        .label = "Y-axis > Z-axis",
        .action = [&]()
        {
            using namespace bg2e::geo;
            _appDelegate->stage()->document()->setUnsavedChanges(true);
            auto drw = _appDelegate->stage()->targetDrawable();
            if (drw.get())
            {
                drw->applyModifier(new ConvertAxisModifier<Mesh>(ConvertAxisModifier<Mesh>::Mode::ZtoY));
            }
        }
    });

    addButton({
        .label = "Center Geometry",
        .action = [&]()
        {
            using namespace bg2e::geo;
            _appDelegate->stage()->document()->setUnsavedChanges(true);
            auto drw = _appDelegate->stage()->targetDrawable();
            if (drw.get())
            {
                drw->applyModifier(new CenterGeometryModifier<Mesh>());
            }
        }
    });

    addButton({
        .label = "cm to m",
        .action = [&]()
        {
            using namespace bg2e::geo;
            _appDelegate->stage()->document()->setUnsavedChanges(true);
            auto drw = _appDelegate->stage()->targetDrawable();
            if (drw.get())
            {
                auto trx = glm::scale(
                    glm::mat4 { 1.0f },
                    glm::vec3(0.01f, 0.01f, 0.01f)
                );
                drw->applyModifier(new ApplyTransformModifier<Mesh>(trx));
            }
        }
    });

    auto & shortcuts = bg2e::app::MainLoop::current()->shortcuts();
    shortcuts.addShortcutMapper(
    {
        .cmdOrCtrlModifier = true,
        .key = bg2e::app::KeyEvent::KeyS,
        .handler = []()
        {
            // TODO: Implement this
            std::cout << "Save changes" << std::endl;
        }
    });

    shortcuts.addShortcutMapper({
        .shiftModifier = true,
        .key = bg2e::app::KeyEvent::KeySpace,
        .handler = [&]()
        {
            _appDelegate->toggleSelectionHighlight();
        }
    });

    shortcuts.addShortcutMapper({
        .ctrlModifier = true,
        .key = bg2e::app::KeyEvent::KeyA,
        .handler = [&]()
        {
            _appDelegate->stage()->orbitCamera()->reset();
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
