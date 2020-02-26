
#ifndef _bg2render_vertex_buffer_hpp_
#define _bg2render_vertex_buffer_hpp_

#include <bg2render/vk_buffer.hpp>
#include <bg2render/vk_device_memory.hpp>
#include <bg2render/buffer_utils.hpp>

#include <vector>

namespace bg2render {

	class VertexBuffer {
	public:
		VertexBuffer(vk::Instance* inst)
			:_instance(inst)
		{}

		// Create vertex buffer in shared memory zone
		template <class VertexT>
		inline void create(const std::vector<VertexT>& vertexData) {
			vk::Buffer* buffer;
			vk::DeviceMemory* memory;
			BufferUtils::CreateBufferMemory(
				sizeof(vertexData[0]) * vertexData.size(),
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				buffer, memory);

			void* data;
			_memory->map(0, _buffer->size(), 0, &data);
			memcpy(data, vertexData.data(), static_cast<size_t>(_buffer->size()));
			_memory->unmap();
		}

		// Create vertex buffer in GPU memory zone. This function needs a commandPool to perform
		// the upload operation from the CPU to the GPU memory
		template <class VertexT>
		inline void create(const std::vector<VertexT> & vertexData, VkCommandPool commandPool) {
			VkDeviceSize bufferSize = sizeof(vertexData[0]) * vertexData.size();

			// Staging buffer
			std::unique_ptr<vk::Buffer> stagingBuffer;
			std::unique_ptr<vk::DeviceMemory> stagingBufferMemory;
			BufferUtils::CreateBufferMemory(
				_instance,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingBufferMemory);
			
			// Copy data to staging buffer
			void* data;
			stagingBufferMemory->map(0, stagingBuffer->size(), 0, &data);
			memcpy(data, vertexData.data(), static_cast<size_t>(stagingBuffer->size()));
			stagingBufferMemory->unmap();

			// GPU buffers
			BufferUtils::CreateBufferMemory(
				_instance,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				_buffer, _memory
			);
			
			// Copy data from staging buffer to GPU buffer
			BufferUtils::CopyBuffer(_instance, commandPool, stagingBuffer.get(), _buffer.get());
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
