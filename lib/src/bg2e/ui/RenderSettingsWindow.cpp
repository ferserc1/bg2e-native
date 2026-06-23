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
#include <bg2e/ui/RenderSettingsWindow.hpp>
#include <bg2e/ui/all.hpp>
#include <bg2e/render/RendererDeferred.hpp>
#include <bg2e/render/RenderSettingsPreferences.hpp>

namespace bg2e {
namespace ui {

void RenderSettingsWindow::init(
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

bool RenderSettingsWindow::drawUI()
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

bool RenderSettingsWindow::drawRenderScaleSection()
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

bool RenderSettingsWindow::drawIndirectLightingModeSection()
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

bool RenderSettingsWindow::drawRTAOSection()
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

bool RenderSettingsWindow::drawRTGISection()
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

bool RenderSettingsWindow::drawRTReflectionsSection()
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

bool RenderSettingsWindow::drawTemporalAccumulatorSection()
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

bool RenderSettingsWindow::drawDenoiseFilterSection()
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

}
}