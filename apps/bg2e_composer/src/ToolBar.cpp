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
#include "ToolBar.hpp"
#include "AppDelegate.hpp"
#include <bg2e/app/MainLoop.hpp>

void ToolBar::init(AppDelegate * delegate, bg2e::ui::UISettingsWindow * uiSettings, UIRenderSettingsWindow * renderSettings)
{
    using namespace bg2e::ui;
    _appDelegate = delegate;
    _uiSettingsWindow = uiSettings;
    _renderSettingsWindow = renderSettings;
    
    bg2e::app::MainLoop::current()->setOnExitFunction([]() -> bool {
        return bg2e::app::MessageBox::showWarning("Quit Composer", "Any unsaved changes will be lost. Are you sure you want to quit?", {
            { .code = 1, .label = "Quit" },
            { .code = 2, .label = "Cancel", .key = bg2e::app::MessageBox::Esc }
        }) == 1;
    });

    bg2e::ui::MenuItem file("File");
    file.addMenuItem({ "Open Scene", {
        .ctrlModifier = true,
        .key = bg2e::app::KeyEvent::KeyO,
        .handler = [&]() {
            if (!_appDelegate->stage()->checkUnsavedChanges()) return;

            bg2e::app::FileDialog fd;
            fd.setFilters({
                { "bg2e scene", "json,vitscnj" }
            });
            auto filePath = fd.openFile();

            if (!filePath.empty())
            {
                bg2e::app::MainLoop::current()->asyncLoad([&, filePath](bg2e::ui::Loader* loader)
                {
                    _appDelegate->stage()->openScene(filePath, [&](const std::string& modelName, uint32_t processed, uint32_t total) {
                        loader->setMessage("Loading model " + modelName + "...");
                        loader->setProgress(static_cast<float>(processed) / static_cast<float>(total));
                    });
                }, glm::vec4{ 0.2, 0.2, 0.31, 1.0f });
            }
        }
    }});
    file.addMenuItem({ "Import bg2 Model", {
        .handler = [&]()
        {
            bg2e::app::FileDialog fd;
            fd.setFilters({
                { "bg2e 3D model", "bg2,vwglb" }
            });
            auto filePath = fd.openFile();

            if (!filePath.empty())
            {
                _appDelegate->stage()->importModelBg2(filePath);
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
                    { "bg2e scene", "json,vitscnj" }
                });
                filePath = fd.saveFile();
            }

            if (!filePath.empty())
            {
                _appDelegate->stage()->saveScene(filePath);
            }
        }
    }});
    file.addMenuItem({ "Save As...", {
        .ctrlModifier = true,
        .shiftModifier = true,
        .key = bg2e::app::KeyEvent::KeyS,
        .handler = [&]()
        {
            bg2e::app::FileDialog fd;
            fd.setFilters({
                { "bg2e scene", "json,vitscnj" }
            });
            auto filePath = fd.saveFile();

            if (!filePath.empty())
            {
                _appDelegate->stage()->saveScene(filePath);
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

    bg2e::ui::MenuItem window("Window");
    window.addMenuItem({ "Submesh Editor", {
        .handler = [&]()
        {
            _appDelegate->workspace().toggleLeftPanel();
        }
    }, [&]() {
        return _appDelegate->workspace().leftPanelVisible();
    }});
    window.addMenuItem({ "Scene", {
        .handler = [&]()
        {
            _appDelegate->workspace().toggleRightPanel();
        }
    }, [&]() {
        return _appDelegate->workspace().rightPanelVisible();
    }});
    window.addMenuItem({});   // Separator
    window.addMenuItem({ "UI Settings", {
        .handler = [&]()
        {
            _uiSettingsWindow->open();
        }
    }});
    window.addMenuItem({ "Render Settings", {
        .handler = [&]()
        {
            _renderSettingsWindow->open();
        }
    }});
    addMenuItem(window);

    addButton({
        .label = "Submesh Editor",
        .action = [&]() {
            _appDelegate->workspace().toggleLeftPanel();
        }
    }, AlignRight);
    
    addButton({
        .label = "Scene",
        .action = [&]() {
            _appDelegate->workspace().toggleRightPanel();
        }
    }, AlignRight);
}
