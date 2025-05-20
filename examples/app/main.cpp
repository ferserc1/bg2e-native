
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
  
        // If you need to use the main descriptor set allocator, add all the required pool size ratios
        // in the init function.
        vulkan->descriptorSetAllocator().requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
        });
        
        _environment = std::unique_ptr<bg2e::render::EnvironmentResources>(
            new bg2e::render::EnvironmentResources(
                _vulkan,

                // Use this parameters to build the SkyboxRenderer in EnvironmentResources
                _colorAttachments->attachmentFormats(),
                _vulkan->swapchain().depthImageFormat()
            )
        );
	}
 
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) override
    {
        frameAllocator->requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 }
        });
        
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
        
        _environment->build(
            imagePath,          // Path to equirectangular image
            { 2048, 2048 },     // Cube map size
            { 32, 32 },         // Irradiance map size
            { 1024, 1024 }      // Specular reflection map size
        );
    
		// You can use plain pointers in this case, because the base::Image and base::Texture objects will not
		// be used outside of this function. Internally, these objects will be stored in a shared_ptr and will be
		// managed by the render::Texture object.
		// But if you plan to use the objects more than once, you ALWAYS must to use a shared_ptr to share the pointer
		// between the Texture object and the rest of the application.
        auto cubePath = assetsPath;
        cubePath.append("logo_2a.png");

        auto image = bg2e::db::loadImage(cubePath);
        auto cubeTexture = new bg2e::base::Texture(image);
        cubeTexture->setMagFilter(bg2e::base::Texture::FilterLinear);
        cubeTexture->setMinFilter(bg2e::base::Texture::FilterLinear);
        cubeTexture->setUseMipmaps(true);
        
        _cubeTexture = std::shared_ptr<bg2e::render::Texture>(new bg2e::render::Texture(
            _vulkan,
            cubeTexture
        ));
        
        _vulkan->cleanupManager().push([&](VkDevice) {
            _cubeTexture = nullptr;
        });
	
        _colorAttachments->build(_vulkan->swapchain().extent());

		createPipeline();

        _sceneData.viewMatrix = glm::lookAt(glm::vec3{ 0.0f, 0.0f, -5.0f}, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
        
        auto vpSize = _vulkan->swapchain().extent();
        _sceneData.projMatrix = glm::perspective(
            glm::radians(50.0f),
            float(vpSize.width) / float(vpSize.height),
            0.1f, 40.0f
        );
        _sceneData.projMatrix[1][1] *= -1.0f;

        _modelData.modelMatrix = glm::mat4{ 1.0f };
        
		createVertexData();
    }

	void swapchainResized(VkExtent2D newExtent) override
	{
        // This function releases all previous resources before recreate the images
		_colorAttachments->build(newExtent);
  
        _sceneData.projMatrix = glm::perspective(
            glm::radians(50.0f),
            float(newExtent.width) / float(newExtent.height),
            0.1f, 40.0f
        );
        _sceneData.projMatrix[1][1] *= -1.0f;	
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
  
        // You can use this function when a descriptor set only contains one unique uniform buffer.
        // The uniformBufferDescriptorSet function automatically create the uniform buffer and descriptor set
        // inside the FrameResources object, upload the buffer data and update the descriptor set in one single
        // call. This function will also manage the allocated Vulkan memory and the heap memory. The descriptor
        // set object memory will be managed by the frameResources, so you don't need to delete it or manage
        // using a smart pointer. The object will be removed when the frame rendering is done.
        // You can save the pointer to the set descriptor and use it safely in other functions, as long as
        // they are used within the same frame.
        auto sceneDS = macros::uniformBufferDescriptorSet(
            _vulkan, frameResources,
            _sceneDSLayout, _sceneData, currentFrame
        );
        
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
        _sceneData.viewMatrix = glm::rotate(_sceneData.viewMatrix, 0.02f, glm::vec3(0.0f, 1.0f, 0.0f));

        // Draw skybox. This functions will only generate a skybox if the skybox renderer
        // is initialized in _environment. To do it, call de EnvironmentResources constructor
        // with the target image and depth format
        _environment->updateSkybox(_sceneData.viewMatrix, _sceneData.projMatrix);
        
        if (_drawSkybox)
        {
            _environment->drawSkybox(cmd, currentFrame, frameResources);
        }

  
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
  
        _modelData.modelMatrix = glm::rotate(_modelData.modelMatrix, 0.018f, glm::vec3(1.0f, 1.0f, 0.0f));
        auto modelDataBuffer = macros::createBuffer(_vulkan, frameResources, _modelData);
        auto modelDS = frameResources.newDescriptorSet(_objectDSLayout);
        modelDS->beginUpdate();
            modelDS->addBuffer(
                0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                modelDataBuffer, sizeof(ObjectData), 0
            );
            modelDS->addImage(
                1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _modelMaterial->albedoTexture()
            );
            modelDS->addImage(
                2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                _environment->irradianceMapImage()->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _environment->irradianceMapSampler()
            );
        modelDS->endUpdate();
        std::array<VkDescriptorSet, 2> objectSets = {
            sceneDS->descriptorSet(),
            modelDS->descriptorSet()
        };
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _layout, 0,
            uint32_t(objectSets.size()),
            objectSets.data(),
            0, nullptr
        );
        _model->drawSubmesh(cmd, 1);
        
        auto modelDS2 = frameResources.newDescriptorSet(_objectDSLayout);
        modelDS2->beginUpdate();
            modelDS2->addBuffer(
                0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                modelDataBuffer, sizeof(ObjectData), 0
            );
            modelDS2->addImage(
                1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _modelMaterial2->albedoTexture()
            );
            modelDS2->addImage(
                2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                _environment->irradianceMapImage()->imageView(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _environment->irradianceMapSampler()
            );
        modelDS2->endUpdate();
        std::array<VkDescriptorSet, 2> objectSets2 = {
            sceneDS->descriptorSet(),
            modelDS2->descriptorSet()
        };
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _layout, 0,
            uint32_t(objectSets2.size()),
            objectSets2.data(),
            0, nullptr
        );
        _model->drawSubmesh(cmd, 0);

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
            
            if (BasicWidgets::button("Environment 1"))
            {
                loadEnvironment1();
            }
            
            if (BasicWidgets::button("Environment 2"))
            {
                loadEnvironment2();
            }
            
            if (BasicWidgets::button("Environment 3"))
            {
                loadEnvironment3();
            }
            
            if (BasicWidgets::button("Environment 4"))
            {
                loadEnvironment4();
            }
            
            if (BasicWidgets::button("Environment 5"))
            {
                loadEnvironment5();
            }
            
            if (BasicWidgets::button("Environment 6"))
            {
                loadEnvironment6();
            }
            
            if (BasicWidgets::button("Environment 7"))
            {
                loadEnvironment7();
            }
            
            if (BasicWidgets::button("Environment 8"))
            {
                loadEnvironment8();
            }
		});
	}
 
    void loadEnvironment1()
    {
        loadEnvironment("country_field_sun.jpg");
    }
    
    void loadEnvironment2()
    {
        loadEnvironment("equirectangular-env.jpg");
    }
    
    void loadEnvironment3()
    {
        loadEnvironment("equirectangular-env2.jpg");
    }
    
    void loadEnvironment4()
    {
        loadEnvironment("equirectangular-env3.jpg");
    }
    
    void loadEnvironment5()
    {
        loadEnvironment("equirectangular-env4.jpg");
    }
    
    void loadEnvironment6()
    {
        loadEnvironment("equirectangular-env5.jpg");
    }
    
    void loadEnvironment7()
    {
        loadEnvironment("equirectangular-env6.jpg");
    }
    
    void loadEnvironment8()
    {
        loadEnvironment("equirectangular-env7.jpg");
    }
    
    void loadEnvironment(const std::string& fileName)
    {
        auto assetsPath = bg2e::base::PlatformTools::assetPath();
        auto imagePath = assetsPath;
        imagePath.append(fileName);
        _environment->swapEnvironmentTexture(imagePath);
    }

	void cleanup() override
	{
        _colorAttachments->cleanup();
	}

