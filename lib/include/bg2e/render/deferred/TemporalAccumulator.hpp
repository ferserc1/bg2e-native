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

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>

#include <vector>
#include <memory>

namespace bg2e {
namespace render {
namespace deferred {

// TemporalAccumulator keeps a SINGLE shared history chain (one ping-pong A/B
// pair, one prev-depth and one prev-normal image), independent of the number
// of frames in flight. Temporal history is global renderer state, not a
// per-swapchain-image resource. This relies on all rendering work being
// submitted to a single queue, which executes command buffers in submission
// order: frame N+1 does not start on the GPU until frame N has finished, so
// the layout transitions recorded each frame act as the barriers between the
// history write of frame N and the history read of frame N+1.
// If the accumulation dispatch is ever moved to a separate async compute
// queue, explicit cross-queue semaphores will be required.
class BG2E_API TemporalAccumulator {
public:
    TemporalAccumulator(Engine* engine);
    ~TemporalAccumulator();

    void build(const GBufferManager* gbuffer, VkExtent2D extent);
    void resize(VkExtent2D newExtent);
    void render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources,
        const GBufferManager* gbuffer,
        const vulkan::Image* aoImage,
        const glm::mat4& currentInverseViewProjection,
        const glm::mat4& currentView,
        const glm::mat4& currentProjection
    );
    void cleanup();

    // The frameIndex parameter is ignored: the accumulator keeps a single
    // shared history chain. It is kept for source compatibility with
    // existing call sites.
    std::shared_ptr<vulkan::Image> outputImage(uint32_t frameIndex) const;
    VkSampler sampler() const;

    void invalidateHistory();

    enum class AccumulationMode { Interactive, Progressive };
    void setAccumulationMode(AccumulationMode mode);
    AccumulationMode accumulationMode() const;

    void setHistoryWeight(float weight);
    float historyWeight() const;

    void setDepthThreshold(float threshold);
    void setNormalThreshold(float threshold);
    float depthThreshold() const;
    float normalThreshold() const;

    void setFormat(VkFormat format) { _format = format; }
    VkFormat format() const { return _format; }

    void setIsHDR(bool value) { _isHDR = value; }
    bool isHDR() const { return _isHDR; }

private:
    Engine* _engine;
    VkExtent2D _extent;

    std::shared_ptr<vulkan::Image> _historyImageA;
    std::shared_ptr<vulkan::Image> _historyImageB;
    std::shared_ptr<vulkan::Image> _prevDepthImage;
    std::shared_ptr<vulkan::Image> _prevNormalImage;

    uint32_t _writeIndex = 0;

    bool _hasHistory = false;
    uint32_t _accumulatedFrameCount = 0;

    glm::mat4 _previousViewProjection = glm::mat4(1.0f);

    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _dsLayout = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;
    VkFormat _depthFormat = VK_FORMAT_D32_SFLOAT;
    VkFormat _normalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkFormat _format = VK_FORMAT_R16_SFLOAT;
    bool _isHDR = false;

    AccumulationMode _accumulationMode = AccumulationMode::Interactive;
    float _historyWeight = 0.9f;
    float _depthThreshold = 0.01f;
    float _normalThreshold = 0.8f;

    struct AccumulatorPushConstants {
        glm::mat4 currentInverseViewProjection;
        glm::mat4 previousViewProjection;
        glm::vec2 outputSize;
        float historyWeight;
        uint32_t accumulatedFrameCount;
        uint32_t useProgressiveMode;
        uint32_t hasHistory;
        float depthThreshold;
        float normalThreshold;
        uint32_t isHDR;
        uint32_t padding0;
    };

    void createHistoryImages(VkExtent2D extent);
    void createPipeline();
    void cleanupImages();

    std::shared_ptr<vulkan::Image> historyReadImage() const;
    std::shared_ptr<vulkan::Image> historyWriteImage() const;
};

}
}
}