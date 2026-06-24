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
#include "AppDelegate.hpp"

void AppDelegate::init(bg2e::render::Engine * engine)
{
    bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererDeferred>::init(engine);
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
    if (_uiSettingsWindow.isOpen())
    {
        _uiSettingsWindow.draw();
    }
    if (_renderSettingsWindow.isOpen())
    {
        _renderSettingsWindow.draw();
    }
}

// InputDelegate
// The SelectionManager sees every mouse event first. It returns false when it
// captures the event (transform gizmo manipulation), in which case we must not
// forward it to the scene graph (so the camera does not move). It also performs
// the click selection internally (on mouse up, only if the press did not move).
void AppDelegate::mouseMove(int x, int y)
{
    if (_selectionManager->mouseMove(renderer()->scene(), x, y))
    {
        _inputVisitor.mouseMove(renderer()->scene()->rootNode(), x, y);
    }
}

void AppDelegate::mouseButtonDown(int button, int x, int y)
{
    if (_selectionManager->mouseButtonDown(renderer()->scene(), button, x, y))
    {
        _inputVisitor.mouseButtonDown(renderer()->scene()->rootNode(), button, x, y);
    }
}

void AppDelegate::mouseButtonUp(int button, int x, int y)
{
    bool additive = bg2e::app::Keyboard::shiftPressed();
    if (_selectionManager->mouseButtonUp(renderer()->scene(), button, x, y, additive))
    {
        _inputVisitor.mouseButtonUp(renderer()->scene()->rootNode(), button, x, y);
    }
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
        stage()->importModelBg2(path);
    }
    else if (ext == ".json" || ext == ".vitscnj")
    {
        bg2e::app::MainLoop::current()->asyncLoad([&, path](bg2e::ui::Loader* loader)
        {
            stage()->openScene(path, [&](const std::string& modelName, uint32_t processed, uint32_t total) {
                loader->setMessage("Loading model " + modelName + "...");
                loader->setProgress(static_cast<float>(processed) / static_cast<float>(total));
            });
        }, glm::vec4{ 0.2, 0.2, 0.31, 1.0f });
    }
}

std::shared_ptr<bg2e::scene::Node> AppDelegate::createScene()
{
    _stage = std::make_shared<StageScene>(_engine, this);
    
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
    _sceneEditor.cleanup();
}

void AppDelegate::toggleSelectionHighlight()
{
    switch (_selectionHighlightMode)
    {
    case SelectionFull:
        _selectionHighlightMode = SelectionHard;
        break;
    case SelectionHard:
        _selectionHighlightMode = SelectionSoft;
        break;
    case SelectionSoft:
        _selectionHighlightMode = SelectionHide;
        break;
    case SelectionHide:
        _selectionHighlightMode = SelectionFull;
        break;
    }
    updateSelectionHighlight();
}

void AppDelegate::setSelectionHighlightMode(SelectionHighlightMode mode)
{
    _selectionHighlightMode = mode;
    updateSelectionHighlight();
}


void AppDelegate::initWorkspace()
{
    // Initialize preferences and load saved settings
    auto* deferredRenderer = dynamic_cast<bg2e::render::RendererDeferred*>(rendererBase());
    _renderPrefs = std::make_unique<bg2e::render::RenderSettingsPreferences>(deferredRenderer);
    _renderPrefs->load();

    // Register timer: persist every 60 seconds, also persist on exit
    bg2e::app::MainLoop::current()->timeout().add([this]() -> bool {
        _renderPrefs->persist();
        return true;
    }, 60000, true);  // executeOnExit = true

    _workspace.leftPanelSize().min = 300;
    _sceneEditor.init(this);

    // The SelectionManager is the single source of truth for selection. The
    // scene tree both reads from it (to highlight rows) and writes to it (on
    // click), so no bidirectional bridge or re-entrancy guard is needed.
    _sceneEditor.sceneTree().setSelectionManager(_selectionManager.get());

    // Keep the node editor in sync with whatever changes the selection (tree
    // clicks, 3D picking or programmatic changes). The material panel reacts on
    // its own through MaterialEditor's onSelect callback.
    _selectionManager->onSelect([&]() {
        std::vector<bg2e::scene::Node*> nodes;
        for (const auto& weakNode : _selectionManager->selectedNodes())
        {
            if (auto node = weakNode.lock())
            {
                nodes.push_back(node.get());
            }
        }

        if (nodes.empty())          _sceneEditor.nodeEditor().setNode(nullptr);
        else if (nodes.size() == 1) _sceneEditor.nodeEditor().setNode(nodes.front());
        else                        _sceneEditor.nodeEditor().setNodes(nodes);

        // Keep the submesh/material panel in sync with the primary selection.
        if (!nodes.empty())
        {
            _submeshPanel.setEditMaterial(_selectionManager->selectedSubmesh());
        }
    });

    // Edge case: clear the selection on scene swap. The onSelect callback above
    // takes care of clearing the node editor.
    _stage->onSceneSwap([&]() {
        _selectionManager->deselect();
    });

    _uiSettingsWindow.init();
    _renderSettingsWindow.init(deferredRenderer, _renderPrefs.get());
    _toolBar.init(this, &_uiSettingsWindow, &_renderSettingsWindow);
    _submeshPanel.init(this);

    _fileStatus = std::make_shared<bg2e::ui::StatusItem>();
    _saveStatus = std::make_shared<bg2e::ui::StatusItem>();
    _statusBar.addItem(_fileStatus, bg2e::ui::StatusBar::AlignLeft);
    _statusBar.addItem(_saveStatus, bg2e::ui::StatusBar::AlignRight);

    // Force update status to setup the initial state of
    // fileStatus and saveStatus
    _stage->document()->updateStatus();
 
    _workspace.setup(
        uiWidth(), uiHeight(),
        &_toolBar,
        &_submeshPanel,
        &_sceneEditor,
        nullptr,  // Use nullptr if you dont't want to use this panel
        &_statusBar
    );
}

void AppDelegate::updateSelectionHighlight()
{
    switch (_selectionHighlightMode)
    {
    case SelectionFull:
        renderer()->gizmoAndSelectionRenderer()->setLineIntensity(0.5f);
        break;
    case SelectionHard:
        renderer()->gizmoAndSelectionRenderer()->setLineIntensity(0.3f);
        break;
    case SelectionSoft:
        renderer()->gizmoAndSelectionRenderer()->setLineIntensity(0.1f);
        break;
    case SelectionHide:
        renderer()->gizmoAndSelectionRenderer()->setLineIntensity(0.0f);
        break;
    }
}
