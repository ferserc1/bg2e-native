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
    _preferences->onSceneEdits([owner = std::weak_ptr<AppDelegate>(delegate), preferences = std::weak_ptr<bg2e::draw::RenderSettingsPreferences>(_preferences)]
        (auto changes, auto completion)
    {
        if (auto active = owner.lock())
        {
            active->editSceneSettings(preferences.lock(), std::move(changes), std::move(completion));
        }
        else if (completion)
        {
            completion({false, {{"target_expired", "", "Composer is no longer available"}}});
        }
    });
    _preferences->connect(delegate); // Before the renderer's first attachment.
    _graphics.bind(_preferences);
    _graphics.setSceneEditInteractionsEnabled(true);
    _graphics.setPosition(320, 80);
    _graphics.setSize(620, 740);
    _graphics.close();
    _properties.setTitle("Model Properties");
    _sceneProperties.setTitle("Scene");
    _workspace.leftPanelSize().min = 300;
}

void EditorUi::initActions()
{
    const auto weakDelegate = _delegate;
    const auto weakUi = weak_from_this();
    const auto assets = bg2e::base::PlatformTools::assetPath();

    // File
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
            .id = "file.new",
            .label = "New Scene",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->newDocument();
                }
            },
            .shortcut = {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyN},
            .path = "File"
        },
        {
            .id = "file.close",
            .label = "Close Scene",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->newDocument();
                }
            },
            .path = "File"
        },
        {
            .id = "file.import",
            .label = "Import Model or Prefab...",
            .execute = [weakUi]
            {
                if (auto ui = weakUi.lock())
                {
                    if (auto owner = ui->_delegate.lock())
                    {
                        ui->chooseImport(owner);
                    }
                }
            },
            .path = "File"
        },
        {
            .id = "file.export-selection",
            .label = "Export Selected Model(s)...",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->exportSelectedModels();
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

    // Edit
    _toolbar.registerActions({
        {
            .id = "edit.undo",
            .label = "Undo",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->undoEdit();
                }
            },
            .dynamicLabel = [weakDelegate]
            {
                const auto owner = weakDelegate.lock();
                return owner ? owner->historyActionLabel(false) : std::string("Undo");
            },
            .enabled = [weakDelegate]
            {
                const auto owner = weakDelegate.lock();
                return owner && owner->historyActionAvailable(false);
            },
            .shortcut = {
                .key = bg2e::app::KeyEvent::KeyZ,
                .cmdOrCtrlModifier = true,
                .allowInTextInput = false
            },
            .path = "Edit"
        },
        {
            .id = "edit.redo",
            .label = "Redo",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->redoEdit();
                }
            },
            .dynamicLabel = [weakDelegate]
            {
                const auto owner = weakDelegate.lock();
                return owner ? owner->historyActionLabel(true) : std::string("Redo");
            },
            .enabled = [weakDelegate]
            {
                const auto owner = weakDelegate.lock();
                return owner && owner->historyActionAvailable(true);
            },
            .shortcut = {
                .shiftModifier = true,
                .key = bg2e::app::KeyEvent::KeyZ,
                .cmdOrCtrlModifier = true,
                .allowInTextInput = false
            },
            .path = "Edit"
        },
        {
            .id = "edit.record-selection-history",
            .label = "Record selection in undo history",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->toggleSelectionHistory();
                }
            },
            .checked = [weakDelegate]
            {
                auto owner = weakDelegate.lock();
                return owner && owner->selectionHistoryEnabled();
            },
            .path = "Edit"
        }
    }, /*addSeparator=*/true);

    _toolbar.registerActions({
        {
            .id = "edit.duplicate",
            .label = "Duplicate",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->duplicateSelection();
                }
            },
            .path = "Edit"
        },
        {
            .id = "edit.remove",
            .label = "Remove",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->removeSelection();
                }
            },
            .path = "Edit"
        }
    }, /*addSeparator=*/true);

    _toolbar.registerActions({
        {
            .id = "geometry.z-up-to-y-up",
            .label = "Z up to Y up",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->editGeometry(command::geometry::Operation::ZUpToYUp);
                }
            },
            .enabled = [weakDelegate]
            {
                const auto owner = weakDelegate.lock();
                return owner && owner->geometryEditAvailable();
            },
            .path = "Edit"
        },
        {
            .id = "geometry.center",
            .label = "Center Geometry",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->editGeometry(command::geometry::Operation::Center);
                }
            },
            .enabled = [weakDelegate]
            {
                const auto owner = weakDelegate.lock();
                return owner && owner->geometryEditAvailable();
            },
            .path = "Edit"
        },
        {
            .id = "geometry.put-on-floor",
            .label = "Put on Floor",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->editGeometry(command::geometry::Operation::PutOnFloor);
                }
            },
            .enabled = [weakDelegate]
            {
                const auto owner = weakDelegate.lock();
                return owner && owner->geometryEditAvailable();
            },
            .path = "Edit"
        },
        {
            .id = "geometry.centimeters-to-meters",
            .label = "centimeters to meters",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->editGeometry(command::geometry::Operation::CentimetersToMeters);
                }
            },
            .enabled = [weakDelegate]
            {
                const auto owner = weakDelegate.lock();
                return owner && owner->geometryEditAvailable();
            },
            .path = "Edit"
        },
        {
            .id = "geometry.meters-to-centimeters",
            .label = "meters to centimeters",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->editGeometry(command::geometry::Operation::MetersToCentimeters);
                }
            },
            .enabled = [weakDelegate]
            {
                const auto owner = weakDelegate.lock();
                return owner && owner->geometryEditAvailable();
            },
            .path = "Edit"
        }
    });

    // View
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
            .shortcut = {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyF},
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

    // Create Node
    _toolbar.registerActions({
        {
            .id = "create.group",
            .label = "Group",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->addSceneItem(0);
                }
            },
            .path = "Create Node"
        },
        {
            .id = "create.camera",
            .label = "Camera",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->addSceneItem(1);
                }
            },
            .path = "Create Node"
        },
        {
            .id = "create.light",
            .label = "Light",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->addSceneItem(2);
                }
            },
            .path = "Create Node"
        },
        {
            .id = "create.environment",
            .label = "Environment",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->addSceneItem(3);
                }
            },
            .path = "Create Node"
        },
        {
            .id = "create.text",
            .label = "Text",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->addSceneItem(4);
                }
            },
            .path = "Create Node"
        }
    }, /*addSeparator=*/true);

    _toolbar.registerActions({
        {
            .id = "create.cube",
            .label = "Create Cube",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->addPrimitive(0);
                }
            },
            .path = "Create Node"
        },
        {
            .id = "create.sphere",
            .label = "Create Sphere",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->addPrimitive(1);
                }
            },
            .path = "Create Node"
        },
        {
            .id = "create.plane",
            .label = "Create Plane",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->addPrimitive(2);
                }
            },
            .path = "Create Node"
        }
    }, /*addSeparator=*/true);

    _toolbar.registerActions({
        {
            .id = "create.instance-from-owner",
            .label = "Instance from selected owner",
            .execute = [weakDelegate]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->createDrawableInstanceFromOwner();
                }
            },
            .enabled = [weakDelegate]
            {
                const auto owner = weakDelegate.lock();
                return owner && owner->canCreateDrawableInstanceFromOwner();
            },
            .path = "Create Node"
        }
    });

    // Window
    _toolbar.registerActions({
        {
            .id = "panel.model-properties",
            .label = "Model Properties",
            .iconPath = assets / "material-editor-icon.png",
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
            .id = "panel.scene",
            .label = "Scene",
            .iconPath = assets / "scene-editor-icon.png",
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
            .label = "User Interface",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_uiSettings.open();
                }
            },
            .path = "Window|Settings"
        },
        {
            .id = "window.graphics-settings",
            .label = "Graphics",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_graphics.open();
                }
            },
            .path = "Window|Settings"
        }
    });

    _toolbar.registerActions({
        {
            .id = "window.debug",
            .label = "Debug",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_debugWindow.open();
                }
            },
            .checked = [weakUi]
            {
                const auto owner = weakUi.lock();
                return owner && owner->_debugWindow.isOpen();
            },
            .path = "Window|Developer"
        },
        {
            .id = "window.performance",
            .label = "Performance",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_performance.open();
                }
            },
            .checked = [weakUi]
            {
                const auto owner = weakUi.lock();
                return owner && owner->_performance.isOpen();
            },
            .path = "Window|Developer"
        },
        {
            .id = "window.import-settings",
            .label = "Import Settings",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_importSettingsWindow.open();
                }
            },
            .path = "Window|Developer"
        }
    }, bg2e::ui::Toolbar::MenuSeparator::AfterSubmenu);

    _toolbar.registerActions({
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
        },
        {
            .id = "window.scene-resources",
            .label = "Scene Resources Manager",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_sceneResources.open();
                }
            },
            .path = "Window"
        },
        {
            .id = "window.drawable-instances",
            .label = "Drawable Instances",
            .execute = [weakUi]
            {
                if (auto owner = weakUi.lock())
                {
                    owner->_drawableInstances.open();
                }
            },
            .checked = [weakUi]
            {
                const auto owner = weakUi.lock();
                return owner && owner->_drawableInstances.isOpen();
            },
            .path = "Window"
        }
    });
}

