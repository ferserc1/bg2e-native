
#ifndef _bg2render_texture_hpp_
#define _bg2render_texture_hpp_

#include <bg2base/image.hpp>

#include <bg2render/vk_image.hpp>
#include <bg2render/vk_device_memory.hpp>
#include <bg2render/vk_image_view.hpp>
#include <bg2render/vk_sampler.hpp>

namespace bg2render {

    class Texture {
	public:
		Texture(vk::Instance*);
		virtual ~Texture();

		void create(bg2base::image* imageData, VkCommandPool cmdPool, VkPipelineStageFlags shaderUsage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		inline VkSamplerCreateInfo& samplerCreateInfo() { return _sampler->createInfo(); }

		inline const vk::Image* image() const { return _image.get(); }
		inline const vk::DeviceMemory* deviceMemory() const { return _deviceMemory.get(); }
		inline const vk::ImageView* imageView() const { return _imageView.get(); }
		inline const vk::Sampler* sampler() const { return _sampler.get(); }

		// Direct access to vulkan handlers
		inline const VkImage vkImage() const { return _image->image(); }
		inline const VkDeviceMemory vkDeviceMemory() const { return _deviceMemory->deviceMemory(); }
		inline const VkImageView vkImageView() const { return _imageView->imageView(); }
		inline const VkSampler vkSampler() const { return _sampler->sampler(); }

	protected:
		vk::Instance* _instance;

		std::shared_ptr<vk::Image> _image;
		std::shared_ptr<vk::DeviceMemory> _deviceMemory;
		std::shared_ptr<vk::ImageView> _imageView;
		std::shared_ptr<vk::Sampler> _sampler;
    };

}

#endif
