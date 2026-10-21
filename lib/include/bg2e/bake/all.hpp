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

#include <bg2e/bake/Types.hpp>
#include <bg2e/bake/LightmapSettings.hpp>
#include <bg2e/bake/LightmapPixels.hpp>
#include <bg2e/bake/BakerContext.hpp>
#include <bg2e/bake/LightmapBaker.hpp>

#include <bg2e/bake/UvSurfaceMesh.hpp>
#include <bg2e/bake/UvSurfacePass.hpp>
#include <bg2e/bake/Uv2Reload.hpp>

#include <bg2e/bake/LightmapBakeExecutor.hpp>
#include <bg2e/bake/IntegratedBakerContext.hpp>
#include <bg2e/bake/StandaloneBakerContext.hpp>
#include <bg2e/bake/StandaloneBakeSceneAssembler.hpp>

#include <bg2e/bake/UvTemporalAccumulator.hpp>
#include <bg2e/bake/LightmapCompositionPass.hpp>
#include <bg2e/bake/UvMapPreviewRenderer.hpp>

#include <bg2e/bake/LightmapOutputWriter.hpp>
#include <bg2e/bake/StandaloneBakeBatch.hpp>