void EditorUi::init(bg2e::draw::Engine* engine, bg2e::ui::UserInterface* session)
{
    _ui = session;
    initDrawableBrowser();
    const auto weakResourcesDelegate = _delegate;
    _sceneResources.configure([weakResourcesDelegate]() -> const bg2e::db::SceneResourceAnalysis*
    {
        auto owner = weakResourcesDelegate.lock();
        return owner ? owner->sceneResourceAnalysis() : nullptr;
    }, [weakResourcesDelegate]
    {
        auto owner = weakResourcesDelegate.lock();
        return owner && owner->sceneResourceOperationAvailable();
    }, [weakResourcesDelegate]
    {
        if (auto owner = weakResourcesDelegate.lock())
        {
            owner->analyzeSceneResources();
        }
    }, [weakResourcesDelegate](bool ignoreWarnings)
    {
        if (auto owner = weakResourcesDelegate.lock())
        {
            owner->cleanupSceneResources(ignoreWarnings);
        }
    }, [weakResourcesDelegate]
    {
        auto owner = weakResourcesDelegate.lock();
        return owner ? owner->status() : std::string{};
    });
    _toolbar.setTitle("Scene tools");
    _toolbar.options.noClose = true;
    const auto weakUi = weak_from_this();
    initActions();

    _toolbar.setDefaultLayout({
        .left = {},
        .right = {"panel.model-properties", "panel.scene"}
    });
    _toolbar.enableCustomization(bg2e::base::PlatformTools::settingsPath() / "composer-scene-tools.toolbar");
    if (auto delegate = _delegate.lock())
    {
        _material.init(engine, delegate);
        const auto weak = weak_from_this();
        delegate->onSettleUiInteractions([weak](bool commit)
        {
            auto ui = weak.lock();
            if (!ui)
            {
                return true;
            }
            if (commit)
            {
                return (!ui->_nameEdit || ui->_nameEdit->commit()) &&
                    (!ui->_metadataEdit || ui->_metadataEdit->commit()) &&
                    (!ui->_textEdit || ui->_textEdit->commit()) &&
                    ui->_componentInspector.commitInteraction() && ui->_material.commitInteraction() && ui->_graphics.commitInteraction();
            }
            if (ui->_nameEdit)
            {
                ui->_nameEdit->cancel();
            }
            if (ui->_metadataEdit)
            {
                ui->_metadataEdit->cancel();
            }
            ui->_componentInspector.cancelInteraction();
            if (ui->_textEdit)
            {
                ui->_textEdit->cancel();
            }
            ui->_material.cancelInteraction();
            ui->_graphics.cancelInteraction();
            return (!ui->_nameEdit || !ui->_nameEdit->active()) &&
                (!ui->_metadataEdit || !ui->_metadataEdit->active()) &&
                (!ui->_textEdit || !ui->_textEdit->active()) &&
                !ui->_componentInspector.interactionActive() && !ui->_material.interactionActive();
        });
        delegate->onToolsCleanup([weak]
        {
            if (auto ui = weak.lock())
            {
                ui->_lightmap.cleanup();
                ui->_material.cleanup();
                if (ui->_nameEdit)
                {
                    ui->_nameEdit->cancel();
                    ui->_nameEdit.reset();
                }
                if (ui->_metadataEdit)
                {
                    ui->_metadataEdit->cancel();
                    ui->_metadataEdit.reset();
                }
                ui->_componentInspector.cancelInteraction();
                if (ui->_textEdit)
                {
                    ui->_textEdit->cancel();
                }
                ui->_componentInspector.onEditRequested({});
                ui->_componentInspector.onActionRequested({});
                ui->_graphics.clear();
                ui->_debugOptions = {};
                ui->_debugRendererGeneration = UINT64_MAX;
                ui->_performance.close();
                ui->_sceneTree.setRootNode(nullptr);
                ui->_sceneTree.setSelectionManager(nullptr);
                ui->_sceneTree.onSelect({});
                ui->_sceneTree.onReparent({});
                ui->_componentInspector.setNode(nullptr);
                ui->_componentInspector.setValidationRoot({});
                ui->_componentInspector.onAddRequested({});
                ui->_componentInspector.onRemoveRequested({});
                ui->_componentInspector.onIssueFixRequested({});
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
                    if (ui->_metadataEdit)
                    {
                        ui->_metadataEdit->cancel();
                    }
                    ui->_material.setSelectionManager(delegate->selection());
                }
            }
        });
    }
    _debugWindow.setTitle("Debug");
    _debugWindow.setPosition(340,100);
    _debugWindow.setSize(500,530);
    _debugWindow.close();
    _debugWindow.setDrawFunction([weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            if (auto delegate = owner->_delegate.lock())
            {
                owner->drawDebug(delegate);
            }
        }
    });
    _uiSettings.init(true, true);
    _uiSettings.setToolbarSettingsVisible(false);
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
    _workspace.setToolBarHeight(68);
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
    _componentInspectorDrawn = false;
    if (_nameEdit)
    {
        _nameEdit->beginFrame();
    }
    if (_metadataEdit)
    {
        _metadataEdit->beginFrame();
    }
    if (_textEdit)
    {
        _textEdit->beginFrame();
    }
    struct SettleMissingControls {
        EditorUi& ui;
        ~SettleMissingControls()
        {
            if (ui._nameEdit)
            {
                ui._nameEdit->endFrame();
            }
            if (ui._metadataEdit)
            {
                ui._metadataEdit->endFrame();
            }
            if (ui._textEdit)
            {
                ui._textEdit->endFrame();
            }
            if (!ui._componentInspectorDrawn)
            {
                ui._componentInspector.cancelInteraction();
            }
        }
    } settleMissing{*this};
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
    _sceneResources.draw();
    _drawableInstances.draw();
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
    _debugWindow.draw();
    if (!_debugWindow.isOpen() && _debugOptions.enabled)
    {
        _debugOptions.enabled = false;
        const auto options = _debugOptions;
        delegate->enqueueRendererUpdate(delegate->rendererGeneration(),[options](bg2e::draw::Renderer& renderer)
        {
            if (auto* deferred = dynamic_cast<bg2e::draw::RendererDeferred*>(&renderer))
            {
                deferred->setDebugOptions(options);
            }
        });
    }
    _performance.draw();
    if (_uiSettings.isOpen())
    {
        _uiSettings.draw();
    }
    _lightmap.draw();
}

}
