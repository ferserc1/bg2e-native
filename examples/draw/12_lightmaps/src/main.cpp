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
#include <bg2e/bake/all.hpp>
#include <bg2e/ui/BakeLightmapPreview.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/geo/UvAtlasValidator.hpp>

#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
using namespace bg2e;
using IndirectMode = base::MaterialAttributes::IndirectTextureMode;

class CameraProjection final : public math::PerspectiveProjection {
  public:
    void apply(glm::mat4& matrix) override
    {
        matrix = glm::perspectiveRH_ZO(glm::radians(fov()), _viewport.width / _viewport.height, near(), far());
    }
};

class SceneDelegate final : public draw::DefaultRenderLoopDelegate {
  public:
    SceneDelegate() : DefaultRenderLoopDelegate(std::make_unique<draw::RendererDeferred>(), rendererConfiguration())
    {
        _settings->setResolution(128);
        _settings->setAccumulationFrames(16);
        _settings->setSamplesPerPixel(4);
        _settings->setCpuFormat(bake::LightmapPixelFormat::RGB32F);
        setContinuousRendering(true);
    }

    static draw::RendererConfiguration rendererConfiguration()
    {
        draw::RendererConfiguration configuration;
        configuration.indirectLighting = draw::IndirectLightingMode::IBL;
        configuration.ambientOcclusion = std::make_shared<draw::pipeline::RTAmbientOcclusion>();
        configuration.ambientOcclusion->setEnabled(false);
        configuration.globalIllumination = std::make_shared<draw::pipeline::RTGlobalIllumination>();
        configuration.globalIllumination->setEnabled(false);
        // Reflections are an independent display choice, disabled for comparison.
        configuration.reflections = std::make_shared<draw::pipeline::RTReflections>();
        configuration.reflections->setEnabled(false);
        return configuration;
    }

    std::shared_ptr<bake::LightmapSettings> settings() const
    {
        return _settings;
    }

    bool running() const
    {
        return _running;
    }

    bool useBaked() const
    {
        return _useBaked;
    }

    bool hasResults() const
    {
        return _results.size() == _targets.size() && _mapTextures.size() == _targets.size() && !_results.empty();
    }

    const std::string& status() const
    {
        return _status;
    }

    const std::string& uvStatus() const
    {
        return _uvStatus;
    }

    std::shared_ptr<gpu::Image> uvImage() const
    {
        return _uvImage;
    }

    std::shared_ptr<gpu::Image> lightmapImage() const
    {
        return _selected < _results.size() ? _results[_selected].image : nullptr;
    }

    std::string selectedName() const
    {
        return _targets.empty() ? "" : _targets[_selected]->name();
    }

    void requestStart()
    {
        _start = true;
        requestSceneFrame();
    }

    void requestCancel()
    {
        _cancel = true;
        requestSceneFrame();
    }

    void requestUseBaked(bool value)
    {
        _pendingUseBaked = value;
        requestSceneFrame();
    }

    void nextPreview()
    {
        if (!_targets.empty())
        {
            _selected = (_selected + 1) % _targets.size();
            _refreshPreview = true;
            requestSceneFrame();
        }
    }

    void setPreviewUv(uint32_t value)
    {
        _previewUv = value;
        _refreshPreview = true;
        requestSceneFrame();
    }

    void setUiCleanup(std::function<void()> callback)
    {
        _uiCleanup = std::move(callback);
    }

    void prepareFrame() override
    {
        DefaultRenderLoopDelegate::prepareFrame();
        if (!renderer() || _targets.empty())
        {
            return;
        }

        if (_cancel)
        {
            _cancel = false;
            _start = false;
            if (_baker)
            {
                _baker->cancel();
            }

            releaseBake();
            _running = false;
            _results.clear();
            _cpuResults.clear();
            _status = "Cancelled. No partial maps were applied.";
        }

        try
        {
            if (_start)
            {
                _start = false;
                beginBake();
            }

            if (_running)
            {
                sample();
            }

            if (_pendingUseBaked && !_running && hasResults())
            {
                setApplied(*_pendingUseBaked);
            }

            _pendingUseBaked.reset();
            if (_refreshPreview)
            {
                refreshPreview();
            }
        }
        catch (const std::exception& error)
        {
            // A failed immediateSubmit has already destroyed its command wrapper.
            // Context destruction drains possible submitted work before dropping
            // invocation owners; no failed sample is published or applied.
            _baker.reset();
            _context.reset();
            _running = false;
            _status = std::string("Bake unavailable or failed: ") + error.what();
            requestSceneFrame();
        }
    }

