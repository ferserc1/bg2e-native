# Build an interactive scene with selection and manipulation tools

In this tutorial you will build the manipulation example from an empty C++
file. You will first give a deferred scene an orbit camera, then make its
objects selectable, add transform handles, and decide which mouse gestures go
to the tools and which go to the camera. Finally, you will connect a material
editor and a best-viewpoint action without changing GPU resources during UI
drawing.

The scene contains a textured model, independent clones, a floor, a cutout
canopy and a cube with packed material channels. Those objects give us different
surfaces on which to try picking and material edits. The rendering concepts are
the same as in [tutorial 10](10_deferred_renderer.md); the new subject here is
how to make that scene interactive.

You need C++20, a bg2e checkout with its dependencies, and the platform graphics
toolchain. We use the experimental `bg2e::draw` API throughout. The production
`bg2e::render` API has different resource types.

## 1. Prepare one source file and the assets

Work in `examples/draw/11_manipulation/src/main.cpp`. The checkout already
registers the `draw_manipulation` target, so editing this source needs no CMake
change. Start with the repository's GPL license comment, then these includes
and file scope:

```cpp
#include <bg2e.hpp>
#include <bg2e/app/Keyboard.hpp>

#include <cmath>
#include <cstdint>
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
```

Keep the unnamed namespace open until the application class is finished. The
steps below identify where to add each declaration, method or function body;
there is no final replacement file to copy over your work.

Use `base::PlatformTools::assetPath()` to find runtime resources. The required
files are `two_submeshes.obj`, `HDR_multi_nebulae_1.hdr`, and the JPEG maps named
`two_submeshes_<outer-or-inner>_<albedo-or-metallic-or-roughness-or-normal-or-ao>.jpg`.
They are supplied in the repository assets and deployed by the project build.
Our other two textures will be generated in memory.

## 2. Define the camera's projection convention

Picking projects geometry using the camera matrices, so the visible scene and
picker must agree on the projection convention. Define a perspective projection
that explicitly uses right-handed coordinates and zero-to-one depth:

```cpp
class CameraProjection final : public math::PerspectiveProjection {
  public:
    void apply(glm::mat4& matrix) override
    {
        matrix = glm::perspectiveRH_ZO(glm::radians(fov()),
            _viewport.width / _viewport.height, near(), far());
    }
};
```

The inherited projection stores FOV, near/far and viewport. Its `apply()`
override builds the matrix from those values. Camera movement will change the
node transform, not these projection parameters.

## 3. Prepare two small texture experiments

The canopy and channel-test cube should each have one material over the whole
cube. A primitive cube normally has six face submeshes. Add this helper after
the projection class to replace those ranges with one range:

```cpp
std::shared_ptr<draw::Drawable::Mesh> singleMaterialCube(
    float width, float height, float depth)
{
    auto mesh = std::shared_ptr<draw::Drawable::Mesh>(
        geo::createCube(width, height, depth));
    mesh->submeshes = {{0, uint32_t(mesh->indices.size())}};
    return mesh;
}
```

A procedural texture supplies a newly allocated pixel array and a stable image
identifier. Implement `CutoutTexture` as follows. Its alpha alternates between
zero and 255 in four-pixel squares, which will let us distinguish rendered
cutout holes from geometric picking.

```cpp
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
        return "draw-manipulation-alpha-checker-v1";
    }
};
```

Now write a second generator, `PackedTexture`, using that same class structure.
Give its constructor dimensions `(8, 8, 4)`, allocate `8 * 8 * 4` bytes, and
iterate `x` and `y` from zero through seven. At each pixel, assign:

| Byte | Value | Later use |
| --- | --- | --- |
| Red | `x < 4 ? 51 : 204` | Metalness, varying between the two halves |
| Green | `166` | Roughness |
| Blue | `230` | Ambient occlusion |
| Alpha | `255` | Fully present |

Return the buffer and use `draw-manipulation-packed-channels-v1` as its image
identifier. Both generators transfer their buffer to the engine's image
loading path; do not free the returned array in the generator. These are
linear numeric values, so the packed texture can store material data rather
than display color.

## 4. Decide what the scene delegate owns

The renderer needs a scene root, while picking and input also need the active
camera and scene references. Keep those references in a small state structure:

```cpp
struct SceneState {
    std::shared_ptr<scene::Node> cameraNode;
    std::shared_ptr<scene::Node> lights;
    std::shared_ptr<scene::Node> canopy;
    std::shared_ptr<draw::Drawable> clone;
    std::shared_ptr<scene::Scene> sceneContext;
    bool animateLights = true;
    float lightAngle = 0.0f;
};
```

Declare `SceneDelegate` next, deriving from `draw::DefaultRenderLoopDelegate`.
We will implement its methods outside the declaration so each step can add one
responsibility. The default delegate owns the renderer and handles recording;
our class owns the optional tools and the drawables it must clean up.

