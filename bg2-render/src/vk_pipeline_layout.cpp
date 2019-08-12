
#include <bg2render/vk_pipeline_layout.hpp>

#include <stdexcept>

namespace bg2render {
    namespace vk {


		PipelineLayout::PipelineLayout(vk::Instance* inst)
			:_instance(inst)
		{

		}

		PipelineLayout::~PipelineLayout() {
			if (_descriptorSetLayout != VK_NULL_HANDLE) {
				vkDestroyDescriptorSetLayout(_instance->renderDevice()->device(), _descriptorSetLayout, nullptr);
			}

			if (_pipelineLayout != VK_NULL_HANDLE) {
				vkDestroyPipelineLayout(_instance->renderDevice()->device(), _pipelineLayout, nullptr);
			}
		}

		void PipelineLayout::create() {
			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			if (_layoutBindings.size() > 0) {
				layoutInfo.bindingCount = static_cast<uint32_t>(_layoutBindings.size());
				layoutInfo.pBindings = _layoutBindings.data();
			}
			if (vkCreateDescriptorSetLayout(_instance->renderDevice()->device(), &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create descriptor set layout");
			}

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges = nullptr;
			if (vkCreatePipelineLayout(_instance->renderDevice()->device(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create pipeline layout");
			}
		}

    }
}
