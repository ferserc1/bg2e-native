#ifndef _bg2render_vk_pipeline_layout_hpp_
#define _bg2render_vk_pipeline_layout_hpp_

#include <bg2render/vk_instance.hpp>

#include <vector>

namespace bg2render {
    namespace vk {

		class PipelineLayout {
		public:
			PipelineLayout(vk::Instance* inst);
			virtual ~PipelineLayout();

			inline void addDescriptorSetLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags flags = 0, const VkSampler * pImmutableSamplers = nullptr) {
				_layoutBindings.push_back({
					binding,
					descriptorType,
					descriptorCount,
					flags,
					pImmutableSamplers
				});
			}
			void create();

			inline VkPipelineLayout pipelineLayout() const { return _pipelineLayout; }
			inline VkDescriptorSetLayout descriptorSetLayout() const { return _descriptorSetLayout; }

		protected:
			vk::Instance* _instance;

			VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
			VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;

			std::vector<VkDescriptorSetLayoutBinding> _layoutBindings;
		};
    }
}

#endif