```cpp
class SceneDelegate final : public draw::DefaultRenderLoopDelegate {
  public:
    SceneDelegate();
    static draw::RendererConfiguration makeRendererConfiguration();

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

    void init(draw::Engine* engine) override;
    void surfaceChanged(gpu::Size2D extent, gpu::PixelFormat format) override;
    void resize(gpu::Size2D extent) override;
    void prepareFrame() override;
    void update(const draw::FrameContext& frame) override;
    void cleanup() override;

    void requestBestView();
    void removeSelected();

  protected:
    std::shared_ptr<scene::Node> createScene() override;

  private:
    void load(const std::shared_ptr<draw::Drawable>& drawable);
    void setLogicalCameraViewport();
    std::shared_ptr<scene::Node> addDrawable(
        const std::shared_ptr<scene::Node>& parent, const std::string& name,
        std::shared_ptr<draw::Drawable> drawable, glm::vec3 position,
        bool selectable = true);

    SceneState _state;
    std::vector<std::shared_ptr<draw::Drawable>> _drawables;
    std::shared_ptr<manipulation::SelectionManager> _selection;
    std::unique_ptr<utils::ViewpointAnalyzer> _analyzer;
    std::weak_ptr<SceneDelegate> _self;
    std::function<void()> _uiCleanup;
};
```

The weak self-reference lets callbacks use the delegate without making an
ownership cycle. The application will assign it after constructing the
`shared_ptr`. GPU work belongs in lifecycle methods after the engine exists,
not in the constructor.

## 5. Add the overlay and explain the fixed render scale

Implement the constructor and configuration factory after the declaration:

```cpp
SceneDelegate::SceneDelegate()
    : DefaultRenderLoopDelegate(std::make_unique<draw::RendererDeferred>(),
        makeRendererConfiguration())
{
    setContinuousRendering(true);
}

// init() configures the picking image with the same fixed scale.
draw::RendererConfiguration SceneDelegate::makeRendererConfiguration()
{
    draw::RendererConfiguration configuration;
    configuration.resolution = std::make_shared<draw::ResolutionSettings>();
    configuration.resolution->setEnabled(true);
    configuration.resolution->setScale(RenderScale);
    configuration.overlays.push_back(
        std::make_shared<manipulation::GizmoAndSelectionRenderer>());
    return configuration;
}
```

`GizmoAndSelectionRenderer` is an optional overlay pass. The deferred renderer
still draws the scene; this pass adds selection highlights and tool geometry.
Giving the configuration a shared pass keeps it alive with the renderer.

The scale is deliberately fixed at 70% to demonstrate picking at a different
pixel density. `ResolutionSettings` controls scene rendering. The picker has
its own setting, which we must configure explicitly in the next step.

## 6. Initialize selection and viewpoint tools

Implement `init()` next. Pass the draw engine to the draw overload of
`SelectionManager`; passing a production engine would create a different
execution mode. Initialize the base first so its engine state is established.

```cpp
void SceneDelegate::init(draw::Engine* engine)
{
    DefaultRenderLoopDelegate::init(engine);
    _selection = std::make_shared<manipulation::SelectionManager>(engine);
    _selection->init();
    _selection->setMultiSelectionEnabled(true);
    _selection->setClearSelectionOnEmptyPick(true);

    // Match makeRendererConfiguration(). Input and viewport are logical units.
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
```

`setPickPixelScale(x, y)` specifies picking pixels per input unit. Here a
100-unit logical viewport dimension produces a 70-pixel pick dimension. Do not
calculate the scale from the renderer extent during initialization: its first
output surface may not have arrived yet, so that extent can be zero.

On Retina, scene rendering uses 70% of the physical output, while our picker
uses 70% of the logical window dimensions. Their pixel counts can differ; the
important agreement is between pick coordinates, pick viewport and pick image
scale. No runtime scale refresh is needed because this factor is constant.

`onSelect()` requests a scene frame so a new highlight becomes visible.
Selection entries are weak references; the callback also uses a weak delegate
reference so neither object keeps the application alive accidentally.

## 7. Make loading and node attachment explicit

Implement `load()` to retain a drawable before uploading it. If an upload
fails, cleanup can still find that partially initialized drawable.

```cpp
void SceneDelegate::load(const std::shared_ptr<draw::Drawable>& drawable)
{
    _drawables.push_back(drawable);
    drawable->load(_engine);
}
```

Implement `addDrawable()` to distinguish visible geometry from selectable
geometry. A `TransformComponent` places the node; `DrawableComponent` connects
its draw resource to scene traversal. The last two components opt the node
into selection identity and viewport handles:

```cpp
std::shared_ptr<scene::Node> SceneDelegate::addDrawable(
    const std::shared_ptr<scene::Node>& parent, const std::string& name,
    std::shared_ptr<draw::Drawable> drawable, glm::vec3 position, bool selectable)
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
```

`SelectableComponent` gives the scene object selection identity. The picker
renders temporary IDs for one submission and maps them back to node, component,
resource and submesh. Do not save the pixel ID as an object identifier.
`GizmoComponent` supplies runtime tool geometry; it is not an ordinary mesh
that you must add to the scene renderer separately.

## 8. Build the original model and name its groups

