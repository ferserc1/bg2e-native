
#include <bg2render/renderer_delegate.hpp>

namespace bg2render {

	void RendererDelegate::beginRenderPass(vk::CommandBuffer* cmdBuffer, Pipeline* pipeline, VkFramebuffer framebuffer, SwapChain* swapChain) {
		cmdBuffer->beginRenderPass(pipeline->renderPass(), framebuffer, swapChain, _clearColor, VK_SUBPASS_CONTENTS_INLINE);
	}

	void RendererDelegate::endRenderPass(vk::CommandBuffer* cmdBuffer) {
		cmdBuffer->endRenderPass();
	}

}
