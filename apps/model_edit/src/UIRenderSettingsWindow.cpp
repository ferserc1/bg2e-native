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

void UIRenderSettingsWindow::drawUI()
{
    drawRTAOSection();
    bg2e::ui::BasicWidgets::separator();
    drawTemporalAccumulatorSection();
    bg2e::ui::BasicWidgets::separator();
    drawDenoiseFilterSection();
}

void UIRenderSettingsWindow::drawRTAOSection()
{
    bg2e::ui::BasicWidgets::text("Ambient Occlusion", true);

    auto renderer = _appDelegate->renderer();
    if (!renderer) return;

    static const std::vector<std::string> qualityItems = { "Low", "Medium", "High", "Ultra" };
    uint32_t qualityIdx = static_cast<uint32_t>(renderer->aoQuality());

    if (bg2e::ui::Input::comboBox("Quality", qualityItems, qualityIdx))
    {
        renderer->setAOQuality(static_cast<bg2e::render::deferred::RTAOQuality>(qualityIdx));
    }

    int sampleCount = renderer->aoSampleCount();
    if (bg2e::ui::Input::sliderInt("Sample Count", &sampleCount, 1, 32))
    {
        renderer->setAOSampleCount(sampleCount);
    }

    int bounceCount = renderer->aoBounceCount();
    if (bg2e::ui::Input::sliderInt("Bounce Count", &bounceCount, 0, 8))
    {
        renderer->setAOBounceCount(bounceCount);
    }

    float radius = renderer->aoRadius();
    if (bg2e::ui::Input::sliderFloat("Radius", &radius, 0.01f, 5.0f))
    {
        renderer->setAORadius(radius);
    }

    float bias = renderer->aoBias();
    if (bg2e::ui::Input::sliderFloat("Bias", &bias, 0.0f, 0.1f))
    {
        renderer->setAOBias(bias);
    }

    float falloff = renderer->aoFalloff();
    if (bg2e::ui::Input::sliderFloat("Falloff", &falloff, 0.0f, 5.0f))
    {
        renderer->setAOFalloff(falloff);
    }

    float bounceAttenuation = renderer->aoBounceAttenuation();
    if (bg2e::ui::Input::sliderFloat("Bounce Attenuation", &bounceAttenuation, 0.0f, 1.0f))
    {
        renderer->setAOBounceAttenuation(bounceAttenuation);
    }
}

void UIRenderSettingsWindow::drawTemporalAccumulatorSection()
{
    bg2e::ui::BasicWidgets::text("Temporal Accumulator", true);

    auto renderer = _appDelegate->renderer();
    if (!renderer) return;

    static const std::vector<std::string> modeItems = { "Interactive", "Progressive" };
    uint32_t modeIdx = static_cast<uint32_t>(renderer->temporalMode());

    if (bg2e::ui::Input::comboBox("Mode", modeItems, modeIdx))
    {
        renderer->setTemporalMode(static_cast<bg2e::render::deferred::TemporalAccumulator::AccumulationMode>(modeIdx));
    }

    float historyWeight = renderer->temporalHistoryWeight();
    if (bg2e::ui::Input::sliderFloat("History Weight", &historyWeight, 0.0f, 1.0f))
    {
        renderer->setTemporalHistoryWeight(historyWeight);
    }

    float depthThreshold = renderer->temporalDepthThreshold();
    if (bg2e::ui::Input::sliderFloat("Depth Threshold##Temporal", &depthThreshold, 0.001f, 0.1f))
    {
        renderer->setTemporalDepthThreshold(depthThreshold);
    }

    float normalThreshold = renderer->temporalNormalThreshold();
    if (bg2e::ui::Input::sliderFloat("Normal Threshold##Temporal", &normalThreshold, 0.1f, 1.0f))
    {
        renderer->setTemporalNormalThreshold(normalThreshold);
    }
}

void UIRenderSettingsWindow::drawDenoiseFilterSection()
{
    bg2e::ui::BasicWidgets::text("Denoise Filter", true);

    auto renderer = _appDelegate->renderer();
    if (!renderer) return;

    int kernelRadius = renderer->denoiseKernelRadius();
    if (bg2e::ui::Input::sliderInt("Kernel Radius", &kernelRadius, 1, 10))
    {
        renderer->setDenoiseKernelRadius(kernelRadius);
    }

    float depthThreshold = renderer->denoiseDepthThreshold();
    if (bg2e::ui::Input::sliderFloat("Depth Threshold##Denoise", &depthThreshold, 0.001f, 0.1f))
    {
        renderer->setDenoiseDepthThreshold(depthThreshold);
    }

    float normalThreshold = renderer->denoiseNormalThreshold();
    if (bg2e::ui::Input::sliderFloat("Normal Threshold##Denoise", &normalThreshold, 0.1f, 1.0f))
    {
        renderer->setDenoiseNormalThreshold(normalThreshold);
    }

    float depthSigma = renderer->denoiseDepthSigma();
    if (bg2e::ui::Input::sliderFloat("Depth Sigma", &depthSigma, 0.001f, 0.5f))
    {
        renderer->setDenoiseDepthSigma(depthSigma);
    }

    float normalSigma = renderer->denoiseNormalSigma();
    if (bg2e::ui::Input::sliderFloat("Normal Sigma", &normalSigma, 0.01f, 1.0f))
    {
        renderer->setDenoiseNormalSigma(normalSigma);
    }
}