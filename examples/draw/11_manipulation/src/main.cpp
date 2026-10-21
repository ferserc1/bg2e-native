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
#include <bg2e/app/PreferencesStore.hpp>
#include <bg2e/draw/RenderSettingsPreferences.hpp>
#include <bg2e/ui/GraphicsSettingsWindow.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <bg2e/app/Keyboard.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace bg2e;

constexpr float RenderScale = 0.7f;

class CameraProjection final : public math::PerspectiveProjection {
  public:
    void apply(glm::mat4& matrix) override
    {
        matrix = glm::perspectiveRH_ZO(glm::radians(fov()), _viewport.width / _viewport.height, near(), far());
    }
};

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
        return "draw-manipulation-alpha-checker-v1";
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
        return "draw-manipulation-packed-channels-v1";
    }
};

struct SceneState {
    std::shared_ptr<scene::Node> cameraNode;
    std::shared_ptr<scene::Node> lights;
    std::shared_ptr<scene::Node> canopy;
    std::shared_ptr<draw::Drawable> clone;
    std::vector<std::shared_ptr<scene::Node>> lightNodes;
    std::shared_ptr<scene::Scene> sceneContext;
    bool animateLights = true;
    float lightAngle = 0.0f;
};

class SceneDelegate final : public draw::DefaultRenderLoopDelegate {
  public:
    SceneDelegate()
        : DefaultRenderLoopDelegate(std::make_unique<draw::RendererDeferred>(), makeRendererConfiguration())
    {
        setContinuousRendering(true);
    }

    static draw::RendererConfiguration makeRendererConfiguration()
    {
        draw::RendererConfiguration configuration;
        // Render at a fixed 70% of the output dimensions to demonstrate picking
        // with a scaled image. The shared settings catalog can change this scale.
        // init() uses the same constant to configure the selection API.
        configuration.resolution = std::make_shared<draw::ResolutionSettings>();
        configuration.resolution->setEnabled(true);
        configuration.resolution->setScale(RenderScale);
        configuration.upscaler = std::make_shared<draw::FsrUpscaler>();
        std::static_pointer_cast<draw::FsrUpscaler>(configuration.upscaler)->setEnabled(false);
        configuration.finalProcessor = std::make_shared<draw::pipeline::SMAAProcessor>();
        configuration.indirectLighting = draw::IndirectLightingMode::GlobalIllumination;
        configuration.globalIllumination = std::make_shared<draw::pipeline::RTGlobalIllumination>();
        configuration.globalIllumination->setEnabled(true);
        configuration.reflections = std::make_shared<draw::pipeline::RTReflections>();
        configuration.reflections->setEnabled(true);
        configuration.overlays.push_back(std::make_shared<manipulation::GizmoAndSelectionRenderer>());
        return configuration;
    }

    SceneState& state()
    {
        return _state;
    }

    std::shared_ptr<manipulation::SelectionManager> selection() const
    {
        return _selection;
    }

    void setWeakSelf(const std::shared_ptr<SceneDelegate>& self)
    {
        _self = self;
    }

    void requestRedraw()
    {
        requestSceneFrame();
    }

    void setUiCleanup(std::function<void()> cleanup)
    {
        _uiCleanup = std::move(cleanup);
    }

    void init(draw::Engine* engine) override
    {
        DefaultRenderLoopDelegate::init(engine);
        _selection = std::make_shared<manipulation::SelectionManager>(engine);
        _selection->init();
        _selection->setMultiSelectionEnabled(true);
        _selection->setClearSelectionOnEmptyPick(true);
        // Match the fixed scale chosen in makeRendererConfiguration(). Configure
        // picking once, without reading renderer extents: the renderer has not
        // received its initial output surface yet, so those extents are zero.
        // Mouse input and the camera viewport use logical window coordinates;
        // this gives the independent picking image 0.7 pixels per logical unit.
        // On Retina, rendering uses 70% of physical output pixels, while picking
        // uses 70% of logical dimensions; their pixel counts need not match.
        _selection->setPickPixelScale(RenderScale, RenderScale);
        const auto weakDelegate = _self;
        _selection->onSelect([weakDelegate]()
        {
            if (auto delegate = weakDelegate.lock())
            {
                delegate->requestSceneFrame();
            }
        });
        _analyzer = std::make_unique<utils::ViewpointAnalyzer>(engine);
        _analyzer->init();
    }

