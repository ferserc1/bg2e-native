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

#pragma once

#include <bg2e/bake/UvMapPreviewRenderer.hpp>
#include <bg2e/app/Uv2SafeReload.hpp>
#include <bg2e/draw/Drawable.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/ui/BakeLightmapPreview.hpp>
#include <bg2e/ui/Window.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace bg2e::draw { class Engine; }
namespace bg2e::ui { class UserInterface; }
namespace model_edit { class AppDelegate; }

namespace model_edit {

class ModelLightmapWindow final : public bg2e::ui::Window {
  public:
    void init(const std::shared_ptr<AppDelegate>& delegate, bg2e::draw::Engine* engine,
        bg2e::ui::UserInterface* ui);
    void draw() override;
    void cleanup();

  private:
    void drawContents();
    void refreshUvPreview(const std::shared_ptr<bg2e::scene::Node>& target);

    std::weak_ptr<AppDelegate> _delegate;
    bg2e::draw::Engine* _engine = nullptr;
    bg2e::ui::UserInterface* _ui = nullptr;
    std::unique_ptr<bg2e::bake::UvMapPreviewRenderer> _uv0Renderer;
    std::unique_ptr<bg2e::bake::UvMapPreviewRenderer> _uv1Renderer;
    bg2e::ui::BakeLightmapPreview _uv0Preview;
    bg2e::ui::BakeLightmapPreview _uv1Preview;
    bg2e::ui::BakeLightmapPreview _resultPreview;
    std::weak_ptr<bg2e::scene::Node> _previewTarget;
    std::weak_ptr<bg2e::draw::Drawable> _previewDrawable;
    std::weak_ptr<bg2e::scene::Node> _uv2Target;
    std::weak_ptr<bg2e::draw::Drawable> _uv2Drawable;
    int _previewResolution = 256;
    std::shared_ptr<bg2e::gpu::Image> _previewResultImage;
    uint64_t _documentGeneration = UINT64_MAX;
    uint32_t _resolutionIndex = 2;
    uint32_t _modeIndex = 0;
    uint32_t _formatIndex = 0;
    int _frames = 16;
    int _samples = 8;
    int _padding = 4;
    int _dilation = 4;
    int _bounces = 2;
    float _aoRadius = 0.1f;
    float _maxDistance = 50.0f;
    float _rayBias = 0.0005f;
    float _exposure = 0.0f;
    bool _uv2Generating = false;
    bool _wasOpen = false;
    std::shared_ptr<bg2e::app::SafeUpdateToken> _uv2Token;
    std::string _message;
    std::string _uvWarning;
};

}
