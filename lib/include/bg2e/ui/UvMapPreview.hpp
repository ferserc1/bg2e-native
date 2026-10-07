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

#include <bg2e/common.hpp>
#include <bg2e/geo/Mesh.hpp>
#include <bg2e/geo/UvAtlasValidator.hpp>
#include <bg2e/render/UvMapPreviewRenderer.hpp>
#include <bg2e/ui/TextureWidgets.hpp>

#include <cstdint>
#include <memory>

namespace bg2e {
namespace ui {

// Embeddable UV map preview widget. Displays the wireframe/coverage preview
// produced by render::UvMapPreviewRenderer for the UV1 or UV2 set of a CPU
// mesh, along with the atlas validity report. It retains only the shared CPU
// mesh (never a Drawable), so it survives mesh regeneration and scene swaps:
// call setMesh() or refresh() after the CPU mesh changes.
// init() must be called before draw(). cleanup() releases the ImGui/Vulkan
// descriptor and must be called before the engine is destroyed.
class BG2E_API UvMapPreview {
public:
    UvMapPreview();
    ~UvMapPreview();

    UvMapPreview(const UvMapPreview &) = delete;
    UvMapPreview & operator=(const UvMapPreview &) = delete;

    void init(render::Engine * engine, uint32_t resolution = 256);

    void setMesh(std::shared_ptr<geo::Mesh> mesh);
    inline std::shared_ptr<geo::Mesh> mesh() const { return _mesh; }

    // 0 = UV1 (texCoord0), 1 = UV2 (texCoord1). Other values are clamped.
    void setUvSet(uint32_t uvSet);
    inline uint32_t uvSet() const { return _uvSet; }

    void setResolution(uint32_t resolution);
    uint32_t resolution() const;

    // Side of the square preview image in the UI, in pixels.
    inline void setDisplaySize(uint32_t size) { _displaySize = size; }
    inline uint32_t displaySize() const { return _displaySize; }

    // Marks the preview dirty so the next draw() re-renders and re-validates.
    void refresh();

    void draw();

    void cleanup();

private:
    render::Engine * _engine = nullptr;
    std::unique_ptr<render::UvMapPreviewRenderer> _renderer;
    std::shared_ptr<geo::Mesh> _mesh;
    uint32_t _uvSet = 1;
    uint32_t _displaySize = 256;
    bool _dirty = true;

    geo::UvAtlasValidation _validation;
    TextureWidgets _imageWidget;

    void updatePreview();
};

}
}
