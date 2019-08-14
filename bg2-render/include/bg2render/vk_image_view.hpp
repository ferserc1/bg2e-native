
#ifndef _bg2render_vk_image_view_hpp_
#define _bg2render_vk_image_view_hpp_

#include <bg2render/vk_instance.hpp>
#include <bg2render/vk_image.hpp>

namespace bg2render {
	namespace vk {

		class ImageView {
		public:
			ImageView(Instance*);
			ImageView(Device*);
			virtual ~ImageView();

			void create(vk::Image * image, VkFormat format);
			void create(VkImage image, VkFormat format);

			const VkImageView imageView() const { return _imageView; }

		protected:
			vk::Device* _device;

			VkImageView _imageView = VK_NULL_HANDLE;
		};
	}
}
#endif
