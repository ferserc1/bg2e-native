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
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/render/vulkan/factory/RayTracingPipeline.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp>
#include <bg2e/render/vulkan/rt/ReflectionLightDataBinding.hpp>
#include <bg2e/base/Light.hpp>

#include <memory>
#include <vector>

namespace bg2e::render::deferred {

enum class RTGIQuality {
    Low,
    Medium,
    High,
    Ultra
};

inline float rtgiResolutionScale(RTGIQuality quality)
{
    switch (quality)
    {
        case RTGIQuality::Ultra:  return 1.0f;
        case RTGIQuality::High:   return 2.0f / 3.0f;
        case RTGIQuality::Medium: return 0.5f;
        case RTGIQuality::Low:    return 1.0f / 3.0f;
    }
    return 1.0f;
}

struct RTGISettings {
    bool enabled = true;
    uint32_t sampleCount = 4;
    uint32_t bounceCount = 2;
    float rayBias = 0.02f;
    float maxDistance = 50.0f;
    RTGIQuality quality = RTGIQuality::Ultra;
};

class BG2E_API RTGlobalIllumination {
public:
    explicit RTGlobalIllumination(Engine* engine);
    ~RTGlobalIllumination();

    void setMaterialDataBinding(vulkan::rt::RTMaterialDataBinding* binding) { _materialDataBinding = binding; }
    void setReflectionLightDataBinding(vulkan::rt::ReflectionLightDataBinding* binding) { _reflectionLightDataBinding = binding; }

    void build(const GBufferManager* gbuffer, VkExtent2D extent);
    void resize(VkExtent2D extent);

    void render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources,
        const GBufferManager* gbuffer,
        const glm::mat4& inverseViewProjection,
        const glm::vec3& cameraPosition,
        VkAccelerationStructureKHR tlas,
        const std::vector<vulkan::rt::RTObjectInstance>& objectInstances,
        vulkan::Image* irradianceMap,
        VkSampler irradianceSampler,
        const std::vector<base::LightData>& giLights
    );

    void cleanup();

    std::shared_ptr<vulkan::Image> giImage(uint32_t frameIndex) const;
    VkSampler sampler() const { return _sampler; }
    bool rtSupported() const { return _rtSupported; }

    void setSettings(const RTGISettings& settings) { _settings = settings; }
    const RTGISettings& settings() const { return _settings; }

    void setEnabled(bool enabled) { _settings.enabled = enabled; }
    void setSampleCount(uint32_t count) { _settings.sampleCount = count; }
    void setBounceCount(uint32_t count) { _settings.bounceCount = count; }
    void setRayBias(float bias) { _settings.rayBias = bias; }
    void setMaxDistance(float distance) { _settings.maxDistance = distance; }
    void setQuality(RTGIQuality quality);

private:
    Engine* _engine = nullptr;
    RTGISettings _settings;
    VkExtent2D _extent{};
    bool _rtSupported = false;

    vulkan::rt::RTMaterialDataBinding* _materialDataBinding = nullptr;
    vulkan::rt::ReflectionLightDataBinding* _reflectionLightDataBinding = nullptr;

    std::vector<std::shared_ptr<vulkan::Image>> _giImages;
    std::shared_ptr<vulkan::Image> _fallbackImage;

    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _dsLayout = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;

    std::unique_ptr<vulkan::Buffer> _sbtBuffer;
    VkStridedDeviceAddressRegionKHR _raygenRegion = {};
    VkStridedDeviceAddressRegionKHR _missRegion = {};
    VkStridedDeviceAddressRegionKHR _hitRegion = {};
    VkStridedDeviceAddressRegionKHR _callableRegion = {};

    struct GIPushConstants {
        glm::mat4 inverseViewProjection;
        glm::vec3 cameraPosition;
        float rayBias;
        glm::vec2 outputSize;
        uint32_t sampleCount;
        uint32_t bounceCount;
        uint32_t frameIndex;
        float maxDistance;
        uint32_t giLightCount;
        uint32_t padding0;
    };

    void createFallbackImage();
    void createGIResources(VkExtent2D extent);
    void createPipeline();
    void cleanupImages();
};

}
