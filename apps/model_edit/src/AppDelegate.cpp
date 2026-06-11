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

void AppDelegate::saveSettings()
{
    bg2e::app::Preferences appPrefs;

    // Render settings
    // RTAO
    uint32_t qualityIdx = static_cast<uint32_t>(renderer()->aoQuality());
    int aoSampleCount = renderer()->aoSampleCount();
    int bounceCount = renderer()->aoBounceCount();
    float radius = renderer()->aoRadius();
    float bias = renderer()->aoBias();
    float falloff = renderer()->aoFalloff();
    float bounceAttenuation = renderer()->aoBounceAttenuation();
    appPrefs.set("render_ao_qualityIndex", qualityIdx);
    appPrefs.set("render_ao_sampleCount", aoSampleCount);
    appPrefs.set("render_ao_bounceCount", bounceCount);
    appPrefs.set("render_ao_radius", radius);
    appPrefs.set("render_ao_bias", bias);
    appPrefs.set("render_ao_falloff", falloff);
    appPrefs.set("render_ao_bounceAttenuation", bounceAttenuation);

    // Reflection
    bool enabled = renderer()->rtReflectionsEnabled();
    int reflectSampleCount = renderer()->rtReflectionSampleCount();
    float maxRoughness = renderer()->rtReflectionMaxRoughness();
    float rayBias = renderer()->rtReflectionRayBias();
    float maxDistance = renderer()->rtReflectionMaxDistance();
    float roughnessSpread = renderer()->rtReflectionRoughnessSpread();
    appPrefs.set("render_reflect_enabled", enabled);
    appPrefs.set("render_reflect_maxRoughness", maxRoughness);
    appPrefs.set("render_reflect_sampleCount", reflectSampleCount);
    appPrefs.set("render_reflect_rayBias", rayBias);
    appPrefs.set("render_reflect_maxDistance", maxDistance);
    appPrefs.set("render_reflect_roughnessSpread", roughnessSpread);

    // Temporal Accumulation
    static const std::vector<std::string> modeItems = { "Interactive", "Progressive" };
    uint32_t modeIdx = static_cast<uint32_t>(renderer()->temporalMode());
    float historyWeight = renderer()->temporalHistoryWeight();
    float taDepthThreshold = renderer()->temporalDepthThreshold();
    float taNormalThreshold = renderer()->temporalNormalThreshold();
    appPrefs.set("render_ta_mode", modeIdx);
    appPrefs.set("render_ta_historyWeight", historyWeight);
    appPrefs.set("render_ta_depthThreshold", taDepthThreshold);
    appPrefs.set("render_ta_normalThreshold", taNormalThreshold);

    // Denoise
    int kernelRadius = renderer()->denoiseKernelRadius();
    float depthThreshold = renderer()->denoiseDepthThreshold();
    float denoiseNormalThreshold = renderer()->denoiseNormalThreshold();
    float depthSigma = renderer()->denoiseDepthSigma();
    float normalSigma = renderer()->denoiseNormalSigma();
    appPrefs.set("render_denoise_kernRadius", kernelRadius);
    appPrefs.set("render_denoise_depthThreshold", depthThreshold);
    appPrefs.set("render_denoise_normalThreshold", denoiseNormalThreshold);
    appPrefs.set("render_denoise_depthSigma", depthSigma);
    appPrefs.set("render_denoise_normalSigma", normalSigma);

    appPrefs.save();
}

