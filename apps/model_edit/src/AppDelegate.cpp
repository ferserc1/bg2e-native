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
    _selectionManager = std::make_shared<bg2e::manipulation::SelectionManager>(engine);
    _selectionManager->init();
}

void AppDelegate::swapchainResized(VkExtent2D extent)
{
    DefaultRenderLoopDelegate::swapchainResized(extent);
    _workspace.resize(uiWidth(), uiHeight());
}

void AppDelegate::drawUI()
{
    _workspace.draw();
}

// InputDelegate
void AppDelegate::mouseMove(int x, int y)
{
    _inputVisitor.mouseMove(renderer()->scene()->rootNode(), x, y);
}

void AppDelegate::mouseButtonDown(int button, int x, int y)
{
    _mouseDownX = x;
    _mouseDownY = y;
    _inputVisitor.mouseButtonDown(renderer()->scene()->rootNode(), button, x, y);
}

void AppDelegate::mouseButtonUp(int button, int x, int y)
{
    if (_mouseDownX == x && _mouseDownY == y && button == 0)
    {
        // Pick selection
        if (_selectionManager->pick(renderer()->scene(), x, y))
        {
            auto selection = _selectionManager->selectedSubmesh();
            _submeshPanel.setEditMaterial(selection);
        }
    }
    _inputVisitor.mouseButtonUp(renderer()->scene()->rootNode(), button, x, y);
}

void AppDelegate::mouseWheel(int deltaX, int deltaY)
{
    _inputVisitor.mouseWheel(renderer()->scene()->rootNode(), deltaX, deltaY);
}

void AppDelegate::fileDropped(const std::filesystem::path& path)
{
    auto ext = path.extension();

    if (!stage()->checkUnsavedChanges())
    {
        return;
    }

    if (ext == ".bg2" || ext == ".vwglb")
    {
        stage()->loadModel(path);
    }
    else if (ext == ".obj")
    {
        stage()->importObj(path);
    }
    else if (ext == ".glb" || ext == ".gltf")
    {
        stage()->importGltf(path);
    }
}

std::shared_ptr<bg2e::scene::Node> AppDelegate::createScene()
{
    _stage = std::make_shared<StageScene>(_engine);
    
    auto scene = _stage->init();
    
    renderer()->setSkyboxBlurLevel(2);
    
    initWorkspace();
    
    return scene;
}

void AppDelegate::cleanup()
{
    DefaultRenderLoopDelegate::cleanup();
    _stage.reset();
    _submeshPanel.cleanup();
}

void AppDelegate::initWorkspace()
{
    _workspace.leftPanelSize().min = 300;
    _environmentPanel.init(this, renderer(), _stage->environment());
    _toolBar.init(this);
    _submeshPanel.init(this);
 
    _workspace.setup(
        uiWidth(), uiHeight(),
        &_toolBar,
        &_submeshPanel,
        &_environmentPanel,
        nullptr
    );
}

