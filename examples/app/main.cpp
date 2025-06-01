
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
	void init(bg2e::render::Vulkan* vulkan) override
	{
		using namespace bg2e::render::vulkan;
		RenderLoopDelegate::init(vulkan);
  
        /// Renderer resources:
        /// - Color Attachments: Color output of the renderer shaders
        _colorAttachments = std::shared_ptr<bg2e::render::ColorAttachments>(
            new bg2e::render::ColorAttachments(_vulkan, {
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_FORMAT_R8G8B8A8_UNORM
            })
        );
        
        /// - Data bindings: used to bind data to the shaders. There are three types of data bindings
        ///     * Frame data bindings: bind frame resources, such as the view and projection matrix
        _frameDataBinding = std::make_unique<bg2e::scene::vk::FrameDataBinding>(vulkan);
        
        ///     * Object data binding: bind resorces for one object, for example a 3D model submesh
        _objectDataBinding = std::make_unique<bg2e::scene::vk::ObjectDataBinding>(vulkan);
        
        ///     * Environment data binding: bind resources for the environment, such as the lights or the environment cube map.
        ///       This is separated from the frame data binding because there is no need to bind environment resources if we are
        ///       rendering g-buffers for deferred render
        _environmentDataBinding = std::make_unique<bg2e::scene::vk::EnvironmentDataBinding>(vulkan);
        
        ///     * Environment resources: used to generate the environment cube map, the irradiance map and the specular
        ///       reflection map. It is also used to draw the skybox.
        _environment = std::unique_ptr<bg2e::render::EnvironmentResources>(
            new bg2e::render::EnvironmentResources(
                _vulkan,
                _colorAttachments->attachmentFormats(),
                _vulkan->swapchain().depthImageFormat()
            )
        );
	}
 
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) override
    {
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
        
        auto assetsPath = bg2e::base::PlatformTools::assetPath();
        auto imagePath = assetsPath;
        imagePath.append("country_field_sun.jpg");
        
        //auto envTexture = bg2e::utils::TextureCache::get().load(_vulkan, imagePath);
        
        // Initialize the environment resources with a procedural generic sky texture
        // TODO: Use a ProceduralTextureGenerator to build an equirectangular sky dome image
        auto skyDomeTexture = std::make_shared<bg2e::base::Texture>();
        auto skyDomeGenerator = new bg2e::scene::SkyDomeTextureGenerator(2048, 1024, 4);
        skyDomeTexture->setProceduralGenerator(skyDomeGenerator);
        skyDomeTexture->setUseMipmaps(false);
        
        auto envTexture = std::make_shared<bg2e::render::Texture>(_vulkan);
        envTexture->load(skyDomeTexture);
        _environment->build(
            envTexture,         // Equirectangular texture
            { 2048, 2048 },     // Cube map size
            { 32, 32 },         // Irradiance map size
            { 1024, 1024 }      // Specular reflection map size
        );
	
        _colorAttachments->build(_vulkan->swapchain().extent());

		createPipeline();
        
        createScene();
    }

	void swapchainResized(VkExtent2D newExtent) override
	{
        // This function releases all previous resources before recreate the images
		_colorAttachments->build(newExtent);
  
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
    
        // TODO: wrap this object in a scene component
        _environment->update(cmd, currentFrame, frameResources);
  
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
  
        auto mainCamera = _scene->mainCamera();
        auto projMatrix = mainCamera->projectionMatrix();
        projMatrix[1][1] *= -1.0f; // Flip Vulkan Y coord
        
        auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();
        
        auto sceneDS = _frameDataBinding->newDescriptorSet(
            frameResources,
            viewMatrix,
            projMatrix
        );
        
        // TODO: Wrap the environment into an scene component
        _environment->updateSkybox(viewMatrix, projMatrix);
        
        if (_drawSkybox)
        {
            _environment->drawSkybox(cmd, currentFrame, frameResources);
        }

  
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
        
        auto envDS = _environmentDataBinding->newDescriptorSet(frameResources, _environment.get());
  
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
	void init(bg2e::render::Vulkan*, bg2e::ui::UserInterface*) override {
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
            
            for (auto & pair : _environments)
            {
                if (BasicWidgets::button(pair.first))
                {
                    loadEnvironment(pair.second);
                }
            }
		});
	}
    
    void loadEnvironment(const std::string& fileName)
    {
        auto assetsPath = bg2e::base::PlatformTools::assetPath();
        auto imagePath = assetsPath;
        imagePath.append(fileName);
        
        auto texture = bg2e::utils::TextureCache::get().load(_vulkan, imagePath);
        
        _environment->swapEnvironmentTexture(texture);
    }

	void cleanup() override
	{
        _colorAttachments->cleanup();
        bg2e::utils::TextureCache::destroy();
	}

protected:

    std::array<std::pair<const std::string, const std::string>, 8> _environments = {{
        { "Environment 1", "country_field_sun.jpg" },
        { "Environment 2", "equirectangular-env.jpg" },
        { "Environment 3", "equirectangular-env2.jpg" },
        { "Environment 4", "equirectangular-env3.jpg" },
        { "Environment 5", "equirectangular-env4.jpg" },
        { "Environment 6", "equirectangular-env5.jpg" },
        { "Environment 7", "equirectangular-env6.jpg" },
        { "Environment 8", "equirectangular-env7.jpg" }
    }};

    std::shared_ptr<bg2e::render::ColorAttachments> _colorAttachments;

	bg2e::ui::Window _window;

	VkPipelineLayout _layout = VK_NULL_HANDLE;
	VkPipeline _pipeline = VK_NULL_HANDLE;
 
    std::shared_ptr<bg2e::scene::Scene> _scene;
    
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
		bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_vulkan);

		plFactory.addShader("test/texture_gi.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        plFactory.addShader("test/texture_gi.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        
        plFactory.setInputState<bg2e::render::vulkan::geo::MeshPNU>();
    
        // Create object and environment descriptor set+
        auto frameDSLayout = _frameDataBinding->createLayout();
        auto objectDSLayout = _objectDataBinding->createLayout();
        auto envDSLayout = _environmentDataBinding->createLayout();
        
        bg2e::render::vulkan::factory::PipelineLayout layoutFactory(_vulkan);
        layoutFactory.addDescriptorSetLayout(frameDSLayout);
        layoutFactory.addDescriptorSetLayout(objectDSLayout);
        layoutFactory.addDescriptorSetLayout(envDSLayout);
        _layout = layoutFactory.build();
        
        plFactory.setDepthFormat(_vulkan->swapchain().depthImageFormat());
        plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
        plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        plFactory.setCullMode(true, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        
        plFactory.setColorAttachmentFormat(_colorAttachments->attachmentFormats());
		_pipeline = plFactory.build(_layout);
  
		_vulkan->cleanupManager().push([&, objectDSLayout, envDSLayout, frameDSLayout](VkDevice dev) {
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
        _resizeVisitor.resizeViewport(sceneRoot.get(), _vulkan->swapchain().extent());
        
        _scene = std::make_shared<bg2e::scene::Scene>();
        
        _scene->setSceneRoot(sceneRoot);
        
        _vulkan->cleanupManager().push([&](VkDevice) {
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
        drawable->material(0).setAlbedo(outerAlbedoTexture);
        drawable->material(1).setAlbedo(innerAlbedoTexture);
        drawable->load(_vulkan);
        
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
