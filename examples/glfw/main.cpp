

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
#include <bg2render/single_time_command_buffer.hpp>
#include <bg2render/vk_image_view.hpp>
#include <bg2render/vk_sampler.hpp>
#include <bg2render/vk_image.hpp>
#include <bg2render/texture.hpp>
#include <bg2math/matrix.hpp>
#include <bg2base/image.hpp>
#include <bg2db/image_load.hpp>

#include <iostream>
#include <array>
#include <chrono>

namespace bg2render {

	struct Descriptor {
		std::shared_ptr<bg2render::vk::DescriptorPool> descriptorPool;
		std::vector<std::shared_ptr<bg2render::vk::DescriptorSet>> descriptorSets;

		inline void operator =(const Descriptor& other) {
			descriptorPool = other.descriptorPool;
			descriptorSets.clear();
			for (auto item : descriptorSets) {
				descriptorSets.push_back(item);
			}
		}
	};

	class DrawableDescriptor {
	public:
		template <class T>
		static void Register(const std::string& identifier) {
			s_drawableDescriptors[identifier] = std::make_unique<T>();
		}

		static DrawableDescriptor* Get(const std::string& identifier) {
			if (s_drawableDescriptors.find(identifier) == s_drawableDescriptors.end()) {
				throw std::invalid_argument("No such DrawableDescriptor with identifier " + identifier);
			}
			return s_drawableDescriptors[identifier].get();
		}

		static void Cleanup() {
			s_drawableDescriptors.clear();
		}

		bg2render::vk::PipelineLayout* createPipelineLayout(vk::Instance* instance) {
			auto result = new bg2render::vk::PipelineLayout(instance);
			configureLayout(result);
			result->create();
			return result;
		}

		Descriptor createDescriptorPool(vk::Instance* instance, vk::PipelineLayout* pipelineLayout, uint32_t poolSize) {
			Descriptor descriptor;
			descriptor.descriptorPool = std::make_shared<bg2render::vk::DescriptorPool>(instance);
			configureDescriptorPool(descriptor.descriptorPool.get(), poolSize);
			descriptor.descriptorPool->create(poolSize);
			descriptor.descriptorPool->allocateDescriptorSets(poolSize, pipelineLayout, descriptor.descriptorSets);
			return descriptor;
		}

		// TODO: update descriptor writes

	protected:
		virtual void configureLayout(bg2render::vk::PipelineLayout* pipelineLayout) = 0;
		virtual void configureDescriptorPool(bg2render::vk::DescriptorPool* descriptorPool, uint32_t poolSize) = 0;

		static std::map<std::string, std::unique_ptr<DrawableDescriptor>> s_drawableDescriptors;
	};

	std::map<std::string, std::unique_ptr<DrawableDescriptor>> DrawableDescriptor::s_drawableDescriptors;

	template <class T>
	class DrawableDescriptorRegistry {
	public:
		DrawableDescriptorRegistry(const std::string& identifier) {
			DrawableDescriptor::Register<T>(identifier);
		}
	};

}

class MyDrawableDescriptor : public bg2render::DrawableDescriptor {
public:

protected:

	virtual void configureLayout(bg2render::vk::PipelineLayout* pipelineLayout) {
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

	virtual void configureDescriptorPool(bg2render::vk::DescriptorPool* descriptorPool, uint32_t poolSize) {
		descriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, poolSize);
		descriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, poolSize);
	}
};

bg2render::DrawableDescriptorRegistry<MyDrawableDescriptor> testDescriptor("testDescriptor");

class PolyList {
public:


protected:

};

class Material {
public:
	
protected:

};

class DrawableItem {
public:

protected:
	std::shared_ptr<PolyList> _polyList;
	std::shared_ptr<Material> _material;
};

class MyRendererDelegate : public bg2render::RendererDelegate {
public:
	struct Vertex {
		bg2math::float3 pos;
		bg2math::float3 color;
		bg2math::float2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

			return attributeDescriptions;
		}
	};

	std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};

	VkVertexInputBindingDescription bindingDescription;
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

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
		bg2render::vk::PipelineLayout* pipelineLayout = bg2render::DrawableDescriptor::Get("testDescriptor")->createPipelineLayout(instance);
		pipeline->setPipelineLayout(pipelineLayout);

		bindingDescription = Vertex::getBindingDescription();
		attributeDescriptions = Vertex::getAttributeDescriptions();

		pipeline->vertexInputInfo().vertexBindingDescriptionCount = 1;
		pipeline->vertexInputInfo().pVertexBindingDescriptions = &bindingDescription;
		pipeline->vertexInputInfo().vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		pipeline->vertexInputInfo().pVertexAttributeDescriptions = attributeDescriptions.data();

		pipeline->inputAssemblyInfo().topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipeline->setViewport(frameSize);
		pipeline->rasterizationStateInfo().cullMode = VK_CULL_MODE_BACK_BIT;
		pipeline->rasterizationStateInfo().frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		pipeline->depthStencilInfo().depthTestEnable = VK_TRUE;

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

		// Texture
		bg2base::path path = "data";
		auto image = std::unique_ptr<bg2base::image>(bg2db::loadImage(path.pathAddingComponent("texture.jpg")));
		_texture = std::make_shared<bg2render::Texture>(instance);
		_texture->create(image.get(), renderer()->commandPool(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		
		// Descriptor sets
		// Every object must store its own Descriptor struct, that contains the descriptor pool and the descriptor sets
		_descriptorPool = std::make_shared<bg2render::vk::DescriptorPool>(instance);
		// Pool size for uniform buffer
		_descriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, simultaneousFrames);
		// Pool size for texture
		_descriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, simultaneousFrames);
		_descriptorPool->create(simultaneousFrames);
		_descriptorPool->allocateDescriptorSets(simultaneousFrames, renderer()->pipeline()->pipelineLayout(), _descriptorSets);

		for (size_t i = 0; i < simultaneousFrames; ++i) {
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = _uniformBuffers[i]->buffer();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = _texture->vkImageView();
			imageInfo.sampler = _texture->vkSampler();

			std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = _descriptorSets[i]->descriptorSet();
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;
			descriptorWrites[0].pImageInfo = nullptr;
			descriptorWrites[0].pTexelBufferView = nullptr;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = _descriptorSets[i]->descriptorSet();
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(instance->renderDevice()->device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	virtual void cleanup() {
		_texture = nullptr;
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

	// DrawableDescriptor
	std::unique_ptr<bg2render::DrawableDescriptor> _drawableDescriptor;

	// Descriptors
	std::shared_ptr<bg2render::vk::DescriptorPool> _descriptorPool;
	std::vector<std::shared_ptr<bg2render::vk::DescriptorSet>> _descriptorSets;

	// Texture
	std::shared_ptr<bg2render::Texture> _texture;
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
		bg2render::DrawableDescriptor::Cleanup();
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
