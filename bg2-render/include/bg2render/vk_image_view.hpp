
#ifndef _bg2render_vk_image_view_hpp_
#define _bg2render_vk_image_view_hpp_

#include <bg2render/vk_instance.hpp>

namespace bg2render {
	namespace vk {

		class ImageView {
		public:
			ImageView(Instance*);
			virtual ~ImageView();

			// TODO: Add a create function that uses vk::Image wrapper class
			void create(VkImage image, VkFormat format);

			const VkImageView imageView() const { return _imageView; }

		protected:
			vk::Instance* _instance;

			VkImageView _imageView = VK_NULL_HANDLE;
		};
	}
}
#endif
