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

#include <bg2e.hpp>
#include <bg2e/ui/TextureWidgets.hpp>
#include <bg2e/ui/UvMapPreview.hpp>

class AppDelegate;

// Scene lightmap baker window: lists every node that directly owns a standard
// Drawable, allows multi-selection and bakes one image per selected target
// through a single shared integrated baker context.
class SceneLightmapWindow : public bg2e::ui::Window {
public:
    void init(AppDelegate * delegate);
    void cleanup();

    void setMessage(const std::string& message) { _message = message; }

private:
    AppDelegate * _appDelegate = nullptr;

    std::vector<std::weak_ptr<bg2e::scene::Node>> _selectedTargets;

    int _mode = 1; // 0 = RTAO, 1 = RTGI
    uint32_t _resolutionIndex = 2; // 512
    int _frames = 16;
    int _samples = 8;
    int _giBounces = 2;
    float _maxDistance = 50.0f;
    float _giRayBias = 0.0005f;
    float _exposureEV = 0.0f;
    int _uv2Padding = 4;
    int _lightmapDilation = 4;
    uint32_t _uv2Pending = 0;
    uint32_t _uv2Failed = 0;

    std::string _message;
    bg2e::ui::TextureWidgets _preview;
    std::filesystem::path _previewPath;

    bg2e::ui::UvMapPreview _uvPreview;
    std::vector<std::shared_ptr<bg2e::app::SafeUpdateToken>> _uv2Tokens;

    [[nodiscard]] std::vector<std::shared_ptr<bg2e::scene::Node>> collectTargets() const;
    [[nodiscard]] std::vector<std::shared_ptr<bg2e::scene::Node>> lockedSelection() const;
    [[nodiscard]] bool isSelected(const std::shared_ptr<bg2e::scene::Node>& node) const;
    void setSelected(const std::shared_ptr<bg2e::scene::Node>& node, bool selected);
    void generateUv2(const std::vector<std::shared_ptr<bg2e::scene::Node>>& nodes);
};
