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
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/draw/Drawable.hpp>
#include <bg2e/draw/EngineConfig.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/draw/pipeline/SoftShadowPass.hpp>
#include <bg2e/geo/plane.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/TextComponent.hpp>
#include <bg2e/scene/TextFontComponent.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/UserInterfaceDelegate.hpp>
#include <bg2e/ui/Value.hpp>
#include <bg2e/ui/Window.hpp>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace {
using namespace bg2e;

// Explicit right-handed, zero-to-one projection, independent of GLM build flags.
class CameraProjection final : public math::PerspectiveProjection {
  public:
    void apply(glm::mat4& matrix) override
    {
        matrix = glm::perspectiveRH_ZO(glm::radians(fov()), _viewport.width / _viewport.height, near(), far());
    }
};

class SceneDelegate final : public draw::DefaultRenderLoopDelegate,
                            public std::enable_shared_from_this<SceneDelegate> {
  public:
    SceneDelegate() : DefaultRenderLoopDelegate(std::make_unique<draw::RendererDeferred>(), configuration())
    {
    }

    // UI callbacks enqueue CPU values only. The base delegate publishes text
    // textures/geometry in prepareFrame, before acquiring the next frame.
    void editText(std::string content)
    {
        enqueueCpuValueUpdate(rendererGeneration(), [weak = weak_from_this(), content = std::move(content)](auto&) mutable
        {
            if (auto owner = weak.lock(); owner && owner->_text)
            {
                owner->_text->text().setContent(std::move(content));
            }
        });
    }

    void reloadFont()
    {
        enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this()](auto&)
        {
            if (auto owner = weak.lock(); owner && owner->_sceneContext)
            {
                owner->_sceneContext->reloadTextFonts();
                if (owner->_text)
                {
                    owner->_text->retryResources();
                }
            }
        });
    }

    bool fontAvailable() const
    {
        return _sceneContext && bool(_sceneContext->textFont("tutorial"));
    }

    bool shadowsAvailable() const
    {
        auto* deferred = dynamic_cast<draw::RendererDeferred*>(renderer());
        const auto shadows = deferred ? deferred->softShadows() : nullptr;
        return shadows && shadows->available();
    }

    void cleanup() override
    {
        // MainLoop has stopped submissions. Release renderer snapshots first,
        // then text/floor native owners while the Engine still exists.
        DefaultRenderLoopDelegate::cleanup();
        if (_text)
        {
            _text->releaseResources();
        }
        if (_floor)
        {
            _floor->cleanup();
        }
        _sceneContext.reset();
        _text.reset();
        _floor.reset();
    }

  protected:
    std::shared_ptr<scene::Node> createScene() override
    {
        auto root = std::make_shared<scene::Node>("Text tutorial");

        auto cameraNode = std::make_shared<scene::Node>("Camera");
        auto camera = std::make_shared<scene::CameraComponent>();
        auto projection = std::make_shared<CameraProjection>();
        projection->setNear(0.1f);
        projection->setFar(50);
        camera->setProjection(projection);
        cameraNode->addComponent(camera);
        cameraNode->addComponent(new scene::TransformComponent(glm::inverse(glm::lookAtRH(
            glm::vec3(0, 7, 10), glm::vec3(0, 0.5f, 0.6f), glm::vec3(0, 1, 0)))));
        root->addChild(cameraNode);

        _floor = std::make_shared<draw::Drawable>();
        _floor->setMesh(geo::createPlane(10, 8));
        _floor->material().setAlbedo(base::Color(0.7f, 0.7f, 0.7f));
        _floor->material().setRoughness(0.9f);
        _floor->material().setMetalness(0);
        auto floorNode = std::make_shared<scene::Node>("Floor");
        floorNode->addComponent(new scene::TransformComponent());
        floorNode->addComponent(new scene::DrawableComponent(_floor));
        root->addChild(floorNode);

        auto lightNode = std::make_shared<scene::Node>("Directional light");
        // Light rays follow local +Z: +45 degrees around X points down and
        // toward +Z, offsetting the floor shadow from the raised text.
        lightNode->addComponent(scene::TransformComponent::makeRotated(glm::radians(45.0f), 1, 0, 0));
        auto light = std::make_shared<scene::LightComponent>();
        light->light().setType(base::Light::TypeDirectional);
        light->light().setIntensity(2.5f);
        light->light().setCastShadows(true);
        light->light().setSourceSize(0);
        light->light().setShadowSamples(1);
        lightNode->addComponent(light);
        root->addChild(lightNode);

        try
        {
            const auto fontDirectory = base::PlatformTools::assetPath() / "fonts/didact-gothic";
            auto fonts = std::make_shared<scene::TextFontComponent>();
            fonts->setDeclarations({{"tutorial", fontDirectory / "DidactGothic-Regular.ttf", 0,
                {fontDirectory / "OFL.txt", fontDirectory / "README.md"}}});
            root->addComponent(fonts);

            _text = std::make_shared<scene::TextComponent>();
            base::Text::Parameters paragraph;
            paragraph.content = "Hello World";
            paragraph.fontIdentifier = "tutorial";
            paragraph.paragraphSize = {1024, 256};
            paragraph.bitmapResolution = {2048, 512};
            paragraph.fontSize = 160;
            paragraph.margins = {24, 24, 24, 24};
            paragraph.alignment = base::Text::Alignment::Center;
            paragraph.verticalAlignment = base::Text::VerticalAlignment::Center;
            _text->text().setParameters(std::move(paragraph));
            _text->setHeight(1.4f); // Width follows logical aspect: 5.6 world units.
            _text->material().setAlbedo(base::Color(0.07f, 0.28f, 0.8f));
            _text->material().setMetalness(0);
            _text->material().setRoughness(0.9f);
            _text->material().setIsUnlit(false);
            _text->material().setDoubleSided(true);
            _text->material().setAlphaCutoff(0.1f);
            auto textNode = std::make_shared<scene::Node>("Hello World");
            auto transform = scene::TransformComponent::makeTranslated(0, 1.2f, 0);
            // Text's readable front is +Z. Rotate it to +Y above the XZ floor.
            transform->rotate(glm::radians(-90.0f), 1, 0, 0);
            textNode->addComponent(std::move(transform));
            textNode->addComponent(_text);
            root->addChild(textNode);
        }
        catch (...)
        {
            std::cerr << "Text setup unavailable; keeping the floor, camera and UI.\n";
        }

        // A Scene owns the registry. Declare every font before realizing any
        // component; missing fonts warn and leave only their text transparent.
        _sceneContext = std::make_shared<scene::Scene>();
        _sceneContext->setSceneRoot(root);
        _sceneContext->setMainCamera(camera.get());
        _sceneContext->prepareTextFonts();
        root->loadResources(*_engine);
        return root;
    }

  private:
    static draw::RendererConfiguration configuration()
    {
        draw::RendererConfiguration config;
        config.indirectLighting = draw::IndirectLightingMode::IBL;
        config.globalIllumination = std::make_shared<draw::pipeline::RTGlobalIllumination>();
        config.globalIllumination->setEnabled(false);
        config.reflections = std::make_shared<draw::pipeline::RTReflections>();
        config.reflections->setEnabled(false);
        config.softShadows = std::make_shared<draw::pipeline::SoftShadowPass>();
        config.softShadows->settings().enabled = true;
        config.softShadows->settings().bias = 0.002f;
        config.softShadows->settings().directionalDistance = 50;
        config.softShadows->settings().maxSamples = 1;
        return config;
    }

    std::shared_ptr<scene::Scene> _sceneContext;
    std::shared_ptr<scene::TextComponent> _text;
    std::shared_ptr<draw::Drawable> _floor;
};

