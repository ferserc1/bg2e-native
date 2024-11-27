
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/Application.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/base/Log.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/vulkan/geo/VertexDescription.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/render/vulkan/macros/descriptor_set.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/geo/Mesh.hpp>
#include <bg2e/render/Texture.hpp>


#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Window.hpp>

#include <bg2e/db/image.hpp>
#include <bg2e/db/mesh_obj.hpp>

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
	}
 
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) override
    {
        frameAllocator->requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
        });
        frameAllocator->initPool();
    }
 
    void initScene() override
    {
        // Use the initScene function to initialize and create scene resources, such as pipelines, 3D models
        // or textures
        
        std::filesystem::path imagePath = bg2e::base::PlatformTools::assetPath().append("two_submeshes_inner_albedo.jpg");
        std::filesystem::path imagePath2 = bg2e::base::PlatformTools::assetPath().append("two_submeshes_outer_albedo.jpg");

		// You can use plain pointers in this case, because the base::Image and base::Texture objects will not
		// be used outside of this function. Internally, these objects will be stored in a shared_ptr and will be
		// managed by the render::Texture object.
		// But if you plan to use the objects more than once, you ALWAYS must to use a shared_ptr to share the pointer
		// between the Texture object and the rest of the application.
        auto image = bg2e::db::loadImage(imagePath);
		auto texture = new bg2e::base::Texture(image);
        texture->setMagFilter(bg2e::base::Texture::FilterLinear);
        texture->setMinFilter(bg2e::base::Texture::FilterLinear);

		_texture = std::shared_ptr<bg2e::render::Texture>(new bg2e::render::Texture(
			_vulkan,
			texture
		));
  
        auto image2 = bg2e::db::loadImage(imagePath2);
        auto texture2 = new bg2e::base::Texture(image2);
        texture2->setMagFilter(bg2e::base::Texture::FilterLinear);
        texture2->setMinFilter(bg2e::base::Texture::FilterLinear);
        
        _texture2 = std::shared_ptr<bg2e::render::Texture>(new bg2e::render::Texture(
            _vulkan,
            texture2
        ));
  
        _vulkan->cleanupManager().push([&](VkDevice) {
            _texture->cleanup();
            _texture2->cleanup();
        });
	
		createImage(_vulkan->swapchain().extent());

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

        _objectData.modelMatrix = glm::mat4{ 1.0f };
        
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
        
		Image::cmdTransitionImage(
			cmd,
			_targetImage->handle(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL
		);

		float flash = std::abs(std::sin(currentFrame / 120.0f));
		VkClearColorValue clearValue{ { 0.0f, 0.0f, flash, 1.0f } };
		auto clearRange = Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdClearColorImage(
			cmd,
			_targetImage->handle(),
			VK_IMAGE_LAYOUT_GENERAL,
			&clearValue, 1, &clearRange
		);

		Image::cmdTransitionImage(
			cmd, _targetImage->handle(),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		auto colorAttachment = Info::attachmentInfo(_targetImage->imageView(), nullptr);
        auto depthAttachment = Info::depthAttachmentInfo(depthImage->imageView());
        auto renderInfo = Info::renderingInfo(_targetImage->extent2D(), &colorAttachment, &depthAttachment);
		cmdBeginRendering(cmd, &renderInfo);

		macros::cmdSetDefaultViewportAndScissor(cmd, _targetImage->extent2D());

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

        
        // You can use macros::createBuffer to create a buffer, copy the data and manage the buffer allocation
        // inside a frame resource in one single function.
        // Vulkan objects as well as buffer stack memory and descriptor set memory are automatically managed by
        // frameResources.
        // Then, you can use frameResources.newDescriptorSet() to allocate a descriptor set and update it with
        // the buffer. The descriptor set memory will be also managed by the frameResources object.
        // Using this method you can manage descriptor sets formed by more than one uniform buffer.
        
        // In this example, we create an uniform buffer to store the object model matrix. Then, we create
        // a descriptor set for each submesh. The descriptor set contains the uniform buffer and the texture.
        // The uniform buffer with the model matrix is the same for every submeshes, but the texture can
        // be different.
        _objectData.modelMatrix = glm::rotate(_objectData.modelMatrix, 0.02f, glm::vec3(0.0f, 1.0f, 0.0f));
        auto objectDataBuffer = macros::createBuffer(_vulkan, frameResources, _objectData);
        
		for (uint32_t i = 0; i < _mesh->submeshCount(); ++i)
		{
            auto objectDS = frameResources.newDescriptorSet(_objectDSLayout);
            auto texture = i == 0 ? _texture2.get() : _texture.get();
            objectDS->beginUpdate();
                objectDS->addBuffer(
                    0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    objectDataBuffer, sizeof(ObjectData), 0
                );
                objectDS->addImage(
                    1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    texture->image()->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    _texture->sampler()
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
			_mesh->drawSubmesh(cmd, i);
		}

		// Use this function to draw one unique mesh including all the indexes
		// _mesh->draw(cmd);

		bg2e::render::vulkan::cmdEndRendering(cmd);

		Image::cmdTransitionImage(
			cmd,
			_targetImage->handle(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		);
		Image::cmdTransitionImage(
			cmd,
			colorImage->handle(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		Image::cmdCopy(
			cmd,
			_targetImage->handle(), _targetImage->extent2D(),
			colorImage->handle(), colorImage->extent2D()
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
 
	std::unique_ptr<bg2e::render::vulkan::geo::MeshPU> _mesh;
 
	std::shared_ptr<bg2e::render::Texture> _texture;
    std::shared_ptr<bg2e::render::Texture> _texture2;
    
    VkDescriptorSetLayout _sceneDSLayout;
    VkDescriptorSetLayout _objectDSLayout;
    
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
    ObjectData _objectData;

	void createPipeline()
	{
		bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_vulkan);

		plFactory.addShader("test/texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		plFactory.addShader("test/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		auto bindingDescription = bg2e::render::vulkan::geo::bindingDescription<bg2e::geo::VertexPU>();
		auto attributeDescriptions = bg2e::render::vulkan::geo::attributeDescriptions<bg2e::geo::VertexPU>();

		plFactory.vertexInputState.vertexBindingDescriptionCount = 1;
		plFactory.vertexInputState.pVertexBindingDescriptions = &bindingDescription;
		plFactory.vertexInputState.vertexAttributeDescriptionCount = uint32_t(attributeDescriptions.size());
		plFactory.vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
  
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;
        
        dsFactory.clear();
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _sceneDSLayout = dsFactory.build(_vulkan->device().handle(), VK_SHADER_STAGE_VERTEX_BIT);
        
        dsFactory.clear();
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _objectDSLayout = dsFactory.build(_vulkan->device().handle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        

		auto layoutInfo = bg2e::render::vulkan::Info::pipelineLayoutInfo();
        VkDescriptorSetLayout setLayouts[] = {
            _sceneDSLayout,
            _objectDSLayout
        };
        layoutInfo.pSetLayouts = setLayouts;
        layoutInfo.setLayoutCount = 2;
		VK_ASSERT(vkCreatePipelineLayout(_vulkan->device().handle(), &layoutInfo, nullptr, &_layout));
        
        plFactory.setDepthFormat(_vulkan->swapchain().depthImageFormat());
        plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
        plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        plFactory.setCullMode(true, VK_FRONT_FACE_CLOCKWISE);
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
            bg2e::db::loadMeshObj<bg2e::geo::MeshPU>(bg2e::base::PlatformTools::assetPath().append("two_submeshes.obj"))
        );

        _mesh = std::unique_ptr<bg2e::render::vulkan::geo::MeshPU>(new bg2e::render::vulkan::geo::MeshPU(_vulkan));
        _mesh->setMeshData(mesh.get());

		_mesh->build();

		_vulkan->cleanupManager().push([this](VkDevice dev) {
			_mesh->cleanup();
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
