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

#include <bg2e/gpu/OffscreenSurface.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/vk/Surface.hpp>
#include <bg2e/gpu/vk/Image.hpp>
#include <bg2e/gpu/vk/SurfaceFrame.hpp>

#include <memory>

namespace bg2e {
namespace gpu {

class Image;

namespace vk {

class OffscreenSurface : public gpu::OffscreenSurface, public Surface {
public:
    explicit OffscreenSurface(const Size2D& size)
        : gpu::OffscreenSurface(size) {}
    ~OffscreenSurface() override;

    void resize(const Size2D& size) override;
    void releaseRenderTarget() override;
    void cleanup() override;

    bool isValid() const override;

    uint32_t    imageCount() const override;
    gpu::Image* colorImage(uint32_t index) const override;
    gpu::Image* depthImage() const override;

    std::shared_ptr<gpu::SurfaceFrame> beginFrame() override;
    void present(gpu::CommandBuffer* cmd) override;
    void endFrame(gpu::SurfaceFrame* frame) override;

protected:
    void createRenderTarget(gpu::Device* device, gpu::PhysicalDevice* physicalDevice) override;

private:
    std::unique_ptr<vk::Image> _colorImage;
    std::shared_ptr<vk::SurfaceFrame> _frame;
};

}
}
}
