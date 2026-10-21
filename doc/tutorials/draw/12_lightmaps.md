# Build an application that bakes and displays lightmaps

In this tutorial you will build the complete `draw_lightmaps` application from
an empty C++ file. You will create a small room, generate a separate UV2 atlas
for each receiving object, accumulate an AO or GI lightmap, and apply the
completed maps to the materials. A settings panel will let you restart or
cancel the bake, inspect UV charts, and compare baked and unbaked lighting.

The application uses `bg2e::draw` and `bg2e::bake`. It does not use the production
 Draw is the production rendering API over GPU backends. You should be familiar with the delegates
and deferred scene in [tutorial 10](10_deferred_renderer.md), and with the UI
pattern in [tutorial 11](11_manipulation.md).

You need C++20, the repository dependencies, and a supported Metal or Vulkan
device with the ray tracing capabilities required by the selected bake mode.
The room uses procedural geometry and needs no external model or texture.

The code blocks below are **consecutive pieces of one source file**. Append
each block after the previous one, in order. Some steps deliberately leave a
class open for the next step; do not add closing braces until shown. Once step
17 is complete, the assembled file matches the shipped
[main.cpp](../../../examples/draw/12_lightmaps/src/main.cpp), including its
resource cleanup and error handling. Intermediate steps are not separate
compilable programs.

## 1. Prepare the source file and camera

Work in `examples/draw/12_lightmaps/src/main.cpp`. The checkout already
registers the `draw_lightmaps` target; no build-system change is needed.
Start with the repository GPL header and the includes below. Keep the unnamed
namespace open until we have defined the application class.

The camera uses right-handed coordinates and zero-to-one depth. Explicitly
choosing this convention keeps the deferred depth buffer consistent with the
renderer. `IndirectMode` will later select how a material interprets its baked
texture.

```cpp
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
```



## 2. Configure the renderer and bake settings

Open `SceneDelegate` and add its constructor and renderer factory. The
inherited delegate owns the engine and deferred renderer. We keep continuous
rendering enabled so its `prepareFrame()` callback can advance the bake.

Disable live AO, GI and reflections for a stable baked/unbaked comparison.
Direct illumination, IBL and emission remain visible. These are choices for
this demonstration; they do not disable the baker's AO or GI implementation.

Use a small 128-pixel atlas, 16 accumulation frames and four samples per pixel
for the initial experiment. Request floating-point CPU output so reading a GI
map preserves its linear values. These are application overrides, not the
library defaults.

```cpp
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
```



## 3. Expose state and queue user actions

Continue inside the public section of `SceneDelegate`. The UI needs read-only
access to progress, images and settings. It must also request actions without
rebuilding meshes or modifying GPU resources during UI drawing.

The request methods therefore set flags and request a scene frame. The next
`prepareFrame()` consumes them. `hasResults()` only enables application of maps
when every target has both a completed result and a display texture. UV set
indices are zero-based: the UI label UV1 means index 0, and UV2 means index 1.

```cpp
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
```



## 4. Advance work at the frame boundary

Append the frame callback, still in the public section. The helper methods
it calls will be defined below in the same class.

First let the default delegate prepare the scene. Cancellation retires the
active bake and discards partial results. Otherwise start a requested bake,
record one sample if it is running, apply a requested comparison change, and
refresh the selected UV preview. Exceptions become visible status messages
instead of escaping the render loop. The context's failure path retains the
responsibility for draining possibly submitted GPU work.

```cpp
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
```



## 5. Release resources in dependency order

Add the public cleanup override before defining the scene. The UI registers
a callback that releases its texture registrations while the engine is still
available. Then retire bake work and completed images, release renderer
snapshots through the base delegate, and clean the drawables.

Keep the original target drawables alive in `_sources`: a later restart will
regenerate UV2 from those unbaked inputs. `_unbaked` owns the other room
objects. Both collections also need explicit cleanup.

```cpp
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
            node->drawable()->drawable()->cleanup();
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
```



## 6. Build the complete test room

Append the protected section and `createScene()`. The floor, white cube and
independent red clone are the three lightmap receivers. The back and green
side walls remain unbaked scene geometry. They provide surrounding surfaces
for the indirect-lighting experiment. The transparent object is display-only;
the small emissive cube is a separate visual reference.

Create one fixed omni light and a camera looking into the room. The static
camera and light make comparison repeatable. The `add()` and `makeDrawable()`
helpers used here are supplied in the next step.

