

#include <bg2wnd/application.hpp>
#include <bg2wnd/window.hpp>
#include <bg2wnd/window_delegate.hpp>
#include <bg2render/vk_instance.hpp>
#include <bg2render/vk_device.hpp>
#include <bg2render/swap_chain.hpp>
#include <bg2render/renderer.hpp>
#include <bg2math/vector.hpp>
#include <bg2math/utils.hpp>
#include <bg2db/buffer_load.hpp>
#include <bg2render/pipeline.hpp>
#include <bg2render/vertex_buffer.hpp>
#include <bg2render/index_buffer.hpp>
#include <bg2render/buffer_utils.hpp>
#include <bg2render/vk_descriptor_pool.hpp>
#include <bg2math/matrix.hpp>
#include <bg2base/image.hpp>
#include <bg2db/image_load.hpp>

#include <iostream>
#include <array>
#include <chrono>

class MyRendererDelegate : public bg2render::RendererDelegate {
public:
	struct Vertex {
		bg2math::float2 pos;
		bg2math::float3 color;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}
	};

	std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};

	VkVertexInputBindingDescription bindingDescription;
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;

	struct UniformBufferObject {
		bg2math::float4x4 model;
		bg2math::float4x4 view;
		bg2math::float4x4 proj;
	};

	virtual bg2render::Pipeline * configurePipeline(bg2render::vk::Instance* instance, bg2render::SwapChain* swapChain, const bg2math::int2& frameSize) {
		bg2render::Pipeline * pipeline = new bg2render::Pipeline(instance);

		bg2base::path shaderPath = "shaders";
		auto vshader = bg2db::loadBuffer(shaderPath.pathAddingComponent("basic.vert.spv"));
		auto fshader = bg2db::loadBuffer(shaderPath.pathAddingComponent("basic.frag.spv"));
		pipeline->addShader(vshader, VK_SHADER_STAGE_VERTEX_BIT, "main");
		pipeline->addShader(fshader, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

		// Pipeline layout
		bg2render::vk::PipelineLayout* pipelineLayout = new bg2render::vk::PipelineLayout(instance);
		pipelineLayout->addDescriptorSetLayoutBinding(
			0,	// binding
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,	// descriptorCount
			VK_SHADER_STAGE_VERTEX_BIT	// stage flags
		);
		pipelineLayout->create();
		pipeline->setPipelineLayout(pipelineLayout);

		bindingDescription = Vertex::getBindingDescription();
		attributeDescriptions = Vertex::getAttributeDescriptions();

		pipeline->vertexInputInfo().vertexBindingDescriptionCount = 1;
		pipeline->vertexInputInfo().pVertexBindingDescriptions = &bindingDescription;
		pipeline->vertexInputInfo().vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		pipeline->vertexInputInfo().pVertexAttributeDescriptions = attributeDescriptions.data();

		pipeline->inputAssemblyInfo().topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipeline->setViewport(frameSize);
		pipeline->rasterizationStateInfo().cullMode = VK_CULL_MODE_NONE;
		pipeline->rasterizationStateInfo().frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		pipeline->loadDefaultBlendAttachments();


		pipeline->createDefaultRenderPass(swapChain->format());

		return pipeline;
	}

	virtual void recordCommandBuffer(float delta, bg2render::vk::CommandBuffer* cmdBuffer, bg2render::Pipeline* pipeline, bg2render::SwapChain* swapChain, uint32_t frameIndex) {
		cmdBuffer->bindPipeline(pipeline);
		cmdBuffer->bindVertexBuffer(0, 1, _vertexBuffer);
		cmdBuffer->bindIndexBuffer(_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		cmdBuffer->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout(), 0, _descriptorSets[frameIndex]);

		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo = {};
		ubo.model = bg2math::float4x4::Rotation(time * bg2math::radians(90.0f), 0.0f, 0.0f, 1.0f);
		ubo.view = bg2math::float4x4::LookAt(bg2math::float3(2.0f, 2.0f, 2.0f), bg2math::float3(0.0f, 0.0f, 0.0f), bg2math::float3(0.0f, 0.0f, 1.0f));
		auto extent = renderer()->swapChain()->extent();
		auto ratio = static_cast<float>(extent.width) / static_cast<float>(extent.height);
		ubo.proj = bg2math::float4x4::Perspective(60.0f, ratio, 0.1f, 100.0f);
		ubo.proj.element(1, 1) *= -1.0;

		void* data;
		_uniformBuffersMemory[frameIndex]->map(0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		_uniformBuffersMemory[frameIndex]->unmap();

		cmdBuffer->drawIndexed(indices.size(), 1, 0, 0, 0);
	}

	virtual void initDone(bg2render::vk::Instance * instance, uint32_t simultaneousFrames) {
		_vertexBuffer = std::make_unique<bg2render::VertexBuffer>(instance);
		_vertexBuffer->create<Vertex>(vertices, renderer()->commandPool());

		_indexBuffer = std::make_unique<bg2render::IndexBuffer>(instance);
		_indexBuffer->create<uint16_t>(indices, renderer()->commandPool());

		// Uniform buffers
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);
		_uniformBuffers.resize(simultaneousFrames);
		_uniformBuffersMemory.resize(simultaneousFrames);

		for (size_t i = 0; i < simultaneousFrames; ++i) {
			bg2render::BufferUtils::CreateBufferMemory(
				instance,
				bufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				_uniformBuffers[i], _uniformBuffersMemory[i]);
		}
		
		// Descriptor sets
		_descriptorPool = std::make_shared<bg2render::vk::DescriptorPool>(instance);
		_descriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, simultaneousFrames);
		_descriptorPool->create(simultaneousFrames);
		_descriptorPool->allocateDescriptorSets(simultaneousFrames, renderer()->pipeline()->pipelineLayout(), _descriptorSets);

		for (size_t i = 0; i < simultaneousFrames; ++i) {
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = _uniformBuffers[i]->buffer();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = _descriptorSets[i]->descriptorSet();
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(instance->renderDevice()->device(), 1, &descriptorWrite, 0, nullptr);
		}

		// Image
		bg2base::path path = "data";
		_textureImage = std::shared_ptr<bg2base::image>(bg2db::loadImage(path.pathAddingComponent("texture.jpg")));

		std::unique_ptr<bg2render::vk::Buffer> stagingBuffer = std::make_unique<bg2render::vk::Buffer>(instance);
		std::unique_ptr<bg2render::vk::DeviceMemory> stagingBufferMemory = std::make_unique<bg2render::vk::DeviceMemory>(instance);
		VkDeviceSize size = _textureImage->size().x() * _textureImage->size().y() * _textureImage->bytesPerPixel();
		bg2render::BufferUtils::CreateBufferMemory(
			instance, size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* data;
		stagingBufferMemory->map(0, size, 0, &data);
		memcpy(data, _textureImage->data(), static_cast<size_t>(size));
		stagingBufferMemory->unmap();

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = _textureImage->size().x();
		imageInfo.extent.height = _textureImage->size().y();
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0;
		if (vkCreateImage(instance->renderDevice()->device(), &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create vulkan image");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(instance->renderDevice()->device(), textureImage, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = instance->renderPhysicalDevice()->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if (vkAllocateMemory(instance->renderDevice()->device(), &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate texture memory");
		}

		vkBindImageMemory(instance->renderDevice()->device(), textureImage, textureImageMemory, 0);

		// TODO: copy image

	}

	virtual void cleanup() {
		_vertexBuffer = nullptr;
		_indexBuffer = nullptr;
		_uniformBuffers.clear();
		_uniformBuffersMemory.clear();
		_descriptorSets.clear();
		_descriptorPool = nullptr;
	}

private:
	// Vertex buffers
	std::unique_ptr<bg2render::VertexBuffer> _vertexBuffer;
	std::unique_ptr<bg2render::IndexBuffer> _indexBuffer;

	// Uniform buffer
	std::vector<std::unique_ptr<bg2render::vk::Buffer>> _uniformBuffers;
	std::vector<std::unique_ptr<bg2render::vk::DeviceMemory>> _uniformBuffersMemory;

	// Descriptors
	std::shared_ptr<bg2render::vk::DescriptorPool> _descriptorPool;
	std::vector<std::shared_ptr<bg2render::vk::DescriptorSet>> _descriptorSets;

	// Texture resources
	std::shared_ptr<bg2base::image> _textureImage;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	//std::unique_ptr<bg2render::vk::DeviceMemory> _textureMemory;
};

