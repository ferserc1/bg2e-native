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
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/RenderLoopDelegate.hpp>

#include <glm/glm.hpp>
#include <memory>
#include <functional>

namespace bg2e {
namespace render {

class BG2E_API RenderLoop {
public:
    // Init engine resources, for example, the main descriptor set allocator,
    // render images, etc
    void init(Engine * engine);
    
    // Init scene resource, for example: create pipelines, load textures,
    // load 3d models...
    void initScene();

    void acquireAndPresent();

    void swapchainResized();

    void initFrameResources(vulkan::DescriptorSetAllocator*);

    VkImageLayout render(
        VkCommandBuffer cmd,
        const vulkan::Image* colorImage,
        const vulkan::Image* depthImage,
        const vulkan::Image* msaaDepthImage,
        vulkan::FrameResources& frameResources
    );

    void cleanup();

    void pauseScene(glm::vec4 clearColor = {0.f, 0.f, 0.f, 1.f});
    void resumeScene();
    inline bool isScenePaused() const { return _scenePaused; }

    inline void setDelegate(std::shared_ptr<RenderLoopDelegate> delegate) { _renderDelegate = delegate; }

	inline void renderUICallback(std::function<void(VkCommandBuffer, VkImageView)> callback) { _renderUICallback = callback; }

    inline void setDelta(float d) { _delta = d; }
    
protected:
	Engine * _engine;
    bool _resizeRequested = false;

    std::shared_ptr<RenderLoopDelegate> _renderDelegate;

    std::function<void(VkCommandBuffer, VkImageView)> _renderUICallback;

    float _delta;

    bool _scenePaused = false;
    glm::vec4 _sceneClearColor { 0.f, 0.f, 0.f, 1.f };

    void cmdClearColorImage(
        VkCommandBuffer cmd,
        const vulkan::Image* targetImage,
        const glm::vec4& clearColor
    );
};

}
}