    void cleanup() override
    {
        if (_uiCleanup)
        {
            _uiCleanup();
            _uiCleanup = {};
        }

        releaseBake();
        _uvImage.reset();
        _results.clear();
        _cpuResults.clear();
        _mapTextures.clear();
        // Releases renderer snapshots before cleaning drawables.
        DefaultRenderLoopDelegate::cleanup();
        for (auto& node : _targets)
        {
            node->drawable()->drawDrawable()->cleanup();
        }

        for (auto& drawable : _sources)
        {
            drawable->cleanup();
        }

        for (auto& drawable : _unbaked)
        {
            drawable->cleanup();
        }

        _targets.clear();
        _sources.clear();
        _unbaked.clear();
    }

  protected:
    std::shared_ptr<scene::Node> createScene() override
    {
        auto root = std::make_shared<scene::Node>("Lightmap room");
        root->addComponent(new scene::EnvironmentComponent());
        auto floor = makeDrawable(std::shared_ptr<geo::Mesh>(geo::createPlane(10, 10)), base::Color(0.65f, 0.65f, 0.65f));
        add(root, "Floor", floor, {0, 0, 0}, true);
        auto cube = makeDrawable(cubeMesh(1.6f, 1.6f, 1.6f), base::Color(0.8f, 0.8f, 0.8f));
        add(root, "White cube", cube, {-1.2f, 0.8f, 0}, true);
        // Clone before UV2 generation; each target receives its own rebuilt mesh.
        auto clone = cube->clone();
        clone->material().setAlbedo(base::Color(0.85f, 0.08f, 0.04f));
        add(root, "Red independent clone", clone, {1.2f, 0.8f, -0.5f}, true);
        auto back = makeDrawable(cubeMesh(10, 4, 0.15f), base::Color(0.7f, 0.7f, 0.7f));
        add(root, "Unbaked back wall", back, {0, 2, -3.5f}, false);
        auto side = makeDrawable(cubeMesh(0.15f, 4, 7), base::Color(0.08f, 0.65f, 0.12f));
        add(root, "Unbaked green wall", side, {-4, 2, 0}, false);
        auto glass = makeDrawable(cubeMesh(0.9f, 0.9f, 0.9f), base::Color(0.5f, 0.7f, 1, 0.35f));
        glass->material().setIsTransparent(true);
        add(root, "Transparent display only", glass, {2.8f, 0.5f, 1.8f}, false);
        auto lamp = makeDrawable(cubeMesh(0.3f, 0.3f, 0.3f), base::Color(1, 0.8f, 0.5f));
        lamp->material().setLightEmission(2.0f);
        add(root, "Emission reference", lamp, {0, 3.2f, 0}, false);
        auto light = std::make_shared<scene::Node>("Fixed direct light");
        light->addComponent(scene::TransformComponent::makeTranslated(0, 3, 1));
        light->addComponent(new scene::LightComponent());
        light->light()->light().setType(base::Light::TypeOmni);
        light->light()->light().setIntensity(25);
        light->light()->light().setCastShadows(true);
        root->addChild(light);
        auto camera = std::make_shared<scene::Node>("Camera");
        camera->addComponent(new scene::TransformComponent(glm::inverse(glm::lookAtRH(
            glm::vec3(8, 6, 12), glm::vec3(0, 0.8f, -0.5f), glm::vec3(0, 1, 0)))));
        camera->addComponent(new scene::CameraComponent());
        camera->camera()->setProjection(new CameraProjection());
        root->addChild(camera);
        return root;
    }

  private:
    static std::shared_ptr<geo::Mesh> cubeMesh(float width, float height, float depth)
    {
        auto mesh = std::shared_ptr<geo::Mesh>(geo::createCube(width, height, depth));
        mesh->submeshes = {{0, uint32_t(mesh->indices.size())}};
        return mesh;
    }

