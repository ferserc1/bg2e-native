
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
  
        /// Renderer resources:
        /// - Color Attachments: Color output of the renderer shaders
        _colorAttachments = std::shared_ptr<bg2e::render::ColorAttachments>(
            new bg2e::render::ColorAttachments(_engine, {
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_FORMAT_R8G8B8A8_UNORM
            })
        );
        
        /// - Data bindings: used to bind data to the shaders. There are three types of data bindings
        ///     * Frame data bindings: bind frame resources, such as the view and projection matrix
        _frameDataBinding = std::make_unique<bg2e::scene::vk::FrameDataBinding>(engine);
        
        ///     * Object data binding: bind resorces for one object, for example a 3D model submesh
        _objectDataBinding = std::make_unique<bg2e::scene::vk::ObjectDataBinding>(engine);
        
        ///     * Environment data binding: bind resources for the environment, such as the lights or the environment cube map.
        ///       This is separated from the frame data binding because there is no need to bind environment resources if we are
        ///       rendering g-buffers for deferred render
        _environmentDataBinding = std::make_unique<bg2e::scene::vk::EnvironmentDataBinding>(engine);
        
        ///     * Environment resources: used to generate the environment cube map, the irradiance map and the specular
        ///       reflection map. It is also used to draw the skybox.
        _environment = std::unique_ptr<bg2e::render::EnvironmentResources>(
            new bg2e::render::EnvironmentResources(
                _engine,
                _colorAttachments->attachmentFormats(),
                _engine->swapchain().depthImageFormat()
            )
        );
	}
 
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) override
    {
        /// Initialize descriptor set frame resources for the renderer resources
        _frameDataBinding->initFrameResources(frameAllocator);
        _objectDataBinding->initFrameResources(frameAllocator);
        _environmentDataBinding->initFrameResources(frameAllocator);
        _environment->initFrameResources(frameAllocator);
        
        frameAllocator->initPool();
    }
 
    void initScene() override
    {
        // Use the initScene function to initialize and create scene resources, such as pipelines, 3D models
        // or textures
        
        
        /// Renderer: initialize the sky dome generator with a procedural texture
        auto skyDomeTexture = std::make_shared<bg2e::base::Texture>();
        auto skyDomeGenerator = new bg2e::scene::SkyDomeTextureGenerator(2048, 1024, 4);
        skyDomeTexture->setProceduralGenerator(skyDomeGenerator);
        skyDomeTexture->setUseMipmaps(false);
        auto envTexture = std::make_shared<bg2e::render::Texture>(_engine);
        envTexture->load(skyDomeTexture);
        _environment->build(
            envTexture,         // Equirectangular texture
            { 2048, 2048 },     // Cube map size
            { 32, 32 },         // Irradiance map size
            { 1024, 1024 }      // Specular reflection map size
        );
	
        _colorAttachments->build(_engine->swapchain().extent());

		createPipeline();
        
        createScene();
    }

	void swapchainResized(VkExtent2D newExtent) override
	{
        // This function releases all previous resources before recreate the images
		_colorAttachments->build(newExtent);
  
        /// Renderer: call the resize visitor to update the camera projection matrix
        // Call resizeViewport() on the scene components
        _resizeVisitor.resizeViewport(_scene->rootNode(), newExtent);
	}

	VkImageLayout render(
		VkCommandBuffer cmd,
		uint32_t currentFrame,
		const bg2e::render::vulkan::Image* colorImage,
		const bg2e::render::vulkan::Image* depthImage,
		bg2e::render::vulkan::FrameResources& frameResources
	) override {
		using namespace bg2e::render::vulkan;
  
        _updateVisitor.update(_scene->rootNode(), delta());
        
        /// Renderer: check if the environment texture has been changed using the environment component image hash
        /// and update the environment matrixes
        if (_scene->mainEnvironment() && _scene->mainEnvironment()->imgHash() != _skyboxImageHash)
        {
            _environment->swapEnvironmentTexture(_scene->mainEnvironment()->environmentImage());
            _skyboxImageHash = _scene->mainEnvironment()->imgHash();
        }
        _environment->update(cmd, currentFrame, frameResources);  // This function will update the sky box textures only if the skybox image has changed
  
		float flash = std::abs(std::sin(currentFrame / 120.0f));
		VkClearColorValue clearValue{ { 0.0f, 0.0f, flash, 1.0f } };
        macros::cmdClearImagesAndBeginRendering(
            cmd,
            _colorAttachments->images(),
            clearValue, VK_IMAGE_LAYOUT_UNDEFINED,
            depthImage, 1.0f
        );
        
		macros::cmdSetDefaultViewportAndScissor(cmd,
            _colorAttachments->extent()
        );
  
        /// Renderer: get the view and projection matrixes from the main scene camera and create the scene descriptor set
        auto mainCamera = _scene->mainCamera();
        auto projMatrix = mainCamera->projectionMatrix();
        projMatrix[1][1] *= -1.0f; // Flip Vulkan Y coord
        auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();
        auto sceneDS = _frameDataBinding->newDescriptorSet(
            frameResources,
            viewMatrix,
            projMatrix
        );
        

        /// Renderer: update and render the skybox. This is only needed if we want to draw the skybox in the output color attachments, it may be not neccesary
        /// in some renderers at this point (for example in deferred renderer)
        if (_drawSkybox)
        {
            _environment->updateSkybox(viewMatrix, projMatrix);
            _environment->drawSkybox(cmd, currentFrame, frameResources);
        }
  
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
        
        auto envDS = _environmentDataBinding->newDescriptorSet(frameResources, _environment.get());
  
        /// Renderer: draw the scene
        _drawVisitor.draw(
            _scene->rootNode(),
            cmd,
            _layout,
            [&](bg2e::render::MaterialBase * mat, const glm::mat4& transform, uint32_t submesh) {
                auto modelDS = _objectDataBinding->newDescriptorSet(
                    frameResources,
                    mat,
                    transform
                );
                return std::vector<VkDescriptorSet>{
                    sceneDS,
                    modelDS,
                    envDS
                };
            });

		bg2e::render::vulkan::cmdEndRendering(cmd);

		Image::cmdCopy(
			cmd,
            _colorAttachments->imageWithIndex(_showRenderTargetIndex)->handle(),
            _colorAttachments->extent(),
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
		_window.draw([&]() {
            BasicWidgets::checkBox("Draw Skybox", &_drawSkybox);
            
            BasicWidgets::text("Show Attachment:");
            BasicWidgets::radioButton("Attachment 1", &_showRenderTargetIndex, 0);
            BasicWidgets::radioButton("Attachment 2", &_showRenderTargetIndex, 1);
		});
	}
    
	void cleanup() override
	{
        _colorAttachments->cleanup();
        bg2e::utils::TextureCache::destroy();
	}

protected:

    std::shared_ptr<bg2e::render::ColorAttachments> _colorAttachments;

	bg2e::ui::Window _window;

	VkPipelineLayout _layout = VK_NULL_HANDLE;
	VkPipeline _pipeline = VK_NULL_HANDLE;
 
    std::shared_ptr<bg2e::scene::Scene> _scene;
    size_t _skyboxImageHash = 0;
    
    bg2e::scene::ResizeViewportVisitor _resizeVisitor;
    bg2e::scene::UpdateVisitor _updateVisitor;
    bg2e::scene::DrawVisitor _drawVisitor;
    
    std::unique_ptr<bg2e::scene::vk::ObjectDataBinding> _objectDataBinding;

    // Load the environment skybox, generate the irradiance and specular reflection maps,
    // and manage the sky box renderer
    std::unique_ptr<bg2e::render::EnvironmentResources> _environment;
    std::unique_ptr<bg2e::scene::vk::EnvironmentDataBinding> _environmentDataBinding;
    std::unique_ptr<bg2e::scene::vk::FrameDataBinding> _frameDataBinding;
    
    bool _drawSkybox = true;
    int _showRenderTargetIndex = 0;
      
	void createPipeline()
	{
		bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_engine);

		plFactory.addShader("test/texture_gi.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        plFactory.addShader("test/texture_gi.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        
        plFactory.setInputState<bg2e::render::vulkan::geo::MeshPNU>();
    
        // Create object and environment descriptor set+
        auto frameDSLayout = _frameDataBinding->createLayout();
        auto objectDSLayout = _objectDataBinding->createLayout();
        auto envDSLayout = _environmentDataBinding->createLayout();
        
        bg2e::render::vulkan::factory::PipelineLayout layoutFactory(_engine);
        layoutFactory.addDescriptorSetLayout(frameDSLayout);
        layoutFactory.addDescriptorSetLayout(objectDSLayout);
        layoutFactory.addDescriptorSetLayout(envDSLayout);
        _layout = layoutFactory.build();
        
        plFactory.setDepthFormat(_engine->swapchain().depthImageFormat());
        plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
        plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        plFactory.setCullMode(false, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        
        plFactory.setColorAttachmentFormat(_colorAttachments->attachmentFormats());
		_pipeline = plFactory.build(_layout);
  
		_engine->cleanupManager().push([&, objectDSLayout, envDSLayout, frameDSLayout](VkDevice dev) {
			vkDestroyPipeline(dev, _pipeline, nullptr);
			vkDestroyPipelineLayout(dev, _layout, nullptr);
            vkDestroyDescriptorSetLayout(dev, objectDSLayout, nullptr);
            vkDestroyDescriptorSetLayout(dev, envDSLayout, nullptr);
            vkDestroyDescriptorSetLayout(dev, frameDSLayout, nullptr);
		});
	}
 
    void createScene()
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
        
        // Set the initial viewport size
        _resizeVisitor.resizeViewport(sceneRoot.get(), _engine->swapchain().extent());
        
        _scene = std::make_shared<bg2e::scene::Scene>();
        
        _scene->setSceneRoot(sceneRoot);
        
        _engine->cleanupManager().push([&](VkDevice) {
            _scene.reset();
        });
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
