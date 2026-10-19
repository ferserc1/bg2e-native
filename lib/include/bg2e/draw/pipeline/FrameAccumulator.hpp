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

#include <bg2e/gpu/Image.hpp>
#include <span>

namespace bg2e::draw::pipeline {
// Exact requested-sample arithmetic mean, without reprojection or EMA. This
// synchronous reference implementation accumulates finite linear RGB in double
// precision on the CPU. Alpha is opaque; final half conversion happens once.
class BG2E_API FrameAccumulator {
  public:
    void reset(gpu::Size2D extent, uint32_t requestedFrames);
    void add(std::span<const float> rgba);
    // Call only after the sample's GPU submission completes. Reads RGBA16F.
    void add(gpu::Image& completedSample);
    // Available only after exactly requestedFrames successful additions.
    std::vector<float> resolve() const;
    void resolveInto(gpu::Image& destination) const;
    uint32_t completedFrames() const;
    bool complete() const;

  private:
    gpu::Size2D _extent{};
    uint32_t _requested = 0, _completed = 0;
    std::vector<double> _sum;
};
}
