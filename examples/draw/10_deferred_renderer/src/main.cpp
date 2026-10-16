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

#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>

namespace {
using namespace bg2e;

// The deferred renderer uses a zero-to-one depth range on both supported backends.
class CameraProjection final : public math::PerspectiveProjection {
  public:
    void apply(glm::mat4& matrix) override
    {
        matrix = glm::perspectiveRH_ZO(glm::radians(fov()), _viewport.width / _viewport.height, near(), far());
    }
};

// A single material range makes the canopy material cover all six cube faces.
std::shared_ptr<draw::Drawable::Mesh> singleMaterialCube(float width, float height, float depth)
{
    auto mesh = std::shared_ptr<draw::Drawable::Mesh>(geo::createCube(width, height, depth));
    mesh->submeshes = {{0, uint32_t(mesh->indices.size())}};
    return mesh;
}

class CutoutTexture final : public base::ProceduralTextureGenerator {
  public:
    CutoutTexture() : ProceduralTextureGenerator(32, 32, 4)
    {
    }

    uint8_t* generate() override
    {
        auto* pixels = new uint8_t[32 * 32 * 4];
        for (uint32_t y = 0; y < 32; ++y)
        {
            for (uint32_t x = 0; x < 32; ++x)
            {
                const auto pixel = (y * 32 + x) * 4;
                pixels[pixel] = 245;
                pixels[pixel + 1] = 180;
                pixels[pixel + 2] = 40;
                pixels[pixel + 3] = ((x / 4 + y / 4) % 2) ? 255 : 0;
            }
        }
        return pixels;
    }

    std::string imageIdentifier() override
    {
        return "draw-deferred-alpha-checker-v1";
    }
};

class PackedTexture final : public base::ProceduralTextureGenerator {
  public:
    PackedTexture() : ProceduralTextureGenerator(8, 8, 4)
    {
    }

    uint8_t* generate() override
    {
        auto* pixels = new uint8_t[8 * 8 * 4];
        for (uint32_t y = 0; y < 8; ++y)
        {
            for (uint32_t x = 0; x < 8; ++x)
            {
                const auto pixel = (y * 8 + x) * 4;
                pixels[pixel] = x < 4 ? 51 : 204;
                pixels[pixel + 1] = 166;
                pixels[pixel + 2] = 230;
                pixels[pixel + 3] = 255;
            }
        }
        return pixels;
    }

    std::string imageIdentifier() override
    {
        return "draw-deferred-packed-channels-v1";
    }
};

struct SceneState {
    std::shared_ptr<scene::Node> cameraPivot;
    std::shared_ptr<scene::Node> lights;
    std::shared_ptr<scene::Node> canopy;
    std::shared_ptr<draw::Drawable> clone;
    std::vector<std::shared_ptr<scene::Node>> lightNodes;
    float cameraAngle = 0.0f;
    float lightAngle = 0.0f;
    bool animateCamera = true;
    bool animateLights = true;
};

class SceneDelegate final : public draw::DefaultRenderLoopDelegate {
  public:
    SceneDelegate()
        : DefaultRenderLoopDelegate(std::make_unique<draw::RendererDeferred>())
    {
        setContinuousRendering(true);
    }

    SceneState& state()
    {
        return _state;
    }

    void update(const draw::FrameContext& frame) override
    {
        if (_state.animateCamera)
        {
            _state.cameraAngle += frame.sceneDeltaSeconds * 0.12f;
            _state.cameraPivot->transform()->setRotation(_state.cameraAngle, 0, 1, 0);
        }
        if (_state.animateLights)
        {
            _state.lightAngle += frame.sceneDeltaSeconds * 0.2f;
            _state.lights->transform()->setRotation(_state.lightAngle, 0, 1, 0);
        }
        DefaultRenderLoopDelegate::update(frame);
    }