Begin the `createScene()` definition. Keep the function open through the next
four steps. Create an HDR environment, load the two-submesh OBJ, and give each
submesh its own texture maps:

```cpp
std::shared_ptr<scene::Node> SceneDelegate::createScene()
{
    auto root = std::make_shared<scene::Node>("Manipulation scene");
    root->addComponent(new scene::EnvironmentComponent(
        base::PlatformTools::assetPath(), "HDR_multi_nebulae_1.hdr"));

    auto model = std::make_shared<draw::Drawable>();
    model->setMesh(db::loadMeshObj<draw::Drawable::Mesh>(
        base::PlatformTools::assetPath() / "two_submeshes.obj"));
    const char* names[] = {"outer", "inner"};
    for (uint32_t i = 0; i < 2; ++i)
    {
        auto texture = [&](const std::string& map)
        {
            return std::make_shared<base::Texture>(base::PlatformTools::assetPath(),
                "two_submeshes_" + std::string(names[i]) + "_" + map + ".jpg");
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
```

The names `outer` and `inner` identify individual submeshes. The group names
`body/front` and `body/back` are deliberately different: later we will compare
an exact group patch with a regex patch that selects both groups. Material
metalness and roughness scalars are one so their maps retain their full range.

## 9. Create independent objects for comparison

Still inside `createScene()`, clone the model for an independent cyan object:

```cpp
    _state.clone = model->clone();
    _state.clone->materialDefinition(0).setAlbedo(base::Color(0.3f, 0.8f, 1));
    _state.clone->material(0).setAlbedo(base::Color(0.3f, 0.8f, 1));
    load(_state.clone);
    addDrawable(nested, "Independent clone", _state.clone, {0, 0, -1});
```

Using the same drawable pointer for two nodes would share their material
state. `clone()` supplies independent state. A clone of a loaded drawable already
has live materials, so set both its stored definition and its live albedo here.
Calling `load()` alone does not copy arbitrary definition changes into existing
live materials. Our helper also retains the clone for cleanup.

Add two more clones, using `iterateMaterials()` to set every submesh. Here is
the first; retain it through `load()` just like the original:

```cpp
    auto glass = model->clone();
    glass->iterateMaterials([](auto& material)
    {
        material.setAlbedo(base::Color(1, 1, 1, 0.45f));
        material.setIsTransparent(true);
        material.setRefractionFactor(0.1f);
    });
    load(glass);
    addDrawable(root, "Transparent model", glass, {3, 0, 0});
```

Construct `solidGlass` using the same sequence: clone `model`, iterate its
materials, load it, and attach it to `root`. For each material use albedo
`base::Color(0.7f, 1, 0.7f, 0.5f)`, set transparent to true, and call
`setIsSolid(true)`. Place this node at `{1.5f, 0, 2.5f}` with name
`"Solid transparent model"`. Its purpose is to provide another material and
placement to select; `isSolid` is already true by default, so omitting that
setter on the first clone does not establish a separate solidness comparison.

## 10. Add the floor, cutout canopy and packed-channel cube

The floor should be visible but not selectable. Construct a `draw::Drawable`
named `floor`, set its mesh to `geo::createPlane(30, 30)`, and configure material
zero with gray albedo `(0.55f, 0.55f, 0.55f)`, metalness zero and roughness
`0.75f`. After `load(floor)`, attach it at `{0, -1.05f, 0}` using
`addDrawable(root, "Floor", floor, position, false)`. The final `false` omits
selection and gizmo components.

The canopy is a single-material thin cube whose albedo comes from the cutout
generator. Add this next:

```cpp
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
```

Alpha cutoff discards texels with alpha below 0.5 when rendering. Draw picking
is geometric, so those texture holes do not remove the triangle from picking.
Keep this distinction in mind when trying the canopy.

For the channel cube, create a mesh with `singleMaterialCube(1.4f, 1.4f, 1.4f)`.
Before assigning it to a drawable named `channels`, give every vertex a second
UV set with `vertex.texCoord1 = vertex.texCoord0 * 2.0f`. Create a
`base::Texture` from `PackedTexture`, turn mipmaps off and use nearest filters
as for `checker`. Name this texture `packed`. Configure `channels->material()`
using the following recipe:

| Property | Setter/value |
| --- | --- |
| Albedo | `setAlbedo(base::Color(0.8f, 0.3f, 0.15f))` |
| Metalness map and multiplier | `setMetalnessTexture(packed)`, `setMetalness(1)` |
| Metalness channel, UV set and scale | `setMetalnessChannel(0)`, `setMetalnessUVSet(1)`, `setMetalnessScale(glm::vec2(2, 1))` |
| Roughness map, multiplier and channel | `setRoughnessTexture(packed)`, `setRoughness(1)`, `setRoughnessChannel(1)` |
| AO map and channel | `setAoTexture(packed)`, `setAoChannel(2)` |

Load the drawable and attach it to `root` at `{-1.5f, -0.3f, 2.5f}` with name
`"Packed texture channels"`. This cube makes UV and channel changes easier to
recognize than they would be on a uniform material.

## 11. Put the camera's three components on one node

