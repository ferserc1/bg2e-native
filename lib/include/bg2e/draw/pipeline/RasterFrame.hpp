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

#include <bg2e/draw/FrameDataBinding.hpp>
#include <bg2e/draw/ObjectDataBinding.hpp>
#include <bg2e/draw/LightDataBinding.hpp>
#include <bg2e/draw/EnvironmentDataBinding.hpp>
#include <bg2e/draw/RenderQueue.hpp>

namespace bg2e::draw::pipeline {
// All pointers are borrowed for one pass. Null item selects the opaque queue;
// non-null item selects exactly one sorted transparent submesh.
struct RasterFrame {
    const RenderQueue* queue = nullptr;
    const RenderQueue::Item* item = nullptr;
    FrameDataBinding* frame = nullptr;
    ObjectDataBinding* object = nullptr;
    LightDataBinding* lights = nullptr;
    EnvironmentDataBinding* environment = nullptr;
    std::optional<gpu::ScissorRegion> region;
};
}
