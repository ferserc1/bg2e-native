
#include <bg2render/renderer_delegate.hpp>

namespace bg2render {

	void RendererDelegate::beginRenderPass(vk::CommandBuffer* cmdBuffer, Pipeline* pipeline, VkFramebuffer framebuffer, SwapChain* swapChain, uint32_t frameIndex) {
		cmdBuffer->beginRenderPass(pipeline->renderPass(), framebuffer, swapChain, _clearColor, 1.0, 0, VK_SUBPASS_CONTENTS_INLINE);
	}

	void RendererDelegate::endRenderPass(vk::CommandBuffer* cmdBuffer, uint32_t frameIndex) {
		cmdBuffer->endRenderPass();
	}

}
