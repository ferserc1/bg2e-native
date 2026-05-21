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
#include <numbers>
#include <random>

class RotateComponent : public bg2e::scene::Component {
public:
    BG2E_COMPONENT_TYPE_NAME("RotateCamera")

    RotateComponent() : _factor(1.0f) {}
    RotateComponent(float factor) : _factor(factor) {}

    void update(float delta) override
    {
        auto transform = ownerNode()->transform();

        if (transform)
        {
            transform->rotate(0.0005f * delta * _factor / 10.0f, 0.0f, 1.0f, 0.0f);
        }
    }

protected:
    float _factor;
};

class DeferredRendererDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererDeferred>,
    public bg2e::app::InputDelegate,
    public bg2e::ui::UserInterfaceDelegate
{
public:
    void init(bg2e::render::Engine*, bg2e::ui::UserInterface*) override {
        _window.setTitle("Deferred renderer");
        _window.options.noClose = true;
        _window.options.minWidth = 190;
        _window.options.minHeight = 90;
        _window.setPosition(0, 0);
        _window.setSize(200, 100);
    }

    void drawUI() override {
        _window.draw([&]() {
            bg2e::ui::BasicWidgets::text("Deferred renderer shell");

            _fpsAccumulator += delta();
            _fpsFrameCount++;
            if (_fpsAccumulator >= 1000.0f)
            {
                _fps = static_cast<int>(1000.0f * _fpsFrameCount / _fpsAccumulator);
                _fpsAccumulator = 0.0f;
                _fpsFrameCount = 0;
            }
            std::string fpsLine = "FPS: " + std::to_string(_fps);
            bg2e::ui::BasicWidgets::text(fpsLine);

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

            auto renderLayer = renderer()->debugVisualization();
            int32_t renderLayerId = static_cast<int32_t>(renderLayer);
            const int32_t numLayers = static_cast<int32_t>(bg2e::render::deferred::DeferredDebugVisualization::MaxLayer) - 1;
            if (bg2e::ui::Input::sliderInt("Render Layer", &renderLayerId, 0, numLayers))
            {
                renderer()->setDebugVisualization(static_cast<bg2e::render::deferred::DeferredDebugVisualization>(renderLayerId));
            }
        });
    }

protected:
    bg2e::ui::Window _window;

    int _fps = 0;
    float _fpsAccumulator = 0.0f;
    int _fpsFrameCount = 0;

    std::shared_ptr<bg2e::scene::Node> createScene() override {
        auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
        sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "HDR_multi_nebulae_1.hdr"));

        auto anotherNode = new bg2e::scene::Node("Transform Node");
        anotherNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 1.0f, 0.0f));
        sceneRoot->addChild(anotherNode);

        auto drawable = std::shared_ptr<bg2e::scene::DrawableBase>(loadDrawable());
        auto drawableComponent = std::make_shared<bg2e::scene::DrawableComponent>(drawable);
        auto modelNode = std::make_shared<bg2e::scene::Node>("3D Model");
        modelNode->addComponent(drawableComponent);
        modelNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(2.0f, 0.0f, 0.0f));
        anotherNode->addChild(modelNode);

        auto secondModel = new bg2e::scene::Node("Second 3D model");
        auto anotherDrawable = std::shared_ptr<bg2e::scene::DrawableBase>(loadDrawable(true));
        auto anotherDrawableComponent = new bg2e::scene::DrawableComponent(anotherDrawable);

        secondModel->addComponent(anotherDrawableComponent);
        secondModel->addComponent(bg2e::scene::TransformComponent::makeTranslated(-2.0f, 0.0f, 0.0f ));
        sceneRoot->addChild(secondModel);


        auto floor = createFloorNode();
        sceneRoot->addChild(floor);

        auto cameraNode = std::shared_ptr<bg2e::scene::Node>(new bg2e::scene::Node("Camera"));
        cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 3.0f, 10.0f ));
        cameraNode->transform()->rotate(0.0f, -1.0f, 0.0f, 0.0f);
        cameraNode->addComponent(new bg2e::scene::CameraComponent());
        auto projection = new bg2e::math::OpticalProjection();
        projection->setFocalLength(35.0f);
        cameraNode->camera()->setProjection(projection);

        auto cameraRotation = new bg2e::scene::Node("Camera Rotation");
        cameraRotation->addComponent(new bg2e::scene::TransformComponent());
        cameraRotation->addComponent(new RotateComponent(5.0f));
        cameraRotation->addChild(cameraNode);
        sceneRoot->addChild(cameraRotation);

        auto lights = createLights();
        sceneRoot->addChild(lights);

        return sceneRoot;
    }

    bg2e::scene::DrawableBase * loadDrawable(bool transparent = false)
    {
        std::filesystem::path modelPath = bg2e::base::PlatformTools::assetPath();
        modelPath.append("two_submeshes.obj");

        auto innerAlbedoTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_inner_albedo.jpg"
        );

        auto innerMetallicTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_inner_metallic.jpg"
        );

        auto innerRoughnessTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_inner_roughness.jpg"
        );

        auto innerNormalTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_inner_normal.jpg"
        );

        auto outerAlbedoTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_outer_albedo.jpg"
        );

        auto outerMetallicTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_outer_metallic.jpg"
        );

        auto outerRoughnessTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_outer_roughness.jpg"
        );

        auto outerNormalTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_outer_normal.jpg"
        );

        auto drawable = new bg2e::scene::Drawable();
        drawable->setMesh(bg2e::db::loadMeshObj<bg2e::geo::Mesh>(modelPath));
        if (transparent)
        {
            drawable->material(0).setAlbedo(bg2e::base::Color(1.0f, 1.0f, 1.0f, 0.7f));
            drawable->material(0).setIsTransparent(true);
            drawable->material(1).setAlbedo(bg2e::base::Color(1.0f, 1.0f, 1.0f, 0.7f));
            drawable->material(1).setIsTransparent(true);
        }
        drawable->material(0).setAlbedoTexture(outerAlbedoTexture);
        drawable->material(0).setMetalnessTexture(outerMetallicTexture);
        drawable->material(0).setRoughnessTexture(outerRoughnessTexture);
        drawable->material(0).setNormalTexture(outerNormalTexture);
        drawable->material(0).setMetalness(1.0f);
        drawable->material(0).setRoughness(1.0f);
        drawable->material(1).setAlbedoTexture(innerAlbedoTexture);
        drawable->material(1).setMetalnessTexture(innerMetallicTexture);
        drawable->material(1).setRoughnessTexture(innerRoughnessTexture);
        drawable->material(1).setNormalTexture(innerNormalTexture);
        drawable->material(1).setMetalness(1.0f);
        drawable->material(1).setRoughness(1.0f);
        drawable->load(_engine);

        return drawable;
    }

    bg2e::scene::Node * createLights()
    {
        auto * lightsRoot = new bg2e::scene::Node("Lights");
        lightsRoot->addComponent(new RotateComponent(-10.0f));
        lightsRoot->addComponent(new bg2e::scene::TransformComponent());
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);

        constexpr float distance = 5.0f;
        constexpr int numLights = 10;
        for (int i = 0; i < numLights; ++i)
        {
            float alpha = std::numbers::pi_v<float> * 2.0f * static_cast<float>(i) / static_cast<float>(numLights);
            float x = std::cos(alpha) * distance;
            float z = std::sin(alpha) * distance;
            lightsRoot->addChild(createLight(
                glm::vec3{x, 1.0f, z},
                bg2e::base::Color(
                    colorDist(gen),
                    colorDist(gen),
                    colorDist(gen),
                    1.0f
                ),
                true
            ));
        }

        return lightsRoot;
    }

    bg2e::scene::Node * createLight(
        const glm::vec3& position,
        const bg2e::base::Color& color,
        bool castShadows
    ) {
        auto * lightNode = new bg2e::scene::Node("Light");

        lightNode->addComponent(new bg2e::scene::LightComponent());
        lightNode->light()->light().setType(bg2e::base::Light::TypeOmni);
        lightNode->light()->light().setColor(color);
        lightNode->addComponent(new bg2e::scene::TransformComponent());
        lightNode->transform()->setTranslation(position);
        lightNode->light()->light().setCastShadows(castShadows);

        return lightNode;
    }

    std::shared_ptr<bg2e::scene::Node> createFloorNode()
    {
        auto floorNode = std::make_shared<bg2e::scene::Node>("Floor");
        floorNode->addComponent(new bg2e::scene::TransformComponent());
        floorNode->transform()->setTranslation(0.0f, -0.75f, 0.0f);

        auto floorGeo = bg2e::geo::createPlane(100.0f, 100.0f);
        auto drawable = std::make_shared<bg2e::scene::Drawable>();
        drawable->setMesh(floorGeo);
        drawable->load(_engine);
        floorNode->addComponent(new bg2e::scene::DrawableComponent(drawable));

        return floorNode;
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
