#ifndef _bg2render_descriptor_set_hpp_
#define _bg2render_descriptor_set_hpp_

#include <bg2render/vk_instance.hpp>

namespace bg2render {
    namespace vk {
		
		class DescriptorSet {
		public:
			DescriptorSet(VkDescriptorSet);

			inline VkDescriptorSet descriptorSet() const { return _descriptorSet; }

		protected:
			VkDescriptorSet _descriptorSet;
		};

    }
}

#endif
