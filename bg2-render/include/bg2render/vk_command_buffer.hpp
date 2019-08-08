
#ifndef _bg2render_vk_command_buffer_hpp_
#define _bg2render_vk_command_buffer_hpp_

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include <bg2render/vk_device.hpp>
#include <bg2render/pipeline.hpp>
#include <bg2render/render_pass.hpp>
#include <bg2render/swap_chain.hpp>
#include <bg2math/vector.hpp>

namespace bg2render {
    namespace vk {
        
		class CommandBuffer {
		public:
			static void Allocate(Device* dev, VkCommandPool pool, VkCommandBufferLevel level, size_t count, std::vector<std::shared_ptr<CommandBuffer>>& result) {
				Allocate(dev, pool, level, static_cast<uint32_t>(count), result);
			}
			static void Allocate(Device *, VkCommandPool pool, VkCommandBufferLevel level, uint32_t count, std::vector<std::shared_ptr<CommandBuffer>> & result);

			static void Free(std::vector<std::shared_ptr<CommandBuffer>>& cmdBuffers);

			inline VkCommandBuffer commandBuffer() const { return _commandBuffer; }

			CommandBuffer(VkDevice,VkCommandPool,VkCommandBuffer);

			// Command buffer wrapper functions
			void beginRenderPass(RenderPass * rp, VkFramebuffer fb, SwapChain * swapChain, const bg2math::color & clearColor, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE);
			void endRenderPass();

			void bindPipeline(Pipeline * pipeline, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
			void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);

		private:
			VkDevice _device;
			VkCommandPool _pool;
			VkCommandBuffer _commandBuffer;
		};

    }
}

#endif