    void cleanup() override
    {
        DefaultRenderLoopDelegate::cleanup();
        // Drawable::cleanup releases GPU resources while the engine is still alive.
        for (auto& drawable : _drawables)
        {
            drawable->cleanup();
        }
    }

  protected:
    std::shared_ptr<scene::Node> createScene() override
    {
        auto root = std::make_shared<scene::Node>("Deferred scene");
        root->addComponent(new scene::EnvironmentComponent(base::PlatformTools::assetPath(), "HDR_multi_nebulae_1.hdr"));

        auto model = std::make_shared<draw::Drawable>();
        model->setMesh(db::loadMeshObj<draw::Drawable::Mesh>(base::PlatformTools::assetPath() / "two_submeshes.obj"));
        const char* submeshNames[] = {"outer", "inner"};
        for (uint32_t i = 0; i < 2; ++i)
        {
            auto texture = [&](const std::string& map)
            {
                return std::make_shared<base::Texture>(base::PlatformTools::assetPath(),
                    "two_submeshes_" + std::string(submeshNames[i]) + "_" + map + ".jpg");
            };
            auto& material = model->material(i);
            material.setAlbedoTexture(texture("albedo"));
            material.setMetalnessTexture(texture("metallic"));
            material.setRoughnessTexture(texture("roughness"));
            material.setNormalTexture(texture("normal"));
            material.setAoTexture(texture("ao"));
            material.setMetalness(1);
            material.setRoughness(1);
            material.setAlphaCutoff(0);
            model->setSubmeshName(submeshNames[i], i);
        }
        model->material(1).setFresnelTint(base::Color(0.8f, 0.2f, 1));
        model->material(1).setSheenColor(base::Color::Blue());
        model->material(1).setSheenIntensity(0.5f);
        load(model);

        auto nested = std::make_shared<scene::Node>("Nested models");
        nested->addComponent(scene::TransformComponent::makeTranslated(0, 0.3f, 0));
        root->addChild(nested);
        addDrawable(nested, "Original", model, {-3, 0, 0});

        _state.clone = model->clone();
        _state.clone->material(0).setAlbedo(base::Color(0.3f, 0.8f, 1));
        _drawables.push_back(_state.clone);
        addDrawable(nested, "Independent clone", _state.clone, {0, 0, -1});

        auto glass = model->clone();
        glass->iterateMaterials([](auto& material)
        {
            material.setAlbedo(base::Color(1, 1, 1, 0.45f));
            material.setIsTransparent(true);
            material.setRefractionFactor(0.1f);
        });
        _drawables.push_back(glass);
        addDrawable(root, "Transparent model", glass, {3, 0, 0});

        auto solidGlass = model->clone();
        solidGlass->iterateMaterials([](auto& material)
        {
            material.setAlbedo(base::Color(0.7f, 1, 0.7f, 0.5f));
            material.setIsTransparent(true);
            material.setIsSolid(true);
        });
        _drawables.push_back(solidGlass);
        addDrawable(root, "Solid transparent model", solidGlass, {1.5f, 0, 2.5f});

        auto floor = std::make_shared<draw::Drawable>();
        floor->setMesh(geo::createPlane(30, 30));
        floor->material().setAlbedo(base::Color(0.55f, 0.55f, 0.55f));
        floor->material().setMetalness(0);
        floor->material().setAlphaCutoff(0);
        floor->material().setRoughness(0.75f);
        load(floor);
        addDrawable(root, "Floor", floor, {0, -1.05f, 0});

        auto canopy = std::make_shared<draw::Drawable>();
        canopy->setMesh(singleMaterialCube(2.4f, 0.06f, 2.4f));
        auto checker = std::make_shared<base::Texture>(std::make_shared<CutoutTexture>());
        checker->setUseMipmaps(false);
        checker->setMinFilter(base::Texture::FilterNearest);
        checker->setMagFilter(base::Texture::FilterNearest);
        canopy->material().setAlbedoTexture(checker);
        canopy->material().setAlphaCutoff(0.5f);
        canopy->material().setMetalness(0);
        load(canopy);
        _state.canopy = addDrawable(root, "Alpha-cutout canopy", canopy, {-3, 2.4f, 1.5f});

        // UV1 is scaled separately so the packed map demonstrates material UV selection.
        auto channelMesh = singleMaterialCube(1.4f, 1.4f, 1.4f);
        for (auto& vertex : channelMesh->vertices)
        {
            vertex.texCoord1 = vertex.texCoord0 * 2.0f;
        }
        auto channels = std::make_shared<draw::Drawable>();
        channels->setMesh(channelMesh);
        auto packed = std::make_shared<base::Texture>(std::make_shared<PackedTexture>());
        packed->setUseMipmaps(false);
        packed->setMinFilter(base::Texture::FilterNearest);
        packed->setMagFilter(base::Texture::FilterNearest);
        auto& packedMaterial = channels->material();
        packedMaterial.setAlbedo(base::Color(0.8f, 0.3f, 0.15f));
        packedMaterial.setAlphaCutoff(0);
        packedMaterial.setMetalnessTexture(packed);
        packedMaterial.setMetalness(1);
        packedMaterial.setMetalnessChannel(0);
        packedMaterial.setMetalnessUVSet(1);
        packedMaterial.setMetalnessScale(glm::vec2(2, 1));
        packedMaterial.setRoughnessTexture(packed);
        packedMaterial.setRoughness(1);
        packedMaterial.setRoughnessChannel(1);
        packedMaterial.setAoTexture(packed);
        packedMaterial.setAoChannel(2);
        load(channels);
        addDrawable(root, "Packed channels", channels, {-1.5f, -0.3f, 2.5f});

        _state.cameraPivot = std::make_shared<scene::Node>("Camera orbit");
        _state.cameraPivot->addComponent(new scene::TransformComponent());
        auto camera = std::make_shared<scene::Node>("Camera");
        camera->addComponent(new scene::TransformComponent(glm::inverse(glm::lookAtRH(
            glm::vec3(0, 5, 13), glm::vec3(0, 0.5f, 0), glm::vec3(0, 1, 0)))));
        camera->addComponent(new scene::CameraComponent());
        camera->camera()->setProjection(new CameraProjection());
        _state.cameraPivot->addChild(camera);
        root->addChild(_state.cameraPivot);

        _state.lights = std::make_shared<scene::Node>("Rotating lights");
        _state.lights->addComponent(new scene::TransformComponent());
        root->addChild(_state.lights);
        for (uint32_t i = 0; i < 4; ++i)
        {
            auto light = std::make_shared<scene::Node>("Light " + std::to_string(i + 1));
            const float angle = float(i) * 1.57079633f;
            light->addComponent(scene::TransformComponent::makeTranslated(std::sin(angle) * 5, 5, std::cos(angle) * 5));
            light->addComponent(new scene::LightComponent());
            auto& value = light->light()->light();
            value.setType(base::Light::TypeOmni);
            value.setColor(base::Color(i == 0 ? 1 : 0.65f, i == 1 ? 1 : 0.7f, i == 2 ? 1 : 0.75f));
            value.setIntensity(35);
            value.setCastShadows(true);
            value.setSourceSize(0.25f);
            value.setShadowSamples(8);
            _state.lightNodes.push_back(light);
            _state.lights->addChild(light);
        }
        return root;
    }

