
#ifndef _bg2render_buffer_utils_hpp_
#define _bg2render_buffer_utils_hpp_

#include <bg2render/vk_instance.hpp>
#include <bg2render/vk_buffer.hpp>
#include <bg2render/vk_device_memory.hpp>
#include <bg2render/vk_image.hpp>
#include <bg2render/vk_image_view.hpp>
#include <bg2base/image.hpp>

namespace bg2render {

    class BufferUtils {
	public:
		static void CopyBuffer(vk::Instance * instance, VkCommandPool pool, vk::Buffer * srcBuffer, vk::Buffer * dstBuffer);

		static void CopyBufferToImage(vk::Instance * instance, VkCommandPool pool, vk::Buffer * srcBuffer, vk::Image* image, uint32_t width, uint32_t height);

		static void CreateBufferMemory(vk::Instance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, vk::Buffer *& buffer, vk::DeviceMemory *& memory);

		// Managed memory buffer and device memory creation
		static void CreateBufferMemory(vk::Instance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::shared_ptr<vk::Buffer>& buffer, std::shared_ptr<vk::DeviceMemory>& memory);
		static void CreateBufferMemory(vk::Instance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::unique_ptr<vk::Buffer>& buffer, std::unique_ptr<vk::DeviceMemory>& memory);

		static void CreateImageMemory(vk::Instance* instance, VkCommandPool pool, bg2base::image* imageData, vk::Image *& image, vk::DeviceMemory *& memory, vk::ImageView *& imageView, VkPipelineStageFlags shaderStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		// Managed memory image and device memory creation
		static void CreateImageMemory(vk::Instance* instance, VkCommandPool pool, bg2base::image* imageData, std::shared_ptr<vk::Image>& image, std::shared_ptr<vk::DeviceMemory>& memory, std::shared_ptr<vk::ImageView>& imageView, VkPipelineStageFlags shaderStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		static void CreateImageMemory(vk::Instance* instance, VkCommandPool pool, bg2base::image* imageData, std::unique_ptr<vk::Image>& image, std::unique_ptr<vk::DeviceMemory>& memory, std::unique_ptr<vk::ImageView>& imageView, VkPipelineStageFlags shaderStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    };

}

#endif
