//
//  AppDelegate.cpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#include "AppDelegate.hpp"

void AppDelegate::init(bg2e::render::Engine * engine)
{
    bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>::init(engine);
}

// ============ User Interface Delegate Functions =========
void AppDelegate::init(bg2e::render::Engine*, bg2e::ui::UserInterface*)
{
    _window.setTitle("Basic forward renderer");
    _window.options.noClose = true;
    _window.options.minWidth = 300;
    _window.options.minHeight = 190;
    _window.setPosition(0, 50);
    _window.setSize(300, 290);

    _environmentSettings = std::make_unique<EnvironmentSettings>();
    _environmentSettings->init(uiWidth(), uiHeight());
    
    _menuBar = std::make_unique<ToolBar>(this);
    _menuBar->init(uiWidth(), _environmentSettings);
}

void AppDelegate::swapchainResized(VkExtent2D extent)
{
    DefaultRenderLoopDelegate::swapchainResized(extent);
    _menuBar->resize(uiWidth());
    _environmentSettings->resizeViewport(uiWidth(), uiHeight());
}

void AppDelegate::drawUI()
{
    _menuBar->draw();
    
    _environmentSettings->draw(renderer(), _stage->environment());
}

// InputDelegate
void AppDelegate::mouseMove(int x, int y)
{
    _inputVisitor.mouseMove(renderer()->scene()->rootNode(), x, y);
}

void AppDelegate::mouseButtonDown(int button, int x, int y)
{
    _inputVisitor.mouseButtonDown(renderer()->scene()->rootNode(), button, x, y);
}

void AppDelegate::mouseButtonUp(int button, int x, int y)
{
    _inputVisitor.mouseButtonUp(renderer()->scene()->rootNode(), button, x, y);
}

void AppDelegate::mouseWheel(int deltaX, int deltaY)
{
    _inputVisitor.mouseWheel(renderer()->scene()->rootNode(), deltaX, deltaY);
}

std::shared_ptr<bg2e::scene::Node> AppDelegate::createScene()
{
    _stage = std::make_unique<StageScene>(_engine);
    
    return _stage->init();
}

void AppDelegate::cleanup()
{
    DefaultRenderLoopDelegate::cleanup();
    _stage.reset();
}
