
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/Application.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/base/Log.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/geo/Vertex.hpp>
#include <bg2e/render/vulkan/geo/VertexDescription.hpp>

#include <bg2e/ui/DemoWindow.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Window.hpp>
#include <bg2e/ui/Input.hpp>

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
	void init(bg2e::render::Vulkan*, bg2e::ui::UserInterface*) override {
		_window.setTitle("ImGui Wrapper Demo");
		_window.options.noClose = true;
        _window.options.minWidth = 200;
        _window.options.minHeight = 480;
        _window.setPosition(0, 0);
        _window.setSize(400, 500);
	}

	void drawUI() override
	{
		using namespace bg2e::ui;
  
        static bool checkBoxTest = true;
        static int buttonTest = 0;
        static int radioButtonTest1 = 0;
        static int radioButtonTest2 = 0;

		//bg2e::ui::DemoWindow::draw();
		_window.draw([]() {
			BasicWidgets::text("Hello, world!");
            BasicWidgets::separator();
            BasicWidgets::text("This is a list:");
            BasicWidgets::listItem("First item");
            BasicWidgets::listItem("Second item");
            BasicWidgets::listItem("Third item");
            BasicWidgets::listItem("Fourth item");
            BasicWidgets::separator();
            BasicWidgets::text("Text - ");
            BasicWidgets::text("inline", true);
            BasicWidgets::separator();
            BasicWidgets::checkBox("Test", &checkBoxTest);
            
            if (BasicWidgets::button("Click Me"))
            {
                ++buttonTest;
            }
            if (buttonTest & 1)
            {
                BasicWidgets::text("Clicked", true);
            }
            
            BasicWidgets::separator("Radio button group 1");
            BasicWidgets::radioButton("Value 1", &radioButtonTest1, 0);
            BasicWidgets::radioButton("Value 2", &radioButtonTest1, 1);
            BasicWidgets::radioButton("Value 3", &radioButtonTest1, 2);
            BasicWidgets::text("Value");
            BasicWidgets::text(std::to_string(radioButtonTest1 + 1), true);
            
            BasicWidgets::separator("Radio button group 2");
            BasicWidgets::radioButton("Value A", &radioButtonTest2, 0);
            BasicWidgets::radioButton("Value B", &radioButtonTest2, 1);
            BasicWidgets::radioButton("Value C", &radioButtonTest2, 2);
            switch (radioButtonTest2)
            {
                case 0: BasicWidgets::text("Value A"); break;
                case 1: BasicWidgets::text("Value B"); break;
                case 2: BasicWidgets::text("Value C"); break;
            }
            
            BasicWidgets::separator("Inputs");
            static std::string testStringValue = "Hola";
            Input::text("Test string", testStringValue);
            BasicWidgets::text(testStringValue.c_str());
            
            static std::string testStringValue2;
            Input::textWithHint("Test string 2", "Enter test here", testStringValue2);
            BasicWidgets::text(testStringValue2.c_str());
            
            static int testIntValue = 0;
            Input::number("Int value", &testIntValue);
            BasicWidgets::text(std::to_string(testIntValue));
            
            static float testFloatValue = 0.0f;
            Input::number("Float value", &testFloatValue);
            BasicWidgets::text(std::to_string(testFloatValue));
            
            static double testDoubleValue = 0.0;
            Input::number("Double value", &testDoubleValue);
            BasicWidgets::text(std::to_string(testDoubleValue));
            
            static int testInt2[2] = { 10, 20 };
            Input::vec2("Vec 2", testInt2);
            BasicWidgets::text(std::to_string(testInt2[0]) + " " + std::to_string(testInt2[1]));
            
            static int testInt3[3] = { 1, 2, 3 };
            Input::vec3("Vec 3", testInt3);
            BasicWidgets::text(
                std::to_string(testInt3[0]) + " " +
                std::to_string(testInt3[1]) + " " +
                std::to_string(testInt3[2])
            );
                
            static float testFloat4[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            Input::vec4("Vec 4", testFloat4);
            BasicWidgets::text(
                std::to_string(testFloat4[0]) + " " +
                std::to_string(testFloat4[1]) + " " +
                std::to_string(testFloat4[2]) + " " +
                std::to_string(testFloat4[3])
            );
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

int main(int argc, char ** argv)
{
    bg2e::app::MainLoop mainLoop;
	MyApplication app;
	app.init(argc, argv);
    return mainLoop.run(&app);
}
