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

#include "EditorUi.hpp"

#include <bg2e/app/PreferencesStore.hpp>
#include <bg2e/app/FileDialog.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>

#include <utility>

namespace model_edit {

EditorUi::EditorUi(const std::shared_ptr<AppDelegate>& delegate) : _delegate(delegate)
{
    auto& store = bg2e::app::PreferencesStore::instance().preferences("draw.model_edit.graphics");
    _preferences = std::make_shared<bg2e::draw::RenderSettingsPreferences>(&store);
    _preferences->connect(delegate); // Before the renderer's first attachment.
    _graphics.bind(_preferences);
    _graphics.setPosition(320, 80);
    _graphics.setSize(620, 740);
    _graphics.close();
    _properties.setTitle("Model Properties");
    _environment.setTitle("Environment Options");
    _workspace.leftPanelSize().min = 300;
}

void EditorUi::initActions()
{
    const auto weakDelegate = _delegate;
    const auto weakUi = weak_from_this();

    _toolbar.registerActions({
        {
            .id = "file.open",
            .label = "Open...",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->openDocument();
                }
            },
            .shortcut = {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyO},
            .path = "File"
        },
        {
            .id = "file.import-gltf",
            .label = "Import glTF...",
            .execute = [weakDelegate]
            {
                const auto path = bg2e::app::FileDialog::getOpenFilePath({{"glTF model", "gltf,glb"}});
                if (auto owner = weakDelegate.lock(); owner && !path.empty())
                {
                    owner->openDocument(path);
                }
            },
            .path = "File"
        },
        {
            .id = "file.import-obj",
            .label = "Import OBJ...",
            .execute = [weakDelegate]
            {
                const auto path = bg2e::app::FileDialog::getOpenFilePath({{"OBJ model", "obj"}});
                if (auto owner = weakDelegate.lock(); owner && !path.empty())
                {
                    owner->openDocument(path);
                }
            },
            .path = "File"
        },
        {
            .id = "file.save",
            .label = "Save",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->saveDocument();
                }
            },
            .shortcut = {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyS},
            .path = "File"
        },
        {
            .id = "file.save-as",
            .label = "Save as...",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->saveDocument(true);
                }
            },
            .shortcut = {.ctrlModifier = true, .shiftModifier = true, .key = bg2e::app::KeyEvent::KeyS},
            .path = "File"
        }
    }, /*addSeparator=*/true);

    _toolbar.registerActions({
        {
            .id = "file.quit",
            .label = "Quit",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->requestQuit();
                }
            },
            .shortcut = {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyQ},
            .path = "File"
        }
    });

    _toolbar.registerActions({
        {
            .id = "view.reset-camera",
            .label = "Reset camera",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->resetCamera();
                }
            },
            .path = "View"
        },
        {
            .id = "view.frame-selection",
            .label = "Frame selection",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->frameSelection();
                }
            },
            .path = "View"
        },
        {
            .id = "view.analyze-best-view",
            .label = "Analyze best view",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->analyzeBestView();
                }
            },
            .shortcut = {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyA},
            .path = "View"
        },
        {
            .id = "view.toggle-selection-highlight",
            .label = "Toggle selection highlight",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_highlight = (owner->_highlight + 1) % 4;
                    const float levels[]{0, 0.25f, 0.8f, 1};
                    if (auto delegate = owner->_delegate.lock())
                    {
                        delegate->setHighlight(levels[owner->_highlight]);
                    }
                }
            },
            .shortcut = {.shiftModifier = true, .key = bg2e::app::KeyEvent::KeySpace},
            .path = "View"
        }
    });

    _toolbar.registerActions({
        {
            .id = "window.model-properties",
            .label = "Model Properties",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_workspace.toggleLeftPanel();
                }
            },
            .checked = [weakUi]
            {
                auto owner = weakUi.lock();
                return owner && owner->_workspace.leftPanelVisible();
            },
            .path = "Window"
        },
        {
            .id = "window.environment-options",
            .label = "Environment Options",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_workspace.toggleRightPanel();
                }
            },
            .checked = [weakUi]
            {
                auto owner = weakUi.lock();
                return owner && owner->_workspace.rightPanelVisible();
            },
            .path = "Window"
        }
    }, /*addSeparator=*/true);

    _toolbar.registerActions({
        {
            .id = "window.ui-settings",
            .label = "UI Settings",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_uiSettings.open();
                }
            },
            .path = "Window"
        },
        {
            .id = "window.graphics-settings",
            .label = "Graphics Settings",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_graphics.open();
                }
            },
            .path = "Window"
        },
        {
            .id = "window.lightmap-baker",
            .label = "UV and Lightmap Baker...",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_lightmap.open();
                }
            },
            .path = "Window"
        }
    });
}

