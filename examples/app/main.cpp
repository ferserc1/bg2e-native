
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/Application.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/base/Log.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/vulkan/geo/VertexDescription.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/render/vulkan/macros/descriptor_set.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/geo/Mesh.hpp>
#include <bg2e/geo/sphere.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/geo/cylinder.hpp>
#include <bg2e/geo/plane.hpp>
#include <bg2e/render/Texture.hpp>
#include <bg2e/geo/modifiers.hpp>
#include <bg2e/render/SphereToCubemapRenderer.hpp>
#include <bg2e/render/SkyboxRenderer.hpp>
#include <bg2e/render/IrradianceCubemapRenderer.hpp>
#include <bg2e/render/SpecularReflectionCubemapRenderer.hpp>


#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Window.hpp>

#include <bg2e/db/image.hpp>
#include <bg2e/db/mesh_obj.hpp>

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
  
        // If you need to use the main descriptor set allocator, add all the required pool size ratios
        // in the init function.
        vulkan->descriptorSetAllocator().requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
        });
        
        _sphereToCubemap = std::unique_ptr<bg2e::render::SphereToCubemapRenderer>(
            new bg2e::render::SphereToCubemapRenderer(_vulkan)
        );
        
        _skyboxRenderer = std::unique_ptr<bg2e::render::SkyboxRenderer>(
            new bg2e::render::SkyboxRenderer(_vulkan)
        );
	}
 
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) override
    {
        frameAllocator->requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
        });
        
        _sphereToCubemap->initFrameResources(frameAllocator);
        
        _skyboxRenderer->initFrameResources(frameAllocator);
        
        _irradianceMapRenderer->initFrameResources(frameAllocator);
        _specularReflectionMapRenderer->initFrameResources(frameAllocator);
        
        frameAllocator->initPool();
    }
 
    void initScene() override
    {
        // Use the initScene function to initialize and create scene resources, such as pipelines, 3D models
        // or textures
        
        
        auto assetsPath = bg2e::base::PlatformTools::assetPath();
        auto imagePath = assetsPath;
        imagePath.append("country_field_sun.jpg");
        
        _sphereToCubemap->build(imagePath);
        _updateCubemap = true;
        
        _irradianceMapRenderer = std::unique_ptr<bg2e::render::IrradianceCubemapRenderer>(new bg2e::render::IrradianceCubemapRenderer(_vulkan));
        _irradianceMapRenderer->build(_sphereToCubemap->cubeMapImage());
        
        _specularReflectionMapRenderer = std::unique_ptr<bg2e::render::SpecularReflectionCubemapRenderer>(new bg2e::render::SpecularReflectionCubemapRenderer(_vulkan));
        _specularReflectionMapRenderer->build(_sphereToCubemap->cubeMapImage());
        
    
		// You can use plain pointers in this case, because the base::Image and base::Texture objects will not
		// be used outside of this function. Internally, these objects will be stored in a shared_ptr and will be
		// managed by the render::Texture object.
		// But if you plan to use the objects more than once, you ALWAYS must to use a shared_ptr to share the pointer
		// between the Texture object and the rest of the application.
        auto image = bg2e::db::loadImage(imagePath);
		auto texture = new bg2e::base::Texture(image);
        texture->setMagFilter(bg2e::base::Texture::FilterLinear);
        texture->setMinFilter(bg2e::base::Texture::FilterLinear);
        texture->setUseMipmaps(true);

		_texture = std::shared_ptr<bg2e::render::Texture>(new bg2e::render::Texture(
			_vulkan,
			texture
		));
 
        auto cubePath = assetsPath;
        cubePath.append("logo_2a.png");

        image = bg2e::db::loadImage(cubePath);
        auto cubeTexture = new bg2e::base::Texture(image);
        cubeTexture->setMagFilter(bg2e::base::Texture::FilterLinear);
        cubeTexture->setMinFilter(bg2e::base::Texture::FilterLinear);
        cubeTexture->setUseMipmaps(true);
        
        _cubeTexture = std::shared_ptr<bg2e::render::Texture>(new bg2e::render::Texture(
            _vulkan,
            cubeTexture
        ));
        
        _vulkan->cleanupManager().push([&](VkDevice) {
            _texture->cleanup();
            _cubeTexture->cleanup();
        });
	
		createImage(_vulkan->swapchain().extent());
  
        // Create the skybox renderer after the target image, because we need access to the
        // target image format to initialize the skybox renderer
        auto cubeMapTexture = std::shared_ptr<bg2e::render::Texture>(
            new bg2e::render::Texture(_vulkan, _sphereToCubemap->cubeMapImage())
        );
        _skyboxRenderer->build(cubeMapTexture, _targetImage->format(), _vulkan->swapchain().depthImageFormat());
        
        _vulkan->cleanupManager().push([&, cubeMapTexture](VkDevice) {
            _sphereToCubemap->cleanup();
            cubeMapTexture->cleanup();
            _skyboxRenderer->cleanup();
        });

		createPipeline();

        _sceneData.viewMatrix = glm::lookAt(glm::vec3{ 0.0f, 0.0f, -5.0f}, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
        
        auto vpSize = _vulkan->swapchain().extent();
        _sceneData.projMatrix = glm::perspective(
            glm::radians(50.0f),
            float(vpSize.width) / float(vpSize.height),
            0.1f, 40.0f
        );
        _sceneData.projMatrix[1][1] *= -1.0f;
        _sceneData.projMatrix[0][0] *= -1.0f;

        _skyData.modelMatrix = glm::mat4{ 1.0f };
        _cubeData.modelMatrix = glm::mat4{ 1.0f };
        
		createVertexData();
    }

	void swapchainResized(VkExtent2D newExtent) override
	{
		_targetImage->cleanup();
		createImage(newExtent);
  
        _sceneData.projMatrix = glm::perspective(
            glm::radians(50.0f),
            float(newExtent.width) / float(newExtent.height),
            0.1f, 40.0f
        );
        _sceneData.projMatrix[1][1] *= -1.0f;
        _sceneData.projMatrix[0][0] *= -1.0f;
	}

	VkImageLayout render(
		VkCommandBuffer cmd,
		uint32_t currentFrame,
		const bg2e::render::vulkan::Image* colorImage,
		const bg2e::render::vulkan::Image* depthImage,
		bg2e::render::vulkan::FrameResources& frameResources
	) override {
		using namespace bg2e::render::vulkan;
  
        if (_updateCubemap) {
            _sphereToCubemap->update(cmd, frameResources);
            
            _irradianceMapRenderer->update(cmd, currentFrame, frameResources);
            
            _specularReflectionMapRenderer->update(cmd, currentFrame, frameResources);
            
            _updateCubemap = false;
        }
        
  
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
        macros::cmdClearImageAndBeginRendering(
            cmd,
            _targetImage.get(), clearValue, VK_IMAGE_LAYOUT_UNDEFINED,
            depthImage, 1.0f
        );
        
		macros::cmdSetDefaultViewportAndScissor(cmd, _targetImage->extent2D());
  
        // Rotate the view along Y axis
        _sceneData.viewMatrix = glm::rotate(_sceneData.viewMatrix, 0.02f, glm::vec3(0.0f, 1.0f, 0.0f));
        
        _skyboxRenderer->update(_sceneData.viewMatrix, _sceneData.projMatrix);
        _skyboxRenderer->draw(cmd, currentFrame, frameResources);
  
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
  
        // Draw the cube
        _cubeData.modelMatrix = glm::rotate(_cubeData.modelMatrix, -0.025f, glm::vec3(1.0f, 1.0f, 0.0f));
        auto cubeDataBuffer = macros::createBuffer(_vulkan, frameResources, _cubeData);
        
        auto objectDS = frameResources.newDescriptorSet(_objectDSLayout);
        objectDS->beginUpdate();
            objectDS->addBuffer(
                0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                cubeDataBuffer, sizeof(ObjectData), 0
            );
            objectDS->addImage(
                1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                _cubeTexture->image()->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _cubeTexture->sampler()
            );
        objectDS->endUpdate();
        
        std::array<VkDescriptorSet, 2> sets = {
            sceneDS->descriptorSet(),
            objectDS->descriptorSet()
        };
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _layout, 0,
            uint32_t(sets.size()),
            sets.data(),
            0, nullptr
        );
        _cube->draw(cmd);
        
        _cylinderRotation += 0.015f;
        if (_cylinderRotation >= std::numbers::pi_v<float> * 2.0f)
        {
            _cylinderRotation = 0.0f;
        }
        _planeData.modelMatrix = glm::translate(glm::mat4{ 1.0f }, glm::vec3(2.0f, 1.5 * std::sin(0.01f * float(currentFrame)), 0.0f));
        _planeData.modelMatrix = glm::rotate(_planeData.modelMatrix, _cylinderRotation, glm::vec3(0.0f, 1.0f, 0.0f));
        auto planeDataBuffer = macros::createBuffer(_vulkan, frameResources, _planeData);
        
        auto planeDS = frameResources.newDescriptorSet(_objectDSLayout);
        planeDS->beginUpdate();
            planeDS->addBuffer(
                0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                planeDataBuffer, sizeof(ObjectData), 0
            );
            planeDS->addImage(
                1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
              _cubeTexture->image()->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              _cubeTexture->sampler()
            );
        planeDS->endUpdate();
        
        std::array<VkDescriptorSet, 2> planeSets = {
            sceneDS->descriptorSet(),
            planeDS->descriptorSet()
        };
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _layout,
            0,
            uint32_t(planeSets.size()),
            planeSets.data(),
            0, nullptr
        );
        _plane->draw(cmd);

		bg2e::render::vulkan::cmdEndRendering(cmd);

		Image::cmdCopy(
			cmd,
            _targetImage->handle(), _targetImage->extent2D(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            colorImage->handle(), colorImage->extent2D(), VK_IMAGE_LAYOUT_UNDEFINED
		);

		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}

	// ============ User Interface Delegate Functions =========
	void init(bg2e::render::Vulkan*, bg2e::ui::UserInterface*) override {
		_window.setTitle("ImGui Wrapper Demo");
		_window.options.noClose = true;
		_window.options.minWidth = 100;
		_window.options.minHeight = 50;
		_window.setPosition(0, 0);
		_window.setSize(150, 90);
	}

	void drawUI() override
	{
		using namespace bg2e::ui;
		_window.draw([]() {
			BasicWidgets::text("Hello, world!");
		});
	}

	void cleanup() override
	{
		_targetImage->cleanup();
	}