    std::shared_ptr<draw::Drawable> makeDrawable(std::shared_ptr<geo::Mesh> mesh, base::Color color)
    {
        auto drawable = std::make_shared<draw::Drawable>();
        drawable->setMesh(std::move(mesh));
        drawable->iterateMaterials([color](auto& material)
        {
            material.setAlbedo(color);
            material.setMetalness(0);
            material.setRoughness(0.8f);
            material.setAlphaCutoff(0);
        });
        drawable->load(_engine);
        return drawable;
    }

    void add(const std::shared_ptr<scene::Node>& root, const std::string& name,
        std::shared_ptr<draw::Drawable> drawable, glm::vec3 position, bool target)
    {
        auto node = std::make_shared<scene::Node>(name);
        node->addComponent(scene::TransformComponent::makeTranslated(position));
        node->addComponent(new scene::DrawableComponent(drawable));
        root->addChild(node);
        if (target)
        {
            _targets.push_back(node);
            _sources.push_back(std::move(drawable));
        }
        else
        {
            _unbaked.push_back(std::move(drawable));
        }
    }

    void releaseBake()
    {
        if (_baker)
        {
            _baker->cleanup();
            _baker.reset();
        }

        if (_context)
        {
            _context->cleanup();
            _context.reset();
        }
    }

    void beginBake()
    {
        releaseBake();
        _settings->validate();
        _running = false;
        auto* deferred = dynamic_cast<draw::RendererDeferred*>(renderer());
        auto* generator = deferred ? deferred->environmentGenerator() : nullptr;
        if (!generator || !generator->resources())
        {
            throw std::runtime_error("The renderer environment is not ready; retry after a scene frame");
        }

        std::vector<std::shared_ptr<draw::Drawable>> replacements;
        for (const auto& source : _sources)
        {
            replacements.push_back(bake::generateUv2AndReload(source,
                {_settings->resolution(), _settings->dilationPixels()}).drawable);
        }

        _results.clear();
        _cpuResults.clear();
        _mapTextures.clear();
        _useBaked = false;
        for (size_t i = 0; i < replacements.size(); ++i)
        {
            _targets[i]->drawable()->setDrawable(replacements[i]);
        }

        deferred->invalidateHistories();
        _refreshPreview = true;
        bake::BakerContextConfiguration configuration;
        configuration.device = _engine->device();
        configuration.backend = _engine->backendType();
        configuration.frameSlots = 1;
        configuration.sceneRoot = scene();
        configuration.environment = generator->resources();
        _context = std::make_unique<bake::IntegratedBakerContext>(*_engine, std::move(configuration));
        _target = 0;
        _serial = 0;
        _baker = _context->createBaker({_targets[_target], _settings});
        _running = true;
        _refreshPreview = true;
    }

    void sample()
    {
        const uint64_t serial = ++_serial;
        // One synchronous sample before window acquisition. This demonstration
        // favors explicit completion over pipelining presentation/bake work.
        _engine->device()->immediateSubmit([&](gpu::CommandBuffer* command)
        {
            bake::BakeFrame frame{*command, 0, serial, _context->sourceGeneration()};
            _context->prepareFrame(frame);
            _baker->record(frame);
        });
        _context->completeFrame(0, serial);
        const auto progress = _baker->progress();
        _status = "Target " + std::to_string(_target + 1) + "/" + std::to_string(_targets.size()) +
            ": " + std::to_string(progress.completedSamples) + "/" + std::to_string(progress.requestedSamples) +
            " samples. " + progress.status;
        if (progress.state != bake::BakeState::Complete)
        {
            if (progress.state != bake::BakeState::Running)
            {
                throw std::runtime_error(progress.status);
            }

            return;
        }

        const auto result = _baker->result();
        if (!result)
        {
            throw std::runtime_error("Completed baker did not publish a current result");
        }

        _results.push_back(*result);
        _cpuResults.push_back(_baker->readPixels());
        _baker->cleanup();
        _baker.reset();
        ++_target;
        if (_target < _targets.size())
        {
            _baker = _context->createBaker({_targets[_target], _settings});
            return;
        }

        _context->cleanup();
        _context.reset();
        // Adoption changes materials/source identity only after every target's
        // immutable unbaked snapshot and typed CPU output have been consumed.
        prepareMaps();
        setApplied(true);
        _running = false;
        _status = "Complete. " + std::to_string(_targets.size()) + " maps applied; live AO/GI disabled.";
    }