void EditorUi::init(bg2e::draw::Engine* engine, bg2e::ui::UserInterface* session)
{
    _ui = session;
    _toolbar.setTitle("Model tools");
    _toolbar.options.noClose = true;
    const auto weakDelegate = _delegate;
    const auto weakUi = weak_from_this();
    initActions();

    const auto addPanelButton = [this](const char* label, auto action, bg2e::ui::Toolbar::Alignment alignment = bg2e::ui::Toolbar::AlignLeft)
    {
        _toolbar.addButton({.label = label, .action = std::move(action)}, alignment);
    };
    addPanelButton("Z up to Y up", [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->editGeometry(0);
        }
    });
    addPanelButton("Center Geometry", [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->editGeometry(1);
        }
    });
    addPanelButton("Put on Floor", [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->editGeometry(2);
        }
    });
    addPanelButton("cm to m", [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->editGeometry(3);
        }
    });
    addPanelButton("Frame", [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->frameSelection();
        }
    });
    addPanelButton("Duplicate", [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->duplicateSelection();
        }
    });
    addPanelButton("Model Properties", [weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            owner->_workspace.toggleLeftPanel();
        }
    }, bg2e::ui::Toolbar::AlignRight);
    addPanelButton("Environment", [weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            owner->_workspace.toggleRightPanel();
        }
    }, bg2e::ui::Toolbar::AlignRight);
    if (auto delegate = _delegate.lock())
    {
        _material.init(engine, delegate);
        const auto weak = weak_from_this();
        delegate->onToolsCleanup([weak]
        {
            if (auto ui = weak.lock())
            {
                ui->_lightmap.cleanup();
                ui->_material.cleanup();
                ui->_graphics.clear();
                ui->_ui = nullptr;
            }
        });
        delegate->onSelectionChanged([weak]
        {
            if (auto ui = weak.lock())
            {
                if (auto delegate = ui->_delegate.lock())
                {
                    ui->_material.setSelectionManager(delegate->selection());
                }
            }
        });
    }
    _uiSettings.init(true);
    _uiSettings.close();
    if (auto delegate = _delegate.lock())
    {
        _lightmap.init(delegate, engine, session);
    }
    _fileStatus = std::make_shared<bg2e::ui::StatusItem>();
    _saveStatus = std::make_shared<bg2e::ui::StatusItem>();
    _statusBar.addItem(_fileStatus, bg2e::ui::StatusBar::AlignLeft);
    _statusBar.addItem(_saveStatus, bg2e::ui::StatusBar::AlignRight);
    _workspace.setup(uiWidth(), uiHeight(), &_toolbar, &_properties, &_environment, nullptr, &_statusBar);
    _workspaceWidth = uiWidth();
    _workspaceHeight = uiHeight();
    _properties.setDrawFunction([weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            if (auto delegate = owner->_delegate.lock())
            {
                owner->drawTools(delegate);
            }
        }
    });
    _environment.setDrawFunction([weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            if (auto delegate = owner->_delegate.lock())
            {
                owner->drawEnvironment(delegate);
            }
        }
    });
}

void EditorUi::drawUI()
{
    auto delegate = _delegate.lock();
    if (!delegate || !delegate->scene())
    {
        return;
    }
    if (_workspaceWidth != uiWidth() || _workspaceHeight != uiHeight())
    {
        _workspace.resize(uiWidth(), uiHeight());
        _workspaceWidth = uiWidth();
        _workspaceHeight = uiHeight();
    }
    delegate->setLogicalExtent(uiWidth(), uiHeight());
    bindDocument(delegate);
    const auto* document = delegate->document();
    if (document && document->root)
    {
        _fileStatus->setText(document->sourcePath.empty() ? document->root->name() : document->sourcePath.filename().string());
    }
    else
    {
        _fileStatus->setText("No model open");
    }
    if (document && document->dirty)
    {
        _saveStatus->setText("Unsaved changes");
    }
    else if (!delegate->status().empty())
    {
        _saveStatus->setText(delegate->status());
    }
    else if (!delegate->viewpointStatus().empty())
    {
        _saveStatus->setText(delegate->viewpointStatus());
    }
    else
    {
        _saveStatus->setText("Ready");
    }
    _workspace.draw();
    _graphics.draw();
    if (_uiSettings.isOpen())
    {
        _uiSettings.draw();
    }
    _lightmap.draw();
}

}
