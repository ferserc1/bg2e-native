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

class DeferredRendererDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererDeferred>,
    public bg2e::app::InputDelegate,
    public bg2e::ui::UserInterfaceDelegate
{
public:
    void init(bg2e::render::Engine*, bg2e::ui::UserInterface*) override {
        _window.setTitle("Deferred renderer (shell)");
        _window.options.noClose = true;
        _window.options.minWidth = 190;
        _window.options.minHeight = 90;
        _window.setPosition(0, 0);
        _window.setSize(200, 100);
    }

    void drawUI() override {
        _window.draw([&]() {
            bg2e::ui::BasicWidgets::text("Deferred renderer shell - Phase 1");
        });
    }

protected:
    bg2e::ui::Window _window;

    std::shared_ptr<bg2e::scene::Node> createScene() override {
        auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
        return sceneRoot;
    }
};

class MyApplication : public bg2e::app::Application {
public:
    void init(int argc, char** argv) override {
        auto delegate = std::make_shared<DeferredRendererDelegate>();
        setRenderDelegate(delegate);
        setInputDelegate(delegate);
        setUiDelegate(delegate);
    }
};

int main(int argc, char** argv) {
    bg2e::app::MainLoop mainLoop("org.bg2engine.examples.deferred-renderer");
    MyApplication app;
    app.init(argc, argv);
    return mainLoop.run(&app);
}
