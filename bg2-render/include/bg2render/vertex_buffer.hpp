
#ifndef _bg2render_vertex_buffer_hpp_
#define _bg2render_vertex_buffer_hpp_

#include <bg2render/vk_buffer.hpp>
#include <bg2render/vk_device_memory.hpp>

#include <vector>

namespace bg2render {

	class VertexBuffer {
	public:
		VertexBuffer(vk::Instance* inst)
			:_instance(inst)
		{}

		template <class VertexT>
		inline void create(const std::vector<VertexT> & vertexData) {
			_buffer = std::make_shared<vk::Buffer>(_instance);
			_buffer->create(
				sizeof(vertexData[0]) * vertexData.size(),
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_SHARING_MODE_EXCLUSIVE
			);

			_memory = std::make_shared<vk::DeviceMemory>(_instance);
			_memory->allocate(
				_buffer->memoryRequirements(),
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);

			vkBindBufferMemory(_instance->renderDevice()->device(), _buffer->buffer(), _memory->deviceMemory(), 0);

			void* data;
			_memory->map(0, _buffer->size(), 0, &data);
			memcpy(data, vertexData.data(), static_cast<size_t>(_buffer->size()));
			_memory->unmap();
		}

		inline vk::Buffer* buffer() { return _buffer.get(); }
		inline const vk::Buffer* buffer() const { return _buffer.get(); }

		inline vk::DeviceMemory* memory() { return _memory.get(); }
		inline const vk::DeviceMemory* memory() const { return _memory.get(); }

	protected:
		vk::Instance* _instance;

		std::shared_ptr<vk::Buffer> _buffer;
		std::shared_ptr<vk::DeviceMemory> _memory;
	};

}

#endif
