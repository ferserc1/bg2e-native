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
#include <exception>
#include <iostream>
#include <memory>

namespace {
using namespace bg2e;

class CameraProjection final : public math::PerspectiveProjection {
  public:
    void apply(glm::mat4& matrix) override
    {
        matrix = glm::perspectiveRH_ZO(glm::radians(fov()),
            _viewport.width / _viewport.height, near(), far());
    }
};

class SceneDelegate final : public draw::DefaultRenderLoopDelegate {
  public:
    SceneDelegate() : DefaultRenderLoopDelegate(std::make_unique<draw::RendererDeferred>())
    {
    }

  protected:
    std::shared_ptr<scene::Node> createScene() override
    {
        auto root = std::make_shared<scene::Node>("Scene");
        root->addComponent(new scene::EnvironmentComponent(base::PlatformTools::assetPath(), "HDR_multi_nebulae_1.hdr"));
        auto modelResource = std::make_shared<draw::Drawable>();
        modelResource->setMesh(std::shared_ptr<geo::Mesh>(geo::createCube(2, 2, 2)));
        modelResource->load(_engine);
        auto model = std::make_shared<scene::Node>("Cube");
        model->addComponent(std::make_shared<scene::DrawableComponent>(modelResource));
        root->addChild(model);
        auto camera = std::make_shared<scene::Node>("Camera");
        camera->addComponent(scene::TransformComponent::makeTranslated(0, 0, 6));
        camera->addComponent(new scene::CameraComponent());
        camera->camera()->setProjection(new CameraProjection());
        root->addChild(camera);
        return root;
    }

};

class UiDelegate final : public ui::UserInterfaceDelegate {
  public:
    void init(draw::Engine*, ui::UserInterface*) override
    {
        _window.setTitle("Deferred scene");
        _window.options.noClose = true;
        _window.setPosition(20, 20);
        _window.setSize(260, 100);
    }

    void drawUI() override
    {
        _window.draw([]
        {
            ui::Text::text("bg2e draw / deferred renderer");
        });
    }

  private:
    ui::Window _window;
};

class Application final : public app::Application {
  public:
    void init(int, char**) override
    {
        setRenderDelegate(std::make_shared<SceneDelegate>());
        setInputDelegate(std::make_shared<app::InputDelegate>());
        setUiDelegate(std::make_shared<UiDelegate>());
    }
};
}

int main(int argc, char** argv)
{
    try
    {
        bg2e::app::MainLoop loop("org.bg2engine.myapp");
        Application application;
        application.init(argc, argv);
        return loop.run(&application);
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
