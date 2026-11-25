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
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/draw/RendererDeferred.hpp>

#include <iterator>
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

bg2e::ui::MenuItem EditorUi::makeMenuAction(bg2e::ui::ActionDefinition definition, ActionPlacement placement)
{
    auto action = std::make_shared<const bg2e::ui::ActionDefinition>(std::move(definition));
    if (placement == ActionPlacement::MenuAndToolbar)
    {
        _toolbar.registerAction(action);
    }
    bg2e::ui::ActionDefinition::registerShortcut(bg2e::app::MainLoop::shortcuts(), action);
    return bg2e::ui::MenuItem::fromAction(std::move(action));
}

void EditorUi::initFileMenu()
{
    const auto weakDelegate = _delegate;
    const auto weakUi = weak_from_this();
    bg2e::ui::MenuItem file("File");
    file.addMenuItem(makeMenuAction({
        .id = "file.open",
        .label = "Open...",
        .execute = [weakDelegate]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->openDocument();
            }
        },
        .shortcut = {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyO}
    }));
    file.addMenuItem(makeMenuAction({
        .id = "file.new",
        .label = "New Scene",
        .execute = [weakDelegate]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->newDocument();
            }
        },
        .shortcut = {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyN}
    }));
    file.addMenuItem(makeMenuAction({
        .id = "file.close",
        .label = "Close Scene",
        .execute = [weakDelegate]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->newDocument();
            }
        }
    }, ActionPlacement::MenuOnly));
    file.addMenuItem(makeMenuAction({
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
        }
    }));
    file.addMenuItem(makeMenuAction({
        .id = "file.export-selection",
        .label = "Export Selected Model(s)...",
        .execute = [weakDelegate]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->exportSelectedModels();
            }
        }
    }));
    file.addMenuItem(makeMenuAction({
        .id = "file.save",
        .label = "Save",
        .execute = [weakDelegate]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->saveDocument();
            }
        },
        .shortcut = {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyS}
    }));
    file.addMenuItem(makeMenuAction({
        .id = "file.save-as",
        .label = "Save as...",
        .execute = [weakDelegate]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->saveDocument(true);
            }
        },
        .shortcut = {.ctrlModifier = true, .shiftModifier = true, .key = bg2e::app::KeyEvent::KeyS}
    }));
    file.addMenuItem({});
    file.addMenuItem(makeMenuAction({
        .id = "file.quit",
        .label = "Quit",
        .execute = [weakDelegate]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->requestQuit();
            }
        },
        .shortcut = {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyQ}
    }, ActionPlacement::MenuOnly));
    _toolbar.addMenuItem(file);
}

void EditorUi::initEditMenu()
{
    const auto weakDelegate = _delegate;
    bg2e::ui::MenuItem edit("Edit");
    const auto historyItem = [this, weakDelegate](bool redo)
    {
        return makeMenuAction({
            .id = redo ? "edit.redo" : "edit.undo",
            .label = redo ? "Redo" : "Undo",
            .execute = [weakDelegate, redo]
            {
                if (auto owner = weakDelegate.lock())
                {
                    if (redo)
                    {
                        owner->redoEdit();
                    }
                    else
                    {
                        owner->undoEdit();
                    }
                }
            },
            .dynamicLabel = [weakDelegate, redo]
            {
                auto owner = weakDelegate.lock();
                return owner ? owner->historyActionLabel(redo) : std::string(redo ? "Redo" : "Undo");
            },
            .enabled = [weakDelegate, redo]
            {
                auto owner = weakDelegate.lock();
                return owner && owner->historyActionAvailable(redo);
            },
            .shortcut = {
                .shiftModifier = redo,
                .key = bg2e::app::KeyEvent::KeyZ,
                .cmdOrCtrlModifier = true,
                .allowInTextInput = false
            }
        });
    };
    edit.addMenuItem(historyItem(false));
    edit.addMenuItem(historyItem(true));
    edit.addMenuItem(makeMenuAction({
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
        }
    }, ActionPlacement::MenuOnly));
    edit.addMenuItem({});
    edit.addMenuItem(makeMenuAction({
        .id = "edit.duplicate",
        .label = "Duplicate",
        .execute = [weakDelegate]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->duplicateSelection();
            }
        }
    }));
    edit.addMenuItem(makeMenuAction({
        .id = "edit.remove",
        .label = "Remove",
        .execute = [weakDelegate]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->removeSelection();
            }
        }
    }));
    edit.addMenuItem({});
    using GeometryOperation = command::geometry::Operation;
    struct GeometryAction {
        const char* id;
        const char* label;
        GeometryOperation operation;
    };
    const GeometryAction geometryActions[]{
        {"geometry.z-up-to-y-up", "Z up to Y up", GeometryOperation::ZUpToYUp},
        {"geometry.center", "Center Geometry", GeometryOperation::Center},
        {"geometry.put-on-floor", "Put on Floor", GeometryOperation::PutOnFloor},
        {"geometry.centimeters-to-meters", "centimeters to meters", GeometryOperation::CentimetersToMeters},
        {"geometry.meters-to-centimeters", "meters to centimeters", GeometryOperation::MetersToCentimeters}
    };
    for (const auto& [id, label, operation] : geometryActions)
    {
        edit.addMenuItem(makeMenuAction({
            .id = id,
            .label = label,
            .execute = [weakDelegate, operation]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->editGeometry(operation);
                }
            },
            .enabled = [weakDelegate]
            {
                const auto owner = weakDelegate.lock();
                return owner && owner->geometryEditAvailable();
            }
        }));
    }
    _toolbar.addMenuItem(edit);
}

