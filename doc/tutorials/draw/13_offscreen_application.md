# Build an offscreen application from an empty C++ file

In this tutorial you will build a command-line application that renders a textured
3D scene to a PNG without opening a window. You will create the renderer, camera,
models, materials, environment and lights; choose output and internal rendering
sizes; accumulate an exact number of completed scene samples; and save only the
finished image. You will also add command-line validation and cleanup after errors.

Only basic C++ knowledge is assumed: variables, functions, classes, loops and
exceptions. Engine concepts and the additional C++ facilities are introduced where
used. You do not need previous bg2e tutorials, GPU programming, Vulkan/Metal commands
or shader-writing knowledge. bg2e::draw is the experimental rendering framework
used here. bg2e::gpu is its low-level backend abstraction; bg2e::render is a separate
production framework that this application does not initialize.

The numbered C++ blocks are **consecutive pieces of one source file**. Append each
block directly after the previous one, in order, without extra wrappers. Several
blocks intentionally leave a class/function/namespace open for the next step.
Intermediate pieces are not separate compilable programs. After step 22, the
assembled file matches the shipped
[main.cpp](../../../examples/draw/13_offscreen_application/src/main.cpp), including
its command-line parser, helper functions, scene and error handling.

## Before you start: project, tools and assets

Use a bg2e-native checkout with its dependencies/submodules present, C++20 compiler,
CMake and Vulkan SDK. Metal also requires Apple's shader compiler on macOS. A
supported GPU and driver are required even though no window appears: headless
rendering is not CPU rendering. Start in the repository root and work in
`examples/draw/13_offscreen_application/src/main.cpp`. The checkout already
registers `draw_offscreen_application` and copies shaders/assets using the standard
example resource helper. No CMake edit is needed to follow the tutorial.

Use the repository's existing configured build directory. If configuring a fresh
one, select the platform's generator and replace the example SDK path:

```sh
# Linux
cmake -S . -B build -G Ninja -DVULKAN_SDK=/path/to/vulkan/sdk
# macOS: use Xcode for the repository's native file dialog support
cmake -S . -B build -G Xcode -DVULKAN_SDK=/path/to/vulkan/sdk
# Windows: run in a developer command prompt; use your actual SDK location
cmake -S . -B build -G "Visual Studio 17 2022" -DVULKAN_SDK=C:/VulkanSDK/your-version
```

Run only the command for your platform, and do not change generators in an already
configured directory. Build after all source blocks have been assembled:

```sh
cmake --build build --config Debug --target draw_offscreen_application
```

