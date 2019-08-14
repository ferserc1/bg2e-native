
#ifndef _bg2render_vk_command_buffer_hpp_
#define _bg2render_vk_command_buffer_hpp_

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include <bg2render/vk_device.hpp>
#include <bg2render/pipeline.hpp>
#include <bg2render/render_pass.hpp>
#include <bg2render/swap_chain.hpp>
#include <bg2render/vertex_buffer.hpp>
#include <bg2render/index_buffer.hpp>
#include <bg2render/vk_descriptor_set.hpp>
#include <bg2math/vector.hpp>

namespace bg2render {
    namespace vk {
        
		class CommandBuffer {
		public:
			static void Allocate(Device* dev, VkCommandPool pool, VkCommandBufferLevel level, size_t count, std::vector<std::shared_ptr<CommandBuffer>>& result) {
				Allocate(dev, pool, level, static_cast<uint32_t>(count), result);
			}
			static void Allocate(Device *, VkCommandPool pool, VkCommandBufferLevel level, uint32_t count, std::vector<std::shared_ptr<CommandBuffer>> & result);
			static CommandBuffer * Allocate(Device*, VkCommandPool pool, VkCommandBufferLevel level);
			static void Free(std::vector<std::shared_ptr<CommandBuffer>>& cmdBuffers);
			static void Free(CommandBuffer* cmdBuffer);

			inline VkCommandBuffer commandBuffer() const { return _commandBuffer; }

			CommandBuffer(VkDevice,VkCommandPool,VkCommandBuffer);

			// Command buffer wrapper functions
			void begin(VkCommandBufferUsageFlags usage);
			void end();

			void beginRenderPass(RenderPass * rp, VkFramebuffer fb, SwapChain * swapChain, const bg2math::color & clearColor, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE);
			void beginRenderPass(RenderPass* rp, VkFramebuffer fb, SwapChain* swapChain, const bg2math::color& clearColor, float depthClearColor, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE);
			void beginRenderPass(RenderPass* rp, VkFramebuffer fb, SwapChain* swapChain, const bg2math::color& clearColor, float depthClearColor, uint32_t stencilClearColor, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE);
			void endRenderPass();

			void bindPipeline(Pipeline * pipeline, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
			inline void bindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount, const std::shared_ptr<VertexBuffer> & buffer, uint32_t offset = 0) {
				bindVertexBuffer(firstBinding, bindingCount, buffer.get(), offset);
			}
			inline void bindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount, const std::unique_ptr<VertexBuffer> & buffer, uint32_t offset = 0) {
				bindVertexBuffer(firstBinding, bindingCount, buffer.get(), offset);
			}
			void bindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount, VertexBuffer* buffer, uint32_t offset = 0);
			void bindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount,vk::Buffer * buffer, uint32_t offset = 0);
			void bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const std::vector<vk::Buffer*>& buffers, const std::vector<VkDeviceSize>& offsets);
			inline void bindIndexBuffer(const std::shared_ptr<IndexBuffer>& buffer, VkDeviceSize offset, VkIndexType indexType) {
				bindIndexBuffer(buffer.get(), offset, indexType);
			}
			inline void bindIndexBuffer(const std::unique_ptr<IndexBuffer>& buffer, VkDeviceSize offset, VkIndexType indexType) {
				bindIndexBuffer(buffer.get(), offset, indexType);
			}
			void bindIndexBuffer(IndexBuffer* buffer, VkDeviceSize offset, VkIndexType indexType);
			void bindIndexBuffer(vk::Buffer* buffer, VkDeviceSize offset, VkIndexType indexType);

			inline void draw(size_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
				draw(static_cast<uint32_t>(vertexCount), instanceCount, firstVertex, firstInstance);
			}
			void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
			inline void drawIndexed(size_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
				drawIndexed(static_cast<uint32_t>(indexCount), instanceCount, firstIndex, vertexOffset, firstInstance);
			}
			void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

			void copyBuffer(vk::Buffer* src, vk::Buffer* dst, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

			inline void bindDescriptorSet(VkPipelineBindPoint bindPoint, PipelineLayout* pl, uint32_t firstSet, const std::shared_ptr<DescriptorSet>& ds) {
				bindDescriptorSet(bindPoint, pl, firstSet, ds.get());
			}
			inline void bindDescriptorSet(VkPipelineBindPoint bindPoint, PipelineLayout* pl, uint32_t firstSet, const std::unique_ptr<DescriptorSet>& ds) {
				bindDescriptorSet(bindPoint, pl, firstSet, ds.get());
			}
			void bindDescriptorSet(VkPipelineBindPoint bindPoint, PipelineLayout * pl, uint32_t firstSet, DescriptorSet * ds);

			void pipelineBarrier(
				VkPipelineStageFlags srcStageMask,
				VkPipelineStageFlags dstStageMask, 
				VkDependencyFlags dependencyFlags,
				uint32_t memoryBarrierCount,
				const VkMemoryBarrier * pMemoryBarriers,
				uint32_t bufferMemoryBarrierCount,
				const VkBufferMemoryBarrier * pBufferMemoryBarriers,
				uint32_t imageMemoryBarrierCount,
				const VkImageMemoryBarrier* pImageMemoryBarriers);
			
		private:
			VkDevice _device;
			VkCommandPool _pool;
			VkCommandBuffer _commandBuffer;
		};

    }
}

#endif
