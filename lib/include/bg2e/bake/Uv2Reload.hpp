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
#include <bg2e/geo/GenerateUv2AtlasModifier.hpp>
#include <bg2e/draw/Drawable.hpp>

namespace bg2e::bake {
struct Uv2ReloadResult {
    std::shared_ptr<draw::Drawable> drawable; // New independent realization, source unchanged.
    geo::Uv2AtlasResult atlas;
};
// Synchronous rare edit. Caller stops all GPU producers; this drains Device users.
// Generate/validate UV2 on a new CPU mesh, deep-clone target resources and reload
// raster/BLAS. Publish the returned Drawable only after success. Re-extract queues,
// invalidate source/target/history generations and refresh selection/preview owners.
BG2E_API Uv2ReloadResult generateUv2AndReload(const std::shared_ptr<draw::Drawable>& source,
    const geo::Uv2AtlasOptions& options = {});
}
