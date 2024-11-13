
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/Application.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/base/Log.hpp>

#include <bg2e/ui/DemoWindow.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Window.hpp>

#include <imgui.h>

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

		vulkan->cleanupManager().push([this](VkDevice) {
			this->cleanup();
		});
	}

	void swapchainResized(VkExtent2D newExtent) override
	{
		_colorImage->cleanup();
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
			_colorImage->image(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL
		);

		float flash = std::abs(std::sin(currentFrame / 120.0f));
		VkClearColorValue clearValue{ { 0.0f, 0.0f, flash, 1.0f } };
		auto clearRange = Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdClearColorImage(
			cmd,
			_colorImage->image(),
			VK_IMAGE_LAYOUT_GENERAL,
			&clearValue, 1, &clearRange
		);

		Image::cmdTransitionImage(
			cmd,
			_colorImage->image(),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		);
		Image::cmdTransitionImage(
			cmd,
			colorImage->image(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		Image::cmdCopy(
			cmd,
			_colorImage->image(), _colorImage->extent2D(),
			colorImage->image(), colorImage->extent2D()
		);

		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}

	void keyDown(const bg2e::app::KeyEvent& event) override
	{
		bg2e_log_debug << "Key down: " << event.key() << bg2e_log_end;
	}

	void keyUp(const bg2e::app::KeyEvent& event) override
	{
		bg2e_log_debug << "Key up: " << event.key() << bg2e_log_end;

		if (event.key() == bg2e::app::KeyEvent::Key::KeySpace)
		{
			_window.open();
		}
		else if (event.key() == bg2e::app::KeyEvent::Key::KeyEscape)
		{
			_window.close();
		}
	}

	void mouseMove(int x, int y) override
	{
		bg2e_log_debug << "Mouse move: " << x << ", " << y << bg2e_log_end;
	}

	void mouseButtonDown(int button, int x, int y) override
	{
		bg2e_log_debug << "Mouse button down: " << button << " at " << x << ", " << y << bg2e_log_end;
	}

	void mouseButtonUp(int button, int x, int y) override
	{
		bg2e_log_debug << "Mouse button up: " << button << " at " << x << ", " << y << bg2e_log_end;
	}

	void mouseWheel(int deltaX, int deltaY) override
	{
		bg2e_log_debug << "Mouse wheel: " << deltaX << ", " << deltaY << bg2e_log_end;
	}

	// ============ User Interface Delegate Functions =========
	void init(bg2e::render::Vulkan*, bg2e::ui::UserInterface*) {
		_window.setTitle("ImGui Wrapper Demo");
		_window.options.noClose = true;
	}

	void drawUI() override
	{
		using namespace bg2e::ui;

		//bg2e::ui::DemoWindow::draw();
		_window.draw([]() {
			BasicWidgets::text("Hello, world!");
		});
	}

protected:
	std::shared_ptr<bg2e::render::vulkan::Image> _colorImage;

	bg2e::ui::Window _window;

	void createImage(VkExtent2D extent)
	{
		using namespace bg2e::render::vulkan;
		auto vulkan = this->vulkan();
		_colorImage = std::shared_ptr<Image>(Image::createAllocatedImage(
			vulkan,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			extent,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT
		));
	}

	void cleanup()
	{
		_colorImage->cleanup();
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
