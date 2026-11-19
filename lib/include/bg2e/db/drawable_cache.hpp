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

#include <bg2e/draw/DrawableCache.hpp>

namespace bg2e::db {

BG2E_API draw::DrawableCacheResult loadCachedDrawableBg2(const std::filesystem::path& path,
    draw::DrawableCache& cache, std::function<void()> onTextureLoaded = nullptr);

// Opt-in resident loading. Legacy loadDrawableBg2 keeps returning independent
// geometry; scene owner/clone semantics are integrated in the lifecycle step.
// Synchronous on the engine's resource-update thread. Recoverable errors become
// structured cache diagnostics; callers decide how to present warnings.
BG2E_API draw::DrawableCacheResult loadCachedDrawableBg2(const std::filesystem::path& path,
    draw::Engine& engine, std::function<void()> onTextureLoaded = nullptr);

}
