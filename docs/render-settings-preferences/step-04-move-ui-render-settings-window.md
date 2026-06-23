# Step 04: Move UIRenderSettingsWindow to bg2e::ui

## Files to Create

- `lib/include/bg2e/ui/UIRenderSettingsWindow.hpp`
- `lib/src/bg2e/ui/UIRenderSettingsWindow.cpp`

## Files to Modify

- `lib/include/bg2e/ui/all.hpp` — add include

## Files to Delete (after integration)

- `apps/model_edit/src/UIRenderSettingsWindow.hpp`
- `apps/model_edit/src/UIRenderSettingsWindow.cpp`
- `apps/bg2e_composer/src/UIRenderSettingsWindow.hpp`
- `apps/bg2e_composer/src/UIRenderSettingsWindow.cpp`

## Interface

```cpp
// lib/include/bg2e/ui/UIRenderSettingsWindow.hpp
#pragma once

#include <bg2e/ui/Window.hpp>

namespace bg2e {
namespace render {
class RendererDeferred;
class RenderSettingsPreferences;
}
namespace ui {

class BG2E_API UIRenderSettingsWindow : public Window {
public:
    void init(render::RendererDeferred* renderer, render::RenderSettingsPreferences* prefs);

private:
    bool drawUI();
    bool drawRenderScaleSection();
    bool drawIndirectLightingModeSection();
    bool drawRTAOSection();
    bool drawRTGISection();
    bool drawRTReflectionsSection();
    bool drawTemporalAccumulatorSection();
    bool drawDenoiseFilterSection();

    render::RendererDeferred* _renderer = nullptr;
    render::RenderSettingsPreferences* _prefs = nullptr;
};

}
}
```

## Implementation Details

### `init()`

```cpp
void UIRenderSettingsWindow::init(
    render::RendererDeferred* renderer,
    render::RenderSettingsPreferences* prefs)
{
    _renderer = renderer;
    _prefs = prefs;
    setTitle("Render Settings");
    setSize(350, 600);
    close();

    setDrawFunction([this]() {
        drawUI();
    });
}
```

### `drawUI()`

No explicit `persist()` call — dirty marking happens in setters, and the timer handles persistence.

```cpp
bool UIRenderSettingsWindow::drawUI()
{
    bool changed = false;
    changed |= drawRenderScaleSection();
    bg2e::ui::BasicWidgets::separator();
    changed |= drawIndirectLightingModeSection();
    bg2e::ui::BasicWidgets::separator();
    changed |= drawRTReflectionsSection();
    bg2e::ui::BasicWidgets::separator();
    changed |= drawTemporalAccumulatorSection();
    bg2e::ui::BasicWidgets::separator();
    changed |= drawDenoiseFilterSection();
    return changed;
}
```

### Section examples (pattern for all sections)

Each section reads from the renderer (via `_prefs->xxx()` or `_renderer->xxx()`) and writes via `_prefs->setXxx()`.

**`drawRenderScaleSection()`:**

```cpp
bool UIRenderSettingsWindow::drawRenderScaleSection()
{
    auto scaleProcessorName = _renderer->scaleProcessorName();
    bg2e::ui::BasicWidgets::text(scaleProcessorName, true);

    auto scaleItems = _renderer->scaleOptions();
    auto scaleIdx = _renderer->scaleOption();
    if (bg2e::ui::Input::comboBox(scaleProcessorName + "##RenderScale", scaleItems, scaleIdx))
    {
        _prefs->setRenderScaleIndex(scaleIdx);
        return true;
    }
    return false;
}
```

**`drawIndirectLightingModeSection()`:**

```cpp
bool UIRenderSettingsWindow::drawIndirectLightingModeSection()
{
    bg2e::ui::BasicWidgets::text("Indirect Lighting", true);

    static const std::vector<std::string> modeItems = {
        "Ambient Occlusion (RTAO)", "Global Illumination (RTGI)"
    };
    uint32_t modeIdx = _prefs->indirectLightingMode();

    bool changed = false;
    if (bg2e::ui::Input::comboBox("Mode##IndirectLighting", modeItems, modeIdx))
    {
        _prefs->setIndirectLightingMode(modeIdx);
        changed = true;
    }

    bg2e::ui::BasicWidgets::separator();

    if (_prefs->indirectLightingMode() == 1)  // RTGI
    {
        changed |= drawRTGISection();
    }
    else
    {
        changed |= drawRTAOSection();
    }
    return changed;
}
```

**`drawRTAOSection()`:**

