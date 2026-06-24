/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <bg2e.hpp>
#include <bg2e/app/Keyboard.hpp>
#include <bg2e/app/Mouse.hpp>

#include <iostream>

class TestKeyMouseDelegate : public bg2e::render::RenderLoopDelegate,
	public bg2e::app::InputDelegate,
	public bg2e::ui::UserInterfaceDelegate
{
public:
	void init(bg2e::render::Engine * vulkan) override
	{
		using namespace bg2e::render::vulkan;
		RenderLoopDelegate::init(vulkan);
	}

    void initScene() override
    {
    }

	void swapchainResized(VkExtent2D newExtent) override
	{
	}

	VkImageLayout render(
		VkCommandBuffer cmd,
		uint32_t currentFrame,
		const bg2e::render::vulkan::Image* colorImage,
		const bg2e::render::vulkan::Image* depthImage,
        const bg2e::render::vulkan::Image* msaaDepthImage,
		bg2e::render::vulkan::FrameResources& frameResources
	) override {
		using namespace bg2e::render::vulkan;

		Image::cmdTransitionImage(
			cmd,
			colorImage->handle(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL
		);

		VkClearColorValue clearValue{ { 0.0f, 0.0f, 0.0f, 1.0f } };
		auto clearRange = Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdClearColorImage(
			cmd,
			colorImage->handle(),
			VK_IMAGE_LAYOUT_GENERAL,
			&clearValue, 1, &clearRange
		);

		Image::cmdTransitionImage(
			cmd, colorImage->handle(),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	// ============ User Interface Delegate Functions =========
	void init(bg2e::render::Engine *, bg2e::ui::UserInterface*) override {
		_window.setTitle("Keyboard & Mouse Test");
		_window.options.noClose = true;
        _window.options.minWidth = 280;
        _window.options.minHeight = 200;
        _window.setPosition(0, 0);
        _window.setSize(280, 200);
	}

	void drawUI() override
	{
		using namespace bg2e::ui;
		using namespace bg2e::app;

		_window.draw([]() {
			BasicWidgets::text("== Keyboard Modifiers ==");
			BasicWidgets::text(
				std::string("Shift:   ") + (Keyboard::shiftPressed() ? "ON" : "OFF")
			);
			BasicWidgets::text(
				std::string("Control: ") + (Keyboard::controlPressed() ? "ON" : "OFF")
			);
			BasicWidgets::text(
				std::string("Alt:     ") + (Keyboard::altPressed() ? "ON" : "OFF")
			);
			BasicWidgets::text(
				std::string("Super:   ") + (Keyboard::superPressed() ? "ON" : "OFF")
			);

			BasicWidgets::text("");
			BasicWidgets::text("== Mouse State ==");
			BasicWidgets::text(
				std::string("Left:    ") + (Mouse::leftButtonPressed() ? "ON" : "OFF")
			);
			BasicWidgets::text(
				std::string("Middle:  ") + (Mouse::middleButtonPressed() ? "ON" : "OFF")
			);
			BasicWidgets::text(
				std::string("Right:   ") + (Mouse::rightButtonPressed() ? "ON" : "OFF")
			);
			BasicWidgets::text(
				"Pos X:   " + std::to_string(Mouse::x())
			);
			BasicWidgets::text(
				"Pos Y:   " + std::to_string(Mouse::y())
			);
		});
	}

protected:
	bg2e::ui::Window _window;
};

class MyApplication : public bg2e::app::Application {
public:
	void init(int argc, char** argv) override
	{
		auto delegate = std::shared_ptr<TestKeyMouseDelegate>(new TestKeyMouseDelegate());
		setRenderDelegate(delegate);
		setInputDelegate(delegate);
		setUiDelegate(delegate);
	}
};

int main(int argc, char ** argv) {

    bg2e::app::MainLoop mainLoop("org.bg2engine.tests.key_and_mouse");
	MyApplication app;
	app.init(argc, argv);
    return mainLoop.run(&app);
}
