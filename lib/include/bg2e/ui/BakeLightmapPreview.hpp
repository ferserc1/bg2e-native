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

#include <bg2e/bake/LightmapBaker.hpp>
#include <bg2e/ui/TexturePreview.hpp>

namespace bg2e::draw { class Engine; class Texture; }
namespace bg2e::ui { class UserInterface; }

namespace bg2e::ui {

// Displays a completed bake image through the draw UI session. The widget
// retains the GPU image owner and its draw texture until clear() or rebind().
// Create and draw it on the UI thread; clear it before UI/Engine teardown.
class BG2E_API BakeLightmapPreview {
  public:
    BakeLightmapPreview() = default;
    ~BakeLightmapPreview();
    BakeLightmapPreview(const BakeLightmapPreview&) = delete;
    BakeLightmapPreview& operator=(const BakeLightmapPreview&) = delete;

    void setResult(UserInterface& ui, draw::Engine& engine, const bake::LightmapResult& result);
    void setImage(UserInterface& ui, draw::Engine& engine, std::shared_ptr<gpu::Image> image);
    void draw(uint32_t width, uint32_t height);
    void clear();
    bool valid() const;

  private:
    std::shared_ptr<draw::Texture> _texture;
    std::shared_ptr<TexturePreview> _preview;
};

}
