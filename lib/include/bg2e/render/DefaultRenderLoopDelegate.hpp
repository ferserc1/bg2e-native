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

#include <bg2e/render/RenderLoopDelegate.hpp>
#include <bg2e/render/RendererBasicForward.hpp>
#include <bg2e/scene/Node.hpp>

#include <memory>

namespace bg2e {
namespace render {

template<typename RendererT>
class BG2E_API DefaultRenderLoopDelegate : public RenderLoopDelegate {
public:
    virtual ~DefaultRenderLoopDelegate() = 0;

    void init(render::Engine * engine) override;

    void initFrameResources(render::vulkan::DescriptorSetAllocator*) override;
    
    void initScene() override;

    void swapchainResized(VkExtent2D) override;

    void update(
        uint32_t currentFrame,
        render::vulkan::FrameResources&
    )override;

    VkImageLayout render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const render::vulkan::Image* colorImage,
        const render::vulkan::Image* depthImage,
        const render::vulkan::Image* msaaDepthImage,
        render::vulkan::FrameResources& frameResources
    ) override;

	void cleanup() override;


    RendererT* renderer();
    
protected:
	std::unique_ptr<RendererT> _renderer;

    virtual std::shared_ptr<scene::Node> createScene() = 0;
};

}
}