Continue `createScene()` with the camera. The transform, orbit controller and
camera component must belong to the same node: the controller changes this
transform, and `CameraComponent` reads this transform as its pose.

```cpp
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
    orbit->setRotateButtons(true, false, false);
    orbit->setPanButtons(false, false, true);
    orbit->setZoomButtons(false, true, false);
    root->addChild(_state.cameraNode);
```

The button arguments are left, middle and right. We allow left-button rotation
because the input delegate will distinguish a click from a drag. A child camera
with an additional transform would introduce an offset between the orbit pose
and the rendered view, so this example has just one camera node.

## 12. Add lights and publish the scene references

Still in `createScene()`, create `_state.lights` as a node named `"Lights"`,
add a `TransformComponent`, and attach it to `root`. Populate it with four
light nodes in a loop over `i = 0..3`. For each node:

1. Compute `angle = float(i) * 1.57079633f`.
2. Add a translated transform at `(sin(angle) * 5, 5, cos(angle) * 5)` and a
   `scene::LightComponent`.
3. Get the light with `lightNode->light()->light()`. Set its type to
   `base::Light::TypeOmni`, intensity to `35`, source size to `0.25f`, and
   shadow samples to `8`.
4. Set its color to `base::Color(i == 0 ? 1 : 0.65f, i == 1 ? 1 : 0.7f,
   i == 2 ? 1 : 0.75f)` and add the node to `_state.lights`.

The parent transform will rotate all four lights together. Finish the function
by creating a scene context over the same root used by the renderer:

```cpp
    _state.sceneContext = std::make_shared<scene::Scene>();
    _state.sceneContext->setSceneRoot(root);
    _state.sceneContext->updateAll();
    setLogicalCameraViewport();
    return root;
}
```

This context gives the picker and input visitor the root and active camera.
It is not another scene with different geometry.

## 13. Keep input coordinates and the camera viewport consistent

Implement `setLogicalCameraViewport()` outside `createScene()`:

```cpp
void SceneDelegate::setLogicalCameraViewport()
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
```

The window wrapper returns logical dimensions without requiring an SDL include.
Guarding zero dimensions avoids an invalid aspect ratio while the window has
no usable viewport.

Implement `surfaceChanged(extent, format)`, `resize(extent)` and `prepareFrame()`
by calling the matching `DefaultRenderLoopDelegate` method first, then
`setLogicalCameraViewport()`. For example:

```cpp
void SceneDelegate::prepareFrame()
{
    DefaultRenderLoopDelegate::prepareFrame();
    setLogicalCameraViewport();
}
```

Use those same two statements in the other two overrides, passing their
arguments to the base. This restores the logical camera viewport after any
base renderer reconfiguration. Do not update the fixed pick scale on resize;
the viewport size changes, but the factor remains 0.7.

Implement `update()` next:

```cpp
void SceneDelegate::update(const draw::FrameContext& frame)
{
    _selection->refreshSelection();
    if (_state.animateLights)
    {
        _state.lightAngle += frame.sceneDeltaSeconds * 0.2f;
        _state.lights->transform()->setRotation(_state.lightAngle, 0, 1, 0);
    }
    DefaultRenderLoopDelegate::update(frame);
}
```

Refreshing selection prunes stale weak entries after changes. Animate before
calling the base update so extraction sees the current transform.

## 14. Give input its own small state machine

Declare `ManipulationInput` after the scene delegate implementations. It holds
a weak delegate reference and the information needed to distinguish clicks
from drags:

```cpp
class ManipulationInput final : public app::InputDelegate {
  public:
    explicit ManipulationInput(std::weak_ptr<SceneDelegate> delegate)
        : _delegate(std::move(delegate))
    {
    }

    void mouseButtonDown(int button, int x, int y) override;
    void mouseMove(int x, int y) override;
    void mouseButtonUp(int button, int x, int y) override;
    void mouseWheel(int deltaX, int deltaY) override;
    void keyDown(const app::KeyEvent& event) override;
    void keyUp(const app::KeyEvent& event) override;

  private:
    std::weak_ptr<SceneDelegate> _delegate;
    bool _controlDown = false;
    bool _leftButtonDown = false;
    int _mouseDownX = 0;
    int _mouseDownY = 0;
};
```

There are two different consumers of a mouse gesture. The selection manager
owns tool interaction. Scene components own camera movement. A `true` return
from a selection-manager input method means the event may continue to the
scene. A captured gizmo gesture must not also rotate the camera.

Implement `mouseButtonDown()` using this ordering:

```cpp
void ManipulationInput::mouseButtonDown(int button, int x, int y)
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
    if (context && selection &&
        selection->mouseButtonDown(context.get(), button, x, y))
    {
        scene::InputVisitor visitor;
        visitor.mouseButtonDown(context->rootNode(), button, x, y);
    }
    else if (context)
    {
        app::MainLoop::current()->requestSceneFrame();
    }
}
```

Forward the uncaptured press immediately. Waiting until release to decide
whether to send it would leave the orbit controller without the start of its
drag. The later release determines whether to perform an additional pick.

## 15. Send motion to exactly the active consumers

