
#include <bg2render/render_pass.hpp>

#include <stdexcept>

namespace bg2render {

	RenderPass::RenderPass(vk::Device* dev)
		:_device(dev)
	{

	}

	RenderPass::~RenderPass() {
		if (_renderPass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(_device->device(), _renderPass, nullptr);
		}
	}

	void RenderPass::create() {
		if (vkCreateRenderPass(_device->device(), &_createInfo, nullptr, &_renderPass) != VK_SUCCESS) {
			throw new std::runtime_error("Failed to create render pass");
		}
	}

}
