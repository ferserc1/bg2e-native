

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
	};

	class DrawableItem;
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

		DrawableDescriptor() {}
		virtual ~DrawableDescriptor() {
		}

		bg2render::vk::PipelineLayout* createPipelineLayout(vk::Instance* instance) {
			auto result = new bg2render::vk::PipelineLayout(instance);
			configureLayout(result);
			result->create();
			return result;
		}

		Descriptor* createDescriptorPool(vk::Instance* instance, vk::PipelineLayout* pipelineLayout, uint32_t poolSize) {
			Descriptor* descriptor = new Descriptor();
			descriptor->descriptorPool = std::make_shared<bg2render::vk::DescriptorPool>(instance);
			configureDescriptorPool(descriptor->descriptorPool.get(), poolSize);
			descriptor->descriptorPool->create(poolSize);
			descriptor->descriptorPool->allocateDescriptorSets(poolSize, pipelineLayout, descriptor->descriptorSets);
			return descriptor;
		}

		virtual void updateDescriptorWrites(vk::Instance* instance, uint32_t frameIndex, DrawableItem*) = 0;

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

	class PolyList {
	public:
		struct Vertex {
			bg2math::float3 position;
			bg2math::float3 normal;
			bg2math::float2 texCoord0;
			bg2math::float2 texCoord1;
			bg2math::float3 tangent;

			static const VkVertexInputBindingDescription& getBindingDescription() {
				return s_bindingDescription;
			}

			static const std::array<VkVertexInputAttributeDescription, 5> & getAttributeDescriptions() {
				if (!s_attributeInitialized) {
					s_attributeDescriptions[0].binding = 0;
					s_attributeDescriptions[0].location = 0;
					s_attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
					s_attributeDescriptions[0].offset = offsetof(Vertex, position);

					s_attributeDescriptions[1].binding = 0;
					s_attributeDescriptions[1].location = 1;
					s_attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
					s_attributeDescriptions[1].offset = offsetof(Vertex, normal);

					s_attributeDescriptions[2].binding = 0;
					s_attributeDescriptions[2].location = 2;
					s_attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
					s_attributeDescriptions[2].offset = offsetof(Vertex, texCoord0);

					s_attributeDescriptions[3].binding = 0;
					s_attributeDescriptions[3].location = 3;
					s_attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
					s_attributeDescriptions[3].offset = offsetof(Vertex, texCoord1);

					s_attributeDescriptions[4].binding = 0;
					s_attributeDescriptions[4].location = 4;
					s_attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
					s_attributeDescriptions[4].offset = offsetof(Vertex, tangent);

					s_attributeInitialized = true;
				}

				return s_attributeDescriptions;
			}
		};

		static void configureVertexInput(bg2render::Pipeline* pipeline) {
			pipeline->vertexInputInfo().vertexBindingDescriptionCount = 1;
			pipeline->vertexInputInfo().pVertexBindingDescriptions = &PolyList::Vertex::getBindingDescription();
			pipeline->vertexInputInfo().vertexAttributeDescriptionCount = static_cast<uint32_t>(PolyList::Vertex::getAttributeDescriptions().size());
			pipeline->vertexInputInfo().pVertexAttributeDescriptions = PolyList::Vertex::getAttributeDescriptions().data();
		}

		inline void addVertex(
			const bg2math::float3& position,
			const bg2math::float3& normal,
			const bg2math::float2& texCoord0,
			const bg2math::float2& texCoord1
		) {
			_vertices.push_back({ position, normal, texCoord0, texCoord1, { 0.0f, 0.0f, 0.0f } });
		}

		inline void addVertex(
			const bg2math::float3& position,
			const bg2math::float3& normal,
			const bg2math::float2& texCoord0
		) {
			_vertices.push_back({ position, normal, texCoord0, texCoord0, { 0.0f, 0.0f, 0.0f } });
		}

		inline void addIndexedTriangle(uint32_t i0, uint32_t i1, uint32_t i2) {
			_indices.push_back(i0); _indices.push_back(i1); _indices.push_back(i2);
		}

		inline void addIndex(uint32_t i) { _indices.push_back(i); }

		inline const std::vector<Vertex>& vertices() const { return _vertices; }
		inline const std::vector<uint32_t>& indices() const { return _indices; }

		PolyList() {}
		virtual ~PolyList() {
			if (_instance) {
				_vertexBuffer = nullptr;
				_indexBuffer = nullptr;
			}
		}

		void create(bg2render::vk::Instance* instance, VkCommandPool commandPool) {
			createTangents();

			_instance = instance;

			_vertexBuffer = std::make_unique<bg2render::VertexBuffer>(instance);
			_vertexBuffer->create<Vertex>(_vertices, commandPool);

			_indexBuffer = std::make_unique<bg2render::IndexBuffer>(instance);
			_indexBuffer->create<uint32_t>(_indices, commandPool);
		}

		void bindBuffers(bg2render::vk::CommandBuffer* commandBuffer) {
			commandBuffer->bindVertexBuffer(0, 1, _vertexBuffer);
			commandBuffer->bindIndexBuffer(_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}

		void draw(bg2render::vk::CommandBuffer* commandBuffer) {
			commandBuffer->drawIndexed(_indices.size(), 1, 0, 0, 0);
		}

	protected:
		static VkVertexInputBindingDescription s_bindingDescription;
		static bool s_attributeInitialized;
		static std::array<VkVertexInputAttributeDescription, 5> s_attributeDescriptions;

		bg2render::vk::Instance* _instance = VK_NULL_HANDLE;

		// Vertex buffers
		std::unique_ptr<bg2render::VertexBuffer> _vertexBuffer;
		std::unique_ptr<bg2render::IndexBuffer> _indexBuffer;

		// Vertex and index data
		std::vector<Vertex> _vertices;
		std::vector<uint32_t> _indices;

		void createTangents() {
			// TODO: Implement this
			
		}
	};

	VkVertexInputBindingDescription PolyList::s_bindingDescription = {
		0,	// binding
		sizeof(PolyList::Vertex),	// Stride
		VK_VERTEX_INPUT_RATE_VERTEX	// Input rate
	};
	bool PolyList::s_attributeInitialized = false;
	std::array<VkVertexInputAttributeDescription, 5> PolyList::s_attributeDescriptions;

	class Material {
	public:
		Material() {}

		inline void setTexture(bg2render::Texture* tex) { _texture = std::shared_ptr<bg2render::Texture>(tex); }
		inline const bg2render::Texture* texture() const { return _texture.get(); }

	protected:
		std::shared_ptr<bg2render::Texture> _texture;
	};

	class DrawableItem {
	public:
		struct UniformBufferObject {
			bg2math::float4x4 model;
			bg2math::float4x4 view;
			bg2math::float4x4 proj;
		};

		DrawableItem(const std::string& descriptorType, bg2render::Renderer* rend)
			:_descriptorType(descriptorType)
			, _renderer(rend)
		{

		}
		virtual ~DrawableItem() {
			_polyList = nullptr;
			_material = nullptr;
			_uniformBuffers.clear();
			_uniformBuffersMemory.clear();
		}

		void create(PolyList* pl, Material* mat) {
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

		void update(uint32_t frameIndex) {
			auto drawableDescriptor = bg2render::DrawableDescriptor::Get(_descriptorType);
			drawableDescriptor->updateDescriptorWrites(_renderer->instance(), frameIndex, this);
			
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

		void draw(bg2render::vk::CommandBuffer* commandBuffer, bg2render::Pipeline* pipeline, uint32_t frameIndex) {
			_polyList->bindBuffers(commandBuffer);
			commandBuffer->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout(), 0, _descriptor->descriptorSets[frameIndex]);
			_polyList->draw(commandBuffer);
		}

		inline const bg2render::vk::Buffer* uniformBuffer(uint32_t frameIndex) const { return _uniformBuffers[frameIndex].get(); }
		inline const bg2render::vk::DeviceMemory* uniformBufferMemory(uint32_t frameIndex) const { return _uniformBuffersMemory[frameIndex].get(); }
		inline const bg2render::Descriptor* descriptor() const { return _descriptor.get(); }
		
		inline const Material* material() const { return _material.get(); }
		inline Material* material() { return _material.get(); }
		inline const PolyList* polyList() const { return _polyList.get(); }
		inline PolyList* polyList() { return _polyList.get(); }

	protected:
		std::string _descriptorType;
		bg2render::Renderer* _renderer;

		std::shared_ptr<PolyList> _polyList;
		std::shared_ptr<Material> _material;
		std::shared_ptr<bg2render::Descriptor> _descriptor;
		std::shared_ptr<bg2render::vk::PipelineLayout> _pipelineLayout;

		std::vector<std::unique_ptr<bg2render::vk::Buffer>> _uniformBuffers;
		std::vector<std::unique_ptr<bg2render::vk::DeviceMemory>> _uniformBuffersMemory;

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

	virtual void updateDescriptorWrites(bg2render::vk::Instance* instance, uint32_t frameIndex, bg2render::DrawableItem* drawableItem) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = drawableItem->uniformBuffer(frameIndex)->buffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(bg2render::DrawableItem::UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = drawableItem->material()->texture()->vkImageView();
		imageInfo.sampler = drawableItem->material()->texture()->vkSampler();

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
};

bg2render::DrawableDescriptorRegistry<MyDrawableDescriptor> testDescriptor("testDescriptor");

class MyRendererDelegate : public bg2render::RendererDelegate {
public:
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

		bg2render::PolyList::configureVertexInput(pipeline);

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
		_drawableItem->update(frameIndex);
		_drawableItem->draw(cmdBuffer, pipeline, frameIndex);
	}

	virtual void initDone(bg2render::vk::Instance* instance, uint32_t simultaneousFrames) {
		// Texture
		bg2base::path path = "data";
		auto image = std::unique_ptr<bg2base::image>(bg2db::loadImage(path.pathAddingComponent("texture.jpg")));
		auto _texture = new bg2render::Texture(instance);
		_texture->create(image.get(), renderer()->commandPool(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		auto polyList = new bg2render::PolyList();

		polyList->addVertex(bg2math::float3{ -0.5f, -0.5f, 0.0f }, bg2math::float3{ 0.0f, 1.0f, 0.0f }, bg2math::float2{ 0.0f, 0.0f });
		polyList->addVertex(bg2math::float3{ 0.5f, -0.5f, 0.0f }, bg2math::float3{ 0.0f, 1.0f, 0.0f }, bg2math::float2{ 1.0f, 0.0f });
		polyList->addVertex(bg2math::float3{ 0.5f, 0.5f, 0.0f }, bg2math::float3{ 0.0f, 1.0f, 0.0f }, bg2math::float2{ 1.0f, 1.0f });
		polyList->addVertex(bg2math::float3{-0.5f, 0.5f, 0.0f}, bg2math::float3{0.0f, 1.0f, 0.0f}, bg2math::float2{0.0f, 1.0f});
		polyList->addVertex(bg2math::float3{-0.5f, -0.5f, -0.5f}, bg2math::float3{0.0f, 1.0f, 0.0f}, bg2math::float2{0.0f, 0.0f});
		polyList->addVertex(bg2math::float3{0.5f, -0.5f, -0.5f}, bg2math::float3{0.0f, 1.0f, 0.0f}, bg2math::float2{1.0f, 0.0f});
		polyList->addVertex(bg2math::float3{0.5f, 0.5f, -0.5f}, bg2math::float3{0.0f, 1.0f, 0.0f}, bg2math::float2{1.0f, 1.0f});
		polyList->addVertex(bg2math::float3{-0.5f, 0.5f, -0.5f}, bg2math::float3{0.0f, 1.0f, 0.0f}, bg2math::float2{0.0f, 1.0f});

		polyList->addIndexedTriangle(0, 1, 2);
		polyList->addIndexedTriangle(2, 3, 0);
		polyList->addIndexedTriangle(4, 5, 6);
		polyList->addIndexedTriangle(6, 7, 4);

		polyList->create(instance, renderer()->commandPool());
		
		auto mat = new bg2render::Material();
		mat->setTexture(_texture);

		_drawableItem = std::make_shared<bg2render::DrawableItem>("testDescriptor", renderer());
		_drawableItem->create(polyList, mat);
	}

	virtual void cleanup() {
		_drawableItem = nullptr;
	}

private:
	std::shared_ptr<bg2render::DrawableItem> _drawableItem;
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