Implement `mouseMove()` by locking the delegate and obtaining `context` and
`selection` exactly as in button-down. Return if either is absent. If
`Keyboard::controlPressed()` is true, latch `_controlDown = true`. Then use
this dispatch block:

```cpp
const auto propagate = selection->mouseMove(context.get(), x, y);
if (propagate)
{
    scene::InputVisitor visitor;
    visitor.mouseMove(context->rootNode(), x, y);
}
app::MainLoop::current()->requestSceneFrame();
```

`mouseMove()` on the selection manager can change a gizmo-controlled transform.
`InputVisitor::mouseMove()` lets `OrbitCameraController` change orbit parameters
when the scene owns the gesture. The normal scene update later turns those
parameters into the camera node's transform. Capturing input without forwarding
it to the scene would leave the camera stationary even though the application
received the event.

## 16. Pick only on a click and always balance scene presses

The most important input branch is release. Implement it in full:

```cpp
void ManipulationInput::mouseButtonUp(int button, int x, int y)
{
    auto delegate = _delegate.lock();
    auto context = delegate ? delegate->state().sceneContext : nullptr;
    auto selection = delegate ? delegate->selection() : nullptr;
    _controlDown = app::Keyboard::controlPressed() || _controlDown;
    if (context && selection)
    {
        if (selection->sceneInputBlocked())
        {
            selection->mouseButtonUp(context.get(), button, x, y, _controlDown);
        }
        else
        {
            const bool click = button == 0 && _leftButtonDown &&
                std::abs(int64_t(x) - _mouseDownX) <= 2 &&
                std::abs(int64_t(y) - _mouseDownY) <= 2;
            if (click)
            {
                selection->pick(context.get(), x, y, _controlDown);
            }
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
```

The tolerance applies independently to X and Y, including exactly two logical
pixels. A larger change is a camera gesture. The fourth `pick()` argument
makes a Control-click additive, allowing the current selection to be toggled
rather than replaced.

When the scene owns a gesture, forward release even after a pick. Otherwise
the controller would retain its pressed-button state. Conversely, a gizmo
release goes only to the manager because its press never reached the scene.
Do not also call the manager's noncaptured `mouseButtonUp()`: that method has
its own click policy, and the example has already performed its tolerant pick.

Finish the three remaining input overrides using these rules:

| Method | Implementation |
| --- | --- |
| `mouseWheel(deltaX, deltaY)` | Lock the delegate, obtain `sceneContext`, and if present call `InputVisitor::mouseWheel(rootNode, deltaX, deltaY)`, then request a scene frame. |
| `keyDown(event)` | For `KeyLeftControl` or `KeyRightControl`, set `_controlDown = true`. For `KeyDelete`, lock the delegate, call `removeSelected()` and return. Otherwise forward `InputVisitor::keyDown(rootNode, event)` when the context exists. |
| `keyUp(event)` | For either Control key, refresh `_controlDown` from `Keyboard::controlPressed()`. Forward `InputVisitor::keyUp(rootNode, event)` when the context exists. |

All key constants above are in `app::KeyEvent`. These overrides need no SDL
header, native window or platform button constant.

## 17. Queue best-view analysis, then change only the camera pose

Return to `SceneDelegate` and implement `requestBestView()`:

```cpp
void SceneDelegate::requestBestView()
{
    const auto weakDelegate = _self;
    enqueueRendererUpdate(rendererGeneration(), [weakDelegate](draw::Renderer& renderer)
    {
        auto delegate = weakDelegate.lock();
        if (!delegate || !delegate->_analyzer || !delegate->_state.sceneContext)
        {
            return;
        }
        const auto samples = delegate->_analyzer->analyze(
            delegate->_state.sceneContext.get());
        const auto* best = utils::ViewpointAnalyzer::bestSample(samples);
        if (!best)
        {
            return;
        }
        auto node = delegate->_state.cameraNode;
        auto* orbit = node->getComponent<scene::OrbitCameraComponent>();
        utils::applyViewpointToOrbitCamera(best->camera, orbit, node->camera());
        delegate->_state.sceneContext->updateCamera();
        if (auto* deferred = dynamic_cast<draw::RendererDeferred*>(&renderer))
        {
            deferred->resetUpscaler();
        }
    });
}
```

Analysis renders and reads candidate views synchronously. Enqueueing this
operation gives it a stopped scene/GPU boundary before frame acquisition;
calling it from UI drawing or command recording would be the wrong lifetime.
The generation argument prevents a queued operation from running against a
renderer that has since been replaced.

The returned samples own their candidate data. Keep `best` local to that
samples array, check for an empty result, and apply it while the array lives.
`applyViewpointToOrbitCamera()` updates orbit center, distance and rotation.
Its candidate near/far and FOV describe analysis rendering; the scene camera's
projection stays unchanged. Resetting the upscaler tells temporal rendering
that this was a camera jump.

## 18. Define the UI owner and connect its material editor

Declare a `UiDelegate` deriving from both `ui::UserInterfaceDelegate` and
`std::enable_shared_from_this<UiDelegate>`. Give it these members:

