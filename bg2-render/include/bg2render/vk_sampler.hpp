
#ifndef _bg2render_vk_sampler_hpp_
#define _bg2render_vk_sampler_hpp_

#include <bg2render/vk_instance.hpp>


namespace bg2render {
	namespace vk {
		
		class Sampler {
		public:

			Sampler(Instance*);
			virtual ~Sampler();

			inline VkSamplerCreateInfo& createInfo() { return _createInfo; }

			void create();

			inline VkSampler sampler() const { return _sampler; }

		protected:
			Instance* _instance;

			VkSampler _sampler = VK_NULL_HANDLE;

			VkSamplerCreateInfo _createInfo = {
				VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				nullptr,
				0,
				VK_FILTER_LINEAR,
				VK_FILTER_LINEAR,
				VK_SAMPLER_MIPMAP_MODE_LINEAR,
				VK_SAMPLER_ADDRESS_MODE_REPEAT,
				VK_SAMPLER_ADDRESS_MODE_REPEAT,
				VK_SAMPLER_ADDRESS_MODE_REPEAT,
				0.0f,
				VK_TRUE,
				16,
				VK_FALSE,
				VK_COMPARE_OP_ALWAYS,
				0.0f,
				0.0f,
				VK_BORDER_COLOR_INT_OPAQUE_BLACK,
				VK_FALSE
			};

		};
	}
}

#endif
