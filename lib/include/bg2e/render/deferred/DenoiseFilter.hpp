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

class BG2E_API DenoiseFilter {
public:
    DenoiseFilter(Engine * engine);
    ~DenoiseFilter();

    void build(const GBufferManager * gbuffer, VkExtent2D extent);
    void resize(VkExtent2D newExtent);
    void render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        vulkan::FrameResources & frameResources,
        const GBufferManager * gbuffer,
        const vulkan::Image * inputImage
    );
    void cleanup();

    std::shared_ptr<vulkan::Image> outputImage(uint32_t frameIndex) const;
    VkSampler sampler() const;

    void setKernelRadius(int radius);
    void setDepthThreshold(float threshold);
    void setNormalThreshold(float threshold);
    void setDepthSigma(float sigma);
    void setNormalSigma(float sigma);

    int kernelRadius() const;
    float depthThreshold() const;
    float normalThreshold() const;
    float depthSigma() const;
    float normalSigma() const;

    // Must be set before build(). When true, uses RGBA16F output and the HDR bilateral shader.
    void setIsHDR(bool hdr) { _isHDR = hdr; }
    bool isHDR() const { return _isHDR; }

private:
    Engine * _engine;
    VkExtent2D _extent;

    std::vector<std::shared_ptr<vulkan::Image>> _outputImages;

    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _dsLayout = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;

    int _kernelRadius = 2;
    float _depthThreshold = 0.01f;
    float _normalThreshold = 0.8f;
    float _depthSigma = 0.01f;
    float _normalSigma = 0.3f;
    bool _isHDR = false;

    struct DenoisePushConstants {
        glm::vec2 outputSize;
        int kernelRadius;
        float depthThreshold;
        float normalThreshold;
        float depthSigma;
        float normalSigma;
        uint32_t padding;
    };

    void createOutputImages(VkExtent2D extent);
    void createPipeline();
    void cleanupImages();
};

}
}
}