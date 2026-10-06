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
    _preview.init(delegate->engine());

    setDrawFunction([this]() {
        using namespace bg2e::ui;

        if (!_appDelegate->stage()->targetModelNode())
        {
            Text::text("No model loaded.");
            return;
        }

        Text::separator("Bake Settings");
        Text::text("Mode: RTAO (RT shadows disabled)");
        Numeric::sliderInt("Resolution", &_resolution, 64, 2048);
        Numeric::sliderInt("Accumulation Frames", &_frames, 1, 512);
        Numeric::sliderInt("Samples per Pixel", &_samples, 1, 256);
        Numeric::drag("Max Ray Distance", &_maxDistance, 0.5f, 0.01f, 10000.0f);

        const bool baking = _appDelegate->lightmapBakeActive();
        if (Button::button("Generate", false, baking))
        {
            bg2e::render::LightmapSettings settings;
            settings.resolution = static_cast<uint32_t>(std::max(_resolution, 1));
            settings.accumulationFrames = static_cast<uint32_t>(std::max(_frames, 1));
            settings.samplesPerPixel = static_cast<uint32_t>(std::max(_samples, 1));
            settings.maxRayDistance = std::max(_maxDistance, 0.01f);
            settings.mode = bg2e::render::LightmapMode::RTAO;
            settings.rtShadows = false;
            settings.cpuFormat = bg2e::render::LightmapPixelFormat::RGB8;
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
    _preview.cleanup();
    _previewPath.clear();
}
