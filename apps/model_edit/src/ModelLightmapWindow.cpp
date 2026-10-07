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
#include "ModelLightmapWindow.hpp"

#include "AppDelegate.hpp"

#include <algorithm>

void ModelLightmapWindow::init(AppDelegate * delegate)
{
    _appDelegate = delegate;
    setTitle("Lightmap Baker");
    close();
    _preview.init(delegate->engine());
    _uvPreview.init(delegate->engine());

    setDrawFunction([this]() {
        using namespace bg2e::ui;

        auto targetNode = _appDelegate->stage()->targetModelNode();
        if (!targetNode)
        {
            Text::text("No model loaded.");
            return;
        }

        auto drawable = _appDelegate->stage()->targetDrawable();
        _uvPreview.setMesh(drawable ? drawable->mesh() : nullptr);

        Text::separator("UV Atlas");
        Numeric::sliderInt("UV2 Padding", &_uv2Padding, 0, 32);
        if (Button::button("Generate UV2", false, _uv2Generating))
        {
            generateUv2();
        }
        if (_uv2Generating)
        {
            Text::text("Generating UV2 atlas...");
        }
        _uvPreview.draw();

        Text::separator("Bake Settings");
        Text::text("Mode: Ray Traced Ambient Occlusion");
        Value::comboBox("Resolution", { "128", "256", "512", "1024", "2048", "4096" }, _resolutionIndex);
        Numeric::sliderInt("Accumulation Frames", &_frames, 1, 512);
        Numeric::sliderInt("Samples per Pixel", &_samples, 1, 256);
        Numeric::sliderInt("Lightmap Dilation", &_lightmapDilation, 1, 32);

        const bool baking = _appDelegate->lightmapBakeActive();
        if (Button::button("Generate", false, baking))
        {
            bg2e::render::LightmapSettings settings;
            settings.resolution = 128u << _resolutionIndex;
            settings.accumulationFrames = static_cast<uint32_t>(std::max(_frames, 1));
            settings.samplesPerPixel = static_cast<uint32_t>(std::max(_samples, 1));
            settings.mode = bg2e::render::LightmapMode::RTAO;
            settings.cpuFormat = bg2e::render::LightmapPixelFormat::RGB8;
            settings.dilationPixels = static_cast<uint32_t>(std::max(_lightmapDilation, 1));
            try
            {
                _message.clear();
                _appDelegate->requestLightmapBake(settings);
            }
            catch (const std::exception& error)
            {
                _message = error.what();
                bg2e::app::MessageBox::showError("Lightmap Baker", error.what());
            }
        }

        if (baking)
        {
            Text::text(
                "Baking frame " +
                std::to_string(_appDelegate->lightmapBakeCompletedFrames()) +
                " / " +
                std::to_string(_appDelegate->lightmapBakeTotalFrames()));
        }

        if (!_message.empty())
        {
            Text::text(_message);
        }

        const auto& resultPath = _appDelegate->lastLightmapPath();
        if (!resultPath.empty() && resultPath != _previewPath)
        {
            _previewPath = resultPath;
            _preview.setEditTexture(std::make_shared<bg2e::render::Texture>(
                _appDelegate->engine(),
                std::make_shared<bg2e::base::Texture>(_previewPath)));
        }
        if (!_previewPath.empty())
        {
            Text::separator("Result Preview");
            _preview.drawImage(192, 192);
        }
    });
}

void ModelLightmapWindow::cleanup()
{
    // Destroying the token cancels a pending UV2 regeneration
    _uv2Token.reset();
    _uv2Generating = false;
    _uvPreview.cleanup();
    _preview.cleanup();
    _previewPath.clear();
}

void ModelLightmapWindow::generateUv2()
{
    auto targetNode = _appDelegate->stage()->targetModelNode();
    if (!targetNode)
    {
        return;
    }

    // A new UV layout invalidates any in-flight bake accumulation
    if (_appDelegate->lightmapBakeActive())
    {
        _appDelegate->cancelLightmapBake();
    }

    bg2e::geo::Uv2AtlasOptions options;
    options.resolution = 128u << _resolutionIndex;
    options.paddingPixels = static_cast<uint32_t>(std::max(_uv2Padding, 0));

    _message.clear();
    _uv2Generating = true;
    _uv2Token = bg2e::app::Uv2SafeReload::regenerate(
        targetNode, options,
        [this](const bg2e::app::Uv2RegenerationResult& result) {
            _uv2Generating = false;
            if (result.success)
            {
                _message =
                    "UV2 atlas generated: " + std::to_string(result.chartCount) +
                    " charts, " + std::to_string(result.atlasWidth) + "x" +
                    std::to_string(result.atlasHeight);
                _uvPreview.refresh();
                _appDelegate->stage()->document()->setUnsavedChanges(true);
            }
            else
            {
                _message = result.message;
                bg2e::app::MessageBox::showError("Generate UV2", result.message);
            }
        });
}