| Member | Type |
| --- | --- |
| `_delegate` | `std::weak_ptr<SceneDelegate>` |
| `_interface` | `ui::UserInterface*`, initially `nullptr` |
| `_controls` | `ui::Window` |
| `_editor` | `ui::DrawMaterialEditor` |

Declare a constructor taking `std::weak_ptr<SceneDelegate>`, the overrides
`init(draw::Engine*, ui::UserInterface*)` and `drawUI()`, and a public
`cleanup()`. Declare these private helpers as well:

```cpp
bool hasSelection(const std::shared_ptr<SceneDelegate>& delegate) const;
void edit(const std::shared_ptr<SceneDelegate>& delegate,
    std::function<void(SceneState&)> operation);
```

The constructor moves its argument into `_delegate`, sets `_controls` title to
`"Manipulation"`, position to `(10, 10)` and size to `(430, 700)`.
`hasSelection()` returns whether `delegate->selection()->selectedItem()` is
non-null. `cleanup()` calls `_editor.cleanup()` and sets `_interface` to
`nullptr`.

Implement `init()` as follows. Unlike the production `MaterialEditor`, this
editor accepts a draw engine and queues mutations for the draw delegate.

```cpp
void UiDelegate::init(draw::Engine* engine, ui::UserInterface* interface)
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
            if (auto* deferred = dynamic_cast<draw::RendererDeferred*>(&renderer))
            {
                deferred->resetUpscaler();
            }
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
```

The UI must have been created with `make_shared` before `init()` calls
`weak_from_this()`. The cleanup callback is weak, so the scene and UI delegates
do not own one another. It also ensures material previews are released while
the engine and UI session are still available.

## 19. Give ordinary UI edits the same safe update boundary

Implement the `edit()` helper before adding UI controls:

```cpp
void UiDelegate::edit(const std::shared_ptr<SceneDelegate>& delegate,
    std::function<void(SceneState&)> operation)
{
    const auto weakDelegate = std::weak_ptr<SceneDelegate>(delegate);
    delegate->enqueueRendererUpdate(delegate->rendererGeneration(),
        [weakDelegate, operation = std::move(operation)](draw::Renderer& renderer)
    {
        if (auto active = weakDelegate.lock())
        {
            operation(active->state());
            if (auto* deferred = dynamic_cast<draw::RendererDeferred*>(&renderer))
            {
                deferred->resetUpscaler();
            }
        }
    });
}
```

UI drawing may happen while renderer resources are in use. The helper records
intent now and applies it at the delegate's synchronized update boundary.
Capturing values such as a slider's new float is safe; retaining a borrowed
renderer pointer for later is not. The callback receives the valid renderer
when it actually executes.

## 20. Build the panel a control at a time

Start `drawUI()` by locking the delegate. Return when the delegate, its scene,
or `_interface` is missing. Obtain `auto& state = delegate->state()`, then open
`_controls.draw([&] { ... });`. Add the following controls inside that window
callback in order. Close the callback and method after step 22.

First display instructions with `ui::Text::text()`: click to select,
Control-click to toggle selection, left drag to orbit, right drag to pan,
middle drag to zoom, and drag a gizmo handle to transform.

Add a `ui::Text::separator("Scene")` and these actions:

| Control | What to do when it changes or is pressed |
| --- | --- |
| `ui::Button::checkBox("Animate lights", &state.animateLights)` | Request a scene frame. The boolean is application animation policy, read by `update()`. |
| `ui::Button::button("Best viewpoint", true)` | Call `delegate->requestBestView()`. |
| `ui::Button::button("Deselect", true, !hasSelection(delegate))` | Call `delegate->selection()->deselect()` and request a scene frame. |
| `ui::Button::button("Delete selected", false, !delegate->selection()->selectedNode())` | Call `delegate->removeSelected()`, implemented in step 23. |

The final boolean passed to `button()` disables it. The second controls
same-line placement. Disabling commands when there is no target avoids a UI
that invites a meaningless operation.

Add a material section and a scalar-patch button:

```cpp
ui::Text::separator("MaterialModifier presets");
if (ui::Button::button("Scalar: roughness 0.22 / metalness 0.65",
    false, !hasSelection(delegate)))
{
    _editor.queuePatch(utils::MaterialModifier(
        std::string{R"({"roughness":0.22,"metalness":0.65})"}));
}
```

`MaterialModifier` is a partial patch, so this changes only those two
properties. The draw editor handles selected typed targets and defers the edit;
it does not rewrite the entire material while drawing the button.

Add a second button, `"Texture: packed channel, UV1 and cutoff"`, with the
same enabled condition and `queuePatch()` pattern. Construct its modifier with
the following JSON and `base::PlatformTools::assetPath()` as the second argument:

```json
{"metalnessTexture":"two_submeshes_outer_metallic.jpg","metalnessUV":1,"metalnessScale":[2,2],"alphaCutoff":0.2}
```

The base path resolves the relative texture file. The other fields choose the
second UV set, texture scale and alpha cutoff. Their absence from the scalar
patch is exactly why applying the scalar patch preserves them.

