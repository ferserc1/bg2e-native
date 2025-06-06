
#include <bg2e.hpp>

#include <array>
#include <numbers>

class RotateCameraComponent : public bg2e::scene::Component {
public:
    void update(float delta) override
    {
        auto transform = ownerNode()->transform();
        
        if (transform)
        {
            transform->rotate(0.02f * delta / 10.0f, 0.0f, 1.0f, 0.0f);
        }
    }
};

class BasicSceneDelegate : public bg2e::render::RenderLoopDelegate,
	public bg2e::app::InputDelegate,
	public bg2e::ui::UserInterfaceDelegate
{
public:
	void init(bg2e::render::Engine * engine) override
	{
		using namespace bg2e::render::vulkan;
		RenderLoopDelegate::init(engine);
  
        _renderer = std::make_unique<bg2e::render::Renderer>();
        _renderer->setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
        _renderer->setDepthAttachmentFormat(engine->swapchain().depthImageFormat());
        _renderer->build(engine);
	}
 
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) override
    {
        _renderer->initFrameResources(frameAllocator);
        
        frameAllocator->initPool();
    }
 
    void initScene() override
    {
        auto scene = createScene();
        _renderer->initScene(scene);
        
		createPipeline();
    }

	void swapchainResized(VkExtent2D newExtent) override
	{
        _renderer->resize(newExtent);
	}

	VkImageLayout render(
		VkCommandBuffer cmd,
		uint32_t currentFrame,
		const bg2e::render::vulkan::Image* colorImage,
		const bg2e::render::vulkan::Image* depthImage,
		bg2e::render::vulkan::FrameResources& frameResources
	) override {
		using namespace bg2e::render::vulkan;
  
        _renderer->update(delta());
        
        _renderer->draw(
            cmd,
            currentFrame,
            depthImage,
            frameResources
        );

        Image::cmdCopy(
            cmd,
            _renderer->colorAttachments()->imageWithIndex(0)->handle(),
            _renderer->colorAttachments()->extent(),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            colorImage->handle(), colorImage->extent2D(), VK_IMAGE_LAYOUT_UNDEFINED
        );

		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}

	// ============ User Interface Delegate Functions =========
	void init(bg2e::render::Engine*, bg2e::ui::UserInterface*) override {
		_window.setTitle("ImGui Wrapper Demo");
		_window.options.noClose = true;
		_window.options.minWidth = 100;
		_window.options.minHeight = 250;
		_window.setPosition(0, 0);
		_window.setSize(150, 270);
	}

	void drawUI() override
	{
		using namespace bg2e::ui;
        auto drawSkybox = _renderer->drawSkybox();
		_window.draw([&]() {
            BasicWidgets::checkBox("Draw Skybox", &drawSkybox);
		});
        _renderer->setDrawSkybox(drawSkybox);
	}
    
	void cleanup() override
	{
        _renderer->cleanup();
        bg2e::utils::TextureCache::destroy();
	}

