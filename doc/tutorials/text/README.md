# Your first editable scene text

This tutorial builds a complete windowed application, not an editor plugin or a
test program. You will see **Hello World** above a floor, edit it in a small UI
window, and observe a glyph-shaped directional shadow where the device supports
the existing query-shadow renderer. No HDR environment, downloaded font, model
file, navigation system or extra light is required.

The canonical complete [main.cpp](../../../examples/tutorials/text/src/main.cpp)
contains all includes and definitions. The [example README](../../../examples/tutorials/text/README.md)
is the compact launch description. Read the sequential sections below alongside
that source; they explain every application-specific helper rather than relying
on hidden setup from another example.

## 1. Tools and repository

Install Git, CMake **3.23 or newer**, a C++20 compiler and the Vulkan SDK. SDK
configuration is required by this repository even on macOS's default Metal
backend. A working graphics driver is required; glyph shadows additionally need
the backend's supported compute ray-query/acceleration features, not merely the
ability to create a window. No RT pipeline/SBT setup belongs in this application.

- **Linux:** Ninja, SDL2/GTK3/GLM development files and native compiler tools. On
  Debian-family systems, the repository's baseline packages are `build-essential
  cmake libsdl2-dev libgtk-3-dev libglm-dev ninja-build`. Install/extract the Vulkan
  SDK separately and source its environment setup as appropriate.
- **macOS:** full Xcode (including Metal tools), its selected developer tools and
  Vulkan SDK components including SDL2, GLM and VMA. Use the **Xcode** CMake
  generator for the repository's native dialogs/bundle integration; this example
  itself opens no file dialog.
- **Windows:** Visual Studio 2022 with Desktop development with C++, the **Visual
  Studio 17 2022** generator, and Vulkan SDK components including SDL2/GLM/VMA.

See the [repository prerequisites](../../../README.md#all-platforms) and
[distribution notes](../../architecture/build_and_distribution.md) for platform
details. From a terminal, obtain the source and vendored dependencies:

```sh
git clone --recurse-submodules https://github.com/ferserc1/bg2e-native.git
cd bg2e-native
git submodule update --init --recursive
```

Commands below are **for you to run**. No configuration, build or application
execution is implied by this source walkthrough. Use a new build directory for
each platform/generator; do not switch generators inside an existing build tree.

## 2. Create the files and target

The repository already contains the finished layout. To recreate it from scratch,
create `examples/tutorials/text/src/`, put the linked complete source in
`src/main.cpp`, and use this entire `examples/tutorials/text/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME tutorial_text)
bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})
```

Add `add_subdirectory(tutorials/text)` once to `examples/CMakeLists.txt` (already
registered in this checkout). This is a subdirectory of the engine build, **not**
a standalone CMake project: the root supplies C++20, engine include/library paths,
SDK discovery, SDL and the `bundle_app_sdl` helper. The root requires CMake 3.23
even though this small subdirectory follows the tutorials' 3.18 declaration.

The helper discovers `src/*.cpp`/`src/*.hpp`, links the shared engine and SDL,
builds/deploys engine shaders and copies the normal runtime asset tree. No custom
shader, generated font atlas, extra source list or new dependency is necessary.

Required resource files already in the repository:

```text
assets/fonts/didact-gothic/DidactGothic-Regular.ttf
assets/fonts/didact-gothic/OFL.txt
assets/fonts/didact-gothic/README.md
assets/licenses/stb_truetype-MIT.txt
```

Do not replace this font with `assets/DidactGothic-Regular.ttf`: that preserved
legacy file has different bytes/provenance. The canonical
[font record](../../../assets/fonts/didact-gothic/README.md) pins its upstream
revision and SHA-256. Its **SIL OFL 1.1** license permits redistribution under its
conditions, including preserving the font notice; the engine GPL and rasterizer
MIT license do not replace the font license. The declaration explicitly lists
OFL/provenance paths for complete scene packaging. This tutorial does not save a
scene, but its application bundle/assets already include those files.

## 3. Configure, build and run

Run from the repository root. Replace SDK paths with your installed SDK directory.
If VULKAN_SDK is set correctly, the explicit variable can be omitted; IDEs may
not inherit your terminal environment. The examples option must be on. Disabling
the separate test-app option is optional and avoids unrelated targets here.

### Linux

```sh
cmake -S . -B build-text -G Ninja -DCMAKE_BUILD_TYPE=Debug -DVULKAN_SDK=/path/to/sdk -DBG2E_BUILD_EXAMPLES=ON -DBG2E_BUILD_TESTS=OFF
cmake --build build-text --target tutorial_text
./bin/linux/tutorial_text
```

### macOS

```sh
cmake -S . -B build-text -G Xcode -DVULKAN_SDK=/path/to/sdk -DBG2E_BUILD_EXAMPLES=ON -DBG2E_BUILD_TESTS=OFF
cmake --build build-text --config Debug --target tutorial_text
./bin/macos/Debug/tutorial_text.app/Contents/MacOS/tutorial_text
```

You can also open that `.app` with Finder/`open`. Code signing is disabled by the
repository; this is a development bundle, not a signed/notarized distribution.

### Windows (PowerShell)

```powershell
cmake -S . -B build-text -G "Visual Studio 17 2022" -A x64 -DVULKAN_SDK="C:/VulkanSDK/your-version" -DBG2E_BUILD_EXAMPLES=ON -DBG2E_BUILD_TESTS=OFF
cmake --build build-text --config Debug --target tutorial_text
& .\bin\windows\Debug\tutorial_text.exe
```

These are default output paths. If you customize PRODUCT_DIR or per-configuration
output properties, use that generated target location instead. The helper copies
assets/shaders beside the executable on Linux/Windows and into
`tutorial_text.app/Contents/Resources` on macOS. Keep runtime libraries and those
directories together. The standard application packaging scripts disable examples
and do **not** install this tutorial among their canonical apps; distribute its
complete built runtime tree/bundle separately under the applicable licenses.

## 4. Understand the includes and application roles

The source uses narrowly selected public headers:

| Headers | Why they are necessary |
|---|---|
| `app/Application.hpp`, `app/MainLoop.hpp` | Register delegates; create the window and own the application/Engine/UI lifecycle. Application also supplies the basic InputDelegate type. |
| `base/PlatformTools.hpp` | Locate executable/bundle-relative assets. |
| `draw/DefaultRenderLoopDelegate.hpp`, `draw/RendererDeferred.hpp`, `draw/EngineConfig.hpp` | Managed renderer, stopped updates and platform backend defaults. RendererDeferred supplies its public effect configuration types. |
| `draw/Drawable.hpp`, `geo/plane.hpp` | One ordinary procedural floor mesh/material. |
| `draw/pipeline/SoftShadowPass.hpp` | Enable/configure the existing query-shadow pass. |
| `scene/Scene.hpp`, camera/light/transform component headers | Own scene references/font registry and position ordinary scene objects. |
| `scene/TextComponent.hpp`, `scene/TextFontComponent.hpp` | Paragraph quad and declarative font resources. TextComponent includes its CPU Text types. |
| `ui/UserInterfaceDelegate.hpp`, `Window.hpp`, `Value.hpp`, `Button.hpp`, `Text.hpp` | The one UI window, multiline input, retry button and warning label; no ImGui types/functions. |
| `<memory>`, `<string>`, `<utility>`, `<iostream>`, `<exception>` | Shared/weak ownership, UTF-8 strings, moving queued values, console warnings and the outer startup/shutdown catch. |

`Application::init` creates one shared SceneDelegate, a basic InputDelegate and
UiDelegate, then registers them. The UI has a weak reference to the scene delegate,
so it cannot keep the scene/Engine alive accidentally. SceneDelegate exclusively
owns the renderer through its base class and retains its own scene/font context,
floor and TextComponent for correctly ordered cleanup.

`main` creates MainLoop with an application identifier, initializes Application,
sets an 1100x760 window title/size, and calls `loop.run(&application,
draw::EngineConfig{})`. MainLoop creates the window/Engine/UI and invokes delegate
initialization, scene construction, resize, prepare/update/render and cleanup.
EngineConfig defaults to Metal on macOS and Vulkan elsewhere. The outer catch is
for application startup/shutdown failure; recoverable **text** failures are
contained inside their text/UI boundaries, not rethrown to terminate the window.

## 5. The fixed camera

A Node is a hierarchy object; Components add transform, camera, drawable or light
behavior. A CameraComponent needs a projection and a node transform. The source
defines the small CameraProjection subclass solely to make
`glm::perspectiveRH_ZO` explicit: right-handed view space and zero-to-one depth
match this renderer, independent of GLM compile flags. It uses the inherited FOV,
viewport, near and far values; the source sets near=0.1 and far=50.

```cpp
cameraNode->addComponent(new scene::TransformComponent(glm::inverse(glm::lookAtRH(
    glm::vec3(0, 7, 10), glm::vec3(0, 0.5f, 0.6f), glm::vec3(0, 1, 0)))));
```

lookAtRH produces a **view** matrix from eye, target and up. A scene node stores
the camera's world transform, so the source stores its inverse. The camera sees
the top of the raised text and the floor-shadow offset. Renderer scene attachment/
resize updates camera viewport/projection automatically; no navigation, gizmo or
custom resize handler is needed. This minimal application does not clone/save
its application-specific camera projection.

## 6. Floor and text geometry

The floor is a draw::Drawable using `geo::createPlane(10,8)`: an XZ rectangle
centered at y=0, with an ordinary gray, metalness=0, roughness=0.9 material. Its
node has an identity TransformComponent and DrawableComponent. The source retains
the drawable before loading so cleanup also owns partially prepared resources.

TextComponent owns its own geometry: do not create a mesh or assign a filesystem
albedo. Its readable front is +Z in local XY, X right/Y up, with top-left UV origin.
The source creates a transform at `(0,1.2,0)` and rotates **-90 degrees around X**
(the rotate API uses radians). This turns its front toward +Y, facing upward above
the floor; local text-top points toward world -Z, readable from the chosen camera.

Three independent scales matter:

| Example value | Meaning |
|---|---|
| `paragraphSize={1024,256}`, fontSize=160, margins=24 | Logical layout, line breaks and font height; aspect 4:1. |
| `bitmapResolution={2048,512}` | Coverage sampling, twice the logical density in each axis. |
| `setHeight(1.4f)` | World rectangle height; default aspect preservation yields width 5.6. |

The default center pivot stays `(0.5,0.5)`. Bitmap changes do not resize the quad
or change logical wrapping. Logical paragraph aspect can change computed world
width when preserveAspect is on. `setSize({width,height})` instead selects manual
world sizing. None of these values belongs in the node's material color.

## 7. Register and load the font

The source declares a single trusted asset on the root:

```cpp
const auto fontDirectory = base::PlatformTools::assetPath() / "fonts/didact-gothic";
auto fonts = std::make_shared<scene::TextFontComponent>();
fonts->setDeclarations({{"tutorial", fontDirectory / "DidactGothic-Regular.ttf", 0,
    {fontDirectory / "OFL.txt", fontDirectory / "README.md"}}});
root->addComponent(fonts);
```

`tutorial` is a scene identifier, not a filename or system font name. Face 0
selects this static font's first face. `notices` records external documentation
files, not rendering resources. Multiple declarations can share one collection;
collections can be on any node, but duplicate identifiers across nodes are
disabled rather than shadowed by ancestors. Text uses the same identifier.

After constructing every node, the source establishes ownership and realizes:

```cpp
_sceneContext = std::make_shared<scene::Scene>();
_sceneContext->setSceneRoot(root);
_sceneContext->setMainCamera(camera.get());
_sceneContext->prepareTextFonts();
root->loadResources(*_engine);
```

Keep Scene alive: it owns the registry, not the declaration component or the
renderer root alone. `prepareTextFonts` gathers all declarations before loading
any text. Node loading also performs this font-first preparation, so node order
does not matter; the explicit call here makes the lesson visible. Floor loading
creates its ordinary GPU resources; Text loading resolves `tutorial`, rasterizes
coverage and realizes its quad/material at this stopped initialization boundary.

Ready means preparation finished, not all fonts succeeded. A missing/corrupt/
unsupported font returns a null resource with console warnings and transparent
dependent text. Other scene content/UI remain usable. Correct the deployed file
and explicitly reload; failures are cached rather than retried each frame. stb
is **not safe for untrusted fonts**; bounds checks and exception handling do not
make downloaded hostile files safe.

## 8. Build the paragraph and conventional material

The source constructs TextComponent, fills `base::Text::Parameters` with initial
content **exactly `Hello World`**, identifier `tutorial`, the dimensions above and
center/center alignment, then calls `text().setParameters(std::move(paragraph))`.
Atomic parameter replacement avoids invalid intermediate margin/box combinations.
Word wrapping/clip overflow keep their Text defaults. Short Latin/Spanish strings
work when glyphs exist; this is not a shaping/bidi/rich-text implementation.

```cpp
_text->material().setAlbedo(base::Color(0.07f, 0.28f, 0.8f));
_text->material().setMetalness(0);
_text->material().setRoughness(0.9f);
_text->material().setIsUnlit(false);
_text->material().setDoubleSided(true);
_text->material().setAlphaCutoff(0.1f);
```

Generated albedo RGB is white, even in transparent pixels; alpha is glyph coverage.
The blue conventional material multiplies that RGB. Alpha cutoff makes the shared
raster/visibility/shadow paths reject transparent texels, rather than treating a
six-index quad as a solid blocker. Double sided permits both sides; the readable
front is +Z, while the back is naturally mirrored. The example stays **lit** so
the directional light affects the text and floor. Unlit is an appearance option,
not a field of base::Text or a special text shader.

Text owns its albedo source and restores its sourceEditable=false /
ignoreSerialization=true policy. UV scale/sampling remain conventional material
data and survive paragraph regeneration. Do not replace its source through mutable
C++ access: supported editors guard it, arbitrary API misuse is not repaired.

## 9. Directional glyph shadows

A LightComponent's local **+Z** is the travel direction of light rays. The source
rotates its node **+45 degrees around X**, pointing rays down toward +Z. At text
height 1.2, the central floor shadow is offset approximately +1.2 along Z. That
offset and the elevated camera make the floor silhouette distinguishable from the
raised text. There is exactly one light: Directional, intensity=2.5, CastShadows=true,
sourceSize=0 and shadowSamples=1 for a clear hard-edged demonstration.

The constructor's `configuration()` helper supplies RendererConfiguration:

- IBL indirect mode; disable the normally enabled default GI/reflection effects.
  These are unnecessary for this scene, not prerequisites for text rendering.
- An enabled public SoftShadowPass, bias=0.002, directionalDistance=50,
  maxSamples=1. The range exceeds the scene size; the small positive bias avoids
  excessive self-intersection without separating the shadow visibly.
- Ordinary default GBuffer/deferred/final stages. No custom shaders or tutorial-only
  alpha/shadow implementation. Renderer defaults need no external HDR asset.

Vulkan/Metal capability and driver availability still matter. If the query-shadow
path is unavailable, there is no software/rectangular fake shadow fallback in this
example. See the console and troubleshooting below; the rest of the UI/scene
remains meaningful.

## 10. Live edits without GPU work in the UI

UiDelegate owns one `ui::Window` and `_content="Hello World"`. Its no-close window
contains `ui::Value::multilineText("Content", _content)` and one explicit retry
button. The read-only fontAvailable/shadowsAvailable helpers supply short labels
when a font or device shadow path is unavailable; they load/rebuild nothing.
All calls go through engine UI wrappers; no ImGui header/type is used.

When the field changes, UiDelegate calls `delegate->editText(_content)`. That method
captures an owned string and a weak delegate in `enqueueCpuValueUpdate`, tagged
with rendererGeneration. The queued callback only calls
`_text->text().setContent(std::move(content))`. It creates no GPU image, mesh,
descriptor or submission inside UI drawing. Weak/generation checks discard stale
actions if the owner/binding changes.

At the next prepareFrame, DefaultRenderLoopDelegate runs queued edits, then
TextComponent resource preparation **before acquisition**. It resolves the font,
coalesces latest CPU state, rasterizes/uploads if needed, and invalidates renderer
histories/requests scene work when publication changes. Pixel-only typing retains
the quad's live geometry identity. No overridden update/render loop is required;
the default delegate records the scene. The application is on-demand rather than
continuously animating an unchanged label.

**Reload font / retry text** uses the conservative renderer-update queue to call
Scene::reloadTextFonts and TextComponent::retryResources explicitly. Do not put
that reload in update/drawUI every frame. Text failures are contained, warning
messages are bounded/cached, and blank content intentionally draws no text or
shadow. A failed UI allocation/queue operation warns once while retaining the
running scene; the main catch is not the text recovery mechanism.

## 11. Cleanup and expected result

MainLoop stops/drains graphics users before delegate cleanup. SceneDelegate first
calls DefaultRenderLoopDelegate::cleanup so renderer snapshots release the scene,
then releases TextComponent resources and floor resources while Engine is alive,
and finally drops Scene/text/floor owners. UiDelegate holds only a weak delegate
and CPU widgets, so it does not extend native resource lifetime. A direct/custom
render loop must implement the same producer-stop/resource-order contract.

Expected display: a neutral floor, a centered raised blue Hello World with a
legible +Z-derived front, its offset glyph-shaped shadow on a capable device, and
the small live text window. Entering an empty string removes glyphs and their
shadow; retyping restores them. No selection/history/scene-saving UI is included
here—those are Composer workflows, not needed for this minimal program.

## Troubleshooting

| Observation | What to check |
|---|---|
| Missing font / transparent text | Restore the canonical font under the runtime asset tree, inspect the console, then click Reload font / retry text. Identifier must be `tutorial` in declaration and paragraph. There is no automatic default/system fallback. |
| Wrong working directory | Configure/build commands assume the repository root. Runtime assetPath is executable/bundle-relative, **not current-directory-relative**: changing cwd does not fix an executable copied without its assets/shaders/libraries. |
| Invisible or reversed side | Text front is local +Z; the example rotates -90 degrees around X to face up. Camera is above the floor on +Z. Do not flip UVs to compensate for a wrong transform. Double sided reveals the mirrored back, not a second readable front. |
| Text clipped or hidden | The fixed logical box/margins do not auto-expand. Shorten content or change logical layout/font size in source; increasing bitmap pixels alone does not make more logical room. Clip warns; Hide produces transparent output. |
| No glyph shadow | Verify query/acceleration support on the selected backend/driver, enabled SoftShadowPass, per-light CastShadows, the example's cutoff=0.1 and both geometry owners loaded. Keep text above the floor and directional range/bias appropriate. No HDR or second light is needed. |
| Rectangular shadow after changing source | Keep TextComponent's generated albedo, an enabled alpha test (cutoff at least 0.02; the example uses 0.1) and ordinary shared shadow path. Do not substitute a bare model quad or disable the glyph test. |
| Fatal startup error | Missing window/backend/shader/runtime prerequisites are application initialization failures, separate from recoverable font/text failures. Deploy/build the normal engine resources first. |

## References

- [Text](../../api/base/Text.md), [TextFont](../../api/base/TextFont.md), [TextRasterizer](../../api/base/TextRasterizer.md), [TextTextureGenerator](../../api/base/TextTextureGenerator.md)
- [TextFontComponent](../../api/scene/TextFontComponent.md), [Scene registry](../../api/scene/Scene.md), [TextComponent](../../api/scene/TextComponent.md)
- [Scene persistence/resource packaging](../../api/db/DrawResourceIO.md#editable-text-and-font-assets), [Composer authoring](../../apps/draw_composer.md#text-and-font-authoring)
- [Complete source](../../../examples/tutorials/text/src/main.cpp), [complete tutorial CMake](../../../examples/tutorials/text/CMakeLists.txt), [parent target registration](../../../examples/CMakeLists.txt)