    void surfaceChanged(gpu::Size2D extent, gpu::PixelFormat format) override
    {
        DefaultRenderLoopDelegate::surfaceChanged(extent, format);
        setLogicalCameraViewport();
    }

    void resize(gpu::Size2D extent) override
    {
        DefaultRenderLoopDelegate::resize(extent);
        setLogicalCameraViewport();
    }

    void prepareFrame() override
    {
        DefaultRenderLoopDelegate::prepareFrame();
        setLogicalCameraViewport();
    }

    void update(const draw::FrameContext& frame) override
    {
        _selection->refreshSelection();
        if (_state.animateLights)
        {
            _state.lightAngle += frame.sceneDeltaSeconds * 0.2f;
            _state.lights->transform()->setRotation(_state.lightAngle, 0, 1, 0);
        }
        DefaultRenderLoopDelegate::update(frame);
    }

    void requestBestView()
    {
        const auto weakDelegate = _self;
        enqueueRendererUpdate(rendererGeneration(), [weakDelegate](draw::Renderer& renderer)
        {
            auto delegate = weakDelegate.lock();
            if (!delegate || !delegate->_analyzer || !delegate->_state.sceneContext)
            {
                return;
            }
            const auto samples = delegate->_analyzer->analyze(delegate->_state.sceneContext.get());
            const auto* best = utils::ViewpointAnalyzer::bestSample(samples);
            if (!best)
            {
                return;
            }
            // Apply the candidate to the controller and camera on the same node.
            auto cameraNode = delegate->_state.cameraNode;
            auto* orbit = cameraNode->getComponent<scene::OrbitCameraComponent>();
            renderer.invalidateHistories();
            utils::applyViewpointToOrbitCamera(best->camera, orbit, cameraNode->camera());
            delegate->_state.sceneContext->updateCamera();
        });
    }

    void removeSelected()
    {
        const auto selection = _selection;
        const auto node = selection ? selection->selectedNode() : nullptr;
        if (!node || !node->parent())
        {
            return;
        }
        const auto parent = node->parent()->shared_from_this();
        const auto selected = node->shared_from_this();
        const auto weakDelegate = _self;
        enqueueRendererUpdate(rendererGeneration(), [weakDelegate, parent, selected, selection](draw::Renderer& renderer)
        {
            if (auto delegate = weakDelegate.lock())
            {
                selection->deselect();
                struct RemovedGizmoCleanupVisitor : scene::NodeVisitor {
                    void visit(scene::Node* node) override
                    {
                        if (auto* gizmo = node->getComponent<manipulation::GizmoComponent>())
                        {
                            auto drawable = gizmo->drawable();
                            auto transformDrawable = gizmo->transformDrawable();
                            if (drawable)
                            {
                                drawable->cleanup();
                            }
                            if (transformDrawable && transformDrawable != drawable)
                            {
                                transformDrawable->cleanup();
                            }
                            gizmo->cleanupDrawResources();
                        }
                    }
                } cleanupVisitor;
                selected->accept(&cleanupVisitor);
                parent->removeChild(selected);
                selection->refreshSelection();
                delegate->_state.sceneContext->updateAll();
                renderer.invalidateHistories();
            }
        });
    }

    void cleanup() override
    {
        if (_uiCleanup)
        {
            _uiCleanup();
            _uiCleanup = nullptr;
        }
        DefaultRenderLoopDelegate::cleanup();
        if (_state.sceneContext && _state.sceneContext->rootNode())
        {
            struct GizmoCleanupVisitor : scene::NodeVisitor {
                void visit(scene::Node* node) override
                {
                    if (auto* gizmo = node->getComponent<manipulation::GizmoComponent>())
                    {
                        auto drawable = gizmo->drawable();
                        auto transformDrawable = gizmo->transformDrawable();
                        if (drawable)
                        {
                            drawable->cleanup();
                        }
                        if (transformDrawable && transformDrawable != drawable)
                        {
                            transformDrawable->cleanup();
                        }
                        gizmo->cleanupDrawResources();
                    }
                }
            } cleanupVisitor;
            _state.sceneContext->rootNode()->accept(&cleanupVisitor);
        }
        if (_analyzer)
        {
            _analyzer->cleanup();
            _analyzer.reset();
        }
        for (auto& drawable : _drawables)
        {
            drawable->cleanup();
        }
        _selection.reset();
    }

