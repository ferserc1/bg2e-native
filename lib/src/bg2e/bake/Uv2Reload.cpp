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

#include <bg2e/bake/Uv2Reload.hpp>
#include <bg2e/bake/UvSurfaceMesh.hpp>
#include <bg2e/draw/Engine.hpp>
#include <stdexcept>

namespace bg2e::bake {
Uv2ReloadResult generateUv2AndReload(const std::shared_ptr<draw::Drawable>& source,
    const geo::Uv2AtlasOptions& options)
{
    if (!source || !source->isLoaded() || !source->engine() || !source->mesh() || !source->drawMesh())
    {
        throw std::invalid_argument("UV2 reload requires a loaded draw Drawable");
    }
    source->engine()->device()->waitIdle();
    // CPU-only generation is staged first; failure never mutates the source/other attachments.
    auto mesh = std::make_shared<draw::Drawable::Mesh>(*source->mesh());
    geo::GenerateUv2AtlasModifier modifier(mesh.get(),options);
    modifier.apply();
    if (mesh->submeshes.size() != source->submeshesCount())
    {
        throw std::runtime_error("UV2 generation changed material/submesh association");
    }
    buildUvSurfaceMesh(*mesh); // Validate atlas/ranges/positions/normals before GPU publication.
    auto replacement = source->clone(); // Deep CPU/GPU/material/texture ownership, including authored levels.
    replacement->setMesh(std::move(mesh)); // Reloads raster and available BLAS; preserves materials/transforms.
    if (!replacement->isLoaded() || replacement->drawMesh() == source->drawMesh())
    {
        throw std::runtime_error("UV2 reload did not publish a fresh complete draw geometry identity");
    }
    return {std::move(replacement),modifier.result()};
}
}
