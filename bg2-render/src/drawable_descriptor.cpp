
#include <bg2render/drawable_descriptor.hpp>

#include <bg2render/drawable_item.hpp>

namespace bg2render {

    std::map<std::string, std::unique_ptr<DrawableDescriptor>> DrawableDescriptor::s_drawableDescriptors;

	bg2render::vk::PipelineLayout* DrawableDescriptor::createPipelineLayout(vk::Instance* instance) {
		auto result = new bg2render::vk::PipelineLayout(instance);
		configureLayout(result);
		result->create();
		return result;
	}

	Descriptor* DrawableDescriptor::createDescriptorPool(vk::Instance* instance, vk::PipelineLayout* pipelineLayout, uint32_t poolSize) {
		Descriptor* descriptor = new Descriptor();
		descriptor->descriptorPool = std::make_shared<bg2render::vk::DescriptorPool>(instance);
		configureDescriptorPool(descriptor->descriptorPool.get(), poolSize);
		descriptor->descriptorPool->create(poolSize);
		descriptor->descriptorPool->allocateDescriptorSets(poolSize, pipelineLayout, descriptor->descriptorSets);
		return descriptor;
	}

}