protected:
	std::shared_ptr<bg2e::render::vulkan::Image> _targetImage;

	bg2e::ui::Window _window;

	VkPipelineLayout _layout;
	VkPipeline _pipeline;
 
    std::unique_ptr<bg2e::render::vulkan::geo::MeshPU> _cube;
    std::unique_ptr<bg2e::render::vulkan::geo::MeshPU> _plane;

	std::shared_ptr<bg2e::render::Texture> _texture;
    std::shared_ptr<bg2e::render::Texture> _cubeTexture;

    VkDescriptorSetLayout _sceneDSLayout;
    VkDescriptorSetLayout _objectDSLayout;
    
    std::unique_ptr<bg2e::render::SphereToCubemapRenderer> _sphereToCubemap;
    bool _updateCubemap;
    std::unique_ptr<bg2e::render::SkyboxRenderer> _skyboxRenderer;
    
    std::unique_ptr<bg2e::render::IrradianceCubemapRenderer> _irradianceMapRenderer;
    std::unique_ptr<bg2e::render::SpecularReflectionCubemapRenderer> _specularReflectionMapRenderer;
    
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
    ObjectData _skyData;
    ObjectData _cubeData;
    ObjectData _planeData;
    float _cylinderRotation = 0.0f;

	void createPipeline()
	{
		bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_vulkan);

		plFactory.addShader("test/texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		plFactory.addShader("test/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        //plFactory.setInputBindingDescription(bg2e::render::vulkan::geo::MeshPU::bindingDescription());
        //plFactory.setInputAttributeDescriptions(bg2e::render::vulkan::geo::MeshPU::attributeDescriptions());
        plFactory.setInputState<bg2e::render::vulkan::geo::MeshPU>();
  
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;
        
        dsFactory.clear();
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _sceneDSLayout = dsFactory.build(_vulkan->device().handle(), VK_SHADER_STAGE_VERTEX_BIT);
        
        dsFactory.clear();
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _objectDSLayout = dsFactory.build(_vulkan->device().handle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        
        bg2e::render::vulkan::factory::PipelineLayout layoutFactory(_vulkan);
        layoutFactory.addDescriptorSetLayout(_sceneDSLayout);
        layoutFactory.addDescriptorSetLayout(_objectDSLayout);
        _layout = layoutFactory.build();
        
        plFactory.setDepthFormat(_vulkan->swapchain().depthImageFormat());
        plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
        plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        plFactory.setCullMode(true, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		plFactory.setColorAttachmentFormat(_targetImage->format());
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
  
        auto mesh = std::unique_ptr<bg2e::geo::MeshPU>(
            bg2e::geo::createCubePU(1.0f, 1.0f, 1.0f)
        );
        
        _cube = std::unique_ptr<bg2e::render::vulkan::geo::MeshPU>(new bg2e::render::vulkan::geo::MeshPU(_vulkan));
        _cube->setMeshData(mesh.get());
        _cube->build();
        
        mesh = std::unique_ptr<bg2e::geo::MeshPU>(
            //bg2e::geo::createPlanePU(5.0f, 5.0f, false)
            bg2e::geo::createCylinderPU(0.5f, 1.0f, 14, false)
        );
        
        _plane = std::unique_ptr<bg2e::render::vulkan::geo::MeshPU>(new bg2e::render::vulkan::geo::MeshPU(_vulkan));
        _plane->setMeshData(mesh.get());
        _plane->build();
        _planeData.modelMatrix = glm::translate(glm::mat4{ 1.0f }, glm::vec3(2.0f, 0.0f, 0.0f));

		_vulkan->cleanupManager().push([this](VkDevice dev) {
            _cube->cleanup();
            _plane->cleanup();
		});
	}

	void createImage(VkExtent2D extent)
	{
		using namespace bg2e::render::vulkan;
		auto vulkan = this->vulkan();
		_targetImage = std::shared_ptr<Image>(Image::createAllocatedImage(
			vulkan,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			extent,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT
		));
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
