

#include <bg2wnd/application.hpp>
#include <bg2wnd/window.hpp>
#include <bg2wnd/window_delegate.hpp>
#include <bg2render/vk_instance.hpp>
#include <bg2render/vk_device.hpp>
#include <bg2render/swap_chain.hpp>
#include <bg2render/renderer.hpp>
#include <bg2math/vector.hpp>
#include <bg2db/buffer_load.hpp>
#include <bg2render/pipeline.hpp>
#include <bg2render/vk_buffer.hpp>
#include <bg2render/vk_device_memory.hpp>
#include <bg2math/matrix.hpp>

#include <iostream>
#include <array>

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
		{ { 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
		{ {-0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f } }
	};

	VkVertexInputBindingDescription bindingDescription;
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;


	virtual bg2render::Pipeline * configurePipeline(bg2render::vk::Instance* instance, bg2render::SwapChain* swapChain, const bg2math::int2& frameSize) {
		bg2render::Pipeline * pipeline = new bg2render::Pipeline(instance);

		// It's important to ensure that we create the vertex buffers only once, because the
		// configurePipeline() function will be called every time the user resize the window
		if (_vertexBuffer == nullptr) {
			createVertexBuffer(instance);
		}

		bg2base::path shaderPath = "shaders";
		auto vshader = bg2db::loadBuffer(shaderPath.pathAddingComponent("basic.vert.spv"));
		auto fshader = bg2db::loadBuffer(shaderPath.pathAddingComponent("basic.frag.spv"));
		pipeline->addShader(vshader, VK_SHADER_STAGE_VERTEX_BIT, "main");
		pipeline->addShader(fshader, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

		bindingDescription = Vertex::getBindingDescription();
		attributeDescriptions = Vertex::getAttributeDescriptions();

		pipeline->vertexInputInfo().vertexBindingDescriptionCount = 1;
		pipeline->vertexInputInfo().pVertexBindingDescriptions = &bindingDescription;
		pipeline->vertexInputInfo().vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		pipeline->vertexInputInfo().pVertexAttributeDescriptions = attributeDescriptions.data();

		pipeline->inputAssemblyInfo().topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipeline->setViewport(frameSize);
		pipeline->rasterizationStateInfo().cullMode = VK_CULL_MODE_BACK_BIT;
		pipeline->rasterizationStateInfo().frontFace = VK_FRONT_FACE_CLOCKWISE;

		pipeline->loadDefaultBlendAttachments();

		pipeline->createDefaultLayout();

		pipeline->createDefaultRenderPass(swapChain->format());

		return pipeline;
	}

	void recordCommandBuffer(float delta, bg2render::vk::CommandBuffer* cmdBuffer, bg2render::Pipeline* pipeline, bg2render::SwapChain* swapChain) {
		cmdBuffer->bindPipeline(pipeline);

		VkBuffer vertexBuffers[] = { _vertexBuffer->buffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer->commandBuffer(), 0, 1, vertexBuffers, offsets);


		cmdBuffer->draw(
			static_cast<uint32_t>(vertices.size()),	// vertex count
			1,	// instance count
			0,	// first vertex
			0	// first instance
		);
	}

private:
	// Vertex buffers
	std::unique_ptr<bg2render::vk::Buffer> _vertexBuffer;
	std::unique_ptr<bg2render::vk::DeviceMemory> _vertexBufferMemory;

	// Utility functions to manage the vertex buffers
	void createVertexBuffer(bg2render::vk::Instance * instance) {
		// Create the buffer
		_vertexBuffer = std::make_unique<bg2render::vk::Buffer>(instance);
		_vertexBuffer->create(
			sizeof(vertices[0]) * vertices.size(), 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
			VK_SHARING_MODE_EXCLUSIVE);

		// Allocate memory
		_vertexBufferMemory = std::make_unique<bg2render::vk::DeviceMemory>(instance);
		_vertexBufferMemory->allocate(
			_vertexBuffer->memoryRequirements(),
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// Bind memory to vetex buffer
		vkBindBufferMemory(instance->renderDevice()->device(), _vertexBuffer->buffer(), _vertexBufferMemory->deviceMemory(), 0);

		// Filling the vertex buffer
		void* data;
		_vertexBufferMemory->map(0, _vertexBuffer->size(), 0, &data);
		memcpy(data, vertices.data(), static_cast<size_t>(_vertexBuffer->size()));
		_vertexBufferMemory->unmap();

		// TODO: Create a vertex buffer utility class:
		// - Contains buffer and buffer memory objects
		// - Automatically binds both buffers
		// - Contains utility functions to upload a std::vector of vertex data
	}

	virtual void cleanup() {
		_vertexBuffer = nullptr;
		_vertexBufferMemory = nullptr;
	}
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
