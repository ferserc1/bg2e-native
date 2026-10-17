# Build a deferred scene from scratch

The steps below develop the scene/manipulation foundation. Their renderer factory
now uses the current shipped defaults. Complete the current effect-settings wiring
in [Advanced lighting and visible controls](#advanced-lighting-and-visible-controls)
to reproduce the shipped example's graphics window; it is not added by MainLoop.
The linked source is authoritative for full class declarations and cleanup.


This tutorial builds a complete C++20 application with bg2e. By the end, you
will have a window displaying a textured scene with an HDR environment, opaque
and transparent models, procedural textures, an orbiting camera, four rotating
lights, and a small control panel. You will be able to change the lights and a
cloned model's material without changing the original model.

You need working C++ knowledge and the basic idea of vertices, triangles,
textures, and lighting. The engine-specific concepts are introduced below.
You do not need to write Vulkan or Metal commands, shaders, or a rendering
pipeline for this example.

The application uses `bg2e::draw`, the experimental scene rendering path built
on `bg2e::gpu`. The existing production API is `bg2e::render`. These are distinct
paths; this tutorial consistently uses `draw` types and the corresponding
`MainLoop::run` overload.

## 1. Prepare the project and assets

Work inside a bg2e checkout with its dependencies and platform SDKs available.
Create or replace `examples/draw/10_deferred_renderer/src/main.cpp` with the
code developed below. The repository already registers this example as the
`draw_deferred_renderer` target and discovers its C++ sources. No build-system
changes are needed when editing this file.

The C++ blocks under the numbered steps are consecutive pieces of that one
file. Copy them in order, without adding namespace or class wrappers. Some
blocks leave a class or function open so the next step can continue it.
Together, the source blocks form the complete application, including its
license header, includes, and `main()`. The two projection blocks marked
**Alternative** illustrate valid API choices and are not part of the assembled
file. Shell commands are separate build instructions.

Use the assets shipped with the repository:

| Asset | Purpose |
| --- | --- |
| `two_submeshes.obj` | Model with two independently textured submeshes |
| `two_submeshes_outer_albedo.jpg`, `two_submeshes_inner_albedo.jpg` | Base color |
| `two_submeshes_outer_metallic.jpg`, `two_submeshes_inner_metallic.jpg` | Metalness |
| `two_submeshes_outer_roughness.jpg`, `two_submeshes_inner_roughness.jpg` | Surface roughness |
| `two_submeshes_outer_normal.jpg`, `two_submeshes_inner_normal.jpg` | Surface normal detail |
| `two_submeshes_outer_ao.jpg`, `two_submeshes_inner_ao.jpg` | Ambient occlusion |
| `HDR_multi_nebulae_1.hdr` | Environment background and image-based lighting |

These files live under `assets/` in the checkout. The normal project build
copies runtime assets and compiled shaders to the platform output or application
bundle. `base::PlatformTools::assetPath()` resolves that runtime asset location;
we will use it instead of depending on the process's working directory.

## 2. Include the engine and establish the file scope

`<bg2e.hpp>` is the engine's umbrella header. The standard headers provide
math functions, byte types, callbacks, logging, smart pointers, strings, moves,
and vectors. The unnamed namespace keeps the example's helper classes private
to this translation unit. `using namespace bg2e` applies only inside that scope.

Start the file with:

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
```

## 3. Choose a camera projection

A camera needs two different matrices. Its **view matrix** describes where the
camera is and which way it looks. Its **projection matrix** maps view-space
positions into clip space, making distant objects appear smaller.

You usually do not need to write a projection class. The engine provides two
ready-to-use choices in `math/projections.hpp`:

- `math::PerspectiveProjection` is the direct choice when you want to specify
a vertical field of view in degrees. Its default is 60 degrees. Set the near
and far clipping distances with the inherited `setNear()` and `setFar()`.
- `math::OpticalProjection` expresses the same perspective in camera-lens
terms. Set `focalLength` and `frameSize` using the same units (commonly
millimeters); its vertical field of view is calculated as
`2 * atan(frameSize / (2 * focalLength))`. A shorter focal length widens the
view, and a longer focal length narrows it. It also inherits `setNear()` and
`setFar()`.

Both built-in classes derive the aspect ratio from the viewport and call
`glm::perspective()`. The camera component takes a shared projection object:

**Alternative: use a field of view directly**

```cpp
auto cameraNode = std::make_shared<scene::Node>("Camera");
cameraNode->addComponent(std::make_shared<scene::CameraComponent>());
auto projection = std::make_shared<math::PerspectiveProjection>();
projection->setFov(60.0f); // Vertical field of view, in degrees.
projection->setNear(0.1f);
projection->setFar(100.0f);
cameraNode->camera()->setProjection(projection);
root->addChild(cameraNode);
```

**Alternative: use lens measurements**

```cpp
auto cameraNode = std::make_shared<scene::Node>("Camera");
cameraNode->addComponent(std::make_shared<scene::CameraComponent>());
auto projection = std::make_shared<math::OpticalProjection>();
projection->setFocalLength(35.0f);
projection->setFrameSize(35.0f);
projection->setNear(0.1f);
projection->setFar(100.0f);
cameraNode->camera()->setProjection(projection);
root->addChild(cameraNode);
```

A custom class is useful when the matrix must use a specific convention or a
projection formula the built-ins do not provide. This example's deferred
renderer expects a right-handed matrix with depth from zero to one. Its custom
`CameraProjection` explicitly calls `glm::perspectiveRH_ZO()` so that choice does
not depend on the translation unit's GLM configuration. This detail is why the
example defines a subclass even though it could use a built-in projection when
the application's GLM configuration matches the renderer. The subclass below
starts with `PerspectiveProjection`, which supplies the field of view, clipping
planes, and viewport that its override uses.

**Source code:**

```cpp
// The deferred renderer uses a zero-to-one depth range on both supported backends.
class CameraProjection final : public math::PerspectiveProjection {
  public:
    void apply(glm::mat4& matrix) override
    {
        matrix = glm::perspectiveRH_ZO(glm::radians(fov()), _viewport.width / _viewport.height, near(), far());
    }
};
```

## 4. Make cubes with one material

A mesh contains vertices and an index buffer that references those vertices to
form triangles. A **submesh** identifies a range within that index buffer, so
different parts of the same mesh can have separate materials.

The engine's cube primitive has six face submeshes. For our canopy and colored
cube, every face should use the same material. This helper replaces those ranges
with one range starting at index zero and spanning the entire index buffer.
It preserves the geometry. Wrapping the returned primitive in `shared_ptr`
transfers ownership to the drawable resources that will use it.

**Source code:**

```cpp
// A single material range makes the canopy material cover all six cube faces.
std::shared_ptr<draw::Drawable::Mesh> singleMaterialCube(float width, float height, float depth)
{
    auto mesh = std::shared_ptr<draw::Drawable::Mesh>(geo::createCube(width, height, depth));
    mesh->submeshes = {{0, uint32_t(mesh->indices.size())}};
    return mesh;
}
```

## 5. Generate a checker texture with transparent holes

A procedural texture generates pixels in C++ instead of reading an image file.
Subclass `base::ProceduralTextureGenerator` and provide the dimensions, byte
layout, pixel buffer, and image identifier.

Our first generator creates a 32 by 32 image with four bytes per pixel: red,
green, blue, and alpha. `(y * 32 + x) * 4` locates pixel `(x, y)` in that flat
array. RGB is a constant yellow-orange. Dividing each coordinate by four forms
four-pixel-wide squares; alternating their parity sets alpha to either 255
(fully present) or zero (absent).

Return a newly allocated buffer. `base::Texture::generateImage()` hands that
buffer to a `base::Image`, which takes responsibility for its image data. Do not
return a pointer to a local array or delete the buffer before the engine uses it.
The identifier describes the generated image and allows engine texture caching
to recognize equivalent procedural images; change it when introducing a
different generated image.

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
```

## 6. Generate a texture containing material data

A texture can store numeric material properties rather than visible color.
Our second generator produces an 8 by 8 RGBA image. Red is 51 on the left half
and 204 on the right; green is always 166, blue is 230, and alpha is 255.
Later, the material will read red for metalness, green for roughness, and blue
for ambient occlusion. Storing these independent values in one image is called
**channel packing**.

Eight-bit samples are interpreted as normalized values: 51 becomes 51/255,
approximately 0.2, and 204 becomes approximately 0.8. The procedural generator's
default color type is linear, appropriate for these numeric material values.
The shader must receive data values rather than a display-color correction.

```cpp
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
```

## 7. Keep the scene data needed after construction

Most scene-building variables can be local: once a node is attached, its parent
retains it, and a drawable component retains its drawable. Keep additional
references only to objects that animation or the UI needs to access later.

`SceneState` contains the camera and light pivots, the canopy node, the editable
clone, individual light nodes, animation angles, and animation switches. It
contains no engine, main loop, renderer settings, or persistence service.

`shared_ptr` references do not duplicate the objects. For example, the canopy
node is owned through both the scene tree and this state so that UI code can
find it directly. The state itself will be a value member of the scene delegate.

```cpp
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
```

## 8. Define the scene delegate and its lifecycle

A **delegate** is an object the application calls at particular lifecycle stages.
`draw::DefaultRenderLoopDelegate` implements the normal connection between a
scene, a renderer, the output surface, and the render loop. We extend it to
provide scene construction and animation.

The constructor passes exclusive ownership of a `RendererDeferred` through a
`unique_ptr`. Deferred rendering first records surface information such as
normals and material values into intermediate images, commonly called a
G-buffer, and then uses that information for lighting. The renderer supplies
its default passes and handles their resources; the example does not implement
those passes itself. The base delegate also handles rendering and output resize
notifications, so we do not override `render()` or recreate output targets here.

`setContinuousRendering(true)` requests successive scene frames, which is needed
for visible animation. `state()` lets our UI access the small scene state.

`update()` advances each enabled angle by speed multiplied by elapsed scene
seconds. This keeps the motion independent of frame rate: 0.12 radians per
second for the camera and 0.2 for the lights. Each angle rotates a parent node
around the Y axis. Updating the pivot moves its children with it. Call the base
implementation after changing transforms so the renderer sees the new scene.

`cleanup()` first lets the base delegate release the renderer's references and
resources, then calls `Drawable::cleanup()` while the engine device is still
alive. The `_drawables` vector, declared later, retains every GPU-loaded
drawable for that purpose, including partially loaded objects if creation fails.
It is a resource-lifecycle collection, separate from the scene state used by UI
and animation. Smart pointers themselves require no individual `reset()` calls;
their references are released when the owning objects are destroyed.

Add the start of the class below. It remains open while we implement
`createScene()` in the following steps.

```cpp
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
        configuration.resolution = std::make_shared<draw::ResolutionSettings>();
        configuration.upscaler = std::make_shared<draw::FsrUpscaler>();
        std::static_pointer_cast<draw::FsrUpscaler>(configuration.upscaler)->setEnabled(false);
        configuration.finalProcessor = std::make_shared<draw::pipeline::SMAAProcessor>();
        return configuration;
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
```

## 9. Start the scene and load the original model

`createScene()` runs when the render engine is available. The inherited
`_engine` pointer can therefore be used to upload drawable resources. Return
the root node at the end of the function; the default delegate retains it and
passes it to the renderer.

A **node** groups components and child nodes. Components give a node a role:
transforms position it, drawable components provide geometry, and environment,
camera, and light components provide their corresponding scene data.

Create a root with an `EnvironmentComponent` pointing to the HDR image. The
renderer uses that environment for the background and image-based lighting:
illumination derived from the surrounding image rather than only explicit lights.

Next, `db::loadMeshObj<draw::Drawable::Mesh>()` reads CPU geometry in the vertex
format expected by the drawable. `setMesh()` establishes the submesh material
slots. This particular OBJ has two slots, addressed as zero and one. We assign
maps explicitly using the `outer` and `inner` filename prefixes; no knowledge of
an OBJ material library is required.

The material properties have these roles:

| Property | Meaning in this scene |
| --- | --- |
| Albedo | Base surface color; a scalar tint can modulate the texture |
| Metalness | How metallic the surface is; the map varies this over the mesh |
| Roughness | How broadly reflected light is spread; higher values look rougher |
| Normal map | Changes the shading normal to add detail without new geometry |
| AO | Ambient occlusion, describing local occlusion of ambient illumination |
| Alpha cutoff | Discards fragments whose resulting alpha is below the threshold |
| Fresnel tint | Tints the angle-dependent reflection response |
| Sheen | Adds a colored grazing-angle response |

Metalness and roughness scalars are set to one so their maps supply the spatial
variation. Alpha cutoff zero keeps the opaque source model uncut. The inner
material receives a purple Fresnel tint and blue sheen. Submesh names provide
readable identifiers in addition to numeric indices.

`load(model)` is our helper, implemented later. It retains the drawable for
cleanup and calls `drawable->load(_engine)` to create the GPU mesh, material,
and texture resources. Defining geometry and materials comes before that upload.

```cpp
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
```

## 10. Place the original and create independent clones

A drawable holds renderable geometry and materials; a scene node provides its
place in the hierarchy. Our `addDrawable()` helper, defined later, creates a node
with a translation and a `DrawableComponent`, then attaches it to a parent.

First create a `Nested models` node translated upward by 0.3. The original and
blue-tinted clone are children of that node, so their local positions inherit
that upward offset. This demonstrates transform composition: a child's world
transform includes the transforms of its ancestors.

`model->clone()` makes an independent drawable. Because the source is already
loaded, cloning also creates the clone's GPU resources; do not call our `load()`
helper on these clones again. Retain them in `_drawables` for cleanup. Editing
the clone's material zero changes only the clone, even though its geometry
initially has the same shape as the original. Attaching the same drawable pointer
to two nodes would instead share that drawable and its material values; use
`clone()` when those values must be independent.

Create two further clones with transparent materials. An alpha value below one
controls their opacity, and `setIsTransparent(true)` selects transparent
rendering. This differs from alpha testing: translucent fragments contribute
partially, whereas alpha-tested fragments are either kept or discarded.
`setRefractionFactor(0.1f)` controls screen-space refraction strength for the
white variant; it is not a physical index of refraction.

The green variant explicitly sets `isSolid` to true. That flag is already true
by default in `MaterialAttributes`, so this example does not establish a
solid-versus-nonsolid comparison simply by omitting the setter on the first
variant. The two variants visibly differ through their color, opacity, and
refraction values. Both also inherit the source model's zero alpha cutoff,
which allows their fractional alpha values to survive the alpha test.

```cpp
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
```

## 11. Add the floor and the cutout canopy

Create a 30 by 30 plane for the floor. It uses a neutral gray, nonmetallic
material with roughness 0.75. A local Y translation of -1.05 puts it below the
models. With no material index argument, `material()` accesses slot zero.

For the canopy, use a thin cube and the procedural checker generator. Construct
a `base::Texture` from the generator and assign it as the albedo map. Disable
mipmaps and choose nearest filtering to keep the small procedural pattern sharp.
Mipmaps are smaller prefiltered copies of a texture used at a distance; nearest
filtering chooses individual texels rather than blending neighbors. These choices
make the generated pattern easier to inspect, though they can alias at a distance.

The cutoff of 0.5 discards checker squares with alpha zero and retains those
with alpha one. Leave the material on the opaque path: the holes come from
alpha testing, not from transparent blending. Keep the returned node in
`_state.canopy` so the UI can enable or disable the whole canopy later.

```cpp
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
```

## 12. Use the packed texture and a second UV set

UV coordinates map a surface to texture coordinates. The drawable's vertex
format has two sets, `texCoord0` and `texCoord1`, which a material can select
independently for a map. Start from another single-material cube and set UV1
to twice UV0 before uploading the geometry.

The packed material uses the same generated texture in three roles:

| Material input | Channel index | Stored value | UV mapping |
| --- | --- | --- | --- |
| Metalness | 0, red | Approximately 0.2 or 0.8 | UV1, then an additional `(2, 1)` scale |
| Roughness | 1, green | Approximately 0.65 | Default UV0 |
| AO | 2, blue | Approximately 0.90 | Default UV0 |

Channel indices count from zero: red, green, blue, alpha. Using UV1 for metalness
and scaling its U coordinate by another factor of two demonstrates that each
map can select its own coordinates. With the texture's default repeat addressing,
coordinates beyond one repeat the image. The roughness and AO values are
constant, so their different UV choice does not introduce a visible pattern.

This cube receives a separate orange albedo. Its packed RGB values describe
material properties rather than the color displayed directly on the cube.

```cpp
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
```

## 13. Place the camera under an orbit pivot

The camera node needs a world transform and a `CameraComponent`. GLM's
`lookAtRH(eye, target, up)` produces a **view** matrix: it transforms world
coordinates into the camera's frame. A node transform does the opposite job,
placing that node in the world. Invert the view matrix before giving it to
`TransformComponent`.

The camera begins at `(0, 5, 13)`, looks toward `(0, 0.5, 0)`, and uses positive
Y as up. Assign the projection class from step 3 to its camera component. The default
renderer uses an enabled camera from the scene; this scene supplies just one,
so there is no separate active-camera selection call.

Attach the camera to an identity-transform parent called `Camera orbit`.
Rotating that parent around Y moves the camera around the origin while rotating
its orientation with it. This is why `update()` changes the pivot instead of
rebuilding the camera position and look-at matrix every frame.

The raw pointers passed to `addComponent()` and `setProjection()` transfer
ownership to their receiving engine objects; they are not objects the
application should delete afterward.

```cpp
        _state.cameraPivot = std::make_shared<scene::Node>("Camera orbit");
        _state.cameraPivot->addComponent(new scene::TransformComponent());
        auto camera = std::make_shared<scene::Node>("Camera");
        camera->addComponent(new scene::TransformComponent(glm::inverse(glm::lookAtRH(
            glm::vec3(0, 5, 13), glm::vec3(0, 0.5f, 0), glm::vec3(0, 1, 0)))));
        camera->addComponent(new scene::CameraComponent());
        camera->camera()->setProjection(new CameraProjection());
        _state.cameraPivot->addChild(camera);
        root->addChild(_state.cameraPivot);
```

## 14. Add four lights to another rotating pivot

Use the same parent-node technique for the lights. Each light node gets a
translation and a `LightComponent`. Angles separated by approximately pi/2
place the four lights at quarter turns around a radius-five circle, all at
height five. `sin(angle)` and `cos(angle)` give their X and Z coordinates.

`TypeOmni` selects an omnidirectional point-style light. Assign each one a
slightly different color and intensity 35. Intensity is the renderer's lighting
strength parameter; it is not a display RGB value.

`setCastShadows(true)` requests shadows. Source size 0.25 controls the emitter
size used for soft shadows, and eight shadow samples controls sampling quality.
Shadow availability depends on the backend and device capabilities; the scene
still expresses its light properties through the same API.

The parent owns the light nodes. `_state.lightNodes` keeps convenient references
for the UI, while `_state.lights` identifies the parent that `update()` rotates.
Finally return the completed root to the default delegate. This closes
`createScene()`, but leaves the delegate class open for its private helpers.

```cpp
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
```

## 15. Finish the delegate with resource and node helpers

`load()` records a drawable before starting its GPU upload. If loading fails
partway through, `cleanup()` still has a reference to the object whose
resources must be released. All newly constructed drawables use this helper;
already-loaded clones are added to `_drawables` directly.

`addDrawable()` separates placement from geometry creation. It creates a node,
adds a translation component and a drawable component, attaches the node to its
parent, and returns it. `std::move(drawable)` transfers this local shared-pointer
reference to the component; other shared references to the same drawable remain
valid. The overload of `addComponent()` accepting a raw pointer takes ownership
of the component.

Declare `_state` and `_drawables`, then close `SceneDelegate`:

```cpp
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
```

## 16. Open a small scene control window

The UI delegate is separate from the scene delegate. Its job is to describe
widgets through `bg2e::ui`; engine code performs the underlying UI rendering.
No ImGui header or function is needed in the application.

Store a `weak_ptr` to the scene delegate. The application owns the render
delegate; the UI merely needs access to it. Locking the weak pointer gives a
temporary `shared_ptr` while drawing the panel, and fails safely if the scene
delegate has been destroyed. Also check `scene()`, because controls must not
access scene objects before `createScene()` has completed.

The window constructor sets its title, initial position, and dimensions. The
`draw()` callback that follows describes the controls shown inside the window.
We will continue that callback in the next steps.

```cpp
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
```

## 17. Add animation switches and light editors

`checkBox()` edits a boolean through its pointer and returns true when the
widget changes. These two switches control the animation branches in
`SceneDelegate::update()`. Obtain the current main loop through
`MainLoop::current()` to request a scene frame; there is no need to store the
loop in the state. Continuous rendering is already enabled in this example,
so the request also documents the change's effect on scene scheduling.

For each light, copy its current intensity or color into a local value and pass
that value to the widget. `sliderFloat()` returns true when the user changes
the value within the supplied range. `colorPicker()` edits a `base::Color`.
Use a label containing the light number so the widgets have distinct identities.

The `edit()` helper used here is defined in step 19. It schedules scene-resource
changes at the default delegate's renderer update boundary. Capture the index
and the edited value by value: the local UI variables will no longer exist
when the queued callback executes. Do not capture the loop-local `light`
reference by reference in that delayed operation.

```cpp
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
```

## 18. Edit the clone and toggle the canopy

Read material zero from the clone, then offer a color picker and a roughness
slider. These controls modify the independent clone created in step 10. The
original keeps its material values. The albedo value acts as a tint over the
existing albedo texture; changing it does not replace the image.

To hide the canopy, change the node's enabled flag instead of removing the node
or deleting its drawable. That lets the same scene object become visible again
without loading its resources a second time.

As with light properties, queue these changes through `edit()`. This completes
the window callback and `drawUI()`.

```cpp
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
```

## 19. Apply UI edits at the renderer update boundary

A UI callback should not rebuild or mutate renderer-facing state arbitrarily
while rendering work may use it. `enqueueRendererUpdate()` schedules the
operation for the default delegate's update boundary.

The helper accepts `std::function<void(SceneState&)>`: any callback that takes
our state and returns nothing. `rendererGeneration()` identifies the renderer
binding for which the operation was created. The default delegate checks that
generation when applying queued work, so an edit belonging to an obsolete
binding is not applied to a different renderer binding.

The callback captures a weak delegate reference rather than retaining the
delegate indefinitely through its own queue. It also captures the operation by
value, moving it into the lambda. At execution time, lock the weak pointer and
apply the operation to the current state if the delegate still exists. The
`draw::Renderer&` parameter is part of the queue API; these scene edits do not
need to access the renderer directly.

This helper is sufficient for our light, scalar-material, and visibility edits.
Replacing meshes or texture images would also need the corresponding drawable
resource update APIs; the controls here deliberately keep the uploaded geometry
and texture bindings intact.

```cpp
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
```

## 20. Register the delegates with the application

`app::Application` is the container for the application's delegates. Implement
`init(int, char**)` because it is the application interface, but leave the
parameters unnamed: this example parses no command-line options.

Create one `SceneDelegate`, register it as the render delegate, and pass it to
the UI delegate as a weak reference. Register a default `InputDelegate` as well;
the application contract expects render, input, and UI delegates even though
this scene does not implement custom keyboard or mouse actions.

Keep a `draw::EngineConfig` with its defaults and expose it through `config()`.
It selects Metal on macOS and Vulkan on the other supported platforms. Its type
also makes the draw rendering path explicit when we call `MainLoop::run()`.

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

## 21. Create the window and enter the main loop

The class definitions are complete, and the unnamed namespace is now closed.
`main()` creates the `MainLoop`, giving it an application identifier, then
constructs and initializes our `Application`.

`WindowConfig::withSize()` sets the window title and requested initial size to
1380 by 800. `run(&application, application.config())` creates the configured
execution path and drives input, scene lifecycle, rendering, and UI until exit.
Passing `draw::EngineConfig` selects the draw overload; omitting that argument
would select a different execution path.

The render loop invokes scene creation when the engine is available, updates
and renders frames, and calls delegate cleanup before destroying the device.
That ordering is why GPU work belongs in `createScene()` and resource cleanup
belongs in `cleanup()`, rather than uploading in the delegate's constructor or
waiting for application destruction to release GPU resources.

Catch initialization or runtime exceptions at the entry point, write their
message to standard error, and return a nonzero exit status.

```cpp
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
```

## 22. Build and run the completed example

All C++ blocks above now form the complete `main.cpp`. You can compare your
assembled file with the [repository example](../../../examples/draw/10_deferred_renderer/src/main.cpp),
but no additional application code is required.

From the repository root, configure a build with a valid Vulkan SDK path and
the supported generator for your platform. Use the same build directory and
generator as an existing checkout if it is already configured. The commands
below illustrate fresh build directories.

Linux:

```sh
cmake -S . -B build-tutorial -G Ninja -DVULKAN_SDK=/path/to/vulkan/sdk
cmake --build build-tutorial --target draw_deferred_renderer
```

macOS:

```sh
cmake -S . -B build-tutorial -G Xcode -DVULKAN_SDK=/path/to/vulkan/sdk
cmake --build build-tutorial --config Debug --target draw_deferred_renderer
```

Windows, from a developer PowerShell with the platform toolchain available:

```powershell
cmake -S . -B build-tutorial -G "Visual Studio 17 2022" -DVULKAN_SDK=C:/path/to/vulkan/sdk
cmake --build build-tutorial --config Debug --target draw_deferred_renderer
```

The SDK path is a placeholder to replace with your installation. Build outputs
and resources are deployed under `bin/<platform>`; on macOS the executable is
inside `bin/macos/draw_deferred_renderer.app/Contents/MacOS/`. The application
has no command-line switches. Keep its deployed assets and shaders alongside
it, or inside the generated bundle, instead of copying only the executable.

When the window opens, you should see the original textured model and its
tinted clone, two transparent variants, a gray floor, a checker canopy, an orange
cube with packed material data, and the environment background. The viewpoint
orbits and the lights rotate independently. Use the panel to stop either motion,
change each light's intensity and color, alter the clone's tint and roughness,
and hide or show the canopy. These interactions connect each visible change to
a scene node, light, or material introduced in the tutorial.

If initialization reports a missing resource, check the deployed asset or shader
location first. If the UI appears but geometry is missing in a modified version,
check that each new drawable has a mesh, is loaded against `_engine`, and is
attached through a `DrawableComponent` to the returned root. For unexpected
placement, remember that child transforms are relative to their parents. For
unexpected holes, distinguish material opacity from the alpha cutoff threshold.

You now have the full application structure: `Application` registers delegates,
`MainLoop` drives their lifecycle, `DefaultRenderLoopDelegate` connects the scene
to `RendererDeferred`, nodes and components describe the scene, and a small UI
queues edits to its rendered data. Additional scene objects can follow the same
sequence: construct geometry and materials, load the drawable, attach a node,
and retain only the references needed for later interaction.

## Advanced lighting and visible controls

Use the existing executable and loop from this tutorial, not a new example target.
The complete [current source](../../../examples/draw/10_deferred_renderer/src/main.cpp)
contains the settings window members, startup wiring and per-frame draw calls.
Add these includes to the UI translation unit:

```cpp
#include <bg2e/app/PreferencesStore.hpp>
#include <bg2e/draw/RenderSettingsPreferences.hpp>
#include <bg2e/ui/GraphicsSettingsWindow.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/reflection/Builder.hpp>
```

The UI delegate retains these members alongside the existing scene controls:

```cpp
std::shared_ptr<draw::RenderSettingsPreferences> _preferences;
ui::GraphicsSettingsWindow _graphics;
```

Replace the basic UI constructor with this current source implementation; construct
the UI object before the render delegate's first target attachment:

```cpp
    explicit UiDelegate(std::weak_ptr<SceneDelegate> delegate) : _delegate(std::move(delegate))
    {
        _controls.setTitle("Deferred scene");
        _controls.setPosition(10, 10);
        _controls.setSize(360, 620);
        auto active = _delegate.lock();
        if (!active)
        {
            throw std::logic_error("Graphics settings require a live scene delegate");
        }
        // Connect before the first renderer attachment; callbacks rebind on replacement.
        auto& store = app::PreferencesStore::instance().preferences("draw.deferred.graphics");
        _preferences = std::make_shared<draw::RenderSettingsPreferences>(&store);
        _preferences->connect(active);
        _graphics.bind(_preferences);
        _graphics.setPosition(390, 10);
        _graphics.setSize(520, 620);
        _graphics.open();
    }
```

The facade's weak lifecycle callbacks clear/rebind targets on renderer changes.
It borrows the PreferencesStore scope, which outlives the facade. Nothing restores
preferences automatically. Call `_graphics.draw()` at the start of the existing
`drawUI()` after checking the delegate/scene. Inside the scene controls add:

```cpp
if (ui::Button::button("Graphics settings"))
{
    _graphics.open();
}
```

Both examples show the window initially. Expand Renderer > Indirect lighting to
select GI and Global illumination > Enabled to activate it. Reflections > Enabled
is independent. Check available/status and selected graph; stored requests may
remain enabled when hardware/shaders are unavailable. GI requires Vulkan native
pipeline support or Metal query support plus indexed tables/HDR formats; FSR is
not a prerequisite. Renderer-created tracing defaults are disabled; manipulation
explicitly enables GI/reflections, while deferred leaves them disabled. Both inject
disabled FSR and SMAA; fallback stays ordinary resampling. Manipulation's initial
scale is 0.7; deferred's resolution policy is disabled/1.0.

Pause camera/light animation for convergence. Compare IBL versus GI, then reflection
off/on. Lighting debug views 11..14 expose resolved visibility, diffuse GI,
reflected radiance and confidence; these are last-layer signals, with final color
still affecting display. Temporarily disable temporal/denoise to inspect raw noise.
Restore them separately; filtering must not feed the history. Read
[signal/quality/ownership contracts](../../architecture/draw_advanced_lighting.md)
for normalized GI versus confidence-weighted reflection meanings.

All edits use queued facade/catalog handles with actual renderer generations. For
custom scene/material mutations, call Renderer::invalidateHistories at a safe
boundary, including partial failures, then request redraw. Do not mutate GPU/scene
resources while drawing controls. Do not equate nominal history frameCount with an
exact frame-average/export API.

### Native resources and verification

Use the common [build/deployment guide](../../apps/draw_deferred_renderer.md), deploy
all paired draw effect/final-processing shaders and retain official SMAA lookup
headers/licenses. Do not infer effect availability from an executable starting.
The [manual integration matrix](../../../tests/draw/advanced_integration/README.md)
covers native diagnostics, missing SDK/profiles, no RT, edits/resize/rebind and
output-owner lifetime. Tutorial snippets were checked against declarations/source;
no compilation/runtime execution was performed by the documentation agent.
