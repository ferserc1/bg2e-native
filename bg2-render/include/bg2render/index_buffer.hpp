
#ifndef _bg2render_index_buffer_hpp_
#define _bg2render_index_buffer_hpp_

#include <bg2render/vk_buffer.hpp>
#include <bg2render/vk_device_memory.hpp>
#include <bg2render/buffer_utils.hpp>

#include <vector>

namespace bg2render {

	class IndexBuffer {
	public:
		IndexBuffer(vk::Instance* inst)
			:_instance(inst)
		{}

		// Create index buffer in shared memory zone
		template <class IndexT>
		inline void create(const std::vector<IndexT>& indexData) {
			vk::Buffer* buffer;
			vk::DeviceMemory* memory;
			BufferUtils::CreateBufferMemory(
				sizeof(indexData[0]) * indexData.size(),
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				_buffer, memory_);

			void* data;
			_memory->map(0, _buffer->size(), 0, &data);
			memcpy(data, indexData.data(), static_cast<size_t>(_buffer->size()));
			_memory->unmap();
		}

		// Create index buffer in GPU memory zone. This function needs a commandPool to perform
		// the upload operation from the CPU to the GPU memory
		template <class IndexT>
		inline void create(const std::vector<IndexT>& indexData, VkCommandPool commandPool) {
			VkDeviceSize bufferSize = sizeof(indexData[0]) * indexData.size();
			
			std::unique_ptr<vk::Buffer> stagingBuffer;
			std::unique_ptr<vk::DeviceMemory> stagingBufferMemory;
			BufferUtils::CreateBufferMemory(
				_instance,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingBufferMemory
			);

			void* data;
			stagingBufferMemory->map(0, stagingBuffer->size(), 0, &data);
			memcpy(data, indexData.data(), static_cast<size_t>(stagingBuffer->size()));
			stagingBufferMemory->unmap();


			BufferUtils::CreateBufferMemory(
				_instance,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				_buffer, _memory
			);

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