```cpp
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
```



## 7. Add geometry and ownership helpers

Open the private section. A primitive cube normally has one submesh per face;
`cubeMesh()` merges those ranges so each cube uses a single material.
`makeDrawable()` establishes consistent opaque material defaults and uploads
the geometry through the draw engine.

`add()` attaches a transform and drawable component to a node. For receivers,
retain both the node that will receive a replacement mesh and its original
source drawable. Other objects enter the cleanup collection without becoming
bake targets.

```cpp
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
```



## 8. Generate UV2 and start an integrated context

Continue in the private section with bake retirement and initialization.
Validate the settings and require the deferred renderer's generated environment
resources before creating the context. If they are not ready, the status
message asks the user to retry after a scene frame.

Generate every replacement before adopting any of them. Each target receives
its own rebuilt drawable, including the clone. Clear previous results only
after those replacements have been produced, install them, and invalidate the
renderer histories.

The context receives the existing engine device, backend, scene and environment.
This application uses one frame slot and one active baker at a time. Start
with the first target; subsequent targets will be scheduled after completion.

```cpp
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
```



## 9. Submit samples and collect completed targets

Append `sample()`. This demonstration submits one synchronous bake sample
before the window frame is acquired. The command passed to `immediateSubmit()`
is already recording. Construct a `BakeFrame` with a monotonically increasing
serial and the context's current source generation, then prepare and record.

Only report `completeFrame()` after `immediateSubmit()` has returned
successfully: that return establishes GPU completion. Recording alone is not
completion. Update the UI status from the baker's progress.

Once a target completes, require a published result, retain its GPU image and
read the typed CPU pixels before cleaning that baker. Create the next baker
until all three targets finish. Finally retire the context, prepare display
textures and apply the entire set. No partial target set is applied.

```cpp
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
```



## 10. Wrap completed images as linear textures

Append `prepareMaps()`. Completed GPU maps contain RGBA32F data. Verify their
format and byte count, then check that their values fit the finite half-float
range used by the display G-buffer. The 65504 limit here is a display-path
constraint; the CPU output retained in the previous step remains floating point.

Wrap each image in a draw texture with linear color interpretation, no mipmaps
and clamp-to-edge addressing. Ask the device whether this format supports
linear filtering and fall back to nearest filtering when necessary. Do not
interpret lightmaps as sRGB color photographs.

```cpp
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
```



## 11. Apply or remove the baked contribution

Append `setApplied()`. Require one texture per target and wait for existing
GPU use before changing live material bindings. The application stores the
map in the ambient-occlusion texture slot, but the explicit indirect texture
mode determines whether it represents baked AO or baked GI.

Select UV set 1, which is the generated UV2 atlas. When disabling the baked
comparison, bind a white texture and restore ordinary AO interpretation.
Copy the updated attributes back to the drawable's material definition and
invalidate temporal histories so stale display data does not hide the change.
The direct light and emission are still evaluated separately.

```cpp
    void setApplied(bool enabled)
    {
        if (_mapTextures.size() != _targets.size())
        {
            throw std::logic_error("All maps must be ready before adoption");
        }

        _engine->device()->waitIdle();
        for (size_t i = 0; i < _targets.size(); ++i)
        {
            const auto drawable = _targets[i]->drawable()->drawable();
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
```



## 12. Render UV diagnostics and finish scene state

Append the preview helper and all scene delegate members, then close the
class exactly as shown. The UV renderer creates a diagnostic image for the
selected mesh and channel; atlas validation supplies the accompanying message.
Retaining the returned image lets the UI display it after the local preview
renderer goes out of scope.

The member collections distinguish original sources, active bake owners,
completed GPU/CPU results, and display textures. Flags transfer UI requests to
the frame callback. The settings owner outlives each baker invocation.

```cpp
    void refreshPreview()
    {
        auto mesh = _targets[_selected]->drawable()->drawable()->mesh();
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
```



## 13. Initialize the settings and preview window

Define `UiDelegate` after `SceneDelegate`. Store a weak reference to the scene
delegate so the two objects do not form an ownership cycle. The window will
contain settings and two image previews.

During initialization, register a cleanup callback with the scene delegate.
It clears both preview widgets and retained image owners before the engine
is torn down. Capturing `weak_from_this()` avoids keeping the UI alive solely
through this callback. We will create the delegate with `make_shared` in step
16, before the framework calls `init()`.