  protected:
    std::shared_ptr<scene::Node> createScene() override
    {
        auto root = std::make_shared<scene::Node>("Manipulation scene");
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
            model->setSubmeshGroupName(i == 0 ? "body/front" : "body/back", i);
        }
        model->material(1).setFresnelTint(base::Color(0.8f, 0.2f, 1));
        model->material(1).setSheenColor(base::Color::Blue());
        model->material(1).setSheenIntensity(0.5f);
        load(model);

        auto nested = std::make_shared<scene::Node>("Models");
        nested->addComponent(scene::TransformComponent::makeTranslated(0, 0.3f, 0));
        root->addChild(nested);
        addDrawable(nested, "Original model", model, {-3, 0, 0});

        _state.clone = model->clone();
        _state.clone->materialDefinition(0).setAlbedo(base::Color(0.3f, 0.8f, 1));
        load(_state.clone);
        addDrawable(nested, "Independent clone", _state.clone, {0, 0, -1});

        auto glass = model->clone();
        glass->iterateMaterials([](auto& material)
        {
            material.setAlbedo(base::Color(1, 1, 1, 0.45f));
            material.setIsTransparent(true);
            material.setRefractionFactor(0.1f);
        });
        load(glass);
        addDrawable(root, "Transparent model", glass, {3, 0, 0});

        auto solidGlass = model->clone();
        solidGlass->iterateMaterials([](auto& material)
        {
            material.setAlbedo(base::Color(0.7f, 1, 0.7f, 0.5f));
            material.setIsTransparent(true);
            material.setIsSolid(true);
        });
        load(solidGlass);
        addDrawable(root, "Solid transparent model", solidGlass, {1.5f, 0, 2.5f});

        auto floor = std::make_shared<draw::Drawable>();
        floor->setMesh(geo::createPlane(30, 30));
        floor->material().setAlbedo(base::Color(0.55f, 0.55f, 0.55f));
        floor->material().setMetalness(0);
        floor->material().setRoughness(0.75f);
        load(floor);
        addDrawable(root, "Floor", floor, {0, -1.05f, 0}, false);

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
        _state.canopy = addDrawable(root, "Alpha cutout", canopy, {-3, 2.4f, 1.5f});

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
        addDrawable(root, "Packed texture channels", channels, {-1.5f, -0.3f, 2.5f});

        // A single camera node owns all three components. The orbit controller
        // updates this transform, and CameraComponent reads it as the camera pose.
        // Do not add a child-camera transform: it would offset the analyzed view.
        _state.cameraNode = std::make_shared<scene::Node>("Camera");
        _state.cameraNode->addComponent(new scene::TransformComponent());
        _state.cameraNode->addComponent(new scene::OrbitCameraComponent());
        _state.cameraNode->addComponent(new scene::CameraComponent());
        _state.cameraNode->camera()->setProjection(new CameraProjection());
        auto* orbit = _state.cameraNode->getComponent<scene::OrbitCameraComponent>();
        orbit->setCenter({0, 0.5f, 0});
        orbit->setDistance(14.5f);
        orbit->setRotation({0.0f, 0.15f});
        orbit->setRotationSpeed(0.008f);
        orbit->setWheelSpeed(0.4f);
        // Use the editor camera controls. ManipulationInput distinguishes a
        // left click (picking) from a left drag (orbiting), unless a gizmo captures it.
        orbit->setRotateButtons(true, false, false);
        orbit->setPanButtons(false, false, true);
        orbit->setZoomButtons(false, true, false);
        root->addChild(_state.cameraNode);

        _state.lights = std::make_shared<scene::Node>("Lights");
        _state.lights->addComponent(new scene::TransformComponent());
        root->addChild(_state.lights);
        for (uint32_t i = 0; i < 4; ++i)
        {
            auto light = std::make_shared<scene::Node>("Light " + std::to_string(i + 1));
            const float angle = float(i) * 1.57079633f;
            light->addComponent(scene::TransformComponent::makeTranslated(std::sin(angle) * 5, 5, std::cos(angle) * 5));
            light->addComponent(new scene::LightComponent());
            light->light()->light().setType(base::Light::TypeOmni);
            light->light()->light().setColor(base::Color(i == 0 ? 1 : 0.65f, i == 1 ? 1 : 0.7f, i == 2 ? 1 : 0.75f));
            light->light()->light().setIntensity(35);
            light->light()->light().setSourceSize(0.25f);
            light->light()->light().setShadowSamples(8);
            _state.lightNodes.push_back(light);
            _state.lights->addChild(light);
        }

