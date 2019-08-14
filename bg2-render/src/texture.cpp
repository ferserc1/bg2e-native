
#include <bg2render/texture.hpp>

#include <bg2render/buffer_utils.hpp>

namespace bg2render {
	Texture::Texture(vk::Instance* inst)
		:_instance(inst)
	{
		_image = std::make_shared<vk::Image>(_instance);
		_deviceMemory = std::make_shared<vk::DeviceMemory>(_instance);
		_imageView = std::make_shared<vk::ImageView>(_instance);
		_sampler = std::make_shared<vk::Sampler>(_instance);
	}

	Texture::~Texture() {
		_sampler = nullptr;
		_imageView = nullptr;
		_deviceMemory = nullptr;
		_image = nullptr;
	}

	void Texture::create(bg2base::image* imageData, VkCommandPool cmdPool, VkPipelineStageFlags shaderUsage) {
		BufferUtils::CreateImageMemory(
			_instance, cmdPool,
			imageData,
			_image, _deviceMemory, _imageView,
			shaderUsage
		);

		_sampler->create();
	}
}