protected:

	bg2e::ui::Window _window;

	VkPipelineLayout _layout = VK_NULL_HANDLE;
	VkPipeline _pipeline = VK_NULL_HANDLE;
 
    std::unique_ptr<bg2e::render::Renderer> _renderer;
      
	void createPipeline()
	{
		bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_engine);

		plFactory.addShader("test/texture_gi.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        plFactory.addShader("test/texture_gi.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        
        plFactory.setInputState<bg2e::render::vulkan::geo::MeshPNU>();
    
        auto frameDSLayout = _renderer->frameDataBinding()->createLayout();
        auto objectDSLayout = _renderer->objectDataBinding()->createLayout();
        auto envDSLayout = _renderer->environmentDataBinding()->createLayout();
        
        bg2e::render::vulkan::factory::PipelineLayout layoutFactory(_engine);
        layoutFactory.addDescriptorSetLayout(frameDSLayout);
        layoutFactory.addDescriptorSetLayout(objectDSLayout);
        layoutFactory.addDescriptorSetLayout(envDSLayout);
        _layout = layoutFactory.build();
        
        plFactory.setDepthFormat(_engine->swapchain().depthImageFormat());
        plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
        plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        plFactory.setCullMode(false, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        
        plFactory.setColorAttachmentFormat(_renderer->colorAttachments()->attachmentFormats());
		_pipeline = plFactory.build(_layout);
  
        _renderer->setPipelineLayout(_layout);
        _renderer->setPipeline(_pipeline);
  
		_engine->cleanupManager().push([&, objectDSLayout, envDSLayout, frameDSLayout](VkDevice dev) {
			vkDestroyPipeline(dev, _pipeline, nullptr);
			vkDestroyPipelineLayout(dev, _layout, nullptr);
            vkDestroyDescriptorSetLayout(dev, objectDSLayout, nullptr);
            vkDestroyDescriptorSetLayout(dev, envDSLayout, nullptr);
            vkDestroyDescriptorSetLayout(dev, frameDSLayout, nullptr);
		});
	}
 
    std::shared_ptr<bg2e::scene::Node> createScene()
    {
        auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
        sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "country_field_sun.jpg"));
        
        auto anotherNode = new bg2e::scene::Node("Transform Node");
        anotherNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 1.0f, 0.0f));
        sceneRoot->addChild(anotherNode);
        
        auto drawable = std::shared_ptr<bg2e::scene::DrawableBase>(loadDrawable());
        auto drawableComponent = std::make_shared<bg2e::scene::DrawableComponent>(drawable);
        auto modelNode = std::make_shared<bg2e::scene::Node>("3D Model");
        modelNode->addComponent(drawableComponent);
        modelNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(2.0f, 0.0f, 0.0f));
        anotherNode->addChild(modelNode);
        
        auto secondModel = new bg2e::scene::Node("Second 3D model");
        auto anotherDrawable = new bg2e::scene::DrawableComponent(drawable);

        secondModel->addComponent(anotherDrawable);
        secondModel->addComponent(bg2e::scene::TransformComponent::makeTranslated(-2.0f, 0.0f, 0.0f ));
        sceneRoot->addChild(secondModel);
        
        auto cameraNode = std::shared_ptr<bg2e::scene::Node>(new bg2e::scene::Node("Camera"));
        cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 1.3f, -10.0f ));
        cameraNode->addComponent(new bg2e::scene::CameraComponent());
        auto projection = new bg2e::math::OpticalProjection();
        cameraNode->camera()->setProjection(projection);
        
        auto cameraRotation = new bg2e::scene::Node("Camera Rotation");
        cameraRotation->addComponent(new bg2e::scene::TransformComponent());
        cameraRotation->addComponent(new RotateCameraComponent());
        cameraRotation->addChild(cameraNode);

        sceneRoot->addChild(cameraRotation);
        
        return sceneRoot;
    }

	bg2e::scene::DrawableBase * loadDrawable()
	{
		using namespace bg2e::render::vulkan;
  
        std::filesystem::path modelPath = bg2e::base::PlatformTools::assetPath();
        modelPath.append("two_submeshes.obj");
        
        auto innerAlbedoTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_inner_albedo.jpg"
        );
        
        auto outerAlbedoTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_outer_albedo.jpg"
        );
        
        auto drawable = new bg2e::scene::DrawablePNU();
        
        drawable->setMesh(bg2e::db::loadMeshObj<bg2e::geo::MeshPNU>(modelPath));
        std::cout << "Submeshes loaded: " << drawable->mesh()->submeshes.size() << std::endl;
        std::cout << "Triangles loaded: " << drawable->mesh()->indices.size() / 3 << std::endl;
        drawable->material(0).setAlbedo(outerAlbedoTexture);
        drawable->material(1).setAlbedo(innerAlbedoTexture);
        drawable->load(_engine);
        
        return drawable;
	}
};

class MyApplication : public bg2e::app::Application {
public:
	void init(int argc, char** argv) override
	{
		auto delegate = std::make_shared<BasicSceneDelegate>();
		setRenderDelegate(delegate);
		setInputDelegate(delegate);
		setUiDelegate(delegate);
	}
};

int main(int argc, char** argv) {
	bg2e::app::MainLoop mainLoop;
	MyApplication app;
	app.init(argc, argv);
	return mainLoop.run(&app);
}