```cpp
bool UIRenderSettingsWindow::drawRTAOSection()
{
    bg2e::ui::BasicWidgets::text("Ambient Occlusion", true);

    static const std::vector<std::string> qualityItems = { "Low", "Medium", "High", "Ultra" };
    uint32_t qualityIdx = _prefs->aoQualityIndex();
    if (bg2e::ui::Input::comboBox("Quality##RTAO", qualityItems, qualityIdx))
    {
        _prefs->setAOQualityIndex(qualityIdx);
    }

    int sampleCount = _prefs->aoSampleCount();
    if (bg2e::ui::Input::sliderInt("Sample Count##RTAO", &sampleCount, 1, 32))
    {
        _prefs->setAOSampleCount(sampleCount);
    }

    int bounceCount = _prefs->aoBounceCount();
    if (bg2e::ui::Input::sliderInt("Bounce Count##RTAO", &bounceCount, 0, 8))
    {
        _prefs->setAOBounceCount(bounceCount);
    }

    float radius = _prefs->aoRadius();
    if (bg2e::ui::Input::sliderFloat("Radius##RTAO", &radius, 0.01f, 5.0f))
    {
        _prefs->setAORadius(radius);
    }

    float bias = _prefs->aoBias();
    if (bg2e::ui::Input::sliderFloat("Bias##RTAO", &bias, 0.0f, 0.1f))
    {
        _prefs->setAOBias(bias);
    }

    float falloff = _prefs->aoFalloff();
    if (bg2e::ui::Input::sliderFloat("Falloff##RTAO", &falloff, 0.0f, 5.0f))
    {
        _prefs->setAOFalloff(falloff);
    }

    float bounceAttenuation = _prefs->aoBounceAttenuation();
    if (bg2e::ui::Input::sliderFloat("Bounce Attenuation##RTAO", &bounceAttenuation, 0.0f, 1.0f))
    {
        _prefs->setAOBounceAttenuation(bounceAttenuation);
    }

    return false;
}
```

**`drawRTGISection()`:**

```cpp
bool UIRenderSettingsWindow::drawRTGISection()
{
    bg2e::ui::BasicWidgets::text("Global Illumination", true);

    bool enabled = _prefs->rtGIEnabled();
    if (bg2e::ui::BasicWidgets::checkBox("Enabled##RTGI", &enabled))
    {
        _prefs->setRTGIEnabled(enabled);
    }

    static const std::vector<std::string> qualityItems = { "Low", "Medium", "High", "Ultra" };
    uint32_t qualityIdx = _prefs->rtGIQualityIndex();
    if (bg2e::ui::Input::comboBox("Quality##RTGI", qualityItems, qualityIdx))
    {
        _prefs->setRTGIQualityIndex(qualityIdx);
    }

    int sampleCount = _prefs->rtGISampleCount();
    if (bg2e::ui::Input::sliderInt("Sample Count##RTGI", &sampleCount, 1, 16))
    {
        _prefs->setRTGISampleCount(sampleCount);
    }

    int bounceCount = _prefs->rtGIBounceCount();
    if (bg2e::ui::Input::sliderInt("Bounce Count##RTGI", &bounceCount, 1, 3))
    {
        _prefs->setRTGIBounceCount(bounceCount);
    }

    float rayBias = _prefs->rtGIRayBias();
    if (bg2e::ui::Input::sliderFloat("Ray Bias##RTGI", &rayBias, 0.001f, 0.1f))
    {
        _prefs->setRTGIRayBias(rayBias);
    }

    float maxDistance = _prefs->rtGIMaxDistance();
    if (bg2e::ui::Input::sliderFloat("Max Distance##RTGI", &maxDistance, 1.0f, 200.0f))
    {
        _prefs->setRTGIMaxDistance(maxDistance);
    }

    return false;
}
```

**`drawRTReflectionsSection()`:**

