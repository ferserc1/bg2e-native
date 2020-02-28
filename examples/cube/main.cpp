


#include <bg2engine.hpp>

#include <iostream>
#include <array>
#include <chrono>

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
		bg2render::vk::PipelineLayout* pipelineLayout = bg2render::DrawableDescriptor::Get("pbrDescriptor")->createPipelineLayout(instance);
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
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		_drawableItem->transform().model = bg2math::float4x4::Rotation(time * bg2math::radians(90.0f), 0.0f, 0.0f, 1.0f);

		cmdBuffer->bindPipeline(pipeline);
		_drawableItem->update(frameIndex);
		_drawableItem->draw(cmdBuffer, pipeline, frameIndex);
	}

	virtual void initDone(bg2render::vk::Instance* instance, uint32_t simultaneousFrames) {
		// Texture
		bg2base::path path = "data";
		auto image = std::unique_ptr<bg2base::image>(bg2db::loadImage(path.pathAddingComponent("texture.jpg")));
		auto diffuse = new bg2render::Texture(instance);
		diffuse->create(image.get(), renderer()->commandPool(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

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
		mat->setDiffuse(diffuse);

		_drawableItem = std::make_shared<bg2render::DrawableItem>("pbrDescriptor", renderer());
		_drawableItem->create(polyList, mat);

		_drawableItem->transform().view = bg2math::float4x4::LookAt(bg2math::float3(2.0f, 2.0f, 2.0f), bg2math::float3(0.0f, 0.0f, 0.0f), bg2math::float3(0.0f, 0.0f, 1.0f));
		auto extent = _renderer->swapChain()->extent();
		auto ratio = static_cast<float>(extent.width) / static_cast<float>(extent.height);
		_drawableItem->transform().proj = bg2math::float4x4::Perspective(60.0f, ratio, 0.1f, 100.0f);
		_drawableItem->transform().proj.element(1, 1) *= -1.0;
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

    void keyUp(const bg2wnd::KeyboardEvent & e) {
		if (e.keyCode()==bg2wnd::KeyCode::KeyESCAPE) {
			window()->close();
		}
		else if (e.keyCode()==bg2wnd::KeyCode::Key1) {
			window()->setSize(bg2math::int2(640,480));
		}
		else if (e.keyCode()==bg2wnd::KeyCode::Key2) {
			window()->setSize(bg2math::int2(800,600));
		}
		else if (e.keyCode()==bg2wnd::KeyCode::Key3) {
			window()->setSize(bg2math::int2(1024,768));
		}
	}

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
	bg2wnd::Application * app = bg2wnd::Application::Get();

    auto window = bg2wnd::Window::New();
    window->setWindowDelegate(new MyWindowDelegate(true));
    app->addWindow(window);

    return app->run();
}
