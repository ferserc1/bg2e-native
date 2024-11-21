
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
#include <bg2e/render/vulkan/geo/Mesh.hpp>
#include <bg2e/render/Texture.hpp>

#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Window.hpp>

#include <bg2e/db/image.hpp>

class ClearScreenDelegate : public bg2e::render::RenderLoopDelegate,
	public bg2e::app::InputDelegate,
	public bg2e::ui::UserInterfaceDelegate
{
public:
	void init(bg2e::render::Vulkan* vulkan) override
	{
		using namespace bg2e::render::vulkan;
		RenderLoopDelegate::init(vulkan);
  
		std::filesystem::path imagePath = bg2e::base::PlatformTools::assetPath().append("country_field_sun.jpg");

		// You can use plain pointers in this case, because the base::Image and base::Texture objects will not
		// be used outside of this function. Internally, these objects will be stored in a shared_ptr and will be
		// managed by the Texture object.
		// But if you plan to use the objects more than once, you ALWAYS must to use a shared_ptr to share the pointer
		// between the Texture object and the rest of the application.
        auto image = bg2e::db::loadImage(imagePath);
		auto texture = new bg2e::base::Texture(image);
        texture->setAddressMode(bg2e::base::Texture::AddressModeRepeat);
        texture->setMagFilter(bg2e::base::Texture::FilterLinear);
        texture->setMinFilter(bg2e::base::Texture::FilterLinear);

		_texture = std::shared_ptr<bg2e::render::Texture>(new bg2e::render::Texture(
			vulkan,
			texture
		));
  
        vulkan->cleanupManager().push([&](VkDevice) {
            _texture->cleanup();
        });
	
		createImage(vulkan->swapchain().extent());
  
        _dsAllocator = std::unique_ptr<bg2e::render::vulkan::DescriptorSetAllocator>(
            new bg2e::render::vulkan::DescriptorSetAllocator()
        );
        _dsAllocator->init(vulkan);
        _dsAllocator->initPool(1, {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
        });
        vulkan->cleanupManager().push([&](VkDevice) {
            _dsAllocator->clearDescriptors();
            _dsAllocator->destroy();
        });

		createPipeline();

		createVertexData();
	}

	void swapchainResized(VkExtent2D newExtent) override
	{
		_targetImage->cleanup();
		createImage(newExtent);
	}

	VkImageLayout render(
		VkCommandBuffer cmd,
		uint32_t currentFrame,
		const bg2e::render::vulkan::Image* colorImage,
		const bg2e::render::vulkan::Image* depthImage,
		bg2e::render::vulkan::FrameResources& frameResources
	) override {
		using namespace bg2e::render::vulkan;

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
		auto renderInfo = Info::renderingInfo(_targetImage->extent2D(), &colorAttachment, nullptr);
		cmdBeginRendering(cmd, &renderInfo);

		macros::cmdSetDefaultViewportAndScissor(cmd, _targetImage->extent2D());

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

        VkDescriptorSet sets[] = {
            _textureDS->descriptorSet()
        };
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _layout, 0,
            1,
            sets,
            0, nullptr
        );
		for (uint32_t i = 0; i < _mesh->submeshCount(); ++i)
		{
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
 
    std::unique_ptr<bg2e::render::vulkan::DescriptorSetAllocator> _dsAllocator;

	std::unique_ptr<bg2e::render::vulkan::geo::MeshPU> _mesh;
 
	std::shared_ptr<bg2e::render::Texture> _texture;
    VkDescriptorSetLayout _textureDSLayout;
    std::unique_ptr<bg2e::render::vulkan::DescriptorSet> _textureDS;

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
        
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _textureDSLayout = dsFactory.build(_vulkan->device().handle(), VK_SHADER_STAGE_FRAGMENT_BIT);
        _textureDS = std::unique_ptr<bg2e::render::vulkan::DescriptorSet>(
            _dsAllocator->allocate(_textureDSLayout)
        );
        _textureDS->updateImage(
            0,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            _texture->image()->imageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            _texture->sampler()
        );

		auto layoutInfo = bg2e::render::vulkan::Info::pipelineLayoutInfo();
        VkDescriptorSetLayout setLayouts[] = {
            _textureDSLayout
        };
        layoutInfo.pSetLayouts = setLayouts;
        layoutInfo.setLayoutCount = 1;
		VK_ASSERT(vkCreatePipelineLayout(_vulkan->device().handle(), &layoutInfo, nullptr, &_layout));
        
		plFactory.setColorAttachmentFormat(_targetImage->format());
		_pipeline = plFactory.build(_layout);

		_vulkan->cleanupManager().push([&](VkDevice dev) {
			vkDestroyPipeline(dev, _pipeline, nullptr);
			vkDestroyPipelineLayout(dev, _layout, nullptr);
		});
	}

	void createVertexData()
	{
		using namespace bg2e::render::vulkan;

        _mesh = std::unique_ptr<bg2e::render::vulkan::geo::MeshPU>(new bg2e::render::vulkan::geo::MeshPU(_vulkan));
		_mesh->setMeshData({
			{
				{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f } },
				{ {  0.5f, -0.5f, 0.0f }, { 2.0f, 0.0f } },
				{ {  0.5f,  0.5f, 0.0f }, { 2.0f, 2.0f } },
				{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 2.0f } }
			},
			{ 0, 1, 2, 2, 3, 0 },
			{ { 0, 3 }, { 3, 3 } }
		});

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