## 21. Apply a patch to a group rather than only the selected submesh

A selection identifies a node, component and submesh. To patch an entire group
of that drawable, first resolve the selected component and get its draw
resource. Add a `"Group: body/front"` button and use this as its action:

```cpp
auto item = delegate->selection()->selectedItem();
auto component = item ? item->drawable.lock() : nullptr;
auto drawable = component ? component->drawDrawable() : nullptr;
if (drawable)
{
    const utils::MaterialModifier patch(std::string{R"({"roughness":0.6})"});
    delegate->enqueueRendererUpdate(delegate->rendererGeneration(),
        [drawable, patch](draw::Renderer& renderer)
    {
        patch.applyAll(*drawable, std::string("body/front"));
        if (auto* deferred = dynamic_cast<draw::RendererDeferred*>(&renderer))
        {
            deferred->resetUpscaler();
        }
    });
}
```

This exact-string overload changes the `body/front` group only, even when a
different submesh of the same drawable supplied the selection. The operation
is queued, so texture or GPU material work does not run while constructing UI.

Add `"Regex: all body groups"` with the same target lookup and queued-callback
structure. Use patch `{"metalness":0.1}` and capture a
`std::regex groupPattern("body/.*")`; replace the `applyAll()` call with
`patch.applyAll(*drawable, groupPattern)`. The regex overload uses
`regex_search`, so this pattern matches both group names created in step 8.

Drawable patches update stored CPU definitions and live material attributes
independently, preserving unrelated edits. Direct `draw::MaterialBase` edits
affect only the live object. Multi-target and texture-load edits are not
transactional: a later failure can occur after earlier changes have applied.

After the preset buttons call `_editor.draw(*_interface)`. If `_editor.error()`
is not empty, display it with `ui::Text::text()`. This exposes queued edit
failures in the panel instead of hiding them.

## 22. Finish the clone, canopy and selection controls

Still inside the window callback, add a clone section and this slider:

```cpp
ui::Text::separator("Clone check");
float cloneRoughness = state.clone->material(0).roughness();
if (ui::Numeric::sliderFloat("Clone roughness", &cloneRoughness, 0.02f, 1.0f))
{
    edit(delegate, [cloneRoughness](SceneState& scene)
    {
        scene.clone->materialDefinition(0).setRoughness(cloneRoughness);
        scene.clone->drawMaterial(0)->materialAttributes().setRoughness(cloneRoughness);
    });
}
```

The slider reads current live state. Its callback patches both the stored
material definition and live attributes. This makes the distinction visible
without replacing a whole material or accidentally discarding other changes.
The original model should retain its own roughness.

Add a canopy checkbox by reading `bool showCanopy = state.canopy->enabled()`.
When `ui::Button::checkBox("Cutout canopy", &showCanopy)` changes, call
`edit(delegate, [showCanopy](SceneState& scene) { scene.canopy->setEnabled(showCanopy); })`.
The lambda captures the requested value, not a reference to the temporary
checkbox variable.

Finally add a `"Selected components"` separator. Obtain `selectedItem()`; if
it exists and its `node.lock()` succeeds, display the node name and
`std::to_string(item->submesh)`. Otherwise display `"No drawable submesh selected"`.
Close the `_controls.draw()` callback and `drawUI()` method. You now have all
the visible controls from the example, built from individual behaviors.

## 23. Remove selected nodes without leaving tool resources behind

Node removal must happen at the same stopped boundary as material edits.
Before writing `removeSelected()`, add a free helper `releaseGizmos()` near the
scene-delegate definitions. Both deletion and final cleanup will use it:

```cpp
void releaseGizmos(scene::Node* root)
{
    struct CleanupVisitor : scene::NodeVisitor {
        void visit(scene::Node* node) override
        {
            if (auto* gizmo = node->getComponent<manipulation::GizmoComponent>())
            {
                auto drawable = gizmo->drawDrawable();
                auto transform = gizmo->transformDrawDrawable();
                if (drawable)
                {
                    drawable->cleanup();
                }
                if (transform && transform != drawable)
                {
                    transform->cleanup();
                }
                gizmo->cleanupDrawResources();
            }
        }
    } visitor;
    root->accept(&visitor);
}
```

A gizmo is runtime-only and owns draw resources that must be released while
the engine is alive. Removing the node reference alone does not express that
GPU cleanup boundary. The helper traverses children as well as the selected
node, and avoids cleaning the same drawable twice.

Implement `SceneDelegate::removeSelected()` with this sequence:

1. Obtain `_selection->selectedNode()`. Return if there is no node or it has
   no parent; the root is not a deletion target.
2. Retain the selected node and parent using their `shared_from_this()` values.
   Capture those two pointers, `_selection`, and `_self` in an
   `enqueueRendererUpdate(rendererGeneration(), callback)` operation.
3. Inside the callback, lock the weak delegate and return if it expired.
4. Call `selection->deselect()`, then `releaseGizmos(selected.get())`, then
   `parent->removeChild(selected)`.