    void prepareMaps()
    {
        for (const auto& result : _results)
        {
            std::vector<uint8_t> rgba;
            result.image->readPixels(rgba, result.image->fullRange());
            if (result.image->pixelFormat() != gpu::PixelFormat::R32G32B32A32_SFLOAT ||
                rgba.size() != static_cast<size_t>(result.extent.width) * result.extent.height * 4 * sizeof(float))
            {
                throw std::runtime_error("Unexpected completed lightmap format or byte count");
            }

            // G-buffer multiplier storage is half float (32 attachment bytes
            // total on Metal). Refuse overflow rather than clipping HDR maps.
            for (size_t i = 0; i < rgba.size(); i += sizeof(float))
            {
                float value;
                std::memcpy(&value, rgba.data() + i, sizeof(value));
                if (!std::isfinite(value) || value < 0 || value > 65504)
                {
                    throw std::runtime_error("Lightmap exceeds the finite RGBA16F display range");
                }
            }

            auto settings = std::make_shared<base::Texture>();
            settings->setUseMipmaps(false);
            settings->setColorType(base::Color::TypeLinear);
            settings->setAddressMode(base::Texture::AddressModeClampToEdge);
            const auto filter = _engine->device()->formatCapabilities(result.image->pixelFormat()).linearFiltering
                ? base::Texture::FilterLinear : base::Texture::FilterNearest;
            settings->setMinFilter(filter);
            settings->setMagFilter(filter);
            auto texture = std::make_shared<draw::Texture>(_engine);
            texture->load(settings, result.image);
            _mapTextures.push_back(std::move(texture));
        }
    }

    void setApplied(bool enabled)
    {
        if (_mapTextures.size() != _targets.size())
        {
            throw std::logic_error("All maps must be ready before adoption");
        }

        _engine->device()->waitIdle();
        for (size_t i = 0; i < _targets.size(); ++i)
        {
            const auto drawable = _targets[i]->drawable()->drawDrawable();
            for (uint32_t submesh = 0; submesh < drawable->submeshesCount(); ++submesh)
            {
                auto material = drawable->drawMaterial(submesh);
                material->setTexture(draw::TextureSlot::AmbientOcclusion, enabled ? _mapTextures[i] :
                    draw::Texture::colorTexture(_engine, base::Color::White()));
                auto& attributes = material->materialAttributes();
                attributes.setAoUVSet(1);
                attributes.setAoScale(glm::vec2(1));
                attributes.setIndirectTextureMode(enabled ?
                    (_results[i].mode == bake::LightmapMode::RTGI ? IndirectMode::BakedGlobalIllumination :
                        IndirectMode::BakedAmbientOcclusion) : IndirectMode::AmbientOcclusion);
                drawable->materialDefinition(submesh) = attributes;
            }
        }

        _useBaked = enabled;
        renderer()->invalidateHistories();
        requestSceneFrame();
    }

    void refreshPreview()
    {
        auto mesh = _targets[_selected]->drawable()->drawDrawable()->mesh();
        bake::UvMapPreviewRenderer preview(*_engine->device(), 256);
        preview.render(*mesh, _previewUv);
        _uvImage = preview.image();
        const auto validation = geo::UvAtlasValidator::validate(*mesh, _previewUv);
        _uvStatus = validation.valid ? "Valid atlas" : "Atlas: " + validation.message;
        _refreshPreview = false;
    }

    std::shared_ptr<bake::LightmapSettings> _settings = std::make_shared<bake::LightmapSettings>();
    std::vector<std::shared_ptr<scene::Node>> _targets;
    std::vector<std::shared_ptr<draw::Drawable>> _sources, _unbaked;
    std::unique_ptr<bake::IntegratedBakerContext> _context;
    std::shared_ptr<bake::IntegratedLightmapBaker> _baker;
    std::vector<bake::LightmapResult> _results;
    std::vector<bake::LightmapPixels> _cpuResults;
    std::vector<std::shared_ptr<draw::Texture>> _mapTextures;
    std::shared_ptr<gpu::Image> _uvImage;
    std::function<void()> _uiCleanup;
    std::optional<bool> _pendingUseBaked;
    size_t _target = 0, _selected = 0;
    uint64_t _serial = 0;
    uint32_t _previewUv = 1;
    bool _running = false, _start = false, _cancel = false, _useBaked = false, _refreshPreview = true;
    std::string _status = "Ready. Configure settings and start baking.", _uvStatus;
};

