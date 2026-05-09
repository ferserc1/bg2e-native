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

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>

namespace bg2e {
namespace render {

class RenderLoopDelegate {
public:
    virtual ~RenderLoopDelegate() {}

    // Init engine resource, for example, the main descriptor set allocator requirements
    virtual void init(render::Engine * engine)
    {
		_engine = engine;
    }

    // Init frame resources, tipically, the frame descriptor set allocator
    virtual void initFrameResources(render::vulkan::DescriptorSetAllocator*) {}
    
    // Init scene data, such as pipelines, textures or 3D models
    virtual void initScene() {}

    virtual void swapchainResized(VkExtent2D) = 0;

    virtual void update(uint32_t /* currentFrame */, render::vulkan::FrameResources& /* frameResources */) {}

    virtual VkImageLayout render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const render::vulkan::Image* colorImage,
        const render::vulkan::Image* depthImage,
        const render::vulkan::Image* msaaDepthImage,
        render::vulkan::FrameResources& frameResources
    ) = 0;

	virtual void cleanup() {}

	inline render::Engine * engine() { return _engine; }

    inline float delta() const { return _delta; }
    inline void setDelta(float d) { _delta = d; }

    virtual bool supportsMsaa() { return true; }

protected:
	render::Engine * _engine = nullptr;
    
    float _delta = 0.0f;
};

}
}
