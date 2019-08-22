
#include <bg2render/pbr_drawable_descriptor.hpp>
#include <bg2render/drawable_item.hpp>

namespace bg2render {

	void PBRDrawableDescriptor::configureLayout(vk::PipelineLayout* pipelineLayout) {
		pipelineLayout->addDescriptorSetLayoutBinding(
			0, // binding
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1, // descriptor count
			VK_SHADER_STAGE_VERTEX_BIT
		);

		pipelineLayout->addDescriptorSetLayoutBinding(
			1,	// binding
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,	// descriptor count
			VK_SHADER_STAGE_FRAGMENT_BIT
		);
	}

	void PBRDrawableDescriptor::configureDescriptorPool(vk::DescriptorPool* descriptorPool, uint32_t poolSize) {
		descriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, poolSize);
		descriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, poolSize);
	}

	void PBRDrawableDescriptor::updateDescriptorWrites(vk::Instance* instance, uint32_t frameIndex, DrawableItem* drawableItem) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = drawableItem->uniformBuffer(frameIndex)->buffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(bg2render::DrawableItem::Transform);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = drawableItem->material()->diffuse()->vkImageView();
		imageInfo.sampler = drawableItem->material()->diffuse()->vkSampler();

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = drawableItem->descriptor()->descriptorSets[frameIndex]->descriptorSet();
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = drawableItem->descriptor()->descriptorSets[frameIndex]->descriptorSet();
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(instance->renderDevice()->device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	DrawableDescriptorRegistry<PBRDrawableDescriptor> pbrDrawableDescriptorRegistar("pbrDescriptor");
}
