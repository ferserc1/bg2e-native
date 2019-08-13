
#ifndef _bg2render_vk_descriptor_pool_hpp_
#define _bg2render_vk_descriptor_pool_hpp_

#include <bg2render/vk_instance.hpp>
#include <bg2render/vk_pipeline_layout.hpp>
#include <bg2render/vk_descriptor_set.hpp>

#include <vector>

namespace bg2render {
	namespace vk {

		class DescriptorPool {
		public:
			DescriptorPool(Instance* instance);
			virtual ~DescriptorPool();

			inline void addPoolSize(VkDescriptorType type, uint32_t count) {
				_poolSizes.push_back({ type, count });
			}

			void create(uint32_t maxSets);

			void allocateDescriptorSets(uint32_t count, PipelineLayout * plLayout, std::vector<std::shared_ptr<DescriptorSet>> & dsl);

			inline const VkDescriptorPool descriptorPool() const { return _descriptorPool; }

		protected:
			Instance* _instance;

			VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;

			std::vector<VkDescriptorPoolSize> _poolSizes;
		};
	}
}

#endif