void EditorUi::initViewMenu()
{
    const auto weakDelegate = _delegate;
    const auto weakUi = weak_from_this();
    bg2e::ui::MenuItem view("View");
    view.addMenuItem(makeMenuAction({
        .id = "view.reset-camera",
        .label = "Reset camera",
        .execute = [weakDelegate]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->resetCamera();
            }
        }
    }));
    view.addMenuItem(makeMenuAction({
        .id = "view.frame-selection",
        .label = "Frame selection",
        .execute = [weakDelegate]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->frameSelection();
            }
        }
    }));
    view.addMenuItem(makeMenuAction({
        .id = "view.analyze-best-view",
        .label = "Analyze best view",
        .execute = [weakDelegate]
        {
            if (auto owner = weakDelegate.lock())
            {
                owner->analyzeBestView();
            }
        },
        .shortcut = {.ctrlModifier = true, .key = bg2e::app::KeyEvent::KeyF}
    }));
    view.addMenuItem(makeMenuAction({
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
        .shortcut = {.shiftModifier = true, .key = bg2e::app::KeyEvent::KeySpace}
    }));
    _toolbar.addMenuItem(view);
}

void EditorUi::initCreateNodeMenu()
{
    const auto weakDelegate = _delegate;
    bg2e::ui::MenuItem create("Create Node");
    struct CreateAction {
        const char* id;
        const char* label;
    };
    constexpr CreateAction nodeTypes[]{
        {"create.group", "Group"},
        {"create.camera", "Camera"},
        {"create.light", "Light"},
        {"create.environment", "Environment"},
        {"create.text", "Text"}
    };
    for (unsigned kind = 0; kind < std::size(nodeTypes); ++kind)
    {
        const auto& [id, label] = nodeTypes[kind];
        create.addMenuItem(makeMenuAction({
            .id = id,
            .label = label,
            .execute = [weakDelegate, kind]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->addSceneItem(kind);
                }
            }
        }));
    }
    create.addMenuItem({});
    constexpr CreateAction primitiveTypes[]{
        {"create.cube", "Create Cube"},
        {"create.sphere", "Create Sphere"},
        {"create.plane", "Create Plane"}
    };
    for (unsigned kind = 0; kind < std::size(primitiveTypes); ++kind)
    {
        const auto& [id, label] = primitiveTypes[kind];
        create.addMenuItem(makeMenuAction({
            .id = id,
            .label = label,
            .execute = [weakDelegate, kind]
            {
                if (auto owner = weakDelegate.lock())
                {
                    owner->addPrimitive(kind);
                }
            }
        }));
    }
    create.addMenuItem({});
    create.addMenuItem(makeMenuAction({
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
        }
    }));
    _toolbar.addMenuItem(create);
}

void EditorUi::initWindowMenu()
{
    const auto weakUi = weak_from_this();
    bg2e::ui::MenuItem window("Window");
    const auto assets = bg2e::base::PlatformTools::assetPath();
    window.addMenuItem(makeMenuAction({
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
        }
    }));
    window.addMenuItem(makeMenuAction({
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
        }
    }));
    window.addMenuItem({});
    window.addMenuItem(makeMenuAction({
        .id = "window.ui-settings",
        .label = "UI Settings",
        .execute = [weakUi]
        {
            if (auto owner = weakUi.lock())
            {
                owner->_uiSettings.open();
            }
        }
    }, ActionPlacement::MenuOnly));
    window.addMenuItem(makeMenuAction({
        .id = "window.graphics-settings",
        .label = "Graphics Settings",
        .execute = [weakUi]
        {
            if (auto owner = weakUi.lock())
            {
                owner->_graphics.open();
            }
        }
    }));
    window.addMenuItem(makeMenuAction({
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
        }
    }, ActionPlacement::MenuOnly));
    window.addMenuItem(makeMenuAction({
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
        }
    }));
    window.addMenuItem(makeMenuAction({
        .id = "window.import-settings",
        .label = "Import Settings",
        .execute = [weakUi]
        {
            if (auto owner = weakUi.lock())
            {
                owner->_importSettingsWindow.open();
            }
        }
    }));
    window.addMenuItem(makeMenuAction({
        .id = "window.lightmap-baker",
        .label = "UV and Lightmap Baker...",
        .execute = [weakUi]
        {
            if (auto owner = weakUi.lock())
            {
                owner->_lightmap.open();
            }
        }
    }));
    window.addMenuItem(makeMenuAction({
        .id = "window.scene-resources",
        .label = "Scene Resources Manager",
        .execute = [weakUi]
        {
            if (auto owner = weakUi.lock())
            {
                owner->_sceneResources.open();
            }
        }
    }));
    window.addMenuItem(makeMenuAction({
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
        }
    }));
    _toolbar.addMenuItem(window);
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
    initFileMenu();
    initEditMenu();
    initViewMenu();

    initCreateNodeMenu();

    initWindowMenu();

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