```cpp
bool UIRenderSettingsWindow::drawRTReflectionsSection()
{
    bg2e::ui::BasicWidgets::text("Ray Traced Reflections", true);

    bool enabled = _prefs->rtReflectionsEnabled();
    if (bg2e::ui::BasicWidgets::checkBox("Enabled##RTReflections", &enabled))
    {
        _prefs->setRTReflectionsEnabled(enabled);
    }

    int sampleCount = _prefs->rtReflectionSampleCount();
    if (bg2e::ui::Input::sliderInt("Sample Count##RTReflections", &sampleCount, 1, 16))
    {
        _prefs->setRTReflectionSampleCount(sampleCount);
    }

    float maxRoughness = _prefs->rtReflectionMaxRoughness();
    if (bg2e::ui::Input::sliderFloat("Max Roughness##RTReflections", &maxRoughness, 0.0f, 1.0f))
    {
        _prefs->setRTReflectionMaxRoughness(maxRoughness);
    }

    float rayBias = _prefs->rtReflectionRayBias();
    if (bg2e::ui::Input::sliderFloat("Ray Bias##RTReflections", &rayBias, 0.0f, 0.1f))
    {
        _prefs->setRTReflectionRayBias(rayBias);
    }

    float maxDistance = _prefs->rtReflectionMaxDistance();
    if (bg2e::ui::Input::sliderFloat("Max Distance##RTReflections", &maxDistance, 1.0f, 200.0f))
    {
        _prefs->setRTReflectionMaxDistance(maxDistance);
    }

    float roughnessSpread = _prefs->rtReflectionRoughnessSpread();
    if (bg2e::ui::Input::sliderFloat("Roughness Spread##RTReflections", &roughnessSpread, 0.0f, 5.0f))
    {
        _prefs->setRTReflectionRoughnessSpread(roughnessSpread);
    }

    return false;
}
```

**`drawTemporalAccumulatorSection()`:**

```cpp
bool UIRenderSettingsWindow::drawTemporalAccumulatorSection()
{
    bg2e::ui::BasicWidgets::text("Temporal Accumulator", true);

    static const std::vector<std::string> modeItems = { "Interactive", "Progressive" };
    uint32_t modeIdx = _prefs->temporalMode();
    if (bg2e::ui::Input::comboBox("Mode##Temporal", modeItems, modeIdx))
    {
        _prefs->setTemporalMode(modeIdx);
    }

    float historyWeight = _prefs->temporalHistoryWeight();
    if (bg2e::ui::Input::sliderFloat("History Weight##Temporal", &historyWeight, 0.0f, 1.0f))
    {
        _prefs->setTemporalHistoryWeight(historyWeight);
    }

    float depthThreshold = _prefs->temporalDepthThreshold();
    if (bg2e::ui::Input::sliderFloat("Depth Threshold##Temporal", &depthThreshold, 0.001f, 0.1f))
    {
        _prefs->setTemporalDepthThreshold(depthThreshold);
    }

    float normalThreshold = _prefs->temporalNormalThreshold();
    if (bg2e::ui::Input::sliderFloat("Normal Threshold##Temporal", &normalThreshold, 0.1f, 1.0f))
    {
        _prefs->setTemporalNormalThreshold(normalThreshold);
    }

    return false;
}
```

**`drawDenoiseFilterSection()`:**

```cpp
bool UIRenderSettingsWindow::drawDenoiseFilterSection()
{
    bg2e::ui::BasicWidgets::text("Denoise Filter", true);

    int kernelRadius = _prefs->denoiseKernelRadius();
    if (bg2e::ui::Input::sliderInt("Kernel Radius", &kernelRadius, 1, 10))
    {
        _prefs->setDenoiseKernelRadius(kernelRadius);
    }

    float depthThreshold = _prefs->denoiseDepthThreshold();
    if (bg2e::ui::Input::sliderFloat("Depth Threshold##Denoise", &depthThreshold, 0.001f, 0.1f))
    {
        _prefs->setDenoiseDepthThreshold(depthThreshold);
    }

    float normalThreshold = _prefs->denoiseNormalThreshold();
    if (bg2e::ui::Input::sliderFloat("Normal Threshold##Denoise", &normalThreshold, 0.1f, 1.0f))
    {
        _prefs->setDenoiseNormalThreshold(normalThreshold);
    }

    float depthSigma = _prefs->denoiseDepthSigma();
    if (bg2e::ui::Input::sliderFloat("Depth Sigma", &depthSigma, 0.001f, 0.5f))
    {
        _prefs->setDenoiseDepthSigma(depthSigma);
    }

    float normalSigma = _prefs->denoiseNormalSigma();
    if (bg2e::ui::Input::sliderFloat("Normal Sigma", &normalSigma, 0.01f, 1.0f))
    {
        _prefs->setDenoiseNormalSigma(normalSigma);
    }

    return false;
}
```

## Integration Points

- Replaces the app-local `UIRenderSettingsWindow` in both model_edit and bg2e_composer
- Depends on `RenderSettingsPreferences` (Step 03) for all read/write operations
- Integrated by both apps in Steps 05/06
