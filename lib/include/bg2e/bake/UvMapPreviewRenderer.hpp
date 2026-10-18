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

#include <bg2e/geo/Mesh.hpp>
#include <bg2e/gpu/Image.hpp>
#include <memory>
#include <vector>

namespace bg2e::bake {

// Produces a sampled UV diagnostic image without changing the source mesh or
// requiring the production renderer. Triangle coverage is rasterized on CPU;
// the resulting RGBA8 image is uploaded through the backend-neutral Device API.
class BG2E_API UvMapPreviewRenderer {
  public:
    explicit UvMapPreviewRenderer(gpu::Device& device, uint32_t resolution = 512);
    void setResolution(uint32_t resolution);
    uint32_t resolution() const;
    void render(const geo::Mesh& mesh, uint32_t uvSet);
    std::shared_ptr<gpu::Image> image() const;
    const std::vector<uint8_t>& pixels() const;

  private:
    gpu::Device* _device;
    uint32_t _resolution;
    std::shared_ptr<gpu::Image> _image;
    std::vector<uint8_t> _pixels;

    void createTarget();
};

}
