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

#include <bg2e/render/BakerContext.hpp>
#include <bg2e/render/LightmapBaker.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>

#include <memory>
#include <vector>

namespace bg2e {
namespace scene {
class Node;
}
namespace render {

class EnvironmentResources;

namespace deferred {
class RTAmbientOcclusion;
class RTGlobalIllumination;
class RTShadowVisibilityPass;
}

class LightmapCompositionPass;

namespace vulkan {
class DescriptorSetAllocator;
namespace rt {
class RayTracingScene;
class RTMaterialDataBinding;
class ReflectionLightDataBinding;
}
}

class BG2E_API IntegratedLightmapBaker final : public LightmapBaker {
public:
    void update(VkCommandBuffer cmd, vulkan::FrameResources& frameResources);

private:
    friend class IntegratedBakerContext;
    IntegratedLightmapBaker(std::shared_ptr<BakerContext> context,
                            std::shared_ptr<scene::Node> targetNode,
                            LightmapSettings settings);

    bool _hasUpdatedFrame = false;
    uint32_t _lastUpdatedFrame = 0;
};

class BG2E_API IntegratedBakerContext final : public BakerContext {
public:
    IntegratedBakerContext(Engine* engine, scene::Node* rootNode);
    ~IntegratedBakerContext() override;

    void prepareFrame(VkCommandBuffer cmd, vulkan::FrameResources& frameResources);
    // Non-owning pointer to the active renderer's scene IBL resources. The
    // renderer must outlive this context and keep the scene locked while baking.
    void setEnvironmentResources(EnvironmentResources* environmentResources);

    [[nodiscard]] std::unique_ptr<IntegratedLightmapBaker> createBaker(
        std::shared_ptr<scene::Node> targetNode,
        LightmapSettings settings = {});

private:
    friend class IntegratedLightmapBaker;

    struct SceneBindingSnapshot;

    struct PreparedFrame {
        bool valid = false;
        uint32_t frameNumber = 0;
        uint32_t frameSlot = 0;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    };

    [[nodiscard]] vulkan::rt::RayTracingScene& requirePreparedFrame(
        VkCommandBuffer cmd,
        vulkan::FrameResources& frameResources);
    [[nodiscard]] vulkan::DescriptorSetAllocator& bakeDescriptorAllocator(uint32_t frameSlot);
    void initializeBakeDescriptorAllocator(uint32_t frameSlot);
    [[nodiscard]] vulkan::rt::RTMaterialDataBinding& rtMaterialDataBinding();
    [[nodiscard]] deferred::RTAmbientOcclusion& rtaoPass();
    [[nodiscard]] deferred::RTGlobalIllumination& rtgiPass();
    [[nodiscard]] deferred::RTShadowVisibilityPass& shadowPass();
    [[nodiscard]] LightmapCompositionPass& compositionPass();
    [[nodiscard]] vulkan::rt::ReflectionLightDataBinding& giLightDataBinding();
    void captureSceneBindings();
    [[nodiscard]] const SceneBindingSnapshot& sceneBindingSnapshot() const;

    std::vector<std::unique_ptr<vulkan::rt::RayTracingScene>> _bakeRayTracingScenes;
    std::vector<std::unique_ptr<vulkan::DescriptorSetAllocator>> _bakeDescriptorAllocators;
    std::vector<bool> _bakeDescriptorPoolsInitialized;
    std::unique_ptr<vulkan::rt::RTMaterialDataBinding> _rtMaterialDataBinding;
    std::unique_ptr<vulkan::rt::ReflectionLightDataBinding> _giLightDataBinding;
    std::unique_ptr<deferred::RTAmbientOcclusion> _rtaoPass;
    std::unique_ptr<deferred::RTGlobalIllumination> _rtgiPass;
    std::unique_ptr<deferred::RTShadowVisibilityPass> _shadowPass;
    std::unique_ptr<LightmapCompositionPass> _compositionPass;
    std::unique_ptr<SceneBindingSnapshot> _sceneBindingSnapshot;
    EnvironmentResources* _environmentResources = nullptr;
    PreparedFrame _preparedFrame;
    bool _hasPreparedAnyFrame = false;
    uint32_t _lastPreparedFrame = 0;
};

}
}