5. Call `selection->refreshSelection()` and
   `delegate->state().sceneContext->updateAll()` to refresh references after
   removal. Reset the callback renderer's upscaler using the same dynamic-cast
   block as the material and viewpoint callbacks.

Deselecting before detaching prevents a visible handle from continuing to
refer to the removed transform. Weak selection references also protect against
resource replacement, but they do not replace explicit GPU cleanup.

## 24. Close the resource lifecycle

Implement the remaining scene-delegate method, `cleanup()`. Place its
definition after `releaseGizmos()` so the helper is declared before use:

```cpp
void SceneDelegate::cleanup()
{
    if (_uiCleanup)
    {
        _uiCleanup();
        _uiCleanup = nullptr;
    }
    DefaultRenderLoopDelegate::cleanup();
    if (_state.sceneContext && _state.sceneContext->rootNode())
    {
        releaseGizmos(_state.sceneContext->rootNode());
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
```

The order matters. UI preview/editor bindings are released first, then renderer
references, then gizmo and analyzer resources and scene drawables. MainLoop
still owns a live engine/device at this point. Waiting for smart-pointer
destruction after the device has disappeared would lose that required order.

## 25. Register the three delegates and start the application

Add the application class after the input and UI classes and all their method
definitions:

```cpp
class Application final : public app::Application {
  public:
    void init(int, char**) override
    {
        auto delegate = std::make_shared<SceneDelegate>();
        delegate->setWeakSelf(delegate);
        setRenderDelegate(delegate);
        setInputDelegate(std::make_shared<ManipulationInput>(delegate));
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

The namespace is now closed. `Application` holds the delegates strongly; input,
UI and queued callbacks use weak links back to the scene owner. Constructing
`UiDelegate` with `make_shared` also makes its cleanup callback's
`weak_from_this()` valid. No command-line configuration is parsed.

Define the entry point outside the namespace:

```cpp
int main(int argc, char** argv)
{
    try
    {
        bg2e::app::MainLoop loop("org.bg2e.examples.draw.manipulation");
        Application application;
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize(
            "bg2e draw: manipulation", 1380, 800));
        return loop.run(&application, application.config());
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_manipulation: " << error.what() << '\n';
        return 1;
    }
}
```

Passing `draw::EngineConfig` selects the draw main-loop overload. Its default
backend is Metal on macOS and Vulkan on other supported platforms. Initialization
creates the engine before scene uploads; teardown invokes delegate cleanup
before destroying it.

## 26. Build and explore what you implemented

Use an existing configured build directory when available. For a new directory,
replace the SDK placeholder with your installation and use the platform's
supported generator.

Linux:

```sh
cmake -S . -B build-tutorial -G Ninja -DVULKAN_SDK=/path/to/vulkan/sdk
cmake --build build-tutorial --target draw_manipulation
```

macOS, with Xcode for native file-dialog support:

```sh
cmake -S . -B build-tutorial -G Xcode -DVULKAN_SDK=/path/to/vulkan/sdk
cmake --build build-tutorial --config Debug --target draw_manipulation
```

Windows, from a developer PowerShell:

```powershell
cmake -S . -B build-tutorial -G "Visual Studio 17 2022" -DVULKAN_SDK=C:/path/to/vulkan/sdk
cmake --build build-tutorial --config Debug --target draw_manipulation
```

Runtime output and resources are under `bin/<platform>`. On macOS the executable
is in `bin/macos/draw_manipulation.app/Contents/MacOS/`. Keep assets, shaders and
runtime libraries with the executable or inside its generated bundle.

Try the interaction in the order you built it:

1. Orbit with the left button, pan with the right, and zoom with the middle.
   The camera should keep looking at the orbit center because its controller
   and camera share one node.
2. Click an original-model submesh without dragging. Its highlight and handle
   should appear. Control-click another submesh to change the selection.
3. Drag a transform handle. The object should move while the camera stays
   stationary. Release, then drag away from a handle to resume camera control.
4. Apply the scalar patch, then the exact-group and regex patches. Compare
   `body/front` with `body/back`, and the original with the independent clone.
5. Change clone roughness and canopy visibility. The corresponding queued
   scene/material edit should appear on the next scene frame.
6. Press **Best viewpoint** and resume mouse movement. The camera pose changes,
   while its projection remains the one configured by `CameraProjection`.
7. Delete a selected object, then continue clicking and dragging. Selection
   must not retain a handle to the removed node.

For an offset pick, check logical viewport dimensions and the fixed pixel
scale before changing matrix math. For an unresponsive camera, check scene
input forwarding and component placement. For a missing object, check mesh
loading and attachment before investigating the picker. These observations
connect each behavior to the responsibility introduced in the corresponding
step.

The [repository example](../../../examples/draw/11_manipulation/src/main.cpp)
is available for comparing an implementation after working through the steps.
For API details see [selection and gizmos](../../api/manipulation/index.md),
[MaterialModifier](../../api/utils/MaterialModifier.md),
[DrawMaterialEditor](../../api/ui/Material_Editors.md#drawmaterialeditor) and
[ViewpointAnalyzer](../../api/utils/ViewpointAnalyzer.md).
