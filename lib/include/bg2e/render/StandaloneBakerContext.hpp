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
#include <bg2e/render/vulkan/common.hpp>

#include <bg2e/base/Light.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace bg2e {
namespace scene {
class Node;
class Scene;
}
namespace render {

namespace vulkan {
struct FrameResources;
class DescriptorSetAllocator;
namespace rt {
class RayTracingScene;
}
}

class BG2E_API StandaloneLightmapBaker final : public LightmapBaker {
public:
    // Submits one bake sample synchronously after the context has prepared a scene generation.
    void update();

private:
    friend class StandaloneBakerContext;
    StandaloneLightmapBaker(std::shared_ptr<BakerContext> context,
                            std::shared_ptr<scene::Node> targetNode,
                            LightmapSettings settings,
                            uint64_t sceneGeneration);

    uint64_t _sceneGeneration = 0;
};

class BG2E_API StandaloneBakerContext final : public BakerContext {
public:
    StandaloneBakerContext(Engine* engine, std::shared_ptr<scene::Scene> scene);
    ~StandaloneBakerContext() override;

    StandaloneBakerContext(const StandaloneBakerContext&) = delete;
    StandaloneBakerContext& operator=(const StandaloneBakerContext&) = delete;

    void initialize(VkExtent2D extent);
    void updateScene(float deltaSeconds);
    [[nodiscard]] std::shared_ptr<StandaloneLightmapBaker> createBaker(
        std::shared_ptr<scene::Node> targetNode,
        LightmapSettings settings = {});
    void cleanup();

private:
    friend class StandaloneLightmapBaker;

    enum class State : uint8_t {
        Created,
        Initialized,
        SceneReady,
        Cleaned
    };

    void requireState(State expected, const char* operation) const;
    void requireCurrentSceneRoot(const char* operation) const;
    void resizeSceneOnce();
    void initializeBakeResources();
    void refreshEnvironmentResource();
    void captureSceneLights();
    void recordAndSubmitSceneTlas();
    void invalidateBakersForSceneUpdate();
    void beginBakerUpdate(uint64_t bakerGeneration);
    void endBakerUpdate() noexcept;

    std::shared_ptr<scene::Scene> _scene;
    VkExtent2D _extent{0, 0};
    State _state = State::Created;
    uint64_t _sceneGeneration = 0;
    bool _updateInProgress = false;
    bool _sceneResizeComplete = false;
    std::vector<std::weak_ptr<StandaloneLightmapBaker>> _bakers;
    std::vector<base::LightData> _sceneLights;
    std::string _environmentImage;
    size_t _environmentImageHash = 0;

    // Owned standalone execution resources are initialized by the scene-update step.
    struct BakeResources;
    std::shared_ptr<BakeResources> _bakeResources;
    std::unique_ptr<vulkan::FrameResources> _frameResources;
    std::unique_ptr<vulkan::rt::RayTracingScene> _rayTracingScene;
};

}
}
