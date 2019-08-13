
#ifndef _bg2render_single_time_command_buffer_hpp_
#define _bg2render_single_time_command_buffer_hpp_

#include <bg2render/vk_instance.hpp>
#include <bg2render/vk_command_buffer.hpp>

#include <functional>

namespace bg2render {
	
	class SingleTimeCommandBuffer {
	public:
		typedef std::function<void (vk::CommandBuffer*)> CommandBufferClosure;

		SingleTimeCommandBuffer(vk::Instance* instance, VkCommandPool pool);

		void begin();
		void end();

		void execute(CommandBufferClosure closure);

		inline vk::CommandBuffer* commandBuffer() { return _commandBuffer.get(); }

	protected:
		vk::Instance* _instance;
		VkCommandPool _pool;

		std::unique_ptr<vk::CommandBuffer> _commandBuffer;
	};
}
#endif