        _state.sceneContext = std::make_shared<scene::Scene>();
        _state.sceneContext->setSceneRoot(root);
        _state.sceneContext->updateAll();
        setLogicalCameraViewport();
        return root;
    }

  private:
    void load(const std::shared_ptr<draw::Drawable>& drawable)
    {
        _drawables.push_back(drawable);
        drawable->load(_engine);
    }

    void setLogicalCameraViewport()
    {
        if (!_engine || !_state.sceneContext || !_state.sceneContext->rootNode())
        {
            return;
        }
        auto* camera = _state.sceneContext->mainCamera();
        if (!camera || !camera->projection())
        {
            return;
        }
        const auto size = app::MainLoop::current()->windowSize();
        if (size.x > 0 && size.y > 0)
        {
            camera->projection()->setViewport(math::Viewport(size.x, size.y));
        }
    }

    std::shared_ptr<scene::Node> addDrawable(const std::shared_ptr<scene::Node>& parent,
        const std::string& name, std::shared_ptr<draw::Drawable> drawable, glm::vec3 position, bool selectable = true)
    {
        auto node = std::make_shared<scene::Node>(name);
        node->addComponent(scene::TransformComponent::makeTranslated(position));
        node->addComponent(new scene::DrawableComponent(std::move(drawable)));
        if (selectable)
        {
            node->addComponent(new manipulation::SelectableComponent());
            node->addComponent(new manipulation::GizmoComponent(_engine));
        }
        parent->addChild(node);
        return node;
    }

    SceneState _state;
    std::vector<std::shared_ptr<draw::Drawable>> _drawables;
    std::shared_ptr<manipulation::SelectionManager> _selection;
    std::unique_ptr<utils::ViewpointAnalyzer> _analyzer;
    std::weak_ptr<SceneDelegate> _self;
    std::function<void()> _uiCleanup;
};

class ManipulationInput final : public app::InputDelegate {
  public:
    explicit ManipulationInput(std::weak_ptr<SceneDelegate> delegate) : _delegate(std::move(delegate))
    {
    }

    void mouseMove(int x, int y) override
    {
        auto delegate = _delegate.lock();
        auto context = delegate ? delegate->state().sceneContext : nullptr;
        auto selection = delegate ? delegate->selection() : nullptr;
        if (context && selection)
        {
            if (app::Keyboard::controlPressed())
            {
                _controlDown = true;
            }
            const auto propagate = selection->mouseMove(context.get(), x, y);
            if (propagate)
            {
                scene::InputVisitor visitor;
                visitor.mouseMove(context->rootNode(), x, y);
            }
            app::MainLoop::current()->requestSceneFrame();
        }
    }

    void mouseButtonDown(int button, int x, int y) override
    {
        auto delegate = _delegate.lock();
        auto context = delegate ? delegate->state().sceneContext : nullptr;
        auto selection = delegate ? delegate->selection() : nullptr;
        _controlDown = app::Keyboard::controlPressed();
        if (button == 0)
        {
            _leftButtonDown = true;
            _mouseDownX = x;
            _mouseDownY = y;
        }
        if (context && selection && selection->mouseButtonDown(context.get(), button, x, y))
        {
            // Forward the press immediately so OrbitCameraController can rotate
            // on subsequent movement. A gizmo-captured press stays out of the scene.
            scene::InputVisitor visitor;
            visitor.mouseButtonDown(context->rootNode(), button, x, y);
        }
        else if (context)
        {
            app::MainLoop::current()->requestSceneFrame();
        }
    }

    void mouseButtonUp(int button, int x, int y) override
    {
        auto delegate = _delegate.lock();
        auto context = delegate ? delegate->state().sceneContext : nullptr;
        auto selection = delegate ? delegate->selection() : nullptr;
        _controlDown = app::Keyboard::controlPressed() || _controlDown;
        if (context && selection)
        {
            if (selection->sceneInputBlocked())
            {
                // Finish a gizmo drag without picking or releasing a scene gesture
                // whose press was never forwarded to the camera controller.
                selection->mouseButtonUp(context.get(), button, x, y, _controlDown);
            }
            else
            {
                // This example owns click detection to allow +/-2 logical pixels
                // on each axis. Do not also call SelectionManager::mouseButtonUp:
                // its exact-position click detection would pick a second time.
                const bool click = button == 0 && _leftButtonDown &&
                    std::abs(int64_t(x) - _mouseDownX) <= 2 &&
                    std::abs(int64_t(y) - _mouseDownY) <= 2;
                if (click)
                {
                    selection->pick(context.get(), x, y, _controlDown);
                }
                // Always balance a forwarded press, including after a click, so
                // OrbitCameraController does not remain in its dragging state.
                scene::InputVisitor visitor;
                visitor.mouseButtonUp(context->rootNode(), button, x, y);
            }
            app::MainLoop::current()->requestSceneFrame();
        }
        if (button == 0)
        {
            _leftButtonDown = false;
        }
        _controlDown = app::Keyboard::controlPressed();
    }

