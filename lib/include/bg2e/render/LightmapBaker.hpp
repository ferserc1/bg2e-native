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

#include <bg2e/render/LightmapSettings.hpp>
#include <bg2e/render/vulkan/common.hpp>

#include <memory>
#include <variant>
#include <vector>

namespace bg2e {
namespace scene {
class Node;
}
namespace render {

class BakerContext;
class LightmapBakeExecutor;
class UvSurfacePass;
class UvTemporalAccumulator;
namespace vulkan {
class Image;
class DescriptorSetAllocator;
}

struct BG2E_API LightmapPixels {
    uint32_t width = 0;
    uint32_t height = 0;
    LightmapPixelFormat format = LightmapPixelFormat::RGB8;
    std::variant<std::vector<uint8_t>, std::vector<float>> rgb;
};

class BG2E_API LightmapBaker {
public:
    virtual ~LightmapBaker();

    [[nodiscard]] uint32_t completedFrames() const { return _completedFrames; }
    void resetAccumulation();
    [[nodiscard]] LightmapPixels readPixels() const;
    [[nodiscard]] std::shared_ptr<vulkan::Image> image() const;
    [[nodiscard]] const LightmapSettings& settings() const { return _settings; }
    [[nodiscard]] std::shared_ptr<scene::Node> targetNode() const { return _targetNode; }

protected:
    friend class LightmapBakeExecutor;

    LightmapBaker(std::shared_ptr<BakerContext> context,
                  std::shared_ptr<scene::Node> targetNode,
                  LightmapSettings settings);
    void validateTarget() const;
    void recordUvSurface(VkCommandBuffer cmd, uint32_t frameSlot);
    [[nodiscard]] vulkan::Image& aoImage(uint32_t frameSlot);
    [[nodiscard]] vulkan::Image& giImage(uint32_t frameSlot);
    [[nodiscard]] vulkan::Image& resultImage(uint32_t frameSlot);
    void recordAccumulation(VkCommandBuffer cmd,
                            vulkan::DescriptorSetAllocator& descriptors,
                            uint32_t frameSlot);
    void markResultImage(uint32_t frameSlot);

    std::shared_ptr<BakerContext> _context;
    std::shared_ptr<scene::Node> _targetNode;
    LightmapSettings _settings;
    uint32_t _completedFrames = 0;
    std::shared_ptr<UvSurfacePass> _uvSurfacePass;
    std::shared_ptr<UvTemporalAccumulator> _accumulator;
    std::vector<std::shared_ptr<vulkan::Image>> _aoImages;
    std::vector<std::shared_ptr<vulkan::Image>> _giImages;
    std::vector<std::shared_ptr<vulkan::Image>> _resultImages;
    uint32_t _resultFrameNumber = 0;
    bool _hasResultImage = false;
};

}
}
