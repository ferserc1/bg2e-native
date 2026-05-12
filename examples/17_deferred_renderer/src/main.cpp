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

class RotateCameraComponent : public bg2e::scene::Component {
public:
    BG2E_COMPONENT_TYPE_NAME("RotateCamera")

    void update(float delta) override
    {
        auto transform = ownerNode()->transform();

        if (transform)
        {
            transform->rotate(0.02f * delta / 10.0f, 0.0f, 1.0f, 0.0f);
        }
    }
};

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

            auto drawSkybox = renderer()->drawSkybox();
            if (bg2e::ui::BasicWidgets::checkBox("Draw skybox", &drawSkybox))
            {
                renderer()->setDrawSkybox(drawSkybox);
            }

            auto blurLevel = renderer()->skyboxBlurLevel();
            if (bg2e::ui::Input::sliderInt("Skybox blur", &blurLevel, 0, 6))
            {
                renderer()->setSkyboxBlurLevel(blurLevel);
            }
        });
    }

protected:
    bg2e::ui::Window _window;

    std::shared_ptr<bg2e::scene::Node> createScene() override {
        auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
        sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "country_field_sun.jpg"));

        auto mainCameraNode = std::make_shared<bg2e::scene::Node>("Main Camera");
        mainCameraNode->addComponent(new bg2e::scene::CameraComponent());
        auto proj = new bg2e::math::OpticalProjection();
        mainCameraNode->camera()->setProjection(proj);
        mainCameraNode->addComponent(new bg2e::scene::TransformComponent());
        mainCameraNode->addComponent(new RotateCameraComponent());
        sceneRoot->addChild(mainCameraNode);


        auto drawable = std::shared_ptr<bg2e::scene::DrawableBase>(loadDrawable());
        auto drawableComponent = std::make_shared<bg2e::scene::DrawableComponent>(drawable);
        auto modelNode = std::make_shared<bg2e::scene::Node>("3D Model");
        modelNode->addComponent(drawableComponent);
        modelNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(2.0f, 0.0f, 0.0f));
        sceneRoot->addChild(modelNode);

        return sceneRoot;
    }

    bg2e::scene::DrawableBase * loadDrawable()
    {
        std::filesystem::path modelPath = bg2e::base::PlatformTools::assetPath();
        modelPath.append("two_submeshes.obj");

        auto innerAlbedoTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_inner_albedo.jpg"
        );

        auto outerAlbedoTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_outer_albedo.jpg"
        );

        auto drawable = new bg2e::scene::Drawable();
        drawable->setMesh(bg2e::db::loadMeshObj<bg2e::geo::Mesh>(modelPath));
        drawable->material(0).setAlbedoTexture(outerAlbedoTexture);
        drawable->material(1).setAlbedoTexture(innerAlbedoTexture);
        drawable->load(_engine);

        return drawable;
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