class UiDelegate final : public ui::UserInterfaceDelegate {
  public:
    explicit UiDelegate(const std::shared_ptr<SceneDelegate>& delegate) : _delegate(delegate)
    {
        _window.setTitle("Edit the text");
        _window.setPosition(20, 20);
        _window.setSize(360, 250);
        _window.options.noClose = true;
    }

    void drawUI() override
    {
        _window.draw([this]
        {
            auto delegate = _delegate.lock();
            if (!delegate)
            {
                return;
            }
            try
            {
                if (ui::Value::multilineText("Content", _content))
                {
                    delegate->editText(_content);
                }
                if (!delegate->fontAvailable())
                {
                    ui::Text::text("Font unavailable: check bundled assets and console warnings.");
                }
                if (!delegate->shadowsAvailable())
                {
                    ui::Text::text("Query shadows unavailable on this device.");
                }
                if (ui::Button::button("Reload font / retry text"))
                {
                    delegate->reloadFont();
                }
            }
            catch (...)
            {
                if (!_warned)
                {
                    std::cerr << "Text edit unavailable; keeping the running scene.\n";
                    _warned = true;
                }
            }
        });
    }

  private:
    std::weak_ptr<SceneDelegate> _delegate;
    std::string _content = "Hello World";
    ui::Window _window;
    bool _warned = false;
};

class Application final : public app::Application {
  public:
    void init(int, char**) override
    {
        auto delegate = std::make_shared<SceneDelegate>();
        setRenderDelegate(delegate);
        setInputDelegate(std::make_shared<app::InputDelegate>());
        setUiDelegate(std::make_shared<UiDelegate>(delegate));
    }
};
}


int main(int argc, char** argv)
{
    try
    {
        bg2e::app::MainLoop loop("org.bg2e.tutorials.text");
        Application application;
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e: Hello World text", 1100, 760));
        return loop.run(&application, bg2e::draw::EngineConfig{});
    }
    catch (const std::exception& error)
    {
        std::cerr << "tutorial_text startup/shutdown failed: " << error.what() << '\n';
        return 1;
    }
}
