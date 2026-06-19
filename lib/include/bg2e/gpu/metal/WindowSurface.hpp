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

#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/gpu/metal/Surface.hpp>
#include <bg2e/gpu/metal/common.hpp>

#include <memory>

namespace bg2e {
namespace gpu {

class Image;

namespace metal {

class Image;
class SurfaceFrame;

class WindowSurface : public gpu::WindowSurface, public Surface {
public:
    void cleanup() override;

    uint32_t width() const override;
    uint32_t height() const override;
    bool isValid() const override;

    MetalLayerHandle metalLayer() const { return _layer; }

    void resize(const Size2D& size) override;
    void releaseRenderTarget() override;

    uint32_t    imageCount() const override;
    uint32_t    inFlightFrames() const override;
    uint32_t    currentFrameIndex() const override;
    gpu::Image* colorImage(uint32_t index) const override;
    gpu::Image* depthImage() const override;

    std::shared_ptr<gpu::SurfaceFrame> beginFrame() override;
    void present(gpu::CommandBuffer* cmd) override;
    void endFrame(gpu::SurfaceFrame* frame) override;

protected:
    void create(gpu::Instance* instance) override;
    void createRenderTarget(gpu::Device* device, gpu::PhysicalDevice* physicalDevice) override;

private:
    void* _metalView = nullptr;
    MetalLayerHandle _layer = nullptr;
    uint32_t _imageCount = 0;
    uint32_t _currentFrameIndex = 0;
    metal::SurfaceFrame* _currentFrame = nullptr;
};

}
}
}