    void mouseWheel(int deltaX, int deltaY) override
    {
        auto delegate = _delegate.lock();
        auto context = delegate ? delegate->state().sceneContext : nullptr;
        if (context)
        {
            scene::InputVisitor visitor;
            visitor.mouseWheel(context->rootNode(), deltaX, deltaY);
            app::MainLoop::current()->requestSceneFrame();
        }
    }

    void keyUp(const app::KeyEvent& event) override
    {
        if (event.key() == app::KeyEvent::KeyLeftControl || event.key() == app::KeyEvent::KeyRightControl)
        {
            _controlDown = app::Keyboard::controlPressed();
        }
        auto delegate = _delegate.lock();
        auto context = delegate ? delegate->state().sceneContext : nullptr;
        if (context)
        {
            scene::InputVisitor visitor;
            visitor.keyUp(context->rootNode(), event);
        }
    }

    void keyDown(const app::KeyEvent& event) override
    {
        if (event.key() == app::KeyEvent::KeyLeftControl || event.key() == app::KeyEvent::KeyRightControl)
        {
            _controlDown = true;
        }
        if (event.key() == app::KeyEvent::KeyDelete)
        {
            if (auto delegate = _delegate.lock())
            {
                delegate->removeSelected();
            }
            return;
        }
        if (auto delegate = _delegate.lock())
        {
            auto context = delegate->state().sceneContext;
            if (context)
            {
                scene::InputVisitor visitor;
                visitor.keyDown(context->rootNode(), event);
            }
        }
    }

  private:
    std::weak_ptr<SceneDelegate> _delegate;
    bool _controlDown = false;
    bool _leftButtonDown = false;
    int _mouseDownX = 0;
    int _mouseDownY = 0;
};

class UiDelegate final : public ui::UserInterfaceDelegate, public std::enable_shared_from_this<UiDelegate> {
  public:
    explicit UiDelegate(std::weak_ptr<SceneDelegate> delegate) : _delegate(std::move(delegate))
    {
        _controls.setTitle("Manipulation");
        _controls.setPosition(10, 10);
        _controls.setSize(430, 700);
        auto active = _delegate.lock();
        if (!active)
        {
            throw std::logic_error("Graphics settings require a live scene delegate");
        }
        // Connect before the first renderer attachment; callbacks rebind on replacement.
        auto& store = app::PreferencesStore::instance().preferences("draw.manipulation.graphics");
        _preferences = std::make_shared<draw::RenderSettingsPreferences>(&store);
        const auto weakDelegate = _delegate;
        _preferences->connect(active, [weakDelegate](draw::Renderer& renderer, draw::SettingsCatalog& catalog)
        {
            auto delegate = weakDelegate.lock();
            if (!delegate)
            {
                return;
            }
            using Transform = scene::TransformComponent;
            reflection::TypeInfoBuilder<Transform> type("draw.manipulation.lightPose.v1");
            type.accessor<glm::vec3>("position", [](const Transform& transform)
            {
                return transform.translation();
            }, [](Transform& transform, const glm::vec3& value)
            {
                transform.setTranslation(value);
            }).displayName("Position XYZ (local)").category("Local transform").drag().step(0.1);
            type.accessor<glm::vec3>("orientationDegrees", [](const Transform& transform)
            {
                return transform.eulerRotation();
            }, [](Transform& transform, const glm::vec3& value)
            {
                transform.setEulerRotation(value);
            }).displayName("Orientation XYZ (degrees)").category("Local transform").drag().step(1).range(-180,180);
            for (const auto& node : delegate->state().lightNodes)
            {
                // Node ownership keeps its component alive; catalog generation guards queued edits.
                catalog.add({"lightPose/"+node->identifier(),type.build(),node->transform(),node,{},
                    [weakDelegate,&renderer]
                    {
                        if (auto owner = weakDelegate.lock())
                        {
                            owner->state().animateLights = false;
                        }
                        renderer.invalidateHistories();
                    },{}});
            }
        });
        _graphics.bind(_preferences);
        _graphics.setPosition(460, 10);
        _graphics.setSize(520, 620);
        _graphics.open();
    }

