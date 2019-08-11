
#include <bg2render/buffer_utils.hpp>
#include <bg2render/vk_command_buffer.hpp>

namespace bg2render {

	void BufferUtils::CopyBuffer(vk::Instance* instance, VkCommandPool pool, vk::Buffer* srcBuffer, vk::Buffer* dstBuffer) {
		std::unique_ptr<vk::CommandBuffer> commandBuffer = std::unique_ptr<vk::CommandBuffer>(
			vk::CommandBuffer::Allocate(instance->renderDevice(), pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

		commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		commandBuffer->copyBuffer(srcBuffer, dstBuffer);
		commandBuffer->end();

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		auto vkCmdBuffer = commandBuffer->commandBuffer();
		submitInfo.pCommandBuffers = &vkCmdBuffer;
		vkQueueSubmit(instance->renderQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(instance->renderQueue());

		vk::CommandBuffer::Free(commandBuffer.get());
	}
}