```cpp
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
```



## 14. Connect every control to the scene delegate

Continue inside the UI class with `drawUI()`. Guard against missing scene or
engine state, then draw the window through the public `bg2e::ui` wrappers.
No ImGui headers or direct ImGui calls are needed.

The start and cancel buttons queue requests. The apply checkbox is unavailable
until all maps are ready and while a bake is running. The reflected settings
editor is also hidden during accumulation, keeping the active settings fixed.
Select AO or GI there before starting a new bake.

The target and UV buttons change the diagnostic preview. The lower preview
shows the selected target's completed linear RGB multiplier, rather than a
camera render. `bind()` in the next step updates registrations when the
underlying image changes.

```cpp
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
```



## 15. Bind preview images and finish the UI state

Append the private image binding helper and members, then close `UiDelegate`.
Clear an old registration before retaining and registering its replacement.
Also re-register an unchanged image if the widget's session is no longer valid.
`BakeLightmapPreview` supplies the bridge between a GPU image and the draw UI;
the application does not create or cache raw backend texture handles.

```cpp
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
```



## 16. Wire the application delegates

Define `Application` and close the unnamed namespace. Install a shared scene
delegate, the default input delegate and a shared UI delegate referring to that
same scene owner. Expose the engine configuration for `MainLoop`.

There is no camera controller in this example: the fixed viewpoint is part of
the comparison setup. Baking is driven by `SceneDelegate::prepareFrame()`, not
by the input delegate or the UI callback.

```cpp
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
```



## 17. Add the entry point

Append `main()` outside the unnamed namespace. Initialize the application,
request a 1440 × 960 window and run the loop with its engine configuration.
An initialization failure is reported to standard error and returns a failing
exit code. This completes the source file.

```cpp
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
```



## 18. Run the application and compare the results

Build the existing `draw_lightmaps` target using your normal repository build
workflow, then launch it from the platform output directory. On macOS the
application is `bin/macos/draw_lightmaps.app`; use the deployed application
bundle so its native shader libraries can be found. On Linux and Windows use
the corresponding platform output directory. The build must deploy both draw
and bake shaders; see [shader deployment](../../api/gpu/ShaderLibraries.md).

Follow this sequence in the **Integrated lightmaps** window:

1. Before baking, inspect **UV1** and **UV2** for each object with **Next target**.
   The current geometry has not yet received this application's generated atlas.
2. Set the bake mode to AO or GI in the settings editor. Keep the initial small
   resolution and sample settings for the first run.
3. Press **Generate UV2 and bake / restart**. The status reports the current
   target and completed/requested samples. UV2 now shows the generated atlas.
4. Wait for the complete status. All three maps are applied together. Select
   each target to inspect its completed multiplier and UV2 validation message.
5. Toggle **Apply baked indirect maps** to compare the same static scene with
   and without the completed maps. Live AO/GI remain disabled in both cases.
6. Change the mode or sample settings while idle and restart. You can also
   cancel during accumulation; cancelled partial results are not applied.

For AO, examine contact and occluded regions. For GI, inspect the indirect
contribution around the colored objects and walls. The lightmap preview is
linear map data, so its appearance is not the same as a tone-mapped view of
the room. Raising resolution or sample settings increases work; this example
waits for each sample rather than overlapping bake and presentation work.

If the environment is not ready, allow another scene frame and retry. A missing
`bake/uv_surface/surface.vert.metallib` means the native bake shader was not
deployed: rebuilding only the executable source will not repair an incomplete
runtime shader directory. Other capability or data failures appear in the
status message through the error path added in step 4.

## What you have built

You now have a complete windowed lightmap experiment: procedural scene creation,
independent UV2 generation, an integrated bake scheduler, completed GPU and CPU
outputs, explicit material application, a settings editor, UV diagnostics and
resource cleanup. It does not write image or scene files, provide a standalone
batch baker, or denoise the output.

Use the [application guide](../../apps/draw_lightmaps.md) for control details,
[LightmapSettings](../../api/bake/LightmapSettings.md) for parameter ranges,
[IntegratedBakerContext](../../api/bake/IntegratedBakerContext.md) for completion
and ownership rules, and [baked indirect maps](../../api/draw/BakedIndirectMaps.md)
for the material interpretation contract.