class UiDelegate final : public ui::UserInterfaceDelegate, public std::enable_shared_from_this<UiDelegate> {
  public:
    explicit UiDelegate(std::weak_ptr<SceneDelegate> delegate) : _delegate(std::move(delegate))
    {
        _window.setTitle("Integrated lightmaps");
        _window.setPosition(10, 10);
        _window.setSize(470, 850);
    }

    void init(draw::Engine* engine, ui::UserInterface* interface) override
    {
        _engine = engine;
        _interface = interface;
        const auto weakUi = weak_from_this();
        if (auto delegate = _delegate.lock())
        {
            delegate->setUiCleanup([weakUi]()
            {
                if (auto ui = weakUi.lock())
                {
                    ui->_uv.clear();
                    ui->_map.clear();
                    ui->_uvImage.reset();
                    ui->_mapImage.reset();
                    ui->_engine = nullptr;
                    ui->_interface = nullptr;
                }

            });
        }
    }

    void drawUI() override
    {
        auto delegate = _delegate.lock();
        if (!delegate || !_interface || !_engine || !delegate->scene())
        {
            return;
        }

        _window.draw([&]()
        {
            ui::Text::text("Live indirect AO/GI: disabled. IBL, direct light and emission remain enabled.");
            ui::Text::text(delegate->status());
            if (ui::Button::button("Generate UV2 and bake / restart", false, delegate->running()))
            {
                delegate->requestStart();
            }

            if (ui::Button::button("Cancel", true, !delegate->running()))
            {
                delegate->requestCancel();
            }

            bool applied = delegate->useBaked();
            if (ui::Button::checkBox("Apply baked indirect maps", &applied, false,
                delegate->running() || !delegate->hasResults()))
            {
                delegate->requestUseBaked(applied);
            }

            if (!delegate->running())
            {
                ui::ReflectionWidget::drawProperties(delegate->settings()->settingsInstance(), delegate->settings()->settingsType());
            }

            ui::Text::separator("UV diagnostic");
            ui::Text::text(delegate->selectedName());
            if (ui::Button::button("Next target"))
            {
                delegate->nextPreview();
            }

            if (ui::Button::button("UV1", true))
            {
                delegate->setPreviewUv(0);
            }

            if (ui::Button::button("UV2", true))
            {
                delegate->setPreviewUv(1);
            }

            bind(delegate->uvImage(), _uvImage, _uv);
            ui::Text::text(delegate->uvStatus());
            _uv.draw(240, 240);
            ui::Text::separator("Completed multiplier (linear RGB)");
            bind(delegate->lightmapImage(), _mapImage, _map);
            _map.draw(240, 240);
            ui::Text::text("Only floor and the two opaque cubes are baked. Walls/emission stay unbaked; transparent geometry is display-only.");
        });
    }

  private:
    void bind(std::shared_ptr<gpu::Image> image, std::shared_ptr<gpu::Image>& previous, ui::BakeLightmapPreview& widget)
    {
        if (image != previous || (image && !widget.valid()))
        {
            widget.clear();
            previous = std::move(image);
            if (previous)
            {
                widget.setImage(*_interface, *_engine, previous);
            }
        }

    }

    std::weak_ptr<SceneDelegate> _delegate;
    draw::Engine* _engine = nullptr;
    ui::UserInterface* _interface = nullptr;
    ui::Window _window;
    ui::BakeLightmapPreview _uv, _map;
    std::shared_ptr<gpu::Image> _uvImage, _mapImage;
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
        bg2e::app::MainLoop loop("org.bg2e.examples.draw.lightmaps");
        Application application;
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e draw: integrated lightmaps", 1440, 960));
        return loop.run(&application, application.config());
    }

    catch (const std::exception& error)
    {
        std::cerr << "draw_lightmaps: " << error.what() << '\n';
        return 1;
    }
}
