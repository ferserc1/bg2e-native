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

#include <bg2e/db/image.hpp>

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

void AppDelegate::update(
    uint32_t currentFrame,
    bg2e::render::vulkan::FrameResources& frameResources
)
{
    DefaultRenderLoopDelegate::update(currentFrame, frameResources);

    if (_bakeState == LightmapBakeState::Readback && _bakeBaker)
    {
        try
        {
            finishLightmapBake();
        }
        catch (const std::logic_error&)
        {
            // The recorded frame has not advanced yet; retry on the next update.
        }
        catch (const std::exception& error)
        {
            _bakeError = error.what();
            cancelLightmapBake();
        }
    }
}

VkImageLayout AppDelegate::render(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const bg2e::render::vulkan::Image* colorImage,
    const bg2e::render::vulkan::Image* depthImage,
    const bg2e::render::vulkan::Image* msaaDepthImage,
    bg2e::render::vulkan::FrameResources& frameResources
)
{
    auto layout = DefaultRenderLoopDelegate::render(
        cmd, currentFrame, colorImage, depthImage, msaaDepthImage, frameResources);

    if (_bakeState == LightmapBakeState::Baking && _bakeBaker)
    {
        try
        {
            _bakeContext->prepareFrame(cmd, frameResources);
            _bakeBaker->update(cmd, frameResources);
            if (_bakeBaker->completedFrames() >= _bakeBaker->settings().accumulationFrames)
            {
                _bakeState = LightmapBakeState::Readback;
            }
        }
        catch (const std::exception& error)
        {
            _bakeError = error.what();
            cancelLightmapBake();
        }
    }
    return layout;
}

void AppDelegate::requestLightmapBake(const bg2e::render::LightmapSettings& settings)
{
    if (lightmapBakeActive())
    {
        throw std::runtime_error("A lightmap bake is already in progress");
    }
    auto target = _stage->targetModelNode();
    if (!target)
    {
        throw std::runtime_error("No active model to bake");
    }

    auto* root = renderer()->scene()->rootNode();
    if (!_bakeContext || _bakeContext->rootNode() != root)
    {
        _bakeContext = std::make_shared<bg2e::render::IntegratedBakerContext>(_engine, root);
        _bakeContext->setEnvironmentResources(renderer()->environmentResources());
    }

    bg2e::render::LightmapSettings bakeSettings = settings;
    bakeSettings.mode = bg2e::render::LightmapMode::RTAO;
    bakeSettings.cpuFormat = bg2e::render::LightmapPixelFormat::RGB8;
    _bakeBaker = _bakeContext->createBaker(target, bakeSettings);
    _bakeTarget = target;
    _bakeError.clear();
    _bakeTempPath = std::filesystem::temp_directory_path() /
        ("bg2e_model_edit_lightmap_" + std::to_string(++_bakeCounter) + ".png");
    _bakeState = LightmapBakeState::Baking;
}

void AppDelegate::cancelLightmapBake()
{
    _bakeState = LightmapBakeState::Idle;
    // Baker and context release their GPU resources through the engine's
    // deferred cleanup, so submitted frames never lose them in flight.
    _bakeBaker.reset();
    _bakeContext.reset();
    _bakeTarget.reset();
}

uint32_t AppDelegate::lightmapBakeCompletedFrames() const
{
    return _bakeBaker ? _bakeBaker->completedFrames() : 0;
}

uint32_t AppDelegate::lightmapBakeTotalFrames() const
{
    return _bakeBaker ? _bakeBaker->settings().accumulationFrames : 0;
}

void AppDelegate::finishLightmapBake()
{
    auto pixels = _bakeBaker->readPixels();
    const auto& rgb = std::get<std::vector<uint8_t>>(pixels.rgb);
    bg2e::db::saveImage(_bakeTempPath, rgb.data(), pixels.width, pixels.height, 3);

    auto drawable = _stage->targetDrawable();
    if (drawable)
    {
        auto texture = std::make_shared<bg2e::base::Texture>(_bakeTempPath);
        for (uint32_t submesh = 0; submesh < drawable->submeshesCount(); ++submesh)
        {
            drawable->material(submesh).setAoTexture(texture);
            drawable->material(submesh).setAoUVSet(1);
            drawable->material(submesh).setAoScale(glm::vec2{ 1.0f, 1.0f });
            auto renderMaterial = drawable->renderMaterial(submesh);
            renderMaterial->setMaterialAttributes(drawable->material(submesh));
            renderMaterial->updateTextures();
        }
        _stage->document()->setUnsavedChanges(true);
    }

    _lastLightmapPath = _bakeTempPath;
    _bakeState = LightmapBakeState::Idle;
    _bakeBaker.reset();
    _bakeTarget.reset();
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
    if (_lightmapWindow.isOpen())
    {
        _lightmapWindow.draw();
    }
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
        if (_selectionManager->pick(renderer()->scene(), x, y, true))
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
    _stage = std::make_shared<StageScene>(_engine, this);
    
    auto scene = _stage->init();
    
    renderer()->setSkyboxBlurLevel(2);
    
    initWorkspace();
    
    
    return scene;
}

void AppDelegate::cleanup()
{
    cancelLightmapBake();
    DefaultRenderLoopDelegate::cleanup();
    _stage.reset();
    _submeshPanel.cleanup();
    _lightmapWindow.cleanup();
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
    _renderPrefs = std::make_unique<bg2e::render::RenderSettingsPreferences>(renderer());
    _renderPrefs->load();

    // Register timer: persist every 60 seconds, also persist on exit
    bg2e::app::MainLoop::current()->timeout().add([this]() -> bool {
        _renderPrefs->persist();
        return true;
    }, 60000, true);  // executeOnExit = true

    _workspace.leftPanelSize().min = 300;
    _environmentPanel.init(this, renderer());
    _uiSettingsWindow.init();
    _renderSettingsWindow.init(renderer(), _renderPrefs.get());
    _toolBar.init(this, &_uiSettingsWindow, &_renderSettingsWindow, _renderPrefs.get());
    _submeshPanel.init(this);
    _lightmapWindow.init(this);

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
        &_environmentPanel,
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
