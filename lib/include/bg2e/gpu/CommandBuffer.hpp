/*
 *    business grade graphic engine (bg2e engine)
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
#include <bg2e/gpu/Common.hpp>
#include <stdexcept>

namespace bg2e {
namespace gpu {

class Image;
class SurfaceFrame;
class GraphicsPipeline;

class BG2E_API CommandBuffer {
public:
    virtual ~CommandBuffer() = default;

    virtual void begin() = 0;
    virtual void end()   = 0;

    virtual void transition(gpu::Image* image, ImageLayout newLayout) = 0;

    virtual void beginRendering(gpu::SurfaceFrame* frame) = 0;
    virtual void endRendering() = 0;

    virtual void clearColor(uint32_t attachmentIndex, const gpu::Color& color) = 0;
    virtual void clearDepth(float depth) = 0;

    virtual void bindPipeline(gpu::GraphicsPipeline* pipeline)
    {
        throw std::runtime_error("bindPipeline not implemented");
    }

    virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0)
    {
        throw std::runtime_error("draw not implemented");
    }

    virtual bool isValid() const = 0;
};

}
}