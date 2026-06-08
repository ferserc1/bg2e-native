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
#include "UIRenderSettingsWindow.hpp"
#include <bg2e/ui/all.hpp>
#include "AppDelegate.hpp"

void UIRenderSettingsWindow::init(AppDelegate * delegate)
{
    _appDelegate = delegate;
    setTitle("Render Settings");
    setSize(350, 600);
    close();

    setDrawFunction([this]() {
        drawUI();
    });
}

bool UIRenderSettingsWindow::drawUI()
{
    bool changed = false;
    changed |= drawRTAOSection();
    bg2e::ui::BasicWidgets::separator();
    changed |= drawRTReflectionsSection();
    bg2e::ui::BasicWidgets::separator();
    changed |= drawTemporalAccumulatorSection();
    bg2e::ui::BasicWidgets::separator();
    changed |= drawDenoiseFilterSection();

    if (changed && _appDelegate)
    {
        _appDelegate->saveSettings();
    }

    return changed;
}

bool UIRenderSettingsWindow::drawRTAOSection()
{
    bg2e::ui::BasicWidgets::text("Ambient Occlusion", true);

    auto renderer = _appDelegate->renderer();
    if (!renderer) return false;

    static const std::vector<std::string> qualityItems = { "Low", "Medium", "High", "Ultra" };
    uint32_t qualityIdx = static_cast<uint32_t>(renderer->aoQuality());

    if (bg2e::ui::Input::comboBox("Quality##RTAO", qualityItems, qualityIdx))
    {
        renderer->setAOQuality(static_cast<bg2e::render::deferred::RTAOQuality>(qualityIdx));
        return true;
    }

    int sampleCount = renderer->aoSampleCount();
    if (bg2e::ui::Input::sliderInt("Sample Count##RTAO", &sampleCount, 1, 32))
    {
        renderer->setAOSampleCount(sampleCount);
        return true;
    }

    int bounceCount = renderer->aoBounceCount();
    if (bg2e::ui::Input::sliderInt("Bounce Count##RTAO", &bounceCount, 0, 8))
    {
        renderer->setAOBounceCount(bounceCount);
        return true;
    }

    float radius = renderer->aoRadius();
    if (bg2e::ui::Input::sliderFloat("Radius##RTAO", &radius, 0.01f, 5.0f))
    {
        renderer->setAORadius(radius);
        return true;
    }

    float bias = renderer->aoBias();
    if (bg2e::ui::Input::sliderFloat("Bias##RTAO", &bias, 0.0f, 0.1f))
    {
        renderer->setAOBias(bias);
        return true;
    }

    float falloff = renderer->aoFalloff();
    if (bg2e::ui::Input::sliderFloat("Falloff##RTAO", &falloff, 0.0f, 5.0f))
    {
        renderer->setAOFalloff(falloff);
        return true;
    }

    float bounceAttenuation = renderer->aoBounceAttenuation();
    if (bg2e::ui::Input::sliderFloat("Bounce Attenuation##RTAO", &bounceAttenuation, 0.0f, 1.0f))
    {
        renderer->setAOBounceAttenuation(bounceAttenuation);
        return true;
    }

    return false;
}

bool UIRenderSettingsWindow::drawRTReflectionsSection()
{
    bg2e::ui::BasicWidgets::text("Ray Traced Reflections", true);

    auto renderer = _appDelegate->renderer();
    if (!renderer) return false;

    bool enabled = renderer->rtReflectionsEnabled();
    if (bg2e::ui::BasicWidgets::checkBox("Enabled##RTReflections", &enabled))
    {
        renderer->setRTReflectionsEnabled(enabled);
        return true;
    }

    int sampleCount = renderer->rtReflectionSampleCount();
    if (bg2e::ui::Input::sliderInt("Sample Count##RTReflections", &sampleCount, 1, 16))
    {
        renderer->setRTReflectionSampleCount(sampleCount);
        return true;
    }

    float maxRoughness = renderer->rtReflectionMaxRoughness();
    if (bg2e::ui::Input::sliderFloat("Max Roughness##RTReflections", &maxRoughness, 0.0f, 1.0f))
    {
        renderer->setRTReflectionMaxRoughness(maxRoughness);
        return true;
    }

    float rayBias = renderer->rtReflectionRayBias();
    if (bg2e::ui::Input::sliderFloat("Ray Bias##RTReflections", &rayBias, 0.0f, 0.1f))
    {
        renderer->setRTReflectionRayBias(rayBias);
        return true;
    }

    float maxDistance = renderer->rtReflectionMaxDistance();
    if (bg2e::ui::Input::sliderFloat("Max Distance##RTReflections", &maxDistance, 1.0f, 200.0f))
    {
        renderer->setRTReflectionMaxDistance(maxDistance);
        return true;
    }

    float roughnessSpread = renderer->rtReflectionRoughnessSpread();
    if (bg2e::ui::Input::sliderFloat("Roughness Spread##RTReflections", &roughnessSpread, 0.0f, 5.0f))
    {
        renderer->setRTReflectionRoughnessSpread(roughnessSpread);
        return true;
    }

    return false;
}

