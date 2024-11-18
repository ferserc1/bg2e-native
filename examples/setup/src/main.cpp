
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/Application.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/base/Log.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>

#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Window.hpp>

class ClearScreenDelegate : public bg2e::render::RenderLoopDelegate,
	public bg2e::app::InputDelegate,
	public bg2e::ui::UserInterfaceDelegate
{
public:
	void init(bg2e::render::Vulkan* vulkan) override
	{
		using namespace bg2e::render::vulkan;
		RenderLoopDelegate::init(vulkan);

		createImage(vulkan->swapchain().extent());
  
        createPipeline();
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

		vkCmdDraw(cmd, 3, 1, 0, 0);

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

protected:
	std::shared_ptr<bg2e::render::vulkan::Image> _targetImage;

	bg2e::ui::Window _window;
 
    VkPipelineLayout _layout;
    VkPipeline _pipeline;
    
    void createPipeline()
    {
        bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_vulkan);
        
        plFactory.addShader("test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        plFactory.addShader("test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        
        auto layoutInfo = bg2e::render::vulkan::Info::pipelineLayoutInfo();
        VK_ASSERT(vkCreatePipelineLayout(_vulkan->device().handle(), &layoutInfo, nullptr, &_layout));
        plFactory.setColorAttachmentFormat(_targetImage->format());
        _pipeline = plFactory.build(_layout);
        
        _vulkan->cleanupManager().push([&](VkDevice dev) {
            vkDestroyPipeline(dev, _pipeline, nullptr);
            vkDestroyPipelineLayout(dev, _layout, nullptr);
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

	void cleanup() override
	{
		_targetImage->cleanup();
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

int main(int argc, char ** argv) {

    bg2e::app::MainLoop mainLoop;
	MyApplication app;
	app.init(argc, argv);
    return mainLoop.run(&app);
}
