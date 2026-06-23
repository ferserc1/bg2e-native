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

#include <bg2e.hpp>

#include "ToolBar.hpp"
#include "EnvironmentSettings.hpp"
#include "SubmeshWindow.hpp"
#include "StageScene.hpp"
#include <bg2e/ui/UISettingsWindow.hpp>
#include <bg2e/render/RenderSettingsPreferences.hpp>
#include <bg2e/ui/RenderSettingsWindow.hpp>

class AppDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererDeferred>,
	public bg2e::app::InputDelegate,
	public bg2e::ui::UserInterfaceDelegate
{
public:
    bg2e::render::Renderer * rendererBase() { return this->renderer(); }

    void init(bg2e::render::Engine * engine) override;
     
    void swapchainResized(VkExtent2D extent) override;

	void drawUI() override;
 
    // InputDelegate
    void mouseMove(int x, int y) override;
    
    void mouseButtonDown(int button, int x, int y) override;
    
    void mouseButtonUp(int button, int x, int y) override;
    
    void mouseWheel(int deltaX, int deltaY) override;

    void fileDropped(const std::filesystem::path&) override;
    
    void cleanup() override;
    
    // Application resources
    inline StageScene * stage() const { return _stage.get(); }
    inline StageScene * stage() { return _stage.get(); }
    
    inline bg2e::ui::Workspace& workspace() { return _workspace; }
    inline const bg2e::ui::Workspace& workspace() const { return _workspace; }

    inline std::shared_ptr<bg2e::manipulation::SelectionManager> selectionManager() const { return _selectionManager; }

    [[nodiscard]] std::shared_ptr<bg2e::ui::StatusItem> fileStatus() const { return _fileStatus; }
    [[nodiscard]] std::shared_ptr<bg2e::ui::StatusItem> saveStatus() const { return _saveStatus; }

    enum SelectionHighlightMode
    {
        SelectionFull,
        SelectionHard,
        SelectionSoft,
        SelectionHide
    };
    void toggleSelectionHighlight();
    void setSelectionHighlightMode(SelectionHighlightMode mode);



protected:
    bg2e::scene::InputVisitor _inputVisitor;
    
    bg2e::ui::Workspace _workspace;
    ToolBar _toolBar {};
    SubmeshWindow _submeshPanel {};
    EnvironmentSettings _environmentPanel {};
    bg2e::ui::UISettingsWindow _uiSettingsWindow {};
    bg2e::ui::RenderSettingsWindow _renderSettingsWindow {};
    std::unique_ptr<bg2e::render::RenderSettingsPreferences> _renderPrefs;

    std::shared_ptr<bg2e::ui::StatusItem> _fileStatus;
    std::shared_ptr<bg2e::ui::StatusItem> _saveStatus;
    bg2e::ui::StatusBar _statusBar;
    
    std::shared_ptr<StageScene> _stage;
    
    std::shared_ptr<bg2e::scene::Node> createScene() override;

    std::shared_ptr<bg2e::manipulation::SelectionManager> _selectionManager;
    uint32_t _mouseDownX = 0;
    uint32_t _mouseDownY = 0;

    SelectionHighlightMode _selectionHighlightMode = SelectionHard;
    
    void initWorkspace();

    void updateSelectionHighlight();
};
