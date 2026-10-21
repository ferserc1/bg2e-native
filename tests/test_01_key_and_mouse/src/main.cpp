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
#include <bg2e/app/Application.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/draw/RenderLoopDelegate.hpp>
#include <bg2e/ui/Window.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/app/Keyboard.hpp>
#include <bg2e/app/Mouse.hpp>

#include <memory>
#include <string>

class TestKeyMouseDelegate : public bg2e::draw::RenderLoopDelegate,
                             public bg2e::app::InputDelegate,
                             public bg2e::ui::UserInterfaceDelegate {
  public:
    void render(const bg2e::draw::FrameContext& context) override
    {
        context.commandBuffer.beginRendering(&context.colorTarget);
        context.commandBuffer.clearColor(0, {0.0f, 0.0f, 0.0f, 1.0f});
        context.commandBuffer.endRendering();
    }

    // ============ User Interface Delegate Functions =========
    void init(bg2e::draw::Engine*, bg2e::ui::UserInterface*) override
    {
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

        _window.draw([]()
                     {
                         Text::text("== Keyboard Modifiers ==");
                         Text::text(
                             std::string("Shift:   ") + (Keyboard::shiftPressed() ? "ON" : "OFF"));
                         Text::text(
                             std::string("Control: ") + (Keyboard::controlPressed() ? "ON" : "OFF"));
                         Text::text(
                             std::string("Alt:     ") + (Keyboard::altPressed() ? "ON" : "OFF"));
                         Text::text(
                             std::string("Super:   ") + (Keyboard::superPressed() ? "ON" : "OFF"));

                         Text::text("");
                         Text::text("== Mouse State ==");
                         Text::text(
                             std::string("Left:    ") + (Mouse::leftButtonPressed() ? "ON" : "OFF"));
                         Text::text(
                             std::string("Middle:  ") + (Mouse::middleButtonPressed() ? "ON" : "OFF"));
                         Text::text(
                             std::string("Right:   ") + (Mouse::rightButtonPressed() ? "ON" : "OFF"));
                         Text::text(
                             "Pos X:   " + std::to_string(Mouse::x()));
                         Text::text(
                             "Pos Y:   " + std::to_string(Mouse::y()));
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

int main(int argc, char** argv)
{

    bg2e::app::MainLoop mainLoop("org.bg2engine.tests.key_and_mouse");
    MyApplication app;
    app.init(argc, argv);
    return mainLoop.run(&app);
}