class MyWindowDelegate : public bg2wnd::WindowDelegate {
public:
	MyWindowDelegate(bool enableValidation) :_enableValidationLayers(enableValidation) {}

	void init() {
		_instance = std::shared_ptr<bg2render::vk::Instance>(bg2render::vk::Instance::CreateDefault (window(),"bg2 vulkan test"));

		_renderer = std::make_unique<bg2render::Renderer>(_instance.get());
		_renderer->setDelegate(new MyRendererDelegate());
		_renderer->init(window()->size());
    }

    void resize(const bg2math::int2 & size) {
		_renderer->resize(size);
    }

    void update(float delta) {
		_renderer->update(delta);
    }

    void draw() {
		_renderer->draw();
    }

    void cleanup() {
		_renderer = nullptr;
		_instance = nullptr;
    }

    void keyUp(const bg2wnd::KeyboardEvent & e) {}
    void keyDown(const bg2wnd::KeyboardEvent & e) {}
    void mouseMove(const bg2wnd::MouseEvent & e) {}
    void mouseDown(const bg2wnd::MouseEvent & e) {}
    void mouseUp(const bg2wnd::MouseEvent & e) {}
    void mouseWheel(const bg2wnd::MouseEvent & e) {}

private:
	std::shared_ptr<bg2render::vk::Instance> _instance;
	std::unique_ptr<bg2render::Renderer> _renderer;

	bool _enableValidationLayers;
};

int main(int argc, char ** argv) {
    bg2wnd::Application app;

    auto window = bg2wnd::Window::Create();
    window->setWindowDelegate(new MyWindowDelegate(true));
    app.addWindow(window);
    app.run();

    return 0;
}
