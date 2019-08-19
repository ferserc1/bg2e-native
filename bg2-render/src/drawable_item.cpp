
#include <bg2render/drawable_item.hpp>

#include <bg2render/drawable_descriptor.hpp>

// TODO: Remove this
#include <chrono>
namespace bg2render {

	DrawableItem::DrawableItem(const std::string& descriptorType, bg2render::Renderer* rend)
		:_descriptorType(descriptorType)
		, _renderer(rend)
	{
	}
	DrawableItem::~DrawableItem() {
		_polyList = nullptr;
		_material = nullptr;
		_uniformBuffers.clear();
		_uniformBuffersMemory.clear();
	}

	void DrawableItem::create(PolyList* pl, Material* mat) {
		_polyList = std::shared_ptr<PolyList>(pl);
		_material = std::shared_ptr<Material>(mat);

		VkDeviceSize uboSize = sizeof(Transform);
		_uniformBuffers.resize(_renderer->simultaneousFrames());
		_uniformBuffersMemory.resize(_renderer->simultaneousFrames());
		for (uint32_t i = 0; i < _renderer->simultaneousFrames(); ++i) {
			bg2render::BufferUtils::CreateBufferMemory(
				_renderer->instance(),
				uboSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				_uniformBuffers[i], _uniformBuffersMemory[i]
			);
		}

		auto drawableDescriptor = bg2render::DrawableDescriptor::Get(_descriptorType);
		_pipelineLayout = std::shared_ptr<bg2render::vk::PipelineLayout>(drawableDescriptor->createPipelineLayout(_renderer->instance()));
		_descriptor = std::shared_ptr<bg2render::Descriptor>(drawableDescriptor->createDescriptorPool(_renderer->instance(), _pipelineLayout.get(), _renderer->simultaneousFrames()));
	}

	void DrawableItem::update(uint32_t frameIndex) {
		auto drawableDescriptor = bg2render::DrawableDescriptor::Get(_descriptorType);
		drawableDescriptor->updateDescriptorWrites(_renderer->instance(), frameIndex, this);

		void* data;
		_uniformBuffersMemory[frameIndex]->map(0, sizeof(_transform), 0, &data);
		memcpy(data, &_transform, sizeof(_transform));
		_uniformBuffersMemory[frameIndex]->unmap();
	}

	void DrawableItem::draw(bg2render::vk::CommandBuffer* commandBuffer, bg2render::Pipeline* pipeline, uint32_t frameIndex) {
		_polyList->bindBuffers(commandBuffer);
		commandBuffer->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout(), 0, _descriptor->descriptorSets[frameIndex]);
		_polyList->draw(commandBuffer);
	}
}