void AppDelegate::restoreSettings()
{
    bg2e::app::Preferences appPrefs;
    appPrefs.load();

    // Render settings
    // RTAO
    uint32_t qualityIdx = appPrefs.get("render_ao_qualityIndex", static_cast<uint32_t>(renderer()->aoQuality()));
    renderer()->setAOQuality(static_cast<bg2e::render::deferred::RTAOQuality>(qualityIdx));
    int aoSampleCount = appPrefs.get("render_ao_sampleCount", renderer()->aoSampleCount());
    renderer()->setAOSampleCount(aoSampleCount);
    int bounceCount = appPrefs.get("render_ao_bounceCount", renderer()->aoBounceCount());
    renderer()->setAOBounceCount(bounceCount);
    float radius = appPrefs.get("render_ao_radius", renderer()->aoRadius());
    renderer()->setAORadius(radius);
    float bias = appPrefs.get("render_ao_bias", renderer()->aoBias());
    renderer()->setAOBias(bias);
    float falloff = appPrefs.get("render_ao_falloff", renderer()->aoFalloff());
    renderer()->setAOFalloff(falloff);
    float bounceAttenuation = appPrefs.get("render_ao_bounceAttenuation", renderer()->aoBounceAttenuation());
    renderer()->setAOBounceAttenuation(bounceAttenuation);

    // Reflection
    bool enabled = appPrefs.get("render_reflect_enabled", renderer()->rtReflectionsEnabled());
    renderer()->setRTReflectionsEnabled(enabled);
    int reflectSampleCount = appPrefs.get("render_reflect_sampleCount", renderer()->rtReflectionSampleCount());
    renderer()->setRTReflectionSampleCount(reflectSampleCount);
    float maxRoughness = appPrefs.get("render_reflect_maxRoughness", renderer()->rtReflectionMaxRoughness());
    renderer()->setRTReflectionMaxRoughness(maxRoughness);
    float rayBias = appPrefs.get("render_reflect_rayBias", renderer()->rtReflectionRayBias());
    renderer()->setRTReflectionRayBias(rayBias);
    float maxDistance = appPrefs.get("render_reflect_maxDistance", renderer()->rtReflectionMaxDistance());
    renderer()->setRTReflectionMaxDistance(maxDistance);
    float roughnessSpread = appPrefs.get("render_reflect_roughnessSpread", renderer()->rtReflectionRoughnessSpread());
    renderer()->setRTReflectionRoughnessSpread(roughnessSpread);

    // Temporal Accumulation
    static const std::vector<std::string> modeItems = { "Interactive", "Progressive" };
    uint32_t modeIdx = appPrefs.get("render_ta_mode", static_cast<uint32_t>(renderer()->temporalMode()));
    renderer()->setTemporalMode(static_cast<bg2e::render::deferred::TemporalAccumulator::AccumulationMode>(modeIdx));
    float historyWeight = appPrefs.get("render_ta_historyWeight", renderer()->temporalHistoryWeight());
    renderer()->setTemporalHistoryWeight(historyWeight);
    float taDepthThreshold = appPrefs.get("render_ta_depthThreshold", renderer()->temporalDepthThreshold());
    renderer()->setTemporalDepthThreshold(taDepthThreshold);
    float taNormalThreshold = appPrefs.get("render_ta_normalThreshold", renderer()->temporalNormalThreshold());
    renderer()->setTemporalNormalThreshold(taNormalThreshold);

    // Denoise
    int kernelRadius = appPrefs.get("render_denoise_kernRadius", renderer()->denoiseKernelRadius());
    renderer()->setDenoiseKernelRadius(kernelRadius);
    float depthThreshold = appPrefs.get("render_denoise_depthThreshold", renderer()->denoiseDepthThreshold());
    renderer()->setDenoiseDepthThreshold(depthThreshold);
    float denoiseNormalThreshold = appPrefs.get("render_denoise_normalThreshold", renderer()->denoiseNormalThreshold());
    renderer()->setDenoiseNormalThreshold(denoiseNormalThreshold);
    float depthSigma = appPrefs.get("render_denoise_depthSigma", renderer()->denoiseDepthSigma());
    renderer()->setDenoiseDepthSigma(depthSigma);
    float normalSigma = appPrefs.get("render_denoise_normalSigma", renderer()->denoiseNormalSigma());
    renderer()->setDenoiseNormalSigma(normalSigma);
}

void AppDelegate::initWorkspace()
{
    restoreSettings();

    _workspace.leftPanelSize().min = 300;
    _environmentPanel.init(this, renderer());
    _uiSettingsWindow.init();
    _renderSettingsWindow.init(this);
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
