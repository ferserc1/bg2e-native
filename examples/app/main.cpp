
#include <bg2e.hpp>

#include <array>
#include <numbers>

class ClearScreenDelegate : public bg2e::render::RenderLoopDelegate,
	public bg2e::app::InputDelegate,
	public bg2e::ui::UserInterfaceDelegate
{
public:
	void init(bg2e::render::Vulkan* vulkan) override
	{
		using namespace bg2e::render::vulkan;
		RenderLoopDelegate::init(vulkan);
  
        _colorAttachments = std::shared_ptr<bg2e::render::ColorAttachments>(
            new bg2e::render::ColorAttachments(_vulkan, {
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_FORMAT_R8G8B8A8_UNORM
            })
        );
        
        _frameDataBinding = std::make_unique<bg2e::scene::vk::FrameDataBinding>(vulkan);
        
        _objectDataBinding = std::make_unique<bg2e::scene::vk::ObjectDataBinding>(vulkan);
        
        _environmentDataBinding = std::make_unique<bg2e::scene::vk::EnvironmentDataBinding>(vulkan);
        
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
        
        auto envTexture = bg2e::utils::TextureCache::get().load(_vulkan, imagePath);
        
        _environment->build(
            envTexture,         // Equirectangular texture
            { 2048, 2048 },     // Cube map size
            { 32, 32 },         // Irradiance map size
            { 1024, 1024 }      // Specular reflection map size
        );
	
        _colorAttachments->build(_vulkan->swapchain().extent());

		createPipeline();

        _viewMatrix = glm::lookAt(glm::vec3{ 0.0f, 0.0f, -5.0f}, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
        
        auto vpSize = _vulkan->swapchain().extent();
        _projMatrix = glm::perspective(
            glm::radians(50.0f),
            float(vpSize.width) / float(vpSize.height),
            0.1f, 40.0f
        );
        _projMatrix[1][1] *= -1.0f;
        
        _modelMatrix = glm::mat4{ 1.0f };
        
		createVertexData();
    }

	void swapchainResized(VkExtent2D newExtent) override
	{
        // This function releases all previous resources before recreate the images
		_colorAttachments->build(newExtent);
  
        _projMatrix = glm::perspective(
            glm::radians(50.0f),
            float(newExtent.width) / float(newExtent.height),
            0.1f, 40.0f
        );
        _projMatrix[1][1] *= -1.0f;
	}

	VkImageLayout render(
		VkCommandBuffer cmd,
		uint32_t currentFrame,
		const bg2e::render::vulkan::Image* colorImage,
		const bg2e::render::vulkan::Image* depthImage,
		bg2e::render::vulkan::FrameResources& frameResources
	) override {
		using namespace bg2e::render::vulkan;
  
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
  
        // Rotate the view along Y axis
        _viewMatrix = glm::rotate(_viewMatrix, 0.02f, glm::vec3(0.0f, 1.0f, 0.0f));
        
        auto sceneDS = _frameDataBinding->newDescriptorSet(
            frameResources,
            _viewMatrix,
            _projMatrix
        );
        
        _environment->updateSkybox(_viewMatrix, _projMatrix);
        
        if (_drawSkybox)
        {
            _environment->drawSkybox(cmd, currentFrame, frameResources);
        }

  
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
        
        auto envDS = _environmentDataBinding->newDescriptorSet(frameResources, _environment.get());
  
        _drawable->draw(
            cmd,
            _layout,
            [&](bg2e::render::MaterialBase * mat, const glm::mat4& transform, uint32_t submesh) {
                auto modelDS = _objectDataBinding->newDescriptorSet(
                    frameResources,
                    mat,
                    _modelMatrix * transform
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
			BasicWidgets::text("Hello, world!");
   
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

	VkPipelineLayout _layout;
	VkPipeline _pipeline;
 
    glm::mat4 _modelMatrix;
    std::unique_ptr<bg2e::scene::DrawablePNU> _drawable;
    
    std::unique_ptr<bg2e::scene::vk::ObjectDataBinding> _objectDataBinding;

    // Load the environment skybox, generate the irradiance and specular reflection maps,
    // and manage the sky box renderer
    std::unique_ptr<bg2e::render::EnvironmentResources> _environment;
    std::unique_ptr<bg2e::scene::vk::EnvironmentDataBinding> _environmentDataBinding;
    
    bool _drawSkybox = true;
    int _showRenderTargetIndex = 0;
      
    glm::mat4 _viewMatrix;
    glm::mat4 _projMatrix;
    std::unique_ptr<bg2e::scene::vk::FrameDataBinding> _frameDataBinding;

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

	void createVertexData()
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
        
        _drawable = std::make_unique<bg2e::scene::DrawablePNU>();
        _drawable->setMesh(bg2e::db::loadMeshObj<bg2e::geo::MeshPNU>(modelPath));
        
        _drawable->material(0).setAlbedo(outerAlbedoTexture);
        _drawable->material(1).setAlbedo(innerAlbedoTexture);
        _drawable->load(_vulkan);
        
        _vulkan->cleanupManager().push([&](VkDevice) {
            _drawable.reset();
        });
        
        auto modelMesh = std::unique_ptr<bg2e::geo::MeshPNU>(
            bg2e::db::loadMeshObj<bg2e::geo::MeshPNU>(modelPath)
        );
	}
};

class MyApplication : public bg2e::app::Application {
public:
	void init(int argc, char** argv) override
	{
		auto delegate = std::shared_ptr<ClearScreenDelegate>(new ClearScreenDelegate());
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
