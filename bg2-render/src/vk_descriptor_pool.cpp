
#include <bg2render/vk_descriptor_pool.hpp>

#include <stdexcept>

namespace bg2render {
	namespace vk {

		DescriptorPool::DescriptorPool(Instance* instance)
			:_instance(instance)
		{

		}

		DescriptorPool::~DescriptorPool() {
			if (_descriptorPool != VK_NULL_HANDLE) {
				vkDestroyDescriptorPool(_instance->renderDevice()->device(), _descriptorPool, nullptr);
			}
		}

		void DescriptorPool::create(uint32_t maxSets) {
			VkDescriptorPoolCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			createInfo.poolSizeCount = static_cast<uint32_t>(_poolSizes.size());
			createInfo.pPoolSizes = _poolSizes.data();
			createInfo.maxSets = maxSets;
			if (vkCreateDescriptorPool(_instance->renderDevice()->device(), &createInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create descriptor pool.");
			}
		}

		void DescriptorPool::allocateDescriptorSets(uint32_t count, PipelineLayout* plLayout, std::vector<std::shared_ptr<DescriptorSet>>& dsl) {
			std::vector<VkDescriptorSetLayout> layouts(count, plLayout->descriptorSetLayout());
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = _descriptorPool;
			allocInfo.descriptorSetCount = count;
			allocInfo.pSetLayouts = layouts.data();

			std::vector<VkDescriptorSet> descriptorSets(count);
			if (vkAllocateDescriptorSets(_instance->renderDevice()->device(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
				throw std::runtime_error("Failed to allocate descriptor sets");
			}

			for (auto ds : descriptorSets) {
				dsl.push_back(std::make_shared<DescriptorSet>(ds));
			}
		}
	}
}
