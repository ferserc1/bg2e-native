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

#pragma once

#include <bg2e/ui/EditInteraction.hpp>
#include <bg2e/draw/RendererDebug.hpp>

#include "../../AppDelegate.hpp"

#include <bg2e/ui/Performance.hpp>
#include <bg2e/ui/Window.hpp>
#include <bg2e/ui/SceneResourcesWindow.hpp>
#include <bg2e/ui/DrawableInstancesWindow.hpp>
#include <bg2e/ui/Toolbar.hpp>
#include <bg2e/ui/DrawMaterialEditor.hpp>
#include <bg2e/ui/StatusBar.hpp>
#include <bg2e/ui/Workspace.hpp>
#include <bg2e/ui/SceneTree.hpp>
#include <bg2e/ui/ComponentInspector.hpp>
#include "SceneLightmapWindow.hpp"

#include <cstdint>
#include <memory>

namespace bg2e_composer::workspaces::common { class SettingsWindows; }

namespace bg2e_composer::workspaces::scene {

class SceneWorkspace final : public std::enable_shared_from_this<SceneWorkspace> {
  public:
    SceneWorkspace(const std::shared_ptr<AppDelegate>& delegate,
        const std::shared_ptr<common::SettingsWindows>& settings);

    void init(bg2e::draw::Engine* engine, bg2e::ui::UserInterface* session, uint32_t width, uint32_t height,
        bg2e::ui::WorkspaceCollection& workspaces);
    void draw(uint32_t width, uint32_t height);

    bg2e::ui::Workspace& layout();

  private:
    void registerActions(bg2e::ui::WorkspaceCollection& workspaces);
    void initDrawableBrowser();
    void drawDebug(const std::shared_ptr<AppDelegate>& delegate);

    void drawImportSettings(const std::shared_ptr<AppDelegate>& delegate);
    void drawImportProgress(const std::shared_ptr<AppDelegate>& delegate);
    void chooseImport(const std::shared_ptr<AppDelegate>& delegate);

    void drawSceneEditor(const std::shared_ptr<AppDelegate>& delegate);
    void drawTextEditors(const std::shared_ptr<AppDelegate>& delegate, bg2e::scene::Node& node);
    void drawTools(const std::shared_ptr<AppDelegate>& delegate);
    void bindDocument(const std::shared_ptr<AppDelegate>& delegate);

    bg2e::ui::UserInterface* _ui = nullptr;
    bg2e::ui::DrawMaterialEditor _material;
    bg2e::ui::SceneTree _sceneTree;
    bg2e::ui::ComponentInspector _componentInspector;
    std::unique_ptr<bg2e::ui::EditSession> _nameEdit;
    std::unique_ptr<bg2e::ui::EditSession> _metadataEdit;
    std::unique_ptr<bg2e::ui::EditSession> _textEdit;
    uint64_t _textEditGeneration = UINT64_MAX;
    bool _textUiWarning = false;
    bool _componentInspectorDrawn = false;
    std::weak_ptr<bg2e::scene::Node> _nameEditNode;
    uint64_t _nameEditGeneration = 0;
    bg2e::ui::Window _debugWindow;
    bg2e::draw::RendererDebugOptions _debugOptions;
    uint64_t _debugRendererGeneration = UINT64_MAX;
    bg2e::ui::Window _properties;
    bg2e::ui::Window _sceneProperties;
    uint64_t _boundGeneration = UINT64_MAX;
    uint32_t _highlight = 2;
    std::weak_ptr<AppDelegate> _delegate;
    std::shared_ptr<common::SettingsWindows> _settings;
    bg2e::ui::Performance _performance;
    SceneLightmapWindow _lightmap;
    bg2e::ui::SceneResourcesWindow _sceneResources;
    bg2e::ui::DrawableInstancesWindow _drawableInstances;
    bg2e::ui::Window _importSettingsWindow;
    bg2e::ui::Window _importQueueWindow;
    std::string _importPort;
    uint32_t _importUnits = 0;
    bool _importZUp = false;
    bg2e::ui::Toolbar _toolbar;
    bg2e::ui::Workspace _workspace;
    bg2e::ui::StatusBar _statusBar;
    std::shared_ptr<bg2e::ui::StatusItem> _fileStatus;
    std::shared_ptr<bg2e::ui::StatusItem> _importStatusItem;
    std::shared_ptr<bg2e::ui::StatusItem> _saveStatus;
    uint32_t _workspaceWidth = 0;
    uint32_t _workspaceHeight = 0;
};

}
