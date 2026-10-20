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

#include "AppDelegate.hpp"

#include <bg2e/draw/RenderSettingsPreferences.hpp>
#include <bg2e/ui/GraphicsSettingsWindow.hpp>
#include <bg2e/ui/UISettingsWindow.hpp>
#include <bg2e/ui/UserInterfaceDelegate.hpp>
#include <bg2e/ui/Window.hpp>
#include <bg2e/ui/Toolbar.hpp>
#include <bg2e/ui/DrawMaterialEditor.hpp>
#include <bg2e/ui/StatusBar.hpp>
#include <bg2e/ui/Workspace.hpp>
#include <bg2e/ui/SceneTree.hpp>
#include <bg2e/ui/ComponentInspector.hpp>
#include "SceneLightmapWindow.hpp"

namespace bg2e_composer {

class EditorUi final : public bg2e::ui::UserInterfaceDelegate, public std::enable_shared_from_this<EditorUi> {
  public:
    explicit EditorUi(const std::shared_ptr<AppDelegate>& delegate);

    void init(bg2e::draw::Engine* engine, bg2e::ui::UserInterface* session) override;
    void drawUI() override;

  private:
    void drawImportSettings(const std::shared_ptr<AppDelegate>& delegate);
    void drawImportProgress(const std::shared_ptr<AppDelegate>& delegate);
    void chooseImport(const std::shared_ptr<AppDelegate>& delegate);

    void drawSceneEditor(const std::shared_ptr<AppDelegate>& delegate);
    void drawTools(const std::shared_ptr<AppDelegate>& delegate);
    void bindDocument(const std::shared_ptr<AppDelegate>& delegate);

    bg2e::ui::UserInterface* _ui = nullptr;
    bg2e::ui::DrawMaterialEditor _material;
    bg2e::ui::SceneTree _sceneTree;
    bg2e::ui::ComponentInspector _componentInspector;
    bg2e::ui::Window _properties;
    bg2e::ui::Window _sceneProperties;
    uint64_t _boundGeneration = UINT64_MAX;
    uint32_t _highlight = 2;
    std::weak_ptr<AppDelegate> _delegate;
    std::shared_ptr<bg2e::draw::RenderSettingsPreferences> _preferences;
    bg2e::ui::GraphicsSettingsWindow _graphics;
    bg2e::ui::RendererDebugWidget _diagnostics;
    bg2e::ui::UISettingsWindow _uiSettings;
    SceneLightmapWindow _lightmap;
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