    void init(draw::Engine* engine, ui::UserInterface* interface) override
    {
        _interface = interface;
        auto delegate = _delegate.lock();
        if (!delegate)
        {
            throw std::logic_error("Manipulation delegate expired during UI initialization");
        }
        _editor.init(engine, delegate);
        _editor.setSelectionManager(delegate->selection());
        const auto weakDelegate = std::weak_ptr<SceneDelegate>(delegate);
        _editor.onChanged([weakDelegate](draw::Renderer& renderer)
        {
            if (auto active = weakDelegate.lock())
            {
                renderer.invalidateHistories();
                active->requestRedraw();
            }
        });
        const auto weakUi = weak_from_this();
        delegate->setUiCleanup([weakUi]()
        {
            if (auto ui = weakUi.lock())
            {
                ui->cleanup();
            }
        });
    }

    void drawUI() override
    {
        auto delegate = _delegate.lock();
        if (!delegate || !delegate->scene() || !_interface)
        {
            return;
        }
        _graphics.draw();
        auto& state = delegate->state();
        _controls.draw([&]
        {
            if (ui::Button::button("Graphics settings"))
            {
                _graphics.open();
            }
            ui::Text::text("Click a submesh to select it. Ctrl-click adds/removes selection; drag the gizmo to transform.");
            ui::Text::text("Drag: left to orbit, right to pan, middle to zoom. Click tolerance: 2 pixels per axis.");
            ui::Text::separator("Scene");
            if (ui::Button::checkBox("Animate lights", &state.animateLights))
            {
                app::MainLoop::current()->requestSceneFrame();
            }
            if (ui::Button::button("Best viewpoint", true))
            {
                delegate->requestBestView();
            }
            if (ui::Button::button("Deselect", true, !delegate->selection()->selectedItem()))
            {
                delegate->selection()->deselect();
                app::MainLoop::current()->requestSceneFrame();
            }
            if (ui::Button::button("Delete selected", false, !delegate->selection()->selectedNode()))
            {
                delegate->removeSelected();
            }
            drawLights(delegate);
            ui::Text::separator("MaterialModifier presets");
            if (ui::Button::button("Scalar: roughness 0.22 / metalness 0.65", false, !hasSelection(delegate)))
            {
                _editor.queuePatch(utils::MaterialModifier(std::string{R"({"roughness":0.22,"metalness":0.65})"}));
            }
            if (ui::Button::button("Texture: packed channel, UV1 and cutoff", false, !hasSelection(delegate)))
            {
                _editor.queuePatch(utils::MaterialModifier(std::string{
                    R"({"metalnessTexture":"two_submeshes_outer_metallic.jpg","metalnessUV":1,"metalnessScale":[2,2],"alphaCutoff":0.2})"},
                    base::PlatformTools::assetPath()));
            }
            if (ui::Button::button("Group: body/front", false, !hasSelection(delegate)))
            {
                auto item = delegate->selection()->selectedItem();
                auto component = item ? item->drawable.lock() : nullptr;
                auto drawable = component ? component->drawable() : nullptr;
                if (drawable)
                {
                    const utils::MaterialModifier patch(std::string{R"({"roughness":0.6})"});
                    delegate->enqueueRendererUpdate(delegate->rendererGeneration(), [drawable, patch](draw::Renderer& renderer)
                    {
                        renderer.invalidateHistories();
                        patch.applyAll(*drawable, std::string("body/front"));
                    });
                }
            }
            if (ui::Button::button("Regex: all body groups", false, !hasSelection(delegate)))
            {
                auto item = delegate->selection()->selectedItem();
                auto component = item ? item->drawable.lock() : nullptr;
                auto drawable = component ? component->drawable() : nullptr;
                if (drawable)
                {
                    const utils::MaterialModifier patch(std::string{R"({"metalness":0.1})"});
                    const std::regex groupPattern("body/.*");
                    delegate->enqueueRendererUpdate(delegate->rendererGeneration(), [drawable, patch, groupPattern](draw::Renderer& renderer)
                    {
                        renderer.invalidateHistories();
                        patch.applyAll(*drawable, groupPattern);
                    });
                }
            }
            _editor.draw(*_interface);
            if (!_editor.error().empty())
            {
                ui::Text::text(_editor.error());
            }
            ui::Text::separator("Clone check");
            ui::Text::text("The cyan clone has independent material state. Select original and clone to compare patches.");
            float cloneRoughness = state.clone->material(0).roughness();
            if (ui::Numeric::sliderFloat("Clone roughness", &cloneRoughness, 0.02f, 1.0f))
            {
                edit(delegate, [cloneRoughness](SceneState& scene)
                {
                    scene.clone->materialDefinition(0).setRoughness(cloneRoughness);
                    scene.clone->drawMaterial(0)->materialAttributes().setRoughness(cloneRoughness);
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
            ui::Text::separator("Selected components");
            auto item = delegate->selection()->selectedItem();
            if (item && item->node.lock())
            {
                ui::Text::text(item->node.lock()->name() + " / submesh " + std::to_string(item->submesh));
            }
            else
            {
                ui::Text::text("No drawable submesh selected");
            }
        });
    }

    void cleanup()
    {
        _editor.cleanup();
        _graphics.clear();
        _preferences->clear();
        _interface = nullptr;
    }

  private:
    void drawLights(const std::shared_ptr<SceneDelegate>& delegate)
    {
        if (!ui::Group::beginTree("Lights"))
        {
            return;
        }
        ui::Text::text("Editing a light pauses animation. Position and Euler angles are local to the Lights group.");
        ui::Text::text("Orientation affects spot and directional lights; point lights emit in every direction.");
        const auto descriptors = _preferences->catalog().properties(true);
        for (const auto& node : delegate->state().lightNodes)
        {
            if (!ui::Group::beginTree(node->name()+"##lightEditor"+node->identifier()))
            {
                continue;
            }
            const auto properties = "light/"+node->identifier();
            const auto pose = "lightPose/"+node->identifier();
            for (const auto& descriptor : descriptors)
            {
                if (descriptor.handle.component != properties && descriptor.handle.component != pose)
                {
                    continue;
                }
                auto value = _preferences->catalog().read(descriptor.handle);
                auto property = descriptor.property;
                if (ui::ReflectionWidget::editValue(value,property,descriptor.available && !_graphics.pending()))
                {
                    // Use the same validated queued path as Graphics settings, not live UI writes.
                    edit(delegate, [](SceneState& state)
                    {
                        state.animateLights = false;
                    });
                    _graphics.queueEdit(descriptor.handle,std::move(value));
                }
            }
            ui::Group::endTree();
        }
        ui::Group::endTree();
    }

    bool hasSelection(const std::shared_ptr<SceneDelegate>& delegate) const
    {
        return delegate->selection()->selectedItem() != nullptr;
    }

    void edit(const std::shared_ptr<SceneDelegate>& delegate, std::function<void(SceneState&)> operation)
    {
        const auto weakDelegate = std::weak_ptr<SceneDelegate>(delegate);
        delegate->enqueueRendererUpdate(delegate->rendererGeneration(), [weakDelegate, operation = std::move(operation)](draw::Renderer& renderer)
        {
            if (auto activeDelegate = weakDelegate.lock())
            {
                renderer.invalidateHistories();
                operation(activeDelegate->state());
            }
        });
    }

    std::weak_ptr<SceneDelegate> _delegate;
    ui::UserInterface* _interface = nullptr;
    std::shared_ptr<draw::RenderSettingsPreferences> _preferences;
    ui::GraphicsSettingsWindow _graphics;
    ui::Window _controls;
    ui::DrawMaterialEditor _editor;
};

class Application final : public app::Application {
  public:
    void init(int, char**) override
    {
        auto delegate = std::make_shared<SceneDelegate>();
        delegate->setWeakSelf(delegate);
        auto ui = std::make_shared<UiDelegate>(delegate);
        setRenderDelegate(delegate);
        setInputDelegate(std::make_shared<ManipulationInput>(delegate));
        setUiDelegate(ui);
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
        bg2e::app::MainLoop loop("org.bg2e.examples.draw.manipulation");
        Application application;
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e draw: manipulation", 1380, 800));
        return loop.run(&application, application.config());
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_manipulation: " << error.what() << '\n';
        return 1;
    }
}
