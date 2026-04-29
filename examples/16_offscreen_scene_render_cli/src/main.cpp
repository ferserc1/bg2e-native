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

class MyOffscreenApplication : public bg2e::render::DefaultOffscreenApplicationDelegate<bg2e::render::RendererBasicForward>
{
public:
    void initConfig(
        [[maybe_unused]] int argc, [[maybe_unused]] char ** argv,
        bg2e::app::OffscreenApplicationConfig & outConfig
    ) override {
        outConfig.width = 1920;
        outConfig.height = 1080;
        outConfig.createColorImage = true;
        outConfig.createDepthImage = true;
    }

    void didRenderFrame(
        uint32_t frameIndex,
        double elapsedMs,
        VkImageLayout colorImageLayout
    ) override {
        auto width = _colorImage->extent2D().width;
        auto height = _colorImage->extent2D().height;
        size_t imageSize = width * height * 4;
        std::vector<uint8_t> outData(imageSize);
        bg2e::render::vulkan::Image::readPixelsRGBA8(
            _engine,
            _colorImage.get(),
            0, 0,
            _colorImage->extent2D().width, _colorImage->extent2D().height,
            outData,
            colorImageLayout
        );

        auto homePath = bg2e::base::PlatformTools::homePath();
        bg2e::db::saveImage(homePath / "out.jpg", outData.data(), width, height, 4);
    }

protected:
    std::shared_ptr<bg2e::scene::Node> createScene() override
    {
        auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
        sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "country_field_sun.jpg"));

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
        auto anotherDrawable = new bg2e::scene::DrawableComponent(drawable);

        secondModel->addComponent(anotherDrawable);
        secondModel->addComponent(bg2e::scene::TransformComponent::makeTranslated(-2.0f, 0.0f, 0.0f ));
        sceneRoot->addChild(secondModel);

        auto cameraNode = std::shared_ptr<bg2e::scene::Node>(new bg2e::scene::Node("Camera"));
        cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 0.0f, 10.0f ));
        cameraNode->addComponent(new bg2e::scene::CameraComponent());
        auto projection = new bg2e::math::OpticalProjection();
        cameraNode->camera()->setProjection(projection);

        auto cameraRotation = new bg2e::scene::Node("Camera Rotation");
        cameraRotation->addComponent(new bg2e::scene::TransformComponent());
        cameraRotation->addChild(cameraNode);
        sceneRoot->addChild(cameraRotation);

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

    bool continueRendering() override
    {
        return false;
    }
};

int main(int argc, char** argv)
{
    bg2e_log_info << "bg2 engine offscreen render CLI example" << bg2e_log_end;

    bg2e::app::OffscreenApplication app;
    app.init(
        argc, argv,
        "my-offscreen-app",
        std::make_shared<MyOffscreenApplication>()
    );

    return app.run();
}
