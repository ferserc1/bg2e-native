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

#include "SceneLightmapWindow.hpp"

#include "AppDelegate.hpp"

#include <bg2e/draw/Engine.hpp>
#include <bg2e/geo/UvAtlasValidator.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Numeric.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>

#include <algorithm>
#include <functional>

namespace bg2e_composer {
namespace {
std::shared_ptr<bg2e::scene::Node> findFirstDrawable(const std::shared_ptr<bg2e::scene::Node>& root)
{
    if (!root)
    {
        return nullptr;
    }
    if (auto component = root->getComponent<bg2e::scene::DrawableComponent>();
        component && component->drawDrawable())
    {
        return root;
    }
    for (const auto& child : root->children())
    {
        if (auto result = findFirstDrawable(child))
        {
            return result;
        }
    }
    return nullptr;
}

const char* stateName(bg2e::bake::BakeState state)
{
    using bg2e::bake::BakeState;
    switch (state)
    {
    case BakeState::Ready: return "Ready";
    case BakeState::Running: return "Running";
    case BakeState::Complete: return "Complete";
    case BakeState::Cancelled: return "Cancelled";
    case BakeState::Invalidated: return "Invalidated";
    case BakeState::Failed: return "Failed";
    }
    return "Unknown";
}
}

void SceneLightmapWindow::init(const std::shared_ptr<AppDelegate>& delegate,
    bg2e::draw::Engine* engine, bg2e::ui::UserInterface* ui)
{
    _delegate = delegate;
    _engine = engine;
    _ui = ui;
    setTitle("UV and Lightmap Baker");
    setPosition(50, 80);
    setSize(680, 760);
    close();
    _uv0Renderer = std::make_unique<bg2e::bake::UvMapPreviewRenderer>(*engine->device(), static_cast<uint32_t>(_previewResolution));
    _uv1Renderer = std::make_unique<bg2e::bake::UvMapPreviewRenderer>(*engine->device(), static_cast<uint32_t>(_previewResolution));
    setDrawFunction([this]
    {
        drawContents();
    });
}

void SceneLightmapWindow::cleanup()
{
    _uvWarning.clear();
    _uv2Token.reset();
    _uv2Generating = false;
    _resultPreview.clear();
    _previewResultImage.reset();
    _uv0Preview.clear();
    _uv1Preview.clear();
    _uv0Renderer.reset();
    _uv1Renderer.reset();
    _previewTarget.reset();
    _previewDrawable.reset();
    _uv2Target.reset();
    _uv2Drawable.reset();
    _ui = nullptr;
    _engine = nullptr;
}

void SceneLightmapWindow::draw()
{
    if (!isOpen())
    {
        if (_wasOpen)
        {
            _uv2Token.reset();
            _uv2Target.reset();
            _uv2Drawable.reset();
            _uv2Generating = false;
            _wasOpen = false;
        }
        return;
    }
    _wasOpen = true;
    bg2e::ui::Window::draw();
}

void SceneLightmapWindow::refreshUvPreview(const std::shared_ptr<bg2e::scene::Node>& target)
{
    auto component = target ? target->getComponent<bg2e::scene::DrawableComponent>() : nullptr;
    auto drawable = component ? component->drawDrawable() : nullptr;
    if (!drawable || !drawable->mesh() || !_uv0Renderer || !_uv1Renderer || !_ui || !_engine)
    {
        _uvWarning.clear();
        _uv0Preview.clear();
        _uv1Preview.clear();
        _previewTarget.reset();
        _previewDrawable.reset();
        return;
    }
    if (_previewTarget.lock() == target && _previewDrawable.lock() == drawable)
    {
        return;
    }
    _uvWarning = bg2e::geo::UvAtlasValidator::validate(*drawable->mesh(), 1,
        bg2e::geo::UvAtlasOverlapPolicy::Warn).warning;
    _uv0Renderer->render(*drawable->mesh(), 0);
    _uv1Renderer->render(*drawable->mesh(), 1);
    _uv0Preview.setImage(*_ui, *_engine, _uv0Renderer->image());
    _uv1Preview.setImage(*_ui, *_engine, _uv1Renderer->image());
    _previewTarget = target;
    _previewDrawable = drawable;
}

void SceneLightmapWindow::drawContents()
{
    auto delegate = _delegate.lock();
    if (!delegate || !delegate->document() || !delegate->document()->root)
    {
        bg2e::ui::Text::text("Open a model or scene to preview UVs and generate a lightmap.");
        return;
    }
    if (_documentGeneration != delegate->document()->generation)
    {
        _documentGeneration = delegate->document()->generation;
        _uv2Token.reset();
        _uv2Target.reset();
        _uv2Drawable.reset();
        _uv2Generating = false;
        _previewTarget.reset();
        _previewDrawable.reset();
        _uv0Preview.clear();
        _uv1Preview.clear();
        _resultPreview.clear();
        _previewResultImage.reset();
    }
    if (_uv2Generating)
    {
        auto uvTarget = _uv2Target.lock();
        auto sourceDrawable = _uv2Drawable.lock();
        auto* component = uvTarget ? uvTarget->drawable() : nullptr;
        if (!uvTarget || !delegate->stage()->contains(uvTarget.get()) || !sourceDrawable || !component ||
            component->drawDrawable() != sourceDrawable)
        {
            _uv2Token.reset();
            _uv2Target.reset();
            _uv2Drawable.reset();
            _uv2Generating = false;
            _message = "UV2 generation cancelled because its document target changed";
        }
    }

    auto selected = delegate->selection() ? delegate->selection()->selectedNode() : nullptr;
    auto target = selected ? selected->shared_from_this() : nullptr;
    if (!target || !target->drawable() || !target->drawable()->drawDrawable())
    {
        target = findFirstDrawable(delegate->document()->root);
    }
    if (!target)
    {
        bg2e::ui::Text::text("The document has no loaded drawable to bake.");
        return;
    }

    bg2e::ui::Text::text("Target: " + target->name());
    refreshUvPreview(target);
    bg2e::ui::Text::separator("UV maps");
    if (bg2e::ui::Numeric::sliderInt("Preview resolution", &_previewResolution, 64, 1024))
    {
        _previewResolution = std::clamp(_previewResolution, 64, 1024);
        _uv0Preview.clear();
        _uv1Preview.clear();
        _uv0Renderer->setResolution(static_cast<uint32_t>(_previewResolution));
        _uv1Renderer->setResolution(static_cast<uint32_t>(_previewResolution));
        _previewTarget.reset();
        refreshUvPreview(target);
    }
    if (bg2e::ui::Button::button("Generate UV2", false, _uv2Generating))
    {
        bg2e::geo::Uv2AtlasOptions options;
        options.resolution = 128u << _resolutionIndex;
        options.paddingPixels = static_cast<uint32_t>(std::max(_padding, 0));
        _uv2Generating = true;
        _uv2Target = target;
        _uv2Drawable = target->drawable()->drawDrawable();
        _message.clear();
        const auto weakTarget = std::weak_ptr<bg2e::scene::Node>(target);
        _uv2Token = delegate->regenerateUv2(target, options, [this, weakTarget](const auto& result)
        {
            _uv2Generating = false;
            _uv2Token.reset();
            _uv2Target.reset();
            _uv2Drawable.reset();
            _message = result.success
                ? "UV2 generated: " + std::to_string(result.chartCount) + " charts, " +
                    std::to_string(result.atlasWidth) + " x " + std::to_string(result.atlasHeight)
                : result.message;
            if (result.success)
            {
                _previewTarget.reset();
                refreshUvPreview(weakTarget.lock());
            }
        });
    }
    if (_uv2Generating)
    {
        bg2e::ui::Text::text("Generating UV2 at a synchronized renderer boundary...");
    }
    bg2e::ui::Text::text("UV1");
    _uv0Preview.draw(224, 224);
    bg2e::ui::Text::text("UV2 (lightmap charts)");
    _uv1Preview.draw(224, 224);
    if (!_uvWarning.empty())
    {
        bg2e::ui::Text::text("Warning: " + _uvWarning);
    }

    bg2e::ui::Text::separator("Bake settings");
    bg2e::ui::Value::comboBox("Signal", {"Ray traced AO", "Ray traced GI"}, _modeIndex);
    bg2e::ui::Value::comboBox("Resolution", {"128", "256", "512", "1024", "2048", "4096"}, _resolutionIndex);
    bg2e::ui::Numeric::sliderInt("Completed samples", &_frames, 1, 512);
    bg2e::ui::Numeric::sliderInt("Samples per pixel", &_samples, 1, 256);
    bg2e::ui::Value::comboBox("CPU export precision", {"RGB8 linear", "RGB32F HDR"}, _formatIndex);
    bg2e::ui::Numeric::sliderInt("GI bounces", &_bounces, 1, 8);
    bg2e::ui::Numeric::slider("AO radius", &_aoRadius, 0.001f, 10.0f);
    bg2e::ui::Numeric::slider("Maximum ray distance", &_maxDistance, 0.01f, 10000.0f);
    bg2e::ui::Numeric::slider("Ray bias", &_rayBias, 0.000001f, 0.1f);
    bg2e::ui::Numeric::slider("GI exposure (RGB8 only)", &_exposure, -16.0f, 16.0f);
    bg2e::ui::Numeric::sliderInt("Chart dilation", &_dilation, 1, 64);

    const auto progress = delegate->lightmapBakeProgress();
    const bool active = delegate->lightmapBakeActive();
    if (bg2e::ui::Button::button("Start bake", false, active || _uv2Generating))
    {
        _resultPreview.clear();
        _previewResultImage.reset();
        bg2e::bake::LightmapSettings settings;
        settings.setMode(_modeIndex == 0 ? bg2e::bake::LightmapMode::RTAO : bg2e::bake::LightmapMode::RTGI);
        settings.setResolution(128u << _resolutionIndex);
        settings.setAccumulationFrames(static_cast<uint32_t>(std::max(_frames, 1)));
        settings.setSamplesPerPixel(static_cast<uint32_t>(std::max(_samples, 1)));
        settings.setGiBounces(static_cast<uint32_t>(std::max(_bounces, 1)));
        settings.setAoRadius(_aoRadius);
        settings.setMaxRayDistance(_maxDistance);
        settings.setRayBias(_rayBias);
        settings.setExposureEV(_exposure);
        settings.setCpuFormat(_formatIndex == 0 ? bg2e::bake::LightmapPixelFormat::RGB8 :
            bg2e::bake::LightmapPixelFormat::RGB32F);
        settings.setDilationPixels(static_cast<uint32_t>(std::max(_dilation, 1)));
        delegate->startLightmapBake(target, settings);
    }
    if (active && bg2e::ui::Button::button("Cancel bake"))
    {
        delegate->cancelLightmapBake();
    }
    bg2e::ui::Text::text(std::string("Bake state: ") + stateName(progress.state) + " — " +
        std::to_string(progress.completedSamples) + " / " + std::to_string(progress.requestedSamples));
    if (!progress.status.empty())
    {
        bg2e::ui::Text::text(progress.status);
    }
    if (auto result = delegate->lightmapResult())
    {
        if (_previewResultImage != result->image)
        {
            _resultPreview.setResult(*_ui, *_engine, *result);
            _previewResultImage = result->image;
        }
        bg2e::ui::Text::separator("Completed lightmap");
        _resultPreview.draw(256, 256);
    }
    else
    {
        _resultPreview.clear();
        _previewResultImage.reset();
    }
    if (!_message.empty())
    {
        bg2e::ui::Text::text(_message);
    }
}

}
