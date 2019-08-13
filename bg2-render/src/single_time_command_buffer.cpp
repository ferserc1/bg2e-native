
#include <bg2render/single_time_command_buffer.hpp>

namespace bg2render {

	SingleTimeCommandBuffer::SingleTimeCommandBuffer(vk::Instance* instance, VkCommandPool pool)
		:_instance(instance)
		,_pool(pool)
	{

	}

	void SingleTimeCommandBuffer::begin() {
		_commandBuffer = std::unique_ptr<vk::CommandBuffer>(vk::CommandBuffer::Allocate(
			_instance->renderDevice(),
			_pool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY
		));

		_commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	}

	void SingleTimeCommandBuffer::end() {
		_commandBuffer->end();

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		VkCommandBuffer commandBuffers[] = { _commandBuffer->commandBuffer() };
		submitInfo.pCommandBuffers = commandBuffers;

		vkQueueSubmit(_instance->renderQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(_instance->renderQueue());

		vk::CommandBuffer::Free(_commandBuffer.get());
	}

	void SingleTimeCommandBuffer::execute(CommandBufferClosure closure) {
		begin();
		closure(_commandBuffer.get());
		end();
	}
}