
#ifndef _bg2render_render_pass_hpp_
#define _bg2render_render_pass_hpp_

#include <bg2render/vk_instance.hpp>
#include <bg2render/vk_device.hpp>

#include <vector>

namespace bg2render {

class RenderPass {
public:
	RenderPass(vk::Device * dev);
	virtual ~RenderPass();


	inline const VkRenderPassCreateInfo& createInfo() const { return _createInfo; }
	inline VkRenderPassCreateInfo& createInfo() { return _createInfo; }

	void create();

	inline VkRenderPass renderPass() const { return _renderPass; }
	
protected:
	vk::Device* _device = nullptr;

	VkRenderPass _renderPass = VK_NULL_HANDLE;

	VkRenderPassCreateInfo _createInfo = {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr,
		0,
		nullptr
	};
};

}

#endif