bool UIRenderSettingsWindow::drawTemporalAccumulatorSection()
{
    bg2e::ui::BasicWidgets::text("Temporal Accumulator", true);

    auto renderer = _appDelegate->renderer();
    if (!renderer) return false;

    static const std::vector<std::string> modeItems = { "Interactive", "Progressive" };
    uint32_t modeIdx = static_cast<uint32_t>(renderer->temporalMode());

    if (bg2e::ui::Input::comboBox("Mode##Temporal", modeItems, modeIdx))
    {
        renderer->setTemporalMode(static_cast<bg2e::render::deferred::TemporalAccumulator::AccumulationMode>(modeIdx));
        return true;
    }

    float historyWeight = renderer->temporalHistoryWeight();
    if (bg2e::ui::Input::sliderFloat("History Weight##Temporal", &historyWeight, 0.0f, 1.0f))
    {
        renderer->setTemporalHistoryWeight(historyWeight);
        return true;
    }

    float depthThreshold = renderer->temporalDepthThreshold();
    if (bg2e::ui::Input::sliderFloat("Depth Threshold##Temporal", &depthThreshold, 0.001f, 0.1f))
    {
        renderer->setTemporalDepthThreshold(depthThreshold);
        return true;
    }

    float normalThreshold = renderer->temporalNormalThreshold();
    if (bg2e::ui::Input::sliderFloat("Normal Threshold##Temporal", &normalThreshold, 0.1f, 1.0f))
    {
        renderer->setTemporalNormalThreshold(normalThreshold);
        return true;
    }

    return false;
}

bool UIRenderSettingsWindow::drawDenoiseFilterSection()
{
    bg2e::ui::BasicWidgets::text("Denoise Filter", true);

    auto renderer = _appDelegate->renderer();
    if (!renderer) return false;

    int kernelRadius = renderer->denoiseKernelRadius();
    if (bg2e::ui::Input::sliderInt("Kernel Radius", &kernelRadius, 1, 10))
    {
        renderer->setDenoiseKernelRadius(kernelRadius);
        return true;
    }

    float depthThreshold = renderer->denoiseDepthThreshold();
    if (bg2e::ui::Input::sliderFloat("Depth Threshold##Denoise", &depthThreshold, 0.001f, 0.1f))
    {
        renderer->setDenoiseDepthThreshold(depthThreshold);
        return true;
    }

    float normalThreshold = renderer->denoiseNormalThreshold();
    if (bg2e::ui::Input::sliderFloat("Normal Threshold##Denoise", &normalThreshold, 0.1f, 1.0f))
    {
        renderer->setDenoiseNormalThreshold(normalThreshold);
        return true;
    }

    float depthSigma = renderer->denoiseDepthSigma();
    if (bg2e::ui::Input::sliderFloat("Depth Sigma", &depthSigma, 0.001f, 0.5f))
    {
        renderer->setDenoiseDepthSigma(depthSigma);
        return true;
    }

    float normalSigma = renderer->denoiseNormalSigma();
    if (bg2e::ui::Input::sliderFloat("Normal Sigma", &normalSigma, 0.01f, 1.0f))
    {
        renderer->setDenoiseNormalSigma(normalSigma);
        return true;
    }

    return false;
}