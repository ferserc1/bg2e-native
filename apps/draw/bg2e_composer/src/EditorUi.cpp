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
#include <bg2e/draw/RendererDeferred.hpp>

#include <utility>

namespace bg2e_composer {

EditorUi::EditorUi(const std::shared_ptr<AppDelegate>& delegate) : _delegate(delegate)
{
    auto& store = bg2e::app::PreferencesStore::instance().preferences("draw.bg2e_composer.graphics");
    _preferences = std::make_shared<bg2e::draw::RenderSettingsPreferences>(&store);
    _preferences->setAutomaticPersistence(true);
    _preferences->setExcludeSceneSettingsFromAutomaticPersistence(true);
    _preferences->connect(delegate); // Before the renderer's first attachment.
    _graphics.bind(_preferences);
    _diagnostics.bind(_preferences);
    _diagnostics.close();
    _graphics.setPosition(320, 80);
    _graphics.setSize(620, 740);
    _graphics.close();
    _properties.setTitle("Model Properties");
    _sceneProperties.setTitle("Scene");
    _workspace.leftPanelSize().min = 300;
}

void EditorUi::init(bg2e::draw::Engine* engine, bg2e::ui::UserInterface* session)
{
    _ui = session;
    _toolbar.setTitle("Scene tools");
    _toolbar.options.noClose = true;
    const auto weakDelegate = _delegate;
    const auto weakUi = weak_from_this();
    bg2e::ui::MenuItem file("File");
    file.addMenuItem({"Open...", {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyO, .handler = [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->openDocument();
        }
    }}});
    file.addMenuItem({"New Scene", {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyN, .handler = [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->newDocument();
        }
    }}});
    file.addMenuItem({"Close Scene", {.handler = [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->newDocument();
        }
    }}});
    file.addMenuItem({"Import Model or Prefab...", {.handler = [weakUi]
    {
        if (auto ui = weakUi.lock())
        {
            if (auto owner = ui->_delegate.lock())
            {
                ui->chooseImport(owner);
            }
        }
    }}});
    file.addMenuItem({"Export Selected Model(s)...", {.handler = [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->exportSelectedModels();
        }
    }}});
    file.addMenuItem({"Save", {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyS, .handler = [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->saveDocument();
        }
    }}});
    file.addMenuItem({"Save as...", {.ctrlModifier = true, .shiftModifier = true, .key = bg2e::app::KeyEvent::KeyS, .handler = [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->saveDocument(true);
        }
    }}});
    file.addMenuItem({});
    file.addMenuItem({"Quit", {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyQ, .handler = [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->requestQuit();
        }
    }}});
    _toolbar.addMenuItem(file);
    bg2e::ui::MenuItem view("View");
    view.addMenuItem({"Reset camera", {.handler = [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->resetCamera();
        }
    }}});
    view.addMenuItem({"Frame selection", {.handler = [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->frameSelection();
        }
    }}});
    view.addMenuItem({"Analyze best view", {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyA, .handler = [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->analyzeBestView();
        }
    }}});
    view.addMenuItem({"Toggle selection highlight", {.shiftModifier = true, .key = bg2e::app::KeyEvent::KeySpace, .handler = [weakUi]
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
    }}});
    _toolbar.addMenuItem(view);

    bg2e::ui::MenuItem create("Create Node");
    const char* nodeTypes[]{"Group", "Camera", "Light", "Environment"};
    for (unsigned kind = 0; kind < 4; ++kind)
    {
        create.addMenuItem({nodeTypes[kind], {.handler = [weakDelegate, kind]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->addSceneItem(kind);
            }
        }}});
    }
    const char* primitiveTypes[]{"Cube", "Sphere", "Plane"};
    for (unsigned kind = 0; kind < 3; ++kind)
    {
        create.addMenuItem({std::string("Create ") + primitiveTypes[kind], {.handler = [weakDelegate, kind]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->addPrimitive(kind);
            }
        }}});
    }
    _toolbar.addMenuItem(create);

    bg2e::ui::MenuItem window("Window");
    window.addMenuItem({"Model Properties", {.handler = [weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            owner->_workspace.toggleLeftPanel();
        }
    }}, [weakUi]
    {
        auto owner = weakUi.lock();
        return owner && owner->_workspace.leftPanelVisible();
    }});
    window.addMenuItem({"Scene", {.handler = [weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            owner->_workspace.toggleRightPanel();
        }
    }}, [weakUi]
    {
        auto owner = weakUi.lock();
        return owner && owner->_workspace.rightPanelVisible();
    }});
    window.addMenuItem({});
    window.addMenuItem({"UI Settings", {.handler = [weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            owner->_uiSettings.open();
        }
    }}});
    window.addMenuItem({"Graphics Settings", {.handler = [weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            owner->_graphics.open();
        }
    }}});
    window.addMenuItem({"Renderer Diagnostics", {.handler = [weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            owner->_diagnostics.open();
        }
    }}});
    window.addMenuItem({"Import Settings", {.handler = [weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            owner->_importSettingsWindow.open();
        }
    }}});
    window.addMenuItem({"UV and Lightmap Baker...", {.handler = [weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            owner->_lightmap.open();
        }
    }}});
    _toolbar.addMenuItem(window);

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
    addPanelButton("Remove", [weakDelegate]
    {
        if (auto owner = weakDelegate.lock())
        {
            owner->removeSelection();
        }
    });
    addPanelButton("Model Properties", [weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            owner->_workspace.toggleLeftPanel();
        }
    }, bg2e::ui::Toolbar::AlignRight);
    addPanelButton("Scene", [weakUi]
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
                ui->_diagnostics.clear();
                ui->_sceneTree.setRootNode(nullptr);
                ui->_sceneTree.setSelectionManager(nullptr);
                ui->_sceneTree.onSelect({});
                ui->_sceneTree.onReparent({});
                ui->_componentInspector.setNode(nullptr);
                ui->_componentInspector.onAddRequested({});
                ui->_componentInspector.onRemoveRequested({});
                ui->_componentInspector.onDrawComponent({});
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
    _uiSettings.init(true, true);
    _uiSettings.close();
    if (auto delegate = _delegate.lock())
    {
        _lightmap.init(delegate, engine, session);
    }
    _importSettingsWindow.setTitle("Import Settings");
    _importSettingsWindow.setSize(420, 260);
    _importSettingsWindow.close();
    _importSettingsWindow.setDrawFunction([weakUi]
    {
        if (auto ui = weakUi.lock())
        {
            if (auto owner = ui->_delegate.lock())
            {
                ui->drawImportSettings(owner);
            }
        }
    });
    _importQueueWindow.setTitle("Import queue");
    _importQueueWindow.setPosition(20, 80);
    _importQueueWindow.setSize(380, 230);
    _importQueueWindow.options.noClose = true;
    _importQueueWindow.setDrawFunction([weakUi]
    {
        if (auto ui = weakUi.lock())
        {
            if (auto owner = ui->_delegate.lock())
            {
                ui->drawImportProgress(owner);
            }
        }
    });
    _fileStatus = std::make_shared<bg2e::ui::StatusItem>();
    _saveStatus = std::make_shared<bg2e::ui::StatusItem>();
    _importStatusItem = std::make_shared<bg2e::ui::StatusItem>();
    _statusBar.addItem(_importStatusItem, bg2e::ui::StatusBar::AlignLeft);
    _statusBar.addItem(_fileStatus, bg2e::ui::StatusBar::AlignLeft);
    _statusBar.addItem(_saveStatus, bg2e::ui::StatusBar::AlignRight);
    _workspace.setup(uiWidth(), uiHeight(), &_toolbar, &_properties, &_sceneProperties, nullptr, &_statusBar);
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
    _sceneProperties.setDrawFunction([weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            if (auto delegate = owner->_delegate.lock())
            {
                owner->drawSceneEditor(delegate);
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
    if (delegate->sceneLoading())
    {
        delegate->drawSceneLoader();
        return;
    }
    if (delegate->imports().busy())
    {
        if (auto job = delegate->imports().active(); job && !job->cancelled.load())
        {
            job->loader.draw();
        }
        _importQueueWindow.draw();
        return;
    }
    _importSettingsWindow.draw();
    if (auto deferred = dynamic_cast<bg2e::draw::RendererDeferred*>(delegate->renderer());
        deferred && !deferred->environmentError().empty())
    {
        bg2e::ui::Text::text("Environment error: " + deferred->environmentError());
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
        _saveStatus->setText(delegate->status().empty() ? "Unsaved changes" :
            "Unsaved changes — " + delegate->status());
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
    _importStatusItem->setText(delegate->importStatus());
    _workspace.draw();
    _graphics.draw();
    _diagnostics.draw();
    if (_uiSettings.isOpen())
    {
        _uiSettings.draw();
    }
    _lightmap.draw();
}

}