The build supplies the engine library and paired native shaders. Preserve their
runtime layout; the executable alone is insufficient. Runtime output normally
lives in `bin/<platform>`; macOS uses a `.app` bundle. See the
[deployment guide](../../apps/draw_offscreen_application.md#build-and-resources).
The scene uses these files from the checkout's `assets/` directory:

| Asset | What it supplies |
|---|---|
| two_submeshes.obj | Triangles grouped into two separately materialized submeshes |
| two_submeshes_outer_albedo.jpg / two_submeshes_inner_albedo.jpg | Base surface color |
| Corresponding outer/inner metallic and roughness JPGs | Metallic response and reflection roughness |
| Corresponding outer/inner normal JPGs | Fine surface-normal detail without extra geometry |
| Corresponding outer/inner ao JPGs | Authored ambient-occlusion data |
| HDR_multi_nebulae_1.hdr | Background and high-dynamic-range environment lighting |

`base::PlatformTools::assetPath()` finds the deployed asset directory. The scene
therefore does not depend on starting the executable from the checkout root.

## 1. Start the file, include the APIs and open the namespace

Begin with the project license header. `<bg2e.hpp>` includes the engine's public
API, including application execution, draw resources, scene components, math and
image IO. The standard headers provide parsing, file IO, containers and ownership
helpers used below. The unnamed namespace makes the application helpers private
to this source file. `using namespace bg2e` shortens engine names; `fs` is an alias
for std::filesystem, the C++ path/directory API. Leave this namespace open.

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

#include <charconv>
#include <cmath>
#include <exception>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {
using namespace bg2e;
namespace fs = std::filesystem;
```

## 2. Represent the requested job

Options is ordinary application data, not an engine object. Output dimensions
are the final PNG dimensions. Internal dimensions or a scale can reduce the work
performed before reconstructing to the output size. `frames` counts completed scene
samples that contribute to the mean. `gpu::BackendType` selects the graphics API;
EngineConfig supplies Metal by default on macOS and Vulkan elsewhere.

`std::optional<T>` holds either a T or no value. This lets us distinguish an omitted
command-line override from an explicit value. `fs::path` stores paths portably.
UnknownSettingPolicy controls how imported renderer settings handle unknown IDs;
Reject is the safe default, with explicit diagnostic skipping available later.

```cpp
struct Options {
    fs::path outputDirectory = "offscreen_output";
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t frames = 1;
    gpu::BackendType backend = draw::EngineConfig{}.backend;
    std::optional<gpu::Size2D> internalExtent;
    std::optional<float> scale;
    fs::path graphicsConfig;
    draw::UnknownSettingPolicy unknownSettings = draw::UnknownSettingPolicy::Reject;
    bool help = false;
};
```

## 3. Validate numeric arguments

Command-line arguments are text. `std::from_chars` converts positive integer text
without accepting leftover characters. Width, height and sample count cannot be
zero. `parseScale` uses strtof to read a float, checks that all text was consumed,
and rejects nonfinite or out-of-range values. Exceptions carry a diagnostic to
main, where a failure becomes process exit code 1. `std::string_view` borrows text;
parseScale creates a string because strtof requires a null-terminated buffer.

```cpp
uint32_t parseUnsigned(std::string_view value, const char* option)
{
    uint32_t result = 0;
    const auto parsed = std::from_chars(value.data(), value.data()+value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data()+value.size() || result == 0)
    {
        throw std::invalid_argument(std::string(option)+" expects a positive integer");
    }
    return result;
}

float parseScale(std::string_view value)
{
    std::string copy(value);
    char* end = nullptr;
    const float result = std::strtof(copy.c_str(), &end);
    if (end != copy.c_str()+copy.size() || !std::isfinite(result) || result < 0.25f || result > 1.0f)
    {
        throw std::invalid_argument("--scale expects a number between 0.25 and 1");
    }
    return result;
}
```

## 4. Read every supported command-line option

argc counts arguments and argv contains their strings. Skip argv[0], the program
name. A set records options already supplied so duplicates reject. The local
`value` lambda is a small function capturing `i`, argv and option by reference:
it checks for a following argument and advances i before returning it.

The loop handles output size, optional internal size or scale, exact frame count,
backend, imported settings and skip policy. `graphicsConfig` is a SettingsCodec
export path, not a scene/model file. The frame-count upper bound matches the
accumulator. Unknown options reject instead of being silently ignored. This block
leaves parseOptions open for the cross-option checks in the next step.

```cpp
Options parseOptions(int argc, char** argv)
{
    Options options;
    bool widthSet = false, heightSet = false, internalWidthSet = false, internalHeightSet = false;
    uint32_t internalWidth = 0, internalHeight = 0;
    std::set<std::string_view> seen;
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view option(argv[i]);
        if (option != "--help" && option != "-h" && !seen.insert(option).second)
        {
            throw std::invalid_argument("Option supplied more than once: "+std::string(option));
        }
        auto value = [&]() -> std::string_view
        {
            if (i+1 >= argc)
            {
                throw std::invalid_argument(std::string(option)+" requires a value");
            }
            return argv[++i];
        };
        if (option == "--help" || option == "-h")
        {
            options.help = true;
        }
        else if (option == "--output-dir")
        {
            options.outputDirectory = value();
        }
        else if (option == "--width")
        {
            options.width = parseUnsigned(value(), "--width");
            widthSet = true;
        }
        else if (option == "--height")
        {
            options.height = parseUnsigned(value(), "--height");
            heightSet = true;
        }
        else if (option == "--internal-width")
        {
            internalWidth = parseUnsigned(value(), "--internal-width");
            internalWidthSet = true;
        }
        else if (option == "--internal-height")
        {
            internalHeight = parseUnsigned(value(), "--internal-height");
            internalHeightSet = true;
        }
        else if (option == "--scale")
        {
            options.scale = parseScale(value());
        }
        else if (option == "--frames")
        {
            options.frames = parseUnsigned(value(), "--frames");
            if (options.frames > 1048576)
            {
                throw std::invalid_argument("--frames must not exceed 1048576");
            }
        }
        else if (option == "--backend")
        {
            const auto backend = value();
            if (backend == "vulkan")
            {
                options.backend = gpu::BackendType::Vulkan;
            }
            else if (backend == "metal")
            {
                options.backend = gpu::BackendType::Metal;
            }
            else
            {
                throw std::invalid_argument("--backend expects vulkan or metal");
            }
        }
        else if (option == "--graphics-config")
        {
            options.graphicsConfig = value();
        }
        else if (option == "--skip-unknown-settings")
        {
            options.unknownSettings = draw::UnknownSettingPolicy::SkipUnknownOrUnavailable;
        }
        else
        {
            throw std::invalid_argument("Unknown option: "+std::string(option));
        }
    }
```

## 5. Validate relationships between options

A lone width or height would leave ambiguous dimensions, so the parser requires
both. Internal width/height are also paired, and a fixed internal extent cannot be
combined with scale. `gpu::Size2D` is simply the engine's width/height value type.
Testing an optional tells us whether it contains a value. An empty destination is
invalid. With these checks passed, return one complete Options object.

```cpp
    if (widthSet != heightSet)
    {
        throw std::invalid_argument("--width and --height must be supplied together");
    }
    if (internalWidthSet != internalHeightSet)
    {
        throw std::invalid_argument("--internal-width and --internal-height must be supplied together");
    }
    if (internalWidthSet)
    {
        options.internalExtent = gpu::Size2D{internalWidth, internalHeight};
    }
    if (options.scale && options.internalExtent)
    {
        throw std::invalid_argument("--scale and explicit internal dimensions are mutually exclusive");
    }
    if (options.outputDirectory.empty())
    {
        throw std::invalid_argument("--output-dir must not be empty");
    }
    return options;
}
```

## 6. Explain the command-line interface

This function writes help without creating an Engine, Device or output file.
The renderer has two size domains: output dimensions determine the saved image,
while internal resolution changes scene rendering cost. The final sentence makes
the example's no-overwrite policy visible before a user starts a long render.

```cpp
void printUsage(const char* executable)
{
    std::cout << "Usage: " << executable << " [options]\n"
        << "  --output-dir DIR                 destination directory (default offscreen_output)\n"
        << "  --width N --height N             output dimensions (default 1920x1080)\n"
        << "  --internal-width N --internal-height N\n"
        << "                                   fixed internal dimensions\n"
        << "  --scale N                        internal scale from 0.25 to 1\n"
        << "  --frames N                       completed samples, 1 to 1048576\n"
        << "  --backend vulkan|metal           GPU backend\n"
        << "  --graphics-config FILE           exported draw settings JSON\n"
        << "  --skip-unknown-settings          skip unavailable/unknown exported targets\n"
        << "  --help                           show this help\n"
        << "Writes scene.png and refuses to overwrite an existing file.\n";
}
```

## 7. Reserve a recognizable pending filename

We will encode and validate before publishing the final file. Starting from
scene.png, this helper constructs scene.pending.png in the same directory. Keeping
`.png` as the extension matters because db::saveImage selects its encoder from the
extension. The helper only constructs a path; it does not write anything yet.

```cpp
fs::path pendingPngPath(const fs::path& destination)
{
    auto pending = destination;
    pending.replace_filename(destination.stem().string()+".pending"+destination.extension().string());
    return pending;
}
```

## 8. Write and verify the complete PNG

The host pixels must contain exactly width × height × four bytes: red, green,
blue and alpha per pixel. Check multiplication bounds before calculating that size.
A vector owns its byte storage; data() gives the pointer required by the DB writer.
The stream API reads characters, so reinterpret_cast<char*> exposes that byte
storage to ifstream without changing its contents. The encoded-size limit also
respects the decoder API's maximum input length.
`db::saveImage(path, data, width, height, 4)` receives ready CPU pixels. It does not
render, wait for GPU work, tone-map or convert BGRA/HDR formats.

After encoding the pending PNG, read its bytes with ifstream and call
`db::decodeImageRGBA8`. The decoder returns a new owned RGBA8 vector and writes
decoded dimensions through reference parameters. PNG is lossless, so dimensions
and every byte must match the original. This checks the codec/file round-trip;
it does not prove that a camera or a scene was configured correctly.

Publishing a same-directory hard link at the final filename refuses an existing
path even if another process creates it during rendering. This requires hard-link
support on the destination filesystem. Removing the pending link leaves the same
file contents accessible through the final name. Error-code cleanup overloads
attempt removal without replacing the original exception; filesystem errors can
leave a pending link. Never remove a pending file that existed before this call.
The catch block removes only the newly written pending file and rethrows errors.

```cpp
void saveVerifiedPng(const fs::path& destination, const std::vector<uint8_t>& pixels,
    uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0 ||
        size_t(width) > std::numeric_limits<size_t>::max() / 4 / size_t(height) ||
        pixels.size() != size_t(width) * size_t(height) * 4)
    {
        throw std::runtime_error("Completed RGBA8 image has an invalid byte count");
    }

    const auto temporary = pendingPngPath(destination);
    if (fs::exists(temporary))
    {
        throw std::runtime_error("Temporary output already exists: "+temporary.string());
    }

    try
    {
        db::saveImage(temporary, pixels.data(), width, height, 4);
        const auto encodedSize = fs::file_size(temporary);
        if (encodedSize == 0 || encodedSize > static_cast<uintmax_t>(std::numeric_limits<int>::max()))
        {
            throw std::runtime_error("Encoded PNG has an invalid size");
        }
        std::vector<uint8_t> encoded(static_cast<size_t>(encodedSize));
        std::ifstream input(temporary, std::ios::binary);
        if (!input || !input.read(reinterpret_cast<char*>(encoded.data()), static_cast<std::streamsize>(encoded.size())))
        {
            throw std::runtime_error("Could not read the encoded PNG for verification");
        }

        uint32_t decodedWidth = 0;
        uint32_t decodedHeight = 0;
        const auto decoded = db::decodeImageRGBA8(encoded.data(), encoded.size(), decodedWidth, decodedHeight);
        if (decodedWidth != width || decodedHeight != height || decoded != pixels)
        {
            throw std::runtime_error("Encoded PNG does not preserve the completed RGBA8 image");
        }
        fs::create_hard_link(temporary, destination);
        std::error_code ignored;
        fs::remove(temporary, ignored);
    }
    catch (...)
    {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        throw;
    }
}
```

## 9. Define the camera projection convention

A camera has a pose (where it is and where it looks) and a projection (how 3D
positions become a 2D image). Perspective makes distant objects look smaller.
CameraProjection derives from math::PerspectiveProjection and overrides apply,
the callback that fills its projection matrix. glm is the engine's vector/matrix
math library. mat4 is a 4×4 matrix; radians converts the field of view from degrees.

perspectiveRH_ZO explicitly chooses a right-handed view and zero-to-one depth,
matching this draw renderer's depth convention. `_viewport.width / height` is the
aspect ratio, so circles keep their proportions. near() and far() are clipping
distances inherited from the projection base. No hand-written matrix math or
native GPU API is needed. `final` prevents further subclassing; `override` lets the
compiler check the virtual callback's signature.

```cpp
class CameraProjection final : public math::PerspectiveProjection {
  public:
    void apply(glm::mat4& matrix) override
    {
        matrix = glm::perspectiveRH_ZO(glm::radians(fov()), _viewport.width/_viewport.height, near(), far());
    }
};
```

## 10. Create the renderer-owning delegate

A delegate is an object whose callbacks the application calls at defined moments.
Our class derives from draw::DefaultOffscreenApplicationDelegate, which owns one
Renderer, retains its scene, imports settings and schedules an exact-N camera
average. Its constructor takes the renderer and configuration factories we will
define near the end of the class. Class member functions can call other members
defined later. `std::unique_ptr` will give the base exclusive renderer ownership;
we do not construct a second render loop or manage Vulkan/Metal commands ourselves.
Leave the class and its public section open.

```cpp
class OffscreenScene final : public draw::DefaultOffscreenApplicationDelegate {
  public:
    OffscreenScene()
        : DefaultOffscreenApplicationDelegate(makeRenderer(), makeConfiguration())
    {
    }
```

## 11. Configure the headless application before GPU creation

The app calls initConfig before allocating its GPU context. Fill the config passed
by reference from the parsed options. The inherited config's extent describes the
output surface; fixed internal dimensions/scale are separate overrides. Explicit
R8G8B8A8_UNORM selects four 8-bit normalized channels in RGBA order: each
channel is stored as a byte and represents a value from zero to one. The specialized
readback used later accepts precisely this format; the default EngineConfig uses
BGRA and is not silently converted by readPixelsRGBA8.

`config.frames` drives the default delegate's exact completed sample count.
graphicsConfig and unknownSettings are applied to initialized live renderer targets
before the first scene sample. Explicit internal-size/scale arguments apply after
imported resolution settings. An explicit document bypasses opted-in persistence;
this application does not enable persistence at all. Output size, backend and N
remain job arguments, not imported graphics values.

Create the destination directory and refuse existing final/pending output before
GPU startup. The driver subsequently creates the selected Device and offscreen
surface without a window or UI. A Device represents the application's connection
to the selected graphics processor; the surface owns the destination GPU image.

```cpp
    void initConfig(int argc, char** argv, app::DrawOffscreenApplicationConfig& config) override
    {
        _options = parseOptions(argc, argv);
        if (_options.help)
        {
            return;
        }
        config.extent = {_options.width, _options.height};
        config.engine.colorFormat = gpu::PixelFormat::R8G8B8A8_UNORM;
        config.engine.backend = _options.backend;
        config.frames = _options.frames;
        config.graphicsConfig = _options.graphicsConfig;
        config.unknownSettings = _options.unknownSettings;
        config.internalExtentOverride = _options.internalExtent;
        config.scaleOverride = _options.scale;
        _outputFile = fs::absolute(_options.outputDirectory).lexically_normal()/"scene.png";
        fs::create_directories(_outputFile.parent_path());
        if (fs::exists(_outputFile) || fs::exists(pendingPngPath(_outputFile)))
        {
            throw std::runtime_error("Output or temporary output already exists in: "+_outputFile.parent_path().string());
        }
    }
```

## 12. Release all GPU owners even if cleanup fails

Although defined here, cleanup executes when the job ends or initialization/run
fails. The base releases renderer, settings bindings and pending sample ownership.
Our vector retains scene drawables, so explicitly clean every one while Engine and
Device still exist. A shared pointer controls C++ lifetime, but GPU-backed objects
also need their resources retired at the correct engine boundary.

exception_ptr saves the first cleanup exception while later resources still get a
cleanup attempt. Rethrow after the loop. The application drains submitted GPU work
before delegate cleanup and preserves an earlier run failure if cleanup also fails.
We register drawables before loading them later, so partially loaded objects are
covered by this same path.

```cpp
    void cleanup() override
    {
        std::exception_ptr error;
        try
        {
            DefaultOffscreenApplicationDelegate::cleanup();
        }
        catch (...)
        {
            error = std::current_exception();
        }
        for (auto& drawable : _drawables)
        {
            try
            {
                drawable->cleanup();
            }
            catch (...)
            {
                if (!error)
                {
                    error = std::current_exception();
                }
            }
        }
        _drawables.clear();
        if (error)
        {
            std::rethrow_exception(error);
        }
    }
```

## 13. Export only the completed accumulated result

didRenderFrame receives a zero-based scene index after that sample's GPU work.
On intermediate callbacks hasResult is false. On the final callback the mean and
final-only processing have also finished. completedFrames counts contributions,
not extra upload/readback/final submissions. Check all three conditions before
exporting so an unfinished average cannot become the output.

`_engine` is borrowed from the base and remains valid during the callback.
offscreenSurface() returns the headless target; colorImage(0) borrows its color
image. Verify dimensions and format before `readPixelsRGBA8`. That operation waits
for any required staging and returns tightly packed CPU bytes. TransferSrc is the
desired final image layout, describing its permitted next GPU use, not a color
conversion. Call the helper from step 8 and report success only after publication.
Pointers to this surface/image must not be used after application teardown.

```cpp
    void didRenderFrame(uint64_t frame, double) override
    {
        if (frame+1 != _options.frames || completedFrames() != _options.frames || !hasResult())
        {
            return;
        }
        auto* surface = _engine->offscreenSurface();
        auto* image = surface ? surface->colorImage(0) : nullptr;
        if (!image || image->size() != gpu::Size2D{_options.width,_options.height} ||
            image->pixelFormat() != gpu::PixelFormat::R8G8B8A8_UNORM)
        {
            throw std::runtime_error("Completed offscreen image has an unexpected format or extent");
        }
        std::vector<uint8_t> pixels;
        image->readPixelsRGBA8(pixels, gpu::ImageLayout::TransferSrc);
        saveVerifiedPng(_outputFile, pixels, _options.width, _options.height);
        std::cout << "Rendered " << completedFrames() << " samples to " << _outputFile.string() << '\n';
    }
```

## 14. Start a scene graph and its environment

createScene runs after Engine initialization. A scene graph is a tree of Node
objects. Nodes organize objects and may carry components: each component supplies
one behavior, such as a transform, camera, light or drawable. addChild will connect
nodes; addComponent transfers ownership of raw components created with new.
std::make_shared creates a reference-counted owner so the scene and renderer can
retain the same node safely. Here the returned root will own the whole scene tree.

EnvironmentComponent stores the HDR image reference for the renderer's environment
services. That image supplies the visible background and image-based lighting:
light reflected from the surrounding environment. assetPath finds the deployed
resources rather than relying on the current working directory. This block leaves
createScene open while we populate the graph.

```cpp
  protected:
    std::shared_ptr<scene::Node> createScene() override
    {
        auto root = std::make_shared<scene::Node>("Offscreen deferred scene");
        root->addComponent(new scene::EnvironmentComponent(base::PlatformTools::assetPath(), "HDR_multi_nebulae_1.hdr"));
```

## 15. Load geometry and explain each material texture

A mesh describes vertices and triangle indices; a drawable combines mesh,
materials and their GPU realization. db::loadMeshObj reads the OBJ geometry into
draw::Drawable::Mesh, the renderer's supported vertex layout. The type between
angle brackets is a C++ template argument selecting that mesh representation. setMesh associates
that CPU geometry with our drawable. Each submesh is a group of triangles with
its own material. This asset has outer and inner groups.

base::Texture describes an image source and sampler settings; assigning it to
MaterialAttributes does not require native GPU commands. The lambda builds each
filename from the current submesh name and map type. The renderer interprets the
maps as follows:

| Setter | Meaning |
|---|---|
| setAlbedoTexture | Base color of the surface |
| setMetalnessTexture | Which areas behave like metal |
| setRoughnessTexture | How spread out the reflected highlights appear |
| setNormalTexture | Fine normal-direction detail used for lighting |
| setAoTexture | Authored ambient occlusion for crevices, separate from our frame mean |

Scalar metalness/roughness values of one allow the corresponding textures to carry
their full range. Alpha cutoff zero avoids discarding texels based on transparency.
Fresnel tint colors the second material's reflection response; sheen adds a colored
surface highlight. setSubmeshName preserves useful group names. Our load helper
will realize mesh/textures on the already-created Device and retain them for cleanup.
addDrawable places the object in the graph with a DrawableComponent and transform;
both helpers are defined in step 21.

```cpp
        auto model = std::make_shared<draw::Drawable>();
        model->setMesh(db::loadMeshObj<draw::Drawable::Mesh>(base::PlatformTools::assetPath()/"two_submeshes.obj"));
        const char* names[] = {"outer", "inner"};
        for (uint32_t i = 0; i < 2; ++i)
        {
            auto texture = [&](const std::string& map)
            {
                return std::make_shared<base::Texture>(base::PlatformTools::assetPath(),
                    "two_submeshes_"+std::string(names[i])+"_"+map+".jpg");
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
        addDrawable(root, "Textured model", model, {0,0,0});
```

## 16. Create independent transparent model variants

clone duplicates the loaded drawable's CPU/GPU resources and material state so
changes here do not modify the original. iterateMaterials calls a lambda for each
material. A Color can hold red, green, blue and alpha; alpha below one and
setIsTransparent(true) enable the renderer's transparent-object handling.
RefractionFactor controls a refraction contribution. setIsSolid(true) selects the
solid-transparent material behavior for the other clone. These are renderer
material choices, not a second PNG alpha/compositing operation.

Clones are already loaded, so retain them directly in the cleanup vector rather
than loading them again. Each is placed at a different scene position, making the
final image asymmetric enough for a useful orientation/content inspection.

```cpp
        auto glass = model->clone();
        glass->iterateMaterials([](auto& material)
        {
            material.setAlbedo(base::Color(1,1,1,0.45f));
            material.setIsTransparent(true);
            material.setRefractionFactor(0.1f);
        });
        _drawables.push_back(glass);
        addDrawable(root, "Transparent model", glass, {3,0,0});

        auto solidGlass = model->clone();
        solidGlass->iterateMaterials([](auto& material)
        {
            material.setAlbedo(base::Color(0.7f,1,0.7f,0.5f));
            material.setIsTransparent(true);
            material.setIsSolid(true);
        });
        _drawables.push_back(solidGlass);
        addDrawable(root, "Solid transparent model", solidGlass, {1.5f,0,2.5f});
```

## 17. Generate a floor without an external model

geo::createPlane generates a mesh procedurally; the two numbers are its dimensions.
Use a neutral nonmetallic, rough material. Place it slightly below the models with
a translation passed to addDrawable. This keeps the scene grounded and provides
visible lighting context. Procedural geometry follows the same drawable loading,
material and cleanup path as imported geometry.

```cpp
        auto floor = std::make_shared<draw::Drawable>();
        floor->setMesh(geo::createPlane(30,30));
        floor->material().setAlbedo(base::Color(0.55f,0.55f,0.55f));
        floor->material().setMetalness(0);
        floor->material().setAlphaCutoff(0);
        floor->material().setRoughness(0.75f);
        load(floor);
        addDrawable(root, "Floor", floor, {0,-1.05f,0});
```

## 18. Position a fixed camera

TransformComponent stores the camera's pose. glm::lookAtRH constructs a view
matrix from an eye position, a point to look at and an up direction. A node needs
its world pose, so inverse converts that view matrix back to a camera transform.
vec3 contains three coordinates. Here the camera is above/in front of the origin
and looks slightly above the ground; positive Y supplies the up direction.

CameraComponent lets the renderer discover this node as a camera. setProjection
takes ownership of our projection object from step 9. Adding the node to the root
makes it part of scene traversal. There is no orbit input controller or animation,
so every sample uses the same view at the default zero time step.

```cpp
        auto camera = std::make_shared<scene::Node>("Camera");
        camera->addComponent(new scene::TransformComponent(glm::inverse(glm::lookAtRH(
            glm::vec3(0,5,13), glm::vec3(0,0.5f,0), glm::vec3(0,1,0)))));
        camera->addComponent(new scene::CameraComponent());
        camera->camera()->setProjection(new CameraProjection());
        root->addChild(camera);
```

## 19. Add four lights around the scene

The Lights parent has a transform, allowing its children to share a coordinate
space. The loop places four child nodes around a circle using sine/cosine and a
quarter-turn angle increment. makeTranslated creates a transform at that position.
LightComponent owns the base::Light description read by the deferred renderer.

TypeOmni emits in all directions, color/intensity choose the contribution, and
setCastShadows requests shadowing when supported. Source size controls the light's
spread for soft shadows; shadowSamples controls requested shadow sampling.
Requested shadow settings do not imply that a backend supports every optional
tracing effect. Without it the renderer retains its ordinary raster/IBL fallback.
The scene remains usable without optional RT/FSR. Return the root after all nodes
have been added and close createScene.

```cpp
        auto lights = std::make_shared<scene::Node>("Lights");
        lights->addComponent(new scene::TransformComponent());
        root->addChild(lights);
        for (uint32_t i = 0; i < 4; ++i)
        {
            auto light = std::make_shared<scene::Node>("Light "+std::to_string(i+1));
            const float angle = float(i)*1.57079633f;
            light->addComponent(scene::TransformComponent::makeTranslated(std::sin(angle)*5,5,std::cos(angle)*5));
            light->addComponent(new scene::LightComponent());
            auto& value = light->light()->light();
            value.setType(base::Light::TypeOmni);
            value.setColor(base::Color(i == 0 ? 1 : 0.65f, i == 1 ? 1 : 0.7f, i == 2 ? 1 : 0.75f));
            value.setIntensity(35);
            value.setCastShadows(true);
            value.setSourceSize(0.25f);
            value.setShadowSamples(8);
            lights->addChild(light);
        }
        return root;
    }
```

## 20. Choose renderer components and understand averaging

RendererConfiguration retains public component owners; no native graphics API is
exposed. ResolutionSettings allows a live internal scale. FsrUpscaler is an optional
reconstruction component: it reconstructs lower-resolution rendering to the output
size and can use temporal information. It starts disabled here; unavailable FSR
falls back to ordinary bilinear reconstruction while retaining reduced resolution.
SMAAProcessor is an optional final color antialiasing component, initially disabled
by its own defaults. std::make_shared permits shared component ownership;
static_pointer_cast accesses the known FsrUpscaler type to set its initial value.
The RendererDeferred factory returns exclusive ownership with make_unique.

The default offscreen delegate updates/renders exactly N scene samples. Each
completed, reconstructed linear RGBA16F image contributes RGB to CPU double sums;
source alpha is ignored and the resolved mean has alpha one. The mean is uploaded
once to RGBA16F, then final tone/color and optional SMAA run once. Linear values are
lighting values before the final display-color operation; averaging already encoded
PNG colors would give a different result. RGBA16F uses half-precision floating-point
storage and has finite-range limits even though host sums use double precision.

Effect and available FSR histories reset before sample zero, then advance for every
sample. No warm-up sample is secretly discarded; correlated history-filtered inputs
are still the signals being averaged. Upload, final-only conversion and readback
are helper submissions, not extra scene samples. At zero timestep the delegate
checks a stable scene/settings fingerprint; detected changes require a fresh job.
The CPU reference path pays a synchronous readback per sample, so high dimensions/N
can be slow. This is camera-image accumulation, separate from UV-space bake maps.

```cpp
  private:
    static draw::RendererConfiguration makeConfiguration()
    {
        draw::RendererConfiguration configuration;
        configuration.resolution = std::make_shared<draw::ResolutionSettings>();
        configuration.upscaler = std::make_shared<draw::FsrUpscaler>();
        std::static_pointer_cast<draw::FsrUpscaler>(configuration.upscaler)->setEnabled(false);
        configuration.finalProcessor = std::make_shared<draw::pipeline::SMAAProcessor>();
        return configuration;
    }

    static std::unique_ptr<draw::Renderer> makeRenderer()
    {
        return std::make_unique<draw::RendererDeferred>();
    }
```

## 21. Complete the resource helpers and class state

load stores a shared pointer before calling Drawable::load, so cleanup can release
partial realizations if loading throws. addDrawable creates a node at its requested
position, associates the loaded drawable through DrawableComponent, then attaches
it to its parent. std::move transfers our local shared-pointer handle into the
component; the vector still retains another owner for explicit GPU cleanup.

The fields retain the job options, final path and every loaded drawable. Close the
class and unnamed namespace exactly as shown. Scene nodes retain their components;
Engine remains the application's owner, not a field duplicated in our subclass.

```cpp
    void load(const std::shared_ptr<draw::Drawable>& drawable)
    {
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

    Options _options;
    fs::path _outputFile;
    std::vector<std::shared_ptr<draw::Drawable>> _drawables;
};
}
```

## 22. Initialize, execute and report errors

main is the process entry point. Parse options early so --help exits without GPU
initialization. Construct our shared OffscreenScene delegate and OffscreenApplication.
The typed init overload is selected from the delegate type; it selects draw instead
of the separate production render delegate contract. appId identifies this
application and provides a graphics application-name fallback.

During init the driver calls initConfig, creates the headless Engine/Device/surface,
initializes the scene and imports live renderer settings at a stopped boundary.
settingsResult exposes skips/errors; print every non-error issue rather than hiding
skipped properties. Unknown scene target IDs reject by default; explicit skip is
a useful choice when importing settings from another run whose generated node IDs
differ. No name-based remap is implied.

run performs the completed-sample loop, final conversion and callback, then drains
and cleans up resources. This application never constructs a window, UI or MainLoop.
Normal completion returns zero. The std::exception handler prints diagnostics and
returns one for parsing, GPU, resource or export errors. You now have the complete
source file: build the registered target using the command given at the start.

```cpp
int main(int argc, char** argv)
{
    try
    {
        auto options = parseOptions(argc, argv);
        if (options.help)
        {
            printUsage(argv[0]);
            return 0;
        }
        auto delegate = std::make_shared<OffscreenScene>();
        app::OffscreenApplication application;
        application.init(argc, argv, "org.bg2e.examples.draw.offscreen-application", delegate);
        for (const auto& issue : delegate->settingsResult().issues)
        {
            if (!issue.error)
            {
                std::cerr << "Skipped graphics setting " << issue.path << ": " << issue.message << '\n';
            }
        }
        application.run();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_offscreen_application: " << error.what() << '\n';
        return 1;
    }
}
```

## Run your completed application and inspect the result

From the directory containing the deployed binary (on macOS, the binary inside
`draw_offscreen_application.app/Contents/MacOS`), start with a fresh destination:

```sh
./draw_offscreen_application --output-dir renders/single --width 1280 --height 720 --frames 1
./draw_offscreen_application --output-dir renders/average --width 1280 --height 720 --scale 0.5 --frames 8
```

On Windows use `draw_offscreen_application.exe` with the same arguments. Add
`--backend vulkan` or `--backend metal` to choose an available backend explicitly.
The first render uses one sample. The second renders internally at 640×360 but
saves a 1280×720 PNG after eight completed samples. Each destination should contain
scene.png, and successful output reports N and its absolute path. Reusing that
same destination rejects rather than overwriting; use another output directory.

Open scene.png with an independent image viewer. Check dimensions, upright floor
and model/light placement. The DB writer preserves image row order and final alpha
bytes and adds no flip/gamma/format conversion. The intended top row is row zero.
The decoder round-trip checks encoded bytes, but viewing asymmetric scene content
is still necessary to verify visual orientation on your backend.

If importing graphics settings exported by the interactive sample, use a real
SettingsCodec JSON document:

```sh
./draw_offscreen_application --output-dir renders/imported --graphics-config graphics.json --skip-unknown-settings --scale 0.5 --frames 8
```

Global renderer/effect IDs that match the live catalog import. Per-run environment,
light and example-only lightPose IDs can differ; our explicit skip flag reports
each skipped property. To map application-owned IDs in your own program, override
extendSettingsCatalog as explained in the [delegate reference](../../api/draw/DefaultOffscreenApplicationDelegate.md).
A graphics file configures renderer settings, not scene loading or camera-image N.

Try malformed/missing arguments, an already existing scene.png/pending file and an
unwritable destination. Each should print a diagnostic and return nonzero. Missing
runtime shaders/assets or unavailable backend initialization are also errors. For a
visual comparison with the interactive deferred example, match camera/settings,
freeze animation and disable editor overlays. N>1 may differ from a single
interactive frame because it averages the reconstructed signals before final color.

This tutorial's code assembly and APIs are checked against the shipped source.
No build, GPU run or independent image inspection was performed while writing it.
The [CLI reference](../../apps/draw_offscreen_application.md) documents all options,
filesystem limitations and runtime checks; [the architecture](../../architecture/draw_offscreen_rendering.md)
explains the ownership/data flow in more depth. To extend the application, keep
scene construction in createScene and output selection in the final callback;
retain Engine-bound resources until completion and release them before Device
teardown. Complete scene/resource saves and lightmap baking need their own APIs.
