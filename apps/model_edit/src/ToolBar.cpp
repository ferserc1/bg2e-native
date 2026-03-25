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
        .ctrlModifier = true,
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
    file.addMenuItem({ "Import GLTF", {
        .handler = [&]()
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
    }});
    file.addMenuItem({ "Import OBJ", {
        .handler = [&]()
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
    }});
    file.addMenuItem({ "Save", {
        .ctrlModifier = true,
        .key = bg2e::app::KeyEvent::KeyS,
        .handler = [&]()
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
    }});
    file.addMenuItem({ "Save As...", {
        .shiftModifier = true,
        .ctrlModifier = true,
        .key = bg2e::app::KeyEvent::KeyS,
        .handler = [&]()
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
    }});
    file.addMenuItem({});   // Separator
    file.addMenuItem({ "Quit", {
        .ctrlModifier = true,
        .key = bg2e::app::KeyEvent::KeyQ,
        .handler = [&]()
        {
            bg2e::app::MainLoop::current()->exit();
        }
    }});
    addMenuItem(file);

    bg2e::ui::MenuItem view("View");
    view.addMenuItem({ "Toggle Selection Highlight", {
        .shiftModifier = true,
        .key = bg2e::app::KeyEvent::KeySpace,
        .handler = [&]()
        {
            _appDelegate->toggleSelectionHighlight();
        }
    }});
    view.addMenuItem({ "Center Camera", {
        .ctrlModifier = true,
        .key = bg2e::app::KeyEvent::KeyA,
        .handler = [&]()
        {
            _appDelegate->stage()->orbitCamera()->reset();
        }
    }});
    addMenuItem(view);

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