  private:
    void load(const std::shared_ptr<draw::Drawable>& drawable)
    {
        // Keep each drawable until cleanup, including if loading fails partway through.
        _drawables.push_back(drawable);
        drawable->load(_engine);
    }

    std::shared_ptr<scene::Node> addDrawable(const std::shared_ptr<scene::Node>& parent,
        const std::string& name, std::shared_ptr<draw::Drawable> drawable, glm::vec3 position)
    {
        auto node = std::make_shared<scene::Node>(name);
        node->addComponent(scene::TransformComponent::makeTranslated(position));
        node->addComponent(new scene::DrawableComponent(std::move(drawable)));
        parent->addChild(node);
        return node;
    }

    SceneState _state;
    std::vector<std::shared_ptr<draw::Drawable>> _drawables;
};

class UiDelegate final : public ui::UserInterfaceDelegate {
  public:
    explicit UiDelegate(std::weak_ptr<SceneDelegate> delegate) : _delegate(std::move(delegate))
    {
        _controls.setTitle("Deferred scene");
        _controls.setPosition(10, 10);
        _controls.setSize(360, 620);
    }

    void drawUI() override
    {
        auto delegate = _delegate.lock();
        if (!delegate || !delegate->scene())
        {
            return;
        }
        auto& state = delegate->state();
        _controls.draw([&]
        {
            ui::Text::text("Scene controls");
            if (ui::Button::checkBox("Animate camera", &state.animateCamera))
            {
                app::MainLoop::current()->requestSceneFrame();
            }
            if (ui::Button::checkBox("Animate lights", &state.animateLights))
            {
                app::MainLoop::current()->requestSceneFrame();
            }
            for (size_t i = 0; i < state.lightNodes.size(); ++i)
            {
                auto& light = state.lightNodes[i]->light()->light();
                float intensity = light.intensity();
                if (ui::Numeric::sliderFloat("Light " + std::to_string(i + 1) + " intensity", &intensity, 0, 60))
                {
                    edit(delegate, [i, intensity](SceneState& scene)
                    {
                        scene.lightNodes[i]->light()->light().setIntensity(intensity);
                    });
                }
                auto color = light.color();
                if (ui::Value::colorPicker("Light " + std::to_string(i + 1) + " color", color))
                {
                    edit(delegate, [i, color](SceneState& scene)
                    {
                        scene.lightNodes[i]->light()->light().setColor(color);
                    });
                }
            }
            ui::Text::separator("Cloned model material");
            auto tint = state.clone->material(0).albedo();
            if (ui::Value::colorPicker("Clone tint", tint))
            {
                edit(delegate, [tint](SceneState& scene)
                {
                    scene.clone->material(0).setAlbedo(tint);
                });
            }
            float roughness = state.clone->material(0).roughness();
            if (ui::Numeric::sliderFloat("Clone roughness", &roughness, 0.02f, 1))
            {
                edit(delegate, [roughness](SceneState& scene)
                {
                    scene.clone->material(0).setRoughness(roughness);
                });
            }
            bool showCanopy = state.canopy->enabled();
            if (ui::Button::checkBox("Cutout canopy", &showCanopy))
            {
                edit(delegate, [showCanopy](SceneState& scene)
                {
                    scene.canopy->setEnabled(showCanopy);
                });
            }
        });
    }

  private:
    void edit(const std::shared_ptr<SceneDelegate>& delegate, std::function<void(SceneState&)> operation)
    {
        const auto weakDelegate = std::weak_ptr<SceneDelegate>(delegate);
        delegate->enqueueRendererUpdate(delegate->rendererGeneration(), [weakDelegate, operation = std::move(operation)](draw::Renderer&)
        {
            if (auto activeDelegate = weakDelegate.lock())
            {
                operation(activeDelegate->state());
            }
        });
    }

    std::weak_ptr<SceneDelegate> _delegate;
    ui::Window _controls;
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

    const draw::EngineConfig& config() const
    {
        return _config;
    }

  private:
    draw::EngineConfig _config;
};
}

int main(int argc, char** argv)
{
    try
    {
        bg2e::app::MainLoop loop("org.bg2e.examples.draw.deferred-renderer");
        Application application;
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e draw: deferred renderer", 1380, 800));
        return loop.run(&application, application.config());
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_deferred_renderer: " << error.what() << '\n';
        return 1;
    }
}