protected:
    std::shared_ptr<bg2e::render::ColorAttachments> _colorAttachments;

	bg2e::ui::Window _window;

	VkPipelineLayout _layout;
	VkPipeline _pipeline;
 
    std::unique_ptr<bg2e::render::vulkan::geo::MeshPNU> _model;
    std::shared_ptr<bg2e::render::MaterialBase> _modelMaterial;
    std::shared_ptr<bg2e::render::MaterialBase> _modelMaterial2;
    
    std::shared_ptr<bg2e::render::Texture> _cubeTexture;

    VkDescriptorSetLayout _sceneDSLayout;
    VkDescriptorSetLayout _objectDSLayout;

    // Load the environment skybox, generate the irradiance and specular reflection maps,
    // and manage the sky box renderer
    std::unique_ptr<bg2e::render::EnvironmentResources> _environment;
    
    bool _drawSkybox = true;
    int _showRenderTargetIndex = 0;
        
    struct SceneData
    {
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
    };
    
    SceneData _sceneData;
    
    struct ObjectData
    {
        glm::mat4 modelMatrix;
    };
    
    ObjectData _modelData;

	void createPipeline()
	{
		bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_vulkan);

		plFactory.addShader("test/texture_gi.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		// plFactory.addShader("test/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        plFactory.addShader("test/texture_gi.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        
        plFactory.setInputState<bg2e::render::vulkan::geo::MeshPNU>();
  
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;
        
        dsFactory.clear();
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _sceneDSLayout = dsFactory.build(_vulkan->device().handle(), VK_SHADER_STAGE_VERTEX_BIT);
        
        dsFactory.clear();
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _objectDSLayout = dsFactory.build(_vulkan->device().handle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        
        bg2e::render::vulkan::factory::PipelineLayout layoutFactory(_vulkan);
        layoutFactory.addDescriptorSetLayout(_sceneDSLayout);
        layoutFactory.addDescriptorSetLayout(_objectDSLayout);
        _layout = layoutFactory.build();
        
        plFactory.setDepthFormat(_vulkan->swapchain().depthImageFormat());
        plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
        plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        plFactory.setCullMode(true, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        
        plFactory.setColorAttachmentFormat(_colorAttachments->attachmentFormats());
		_pipeline = plFactory.build(_layout);
  
		_vulkan->cleanupManager().push([&](VkDevice dev) {
			vkDestroyPipeline(dev, _pipeline, nullptr);
			vkDestroyPipelineLayout(dev, _layout, nullptr);
            vkDestroyDescriptorSetLayout(dev, _sceneDSLayout, nullptr);
            vkDestroyDescriptorSetLayout(dev, _objectDSLayout, nullptr);
		});
	}

	void createVertexData()
	{
		using namespace bg2e::render::vulkan;
  
        std::filesystem::path modelPath = bg2e::base::PlatformTools::assetPath();
        modelPath.append("two_submeshes.obj");
        
        auto modelMesh = std::unique_ptr<bg2e::geo::MeshPNU>(
            bg2e::db::loadMeshObj<bg2e::geo::MeshPNU>(modelPath)
        );
        
        _model = std::unique_ptr<bg2e::render::vulkan::geo::MeshPNU>(new bg2e::render::vulkan::geo::MeshPNU(_vulkan));
        _model->setMeshData(modelMesh.get());
        _model->build();
        
        auto modelTexture = std::shared_ptr<bg2e::base::Texture>(
            bg2e::db::loadImageAsTexture(bg2e::base::PlatformTools::assetPath(), "two_submeshes_inner_albedo.jpg")
        );
        
        _modelMaterial = std::make_shared<bg2e::render::MaterialBase>(_vulkan);
        _modelMaterial->materialAttributes().setAlbedo(modelTexture);
        
        // Call this function every time you change something in materialAttributes
        _modelMaterial->update();
        
        auto modelTexture2 = std::shared_ptr<bg2e::base::Texture>(
            bg2e::db::loadImageAsTexture(bg2e::base::PlatformTools::assetPath(), "two_submeshes_outer_albedo.jpg")
        );
        
        _modelMaterial2 = std::make_shared<bg2e::render::MaterialBase>(_vulkan);
        _modelMaterial2->materialAttributes().setAlbedo(modelTexture2);
        
        _modelMaterial2->update();
        
		_vulkan->cleanupManager().push([this](VkDevice dev) {
            _model.reset();
            _modelMaterial.reset();
            _modelMaterial2.reset();
  		});
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
