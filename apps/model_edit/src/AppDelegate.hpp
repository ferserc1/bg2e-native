//
//  SceneDelegate.hpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#pragma once

#include <bg2e.hpp>

#include "ToolBar.hpp"
#include "EnvironmentSettings.hpp"
#include "StageScene.hpp"

class AppDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>,
	public bg2e::app::InputDelegate,
	public bg2e::ui::UserInterfaceDelegate
{
public:

    void init(bg2e::render::Engine * engine) override;
    
	// ============ User Interface Delegate Functions =========
	void init(bg2e::render::Engine*, bg2e::ui::UserInterface*) override;
 
    void swapchainResized(VkExtent2D extent) override;

	void drawUI() override;
 
    // InputDelegate
    void mouseMove(int x, int y) override;
    
    void mouseButtonDown(int button, int x, int y) override;
    
    void mouseButtonUp(int button, int x, int y) override;
    
    void mouseWheel(int deltaX, int deltaY) override;
    
    void cleanup() override;
    
    // Application resources
    inline StageScene * stage() const { return _stage.get(); }
    inline StageScene * stage() { return _stage.get(); }

protected:
	bg2e::ui::Window _window;
    bg2e::scene::InputVisitor _inputVisitor;
    
    std::unique_ptr<ToolBar> _menuBar;
    std::shared_ptr<EnvironmentSettings> _environmentSettings;
    
    std::unique_ptr<StageScene> _stage;
    
    std::shared_ptr<bg2e::scene::Node> createScene() override;
};
