#include <bg2e/app/Application.hpp>
#include <bg2e/app/InputDelegate.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/Preferences.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/draw/all.hpp>
#include <bg2e/db/mesh_obj.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/geo/plane.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <bg2e/ui/GraphicsSettingsWindow.hpp>
#include <bg2e/ui/UserInterfaceDelegate.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Numeric.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
using namespace bg2e;
class SceneDelegate;

// This application's menu is not an engine upscaler enum/registration table.
class ApplicationUpscaler final : public draw::ResamplingUpscaler {
  public:
    std::string identifier() const override
    {
        return "example.resampler.v1";
    }
    std::string label() const override
    {
        return "Application resampler";
    }
    bool enabled() const
    {
        return _enabled;
    }
    void setEnabled(bool value)
    {
        _enabled = value;
    }
    uint32_t calls() const
    {
        return _calls;
    }
    bool record(const draw::UpscalerInput& input) override
    {
        if (!_enabled)
        {
            return false;
        }
        ++_calls;
        return ResamplingUpscaler::record(input);
    }
    reflection::TypeInfo settingsType() const override
    {
        reflection::TypeInfoBuilder<ApplicationUpscaler> type(identifier());
        type.property("enabled", &ApplicationUpscaler::enabled, &ApplicationUpscaler::setEnabled);
        type.property("recordedFrames", &ApplicationUpscaler::calls);
        return type.build();
    }
  private:
    bool _enabled = true;
    uint32_t _calls = 0;
};
class CountingGBuffer final : public draw::pipeline::GBuffer {
  public:
    uint64_t opaque = 0, transparent = 0;
    void record(const draw::pipeline::PassContext& context) override
    {
        GBuffer::record(context);
        if (context.raster && context.raster->item)
        {
            ++transparent;
        }
        else
        {
            ++opaque;
        }
    }
};
// Explicit zero-to-one depth avoids depending on a translation unit's GLM defines.
class CameraProjection final : public math::PerspectiveProjection {
  public:
    void apply(glm::mat4& matrix) override
    {
        matrix = glm::perspectiveRH_ZO(glm::radians(fov()), _viewport.width / _viewport.height, near(), far());
    }
};
// These fixtures have one material across every cube face. The generic cube
// primitive exposes six face submeshes; retain all indices in one material domain.
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
                const auto p = (y * 32 + x) * 4;
                pixels[p] = 245;
                pixels[p + 1] = 180;
                pixels[p + 2] = 40;
                pixels[p + 3] = ((x / 4 + y / 4) % 2) ? 255 : 0;
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
                const auto p = (y * 8 + x) * 4;
                pixels[p] = x < 4 ? 51 : 204;
                pixels[p + 1] = 166;
                pixels[p + 2] = 230;
                pixels[p + 3] = 255;
            }
        }
        return pixels;
    }
    std::string imageIdentifier() override
    {
        return "draw-deferred-packed-channels-v1";
    }
};
struct State {
    explicit State(app::MainLoop& mainLoop) : loop(mainLoop), persistence("draw_deferred_renderer")
    {
        persistence.load();
    }
    app::MainLoop& loop;
    app::Preferences persistence; // Must outlive facade, borrowed by it.
    std::shared_ptr<draw::RenderSettingsPreferences> preferences;
    std::weak_ptr<SceneDelegate> delegate;
    std::shared_ptr<draw::Drawable> original, clone, glass, solidGlass, cutout, channels;
    std::shared_ptr<draw::pipeline::SoftShadowPass> shadows;
    std::shared_ptr<CountingGBuffer> gbuffer = std::make_shared<CountingGBuffer>();
    std::shared_ptr<draw::pipeline::FinalLayer> final;
    std::shared_ptr<draw::ResolutionSettings> resolution = std::make_shared<draw::ResolutionSettings>();
    std::shared_ptr<scene::Node> cutoutNode, glassNode, solidGlassNode;
    bool forcedNoRt = false, animateCamera = true, animateLights = true, paused = false;
    uint32_t upscalerChoice = 0;
    uint64_t sceneFrames = 0, uiFrames = 0, replacements = 0;
    std::string backend, message;
    std::filesystem::path settingsFile;
    bool restorePreferences = false;
};
class SceneDelegate final : public draw::DefaultRenderLoopDelegate {
  public:
    explicit SceneDelegate(std::shared_ptr<State> state)
        : DefaultRenderLoopDelegate(std::make_unique<draw::RendererDeferred>(), configurationFor(*state)), _state(std::move(state))
    {
        setContinuousRendering(true);
    }
    static draw::RendererConfiguration configurationFor(State& state)
    {
        draw::RendererConfiguration configuration;
        configuration.outputFormat = state.final->contract().outputs.at(0).format;
        configuration.depthFormat = state.final->contract().inputs.at(7).format;
        configuration.resolution = state.resolution;
        if (state.upscalerChoice == 1)
        {
            configuration.upscaler = std::make_shared<draw::FsrUpscaler>();
        }
        else if (state.upscalerChoice == 2)
        {
            configuration.upscaler = std::make_shared<ApplicationUpscaler>();
        }
        if (!state.shadows)
        {
            state.shadows = std::make_shared<draw::pipeline::SoftShadowPass>();
            state.shadows->settings().enabled = !state.forcedNoRt;
        }
        configuration.softShadows = state.shadows;
        configuration.layers.push_back({state.gbuffer, {{}, {}, state.gbuffer->contract()}});
        configuration.layers.push_back({state.final, {{}, {}, draw::pipeline::finalContract(configuration.outputFormat, configuration.depthFormat, true)}});
        return configuration;
    }
    void replace(uint32_t upscaler)
    {
        _state->upscalerChoice = upscaler;
        setRenderer(std::make_unique<draw::RendererDeferred>(), configurationFor(*_state));
        ++_state->replacements;
    }
    void prepareFrame() override
    {
        DefaultRenderLoopDelegate::prepareFrame();
        if (!_restored && renderer() && !_state->preferences->catalog().rendererId().empty())
        {
            _restored = true;
            if (!_state->settingsFile.empty())
            {
                const auto document = draw::SettingsCodec::readFile(_state->settingsFile);
                _state->preferences->queueImport(*document, draw::UnknownSettingPolicy::Reject, startupCompletion());
            }
            else if (_state->restorePreferences)
            {
                _state->preferences->queueRestore(draw::UnknownSettingPolicy::Reject, startupCompletion());
            }
        }
    }
    void update(const draw::FrameContext& frame) override
    {
        if (_state->animateCamera)
        {
            _cameraAngle += frame.sceneDeltaSeconds * 0.12f;
        }
        if (_state->animateLights)
        {
            _lightAngle += frame.sceneDeltaSeconds * 0.2f;
        }
        _cameraPivot->transform()->setRotation(_cameraAngle, 0, 1, 0);
        _lights->transform()->setRotation(_lightAngle, 0, 1, 0);
        DefaultRenderLoopDelegate::update(frame);
        ++_state->sceneFrames;
    }
    void cleanup() override
    {
        DefaultRenderLoopDelegate::cleanup();
        _state->preferences->clear();
        for (auto& drawable : _owned)
        {
            drawable->cleanup();
        }
        _owned.clear();
        _state->original.reset();
        _state->clone.reset();
        _state->glass.reset();
        _state->solidGlass.reset();
        _state->cutout.reset();
        _state->channels.reset();
        _state->cutoutNode.reset();
        _state->glassNode.reset();
        _state->solidGlassNode.reset();
        _cameraPivot.reset();
        _lights.reset();
    }
  protected:
    std::shared_ptr<scene::Node> createScene() override
    {
        auto root = std::make_shared<scene::Node>("Draw deferred scene");
        root->addComponent(new scene::EnvironmentComponent(base::PlatformTools::assetPath(), "HDR_multi_nebulae_1.hdr"));
        auto model = std::make_shared<draw::Drawable>();
        model->setMesh(db::loadMeshObj<draw::Drawable::Mesh>(base::PlatformTools::assetPath() / "two_submeshes.obj"));
        if (model->submeshesCount() != 2)
        {
            throw std::runtime_error("Deferred example expects two OBJ submeshes");
        }
        const char* names[] = {"outer", "inner"};
        for (uint32_t i = 0; i < 2; ++i)
        {
            auto texture = [&](const std::string& suffix)
            {
                return std::make_shared<base::Texture>(base::PlatformTools::assetPath(),
                    "two_submeshes_" + std::string(names[i]) + "_" + suffix + ".jpg");
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
            model->setSubmeshName(names[i], i);
        }
        model->material(1).setFresnelTint(base::Color(0.8f, 0.2f, 1));
        model->material(1).setSheenColor(base::Color::Blue());
        model->material(1).setSheenIntensity(0.5f);
        load(model);
        _state->original = model;
        auto nested = std::make_shared<scene::Node>("Nested models");
        nested->addComponent(scene::TransformComponent::makeTranslated(0, 0.3f, 0));
        root->addChild(nested);
        addDrawable(nested, "Original opaque", model, {-3, 0, 0});
        _state->clone = model->clone();
        _owned.push_back(_state->clone);
        _state->clone->material(0).setAlbedo(base::Color(0.3f, 0.8f, 1));
        verifyClone();
        addDrawable(nested, "Independent clone", _state->clone, {0, 0, -1});
        _state->glass = model->clone();
        _owned.push_back(_state->glass);
        _state->glass->iterateMaterials([](auto& material)
        {
            material.setAlbedo(base::Color(1, 1, 1, 0.45f));
            material.setIsTransparent(true);
            material.setRefractionFactor(0.1f);
        });
        _state->glassNode = addDrawable(root, "Transparent model", _state->glass, {3, 0, 0});
        _state->solidGlass = model->clone();
        _owned.push_back(_state->solidGlass);
        _state->solidGlass->iterateMaterials([](auto& material)
        {
            material.setAlbedo(base::Color(0.7f, 1, 0.7f, 0.5f));
            material.setIsTransparent(true);
            material.setIsSolid(true);
        });
        _state->solidGlassNode = addDrawable(root, "Solid transparent model", _state->solidGlass, {1.5f, 0, 2.5f});
        auto floor = std::make_shared<draw::Drawable>();
        floor->setMesh(geo::createPlane(30, 30));
        floor->material().setAlbedo(base::Color(0.55f, 0.55f, 0.55f));
        floor->material().setMetalness(0);
        floor->material().setAlphaCutoff(0);
        floor->material().setRoughness(0.75f);
        load(floor);
        addDrawable(root, "Floor", floor, {0, -1.05f, 0});
        _state->cutout = std::make_shared<draw::Drawable>();
        _state->cutout->setMesh(singleMaterialCube(2.4f, 0.06f, 2.4f));
        auto checker = std::make_shared<base::Texture>(std::make_shared<CutoutTexture>());
        checker->setUseMipmaps(false);
        checker->setMinFilter(base::Texture::FilterNearest);
        checker->setMagFilter(base::Texture::FilterNearest);
        _state->cutout->material().setAlbedoTexture(checker);
        _state->cutout->material().setAlphaCutoff(0.5f);
        _state->cutout->material().setMetalness(0);
        load(_state->cutout);
        _state->cutoutNode = addDrawable(root, "Alpha-cutout canopy", _state->cutout, {-3, 2.4f, 1.5f});
        _state->channels = std::make_shared<draw::Drawable>();
        auto channelMesh = singleMaterialCube(1.4f, 1.4f, 1.4f);
        for (auto& vertex : channelMesh->vertices)
        {
            vertex.texCoord1 = vertex.texCoord0 * 2.0f;
        }
        _state->channels->setMesh(channelMesh);
        auto packed = std::make_shared<base::Texture>(std::make_shared<PackedTexture>());
        packed->setUseMipmaps(false);
        packed->setMinFilter(base::Texture::FilterNearest);
        packed->setMagFilter(base::Texture::FilterNearest);
        auto& packedMaterial = _state->channels->material();
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
        load(_state->channels);
        addDrawable(root, "Packed channel and UV1 fixture", _state->channels, {-1.5f, -0.3f, 2.5f});
        _cameraPivot = std::make_shared<scene::Node>("Camera orbit");
        _cameraPivot->addComponent(new scene::TransformComponent());
        auto camera = std::make_shared<scene::Node>("Camera");
        camera->addComponent(new scene::TransformComponent(glm::inverse(glm::lookAtRH(glm::vec3(0, 5, 13), glm::vec3(0, 0.5f, 0), glm::vec3(0, 1, 0)))));
        camera->addComponent(new scene::CameraComponent());
        camera->camera()->setProjection(new CameraProjection());
        _cameraPivot->addChild(camera);
        root->addChild(_cameraPivot);
        _lights = std::make_shared<scene::Node>("Rotating lights");
        _lights->addComponent(new scene::TransformComponent());
        root->addChild(_lights);
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
            value.setCastShadows(!_state->forcedNoRt);
            value.setSourceSize(0.25f);
            value.setShadowSamples(8);
            _lights->addChild(light);
        }
        return root;
    }
  private:
    draw::RenderSettingsPreferences::Completion startupCompletion()
    {
        const std::weak_ptr<State> weak = _state;
        return [weak](draw::SettingsResult result)
        {
            if (auto state = weak.lock())
            {
                state->message = result.valid() ? "Startup settings applied" : "Startup settings rejected";
                for (const auto& issue : result.issues)
                {
                    state->message += ": " + issue.message;
                }
            }
        };
    }
    void load(const std::shared_ptr<draw::Drawable>& drawable)
    {
        _owned.push_back(drawable); // Retain even if realization fails halfway.
        drawable->setRayTracingEnabled(!_state->forcedNoRt);
        drawable->load(_engine);
    }
    static std::shared_ptr<scene::Node> addDrawable(const std::shared_ptr<scene::Node>& parent,
        const std::string& name, std::shared_ptr<draw::Drawable> drawable, glm::vec3 position)
    {
        auto node = std::make_shared<scene::Node>(name);
        node->addComponent(scene::TransformComponent::makeTranslated(position));
        node->addComponent(new scene::DrawableComponent(std::move(drawable)));
        parent->addChild(node);
        return node;
    }
    void verifyClone()
    {
        const auto& a = _state->original;
        const auto& b = _state->clone;
        const auto ta = a->drawMaterial(0)->albedoTexture();
        const auto tb = b->drawMaterial(0)->albedoTexture();
        if (a->mesh() == b->mesh() || a->drawMesh() == b->drawMesh() ||
            a->drawMaterial(0) == b->drawMaterial(0) || ta == tb ||
            ta->image() == tb->image() || ta->sampler() == tb->sampler() || a->material(0).albedo().r == b->material(0).albedo().r)
        {
            throw std::runtime_error("Drawable deep-clone ownership/material independence failed");
        }
        _state->message = "Independent mesh, material, texture, image and sampler clones verified.";
    }
    std::shared_ptr<State> _state;
    std::shared_ptr<scene::Node> _cameraPivot, _lights;
    std::vector<std::shared_ptr<draw::Drawable>> _owned;
    float _cameraAngle = 0, _lightAngle = 0;
    bool _restored = false;
};
class UiDelegate final : public ui::UserInterfaceDelegate {
  public:
    explicit UiDelegate(std::shared_ptr<State> state) : _state(std::move(state))
    {
        _settings.bind(_state->preferences);
        _settings.setPosition(920, 10);
        _settings.setSize(430, 650);
        _controls.setTitle("Deferred scene");
        _controls.setPosition(10, 10);
        _controls.setSize(320, 650);
        _controls.options.noClose = true;
    }
    void drawUI() override
    {
        ++_state->uiFrames;
        auto loop = _state->delegate.lock();
        if (!loop)
        {
            return;
        }
        _controls.draw([&]
        {
            ui::Text::text("Backend: " + _state->backend);
            ui::Text::text("Scene / UI frames: " + std::to_string(_state->sceneFrames) + " / " + std::to_string(_state->uiFrames));
            ui::Text::text("G-buffer opaque / transparent passes: " + std::to_string(_state->gbuffer->opaque) + " / " + std::to_string(_state->gbuffer->transparent));
            ui::Text::text("Renderer replacements: " + std::to_string(_state->replacements));
            if (ui::Button::checkBox("Pause scene", &_state->paused))
            {
                if (_state->paused)
                {
                    _state->loop.pauseScene();
                }
                else
                {
                    _state->loop.resumeScene();
                }
            }
            const bool cameraChanged = ui::Button::checkBox("Orbit camera", &_state->animateCamera);
            const bool lightsChanged = ui::Button::checkBox("Rotate lights", &_state->animateLights);
            if (cameraChanged || lightsChanged)
            {
                _state->loop.requestSceneFrame();
            }
            if (ui::Button::button("Refresh scene"))
            {
                _state->loop.requestSceneFrame();
            }
            if (ui::Button::button("Graphics settings"))
            {
                _settings.open();
            }
            auto selected = _state->upscalerChoice;
            if (ui::Value::comboBox("Upscaler", {"Bilinear", "FSR 3.1", "Application resampler"}, selected))
            {
                loop->replace(selected);
            }
            if (ui::Button::button("Replace renderer"))
            {
                loop->replace(_state->upscalerChoice);
            }
            if (ui::Button::button("Queue stale edit and replace"))
            {
                const auto weak = std::weak_ptr<State>(_state);
                auto& catalog = _state->preferences->catalog();
                if (!catalog.rendererId().empty())
                {
                    _state->preferences->queueSet(catalog.handle("final", "exposure"), 1.0f, [weak](draw::SettingsResult result)
                    {
                        if (auto state = weak.lock())
                        {
                            state->message = result.valid() ? "Unexpected: stale edit was accepted" : "Stale edit rejected after replacement";
                        }
                    });
                    loop->replace(_state->upscalerChoice);
                }
            }
            if (auto* renderer = dynamic_cast<draw::RendererDeferred*>(loop->renderer()))
            {
                const auto& configuration = renderer->configuration();
                ui::Text::text("Internal: " + std::to_string(configuration.internalExtent.width) + " x " + std::to_string(configuration.internalExtent.height));
                if (auto upscaler = renderer->upscaler())
                {
                    ui::Text::text(upscaler->label() + ": " + upscaler->status());
                }
                if (auto shadows = renderer->softShadows())
                {
                    ui::Text::text("Query shadows: " + shadows->status());
                }
                if (_state->forcedNoRt)
                {
                    ui::Text::text("Forced no-RT fixture: no tracing geometry; shadow lights disabled.");
                }
            }
            if (_state->original && _state->clone)
            {
                ui::Text::separator("Independent clone material");
                auto color = _state->clone->material(0).albedo();
                if (ui::Value::colorPicker("Clone outer tint", color))
                {
                    edit(loop, [color](State& state)
                    {
                        state.clone->material(0).setAlbedo(color);
                    });
                }
                float roughness = _state->clone->material(0).roughness();
                if (ui::Numeric::sliderFloat("Clone roughness", &roughness, 0.02f, 1))
                {
                    edit(loop, [roughness](State& state)
                    {
                        state.clone->material(0).setRoughness(roughness);
                    });
                }
                bool showGlass = _state->glassNode->enabled();
                if (ui::Button::checkBox("Transparent models", &showGlass))
                {
                    edit(loop, [showGlass](State& state)
                    {
                        state.glassNode->setEnabled(showGlass);
                        state.solidGlassNode->setEnabled(showGlass);
                    });
                }
                auto alpha = _state->glass->material(0).albedo().a;
                if (ui::Numeric::sliderFloat("Glass opacity", &alpha, 0.05f, 1))
                {
                    edit(loop, [alpha](State& state)
                    {
                        state.glass->iterateMaterials([alpha](auto& material)
                        {
                            auto color = material.albedo();
                            color.a = alpha;
                            material.setAlbedo(color);
                        });
                    });
                }
                const std::vector<std::string> channelLabels{"Red", "Green", "Blue", "Alpha"};
                auto channel = _state->channels->material().metalnessChannel();
                if (ui::Value::comboBox("Fixture metalness channel", channelLabels, channel))
                {
                    edit(loop, [channel](State& state)
                    {
                        state.channels->material().setMetalnessChannel(channel);
                    });
                }
                bool inverted = _state->channels->material().metalnessInvert();
                if (ui::Button::checkBox("Invert fixture metalness", &inverted))
                {
                    edit(loop, [inverted](State& state)
                    {
                        state.channels->material().setMetalnessInvert(inverted);
                    });
                }
                auto cutoff = _state->cutout->material().alphaCutoff();
                if (ui::Numeric::sliderFloat("Cutout threshold", &cutoff, 0, 1))
                {
                    edit(loop, [cutoff](State& state)
                    {
                        state.cutout->material().setAlphaCutoff(cutoff);
                    });
                }
                bool visible = _state->cutoutNode->enabled();
                if (ui::Button::checkBox("Cutout canopy", &visible))
                {
                    edit(loop, [visible](State& state)
                    {
                        state.cutoutNode->setEnabled(visible);
                    });
                }
            }
            ui::Text::text(_state->message);
        });
        _settings.draw();
    }
  private:
    void edit(const std::shared_ptr<SceneDelegate>& loop, std::function<void(State&)> operation)
    {
        const auto weak = std::weak_ptr<State>(_state);
        loop->enqueueRendererUpdate(loop->rendererGeneration(), [weak, operation](draw::Renderer& renderer)
        {
            if (auto state = weak.lock())
            {
                operation(*state);
                if (auto* deferred = dynamic_cast<draw::RendererDeferred*>(&renderer))
                {
                    deferred->resetUpscaler();
                }
            }
        }, [weak]
        {
            if (auto state = weak.lock())
            {
                state->message = "Material edit cancelled after renderer rebind";
            }
        });
    }
    std::shared_ptr<State> _state;
    ui::Window _controls;
    ui::GraphicsSettingsWindow _settings;
};
class Application final : public app::Application {
  public:
    explicit Application(app::MainLoop& loop) : _state(std::make_shared<State>(loop))
    {
    }
    void init(int argc, char** argv) override
    {
        std::set<std::string> seen;
        for (int i = 1; i < argc; ++i)
        {
            const std::string argument = argv[i];
            const auto key = argument.substr(0, argument.find('='));
            if (!seen.insert(key).second)
            {
                throw std::invalid_argument("Repeated option: " + key);
            }
            if (argument == "--backend=vulkan" || argument == "--backend=metal")
            {
                _config.backend = argument == "--backend=metal" ? gpu::BackendType::Metal : gpu::BackendType::Vulkan;
            }
            else if (argument == "--upscaler=bilinear" || argument == "--upscaler=fsr" || argument == "--upscaler=application")
            {
                _state->upscalerChoice = argument == "--upscaler=fsr" ? 1 : argument == "--upscaler=application" ? 2 : 0;
            }
            else if (argument == "--no-rt")
            {
                _state->forcedNoRt = true;
            }
            else if (argument == "--validation")
            {
                _config.debug = true;
            }
            else if (argument.starts_with("--settings=") && argument.size() > 11)
            {
                _state->settingsFile = argument.substr(11);
            }
            else if (argument == "--restore-preferences")
            {
                _state->restorePreferences = true;
            }
            else
            {
                throw std::invalid_argument("Unknown option: " + argument);
            }
        }
#ifndef BG2E_IS_MAC
        if (_config.backend == gpu::BackendType::Metal)
        {
            throw std::invalid_argument("Metal requires macOS");
        }
#endif
        _state->backend = _config.backend == gpu::BackendType::Metal ? "Metal" : "Vulkan";
        _state->resolution->setEnabled(true);
        _state->resolution->setScale(0.67f);
        _state->final = std::make_shared<draw::pipeline::FinalLayer>(std::filesystem::path{}, 0,
            _config.colorFormat, _config.depthFormat, true);
        _state->preferences = std::make_shared<draw::RenderSettingsPreferences>(&_state->persistence);
        auto delegate = std::make_shared<SceneDelegate>(_state);
        _state->delegate = delegate;
        _state->preferences->connect(delegate);
        setRenderDelegate(delegate);
        setInputDelegate(std::make_shared<app::InputDelegate>());
        setUiDelegate(std::make_shared<UiDelegate>(_state));
    }
    const draw::EngineConfig& config() const
    {
        return _config;
    }
  private:
    draw::EngineConfig _config;
    std::shared_ptr<State> _state;
};
}
int main(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--help")
    {
        std::cout << "draw_deferred_renderer [--backend=vulkan|metal] [--upscaler=bilinear|fsr|application] [--no-rt] [--validation] [--settings=FILE] [--restore-preferences]\n";
        return 0;
    }
    try
    {
        bg2e::app::MainLoop loop("org.bg2e.examples.draw.deferred-renderer");
        Application application(loop);
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
