
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

		VkDeviceSize uboSize = sizeof(UniformBufferObject);
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

		// TODO: this is a testing code
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();


		UniformBufferObject ubo = {};
		ubo.model = bg2math::float4x4::Rotation(time * bg2math::radians(90.0f), 0.0f, 0.0f, 1.0f);
		ubo.view = bg2math::float4x4::LookAt(bg2math::float3(2.0f, 2.0f, 2.0f), bg2math::float3(0.0f, 0.0f, 0.0f), bg2math::float3(0.0f, 0.0f, 1.0f));
		auto extent = _renderer->swapChain()->extent();
		auto ratio = static_cast<float>(extent.width) / static_cast<float>(extent.height);
		ubo.proj = bg2math::float4x4::Perspective(60.0f, ratio, 0.1f, 100.0f);
		ubo.proj.element(1, 1) *= -1.0;


		void* data;
		_uniformBuffersMemory[frameIndex]->map(0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		_uniformBuffersMemory[frameIndex]->unmap();
	}

	void DrawableItem::draw(bg2render::vk::CommandBuffer* commandBuffer, bg2render::Pipeline* pipeline, uint32_t frameIndex) {
		_polyList->bindBuffers(commandBuffer);
		commandBuffer->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout(), 0, _descriptor->descriptorSets[frameIndex]);
		_polyList->draw(commandBuffer);
	}
}

