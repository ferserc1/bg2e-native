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

#include "SceneWorkspace.hpp"
#include "../common/SettingsWindows.hpp"
#include "../common/WorkspaceSlots.hpp"

#include <bg2e/base/PlatformTools.hpp>

namespace bg2e_composer::workspaces::scene {

void SceneWorkspace::registerActions(bg2e::ui::WorkspaceCollection& workspaces)
{
    const auto weakDelegate = _delegate;
    const auto weakUi = weak_from_this();
    const std::weak_ptr<common::SettingsWindows> weakSettings = _settings;
    using MenuSeparator = bg2e::ui::Toolbar::MenuSeparator;
    const auto assets = bg2e::base::PlatformTools::assetPath();

    // File
    workspaces.registerActions({
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
            .path = "File",
            .workspaceMask = SceneSlot
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
            .path = "File",
            .workspaceMask = SceneSlot
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
            .path = "File",
            .workspaceMask = SceneSlot
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
            .path = "File",
            .workspaceMask = SceneSlot
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
            .path = "File",
            .workspaceMask = SceneSlot
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
            .path = "File",
            .workspaceMask = SharedSlots
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
            .path = "File",
            .workspaceMask = SharedSlots
        }
    }, MenuSeparator::AfterAction);

    workspaces.registerActions({
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
            .path = "File",
            .workspaceMask = SceneSlot
        }
    });

    // Edit
    workspaces.registerActions({
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
            .path = "Edit",
            .workspaceMask = SceneSlot
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
            .path = "Edit",
            .workspaceMask = SceneSlot
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
            .path = "Edit",
            .workspaceMask = SceneSlot
        }
    }, MenuSeparator::AfterAction);

    workspaces.registerActions({
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
            .path = "Edit",
            .workspaceMask = SceneSlot
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
            .path = "Edit",
            .workspaceMask = SceneSlot
        }
    }, MenuSeparator::AfterAction);

    workspaces.registerActions({
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
            .path = "Edit",
            .workspaceMask = SceneSlot
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
            .path = "Edit",
            .workspaceMask = SceneSlot
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
            .path = "Edit",
            .workspaceMask = SceneSlot
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
            .path = "Edit",
            .workspaceMask = SceneSlot
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
            .path = "Edit",
            .workspaceMask = SceneSlot
        }
    });

    // View
    workspaces.registerActions({
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
            .path = "View",
            .workspaceMask = SceneSlot
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
            .path = "View",
            .workspaceMask = SceneSlot
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
            .path = "View",
            .workspaceMask = SceneSlot
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
            .path = "View",
            .workspaceMask = SceneSlot
        }
    });

    // Create Node
    workspaces.registerActions({
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
            .path = "Create Node",
            .workspaceMask = SceneSlot
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
            .path = "Create Node",
            .workspaceMask = SceneSlot
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
            .path = "Create Node",
            .workspaceMask = SceneSlot
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
            .path = "Create Node",
            .workspaceMask = SceneSlot
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
            .path = "Create Node",
            .workspaceMask = SceneSlot
        }
    }, MenuSeparator::AfterAction);

    workspaces.registerActions({
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
            .path = "Create Node",
            .workspaceMask = SceneSlot
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
            .path = "Create Node",
            .workspaceMask = SceneSlot
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
            .path = "Create Node",
            .workspaceMask = SceneSlot
        }
    }, MenuSeparator::AfterAction);

    workspaces.registerActions({
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
            .path = "Create Node",
            .workspaceMask = SceneSlot
        }
    });

    // Window
    workspaces.registerActions({
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
            .path = "Window",
            .workspaceMask = SceneSlot
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
            .path = "Window",
            .workspaceMask = SceneSlot
        }
    }, MenuSeparator::AfterAction);

    workspaces.registerActions({
        {
            .id = "window.ui-settings",
            .label = "User Interface",
            .execute = [weakSettings]
            {
                if (auto settings = weakSettings.lock())
                {
                    settings->openInterface();
                }
            },
            .path = "Window|Settings",
            .workspaceMask = SharedSlots
        },
        {
            .id = "window.graphics-settings",
            .label = "Graphics",
            .execute = [weakSettings]
            {
                if (auto settings = weakSettings.lock())
                {
                    settings->graphics().open();
                }
            },
            .path = "Window|Settings",
            .workspaceMask = SharedSlots
        }
    });

    workspaces.registerActions({
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
            .path = "Window|Developer",
            .workspaceMask = SceneSlot
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
            .path = "Window|Developer",
            .workspaceMask = SceneSlot
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
            .path = "Window|Developer",
            .workspaceMask = SceneSlot
        }
    }, MenuSeparator::AfterSubmenu);

    workspaces.registerActions({
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
            .path = "Window",
            .workspaceMask = SceneSlot
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
            .path = "Window",
            .workspaceMask = SceneSlot
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
            .path = "Window",
            .workspaceMask = SceneSlot
        }
    });
}

}
