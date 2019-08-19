

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
#include <bg2render/drawable_item.hpp>
#include <bg2render/drawable_descriptor.hpp>
#include <bg2math/matrix.hpp>
#include <bg2base/image.hpp>
#include <bg2db/image_load.hpp>

#include <iostream>
#include <array>
#include <chrono>

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
