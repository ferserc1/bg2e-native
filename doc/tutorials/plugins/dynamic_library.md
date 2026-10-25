# Tutorial: Create and load a dynamic library

This tutorial builds two binaries: a small library exporting a message function,
and a bg2 engine application that loads it when you click a button. The window
has a fixed blue-gray background and a panel that displays the plugin's message.
No knowledge of the engine's application, rendering, or UI lifecycle is assumed.

**Sources:** [application](../../../examples/plugins/dynamic_library/src/main.cpp),
[library](../../../examples/plugins/dynamic_library/plugin/getMessage.cpp),
[CMake](../../../examples/plugins/dynamic_library/CMakeLists.txt).

## What you will learn

- Export a C entry point from a C++ dynamic library.
- Build the library separately from its host application.
- Set build dependencies and deploy the library to a known location.
- Open an engine window, clear its scene image, and draw UI controls.
- Load a library, resolve a function, call it, and manage its lifetime safely.

## Prerequisites and project layout

Use a checkout of bg2e-native with its submodules and normal platform dependencies
installed, a C++20 compiler, CMake, and a valid Vulkan SDK. The engine build needs
the SDK even when using Metal on macOS. No application shaders, models, or textures
are required. You should understand C++ classes, exceptions, and pointers; the
function pointer used at the plugin boundary is explained below.

The files already exist in the repository:

```text
examples/plugins/dynamic_library/
  CMakeLists.txt
  plugin/getMessage.cpp
  src/main.cpp
```

The repository's `examples/CMakeLists.txt` registers them with
`add_subdirectory(plugins/dynamic_library)`. Keep the plugin source outside
`src`: the application helper collects `.cpp` files in `src` for the executable.

## 1. Define the boundary between the two binaries

A dynamic library contains code that the operating system loads into an existing
process. Its usual filename ends in `.so` on Linux, `.dylib` on macOS, or `.dll`
on Windows. Loading supplies a native handle; resolving a symbol supplies the
address of an exported function or variable.

Our contract is a function named `getMessage`, taking no arguments and returning
`const char*`: a pointer to a null-terminated C string. The library owns that
string. The host reads it and copies it into its own `std::string`.

The application is not linked against this library. It only knows the symbol
name and signature, so it can decide when to load it. Both binaries must target
the same platform/architecture and agree on the calling convention and signature.
This example uses the toolchain's default C calling convention on both sides.

## 2. Implement the library

Put this in `plugin/getMessage.cpp` (repository files also carry the GPL header):

```cpp
#ifdef _WIN32
#define DYLIB_EXAMPLE_EXPORT __declspec(dllexport)
#else
#define DYLIB_EXAMPLE_EXPORT __attribute__((visibility("default")))
#endif

extern "C" DYLIB_EXAMPLE_EXPORT const char* getMessage()
{
    return "Hello from the bg2e dynamic library!";
}
```

`extern "C"` gives the function C language linkage, avoiding C++ overload name
mangling at this boundary. Export visibility is a separate requirement:
`__declspec(dllexport)` exports it on Windows, while the visibility attribute
makes it visible on macOS/Linux. The macro belongs to this tiny library, not
the engine's own import/export mechanism.

The string literal has static storage duration. Do not return the `c_str()` of a
local `std::string`: that object would be destroyed before the caller could read
it. Do not free or modify the returned pointer. Its storage is usable while the
library remains loaded. The library has no engine headers, engine dependency,
`main()` function, or GPU initialization.

## 3. Build and deploy both targets with CMake

Here is the complete example CMake file:

```cmake
cmake_minimum_required(VERSION 3.23) # CMP0112 NEW: deployment must not depend back on the app.

add_library(dylib_example_lib SHARED plugin/getMessage.cpp)
set_target_properties(dylib_example_lib PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugin"
)

set(APP_TARGET_NAME dylib_example_app)
if(APPLE)
    bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})
else()
    bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME}
        OUTPUT_DIRECTORY "${PRODUCT_DIR}/examples/dynamic_library")
endif()
target_compile_definitions(dylib_example_app PRIVATE
    DYLIB_EXAMPLE_FILENAME="$<TARGET_FILE_NAME:dylib_example_lib>"
)

if(APPLE)
    set(PLUGIN_DESTINATION "$<TARGET_FILE_DIR:dylib_example_app>/../Resources/plugins")
else()
    set(PLUGIN_DESTINATION "$<TARGET_FILE_DIR:dylib_example_app>/plugins")
endif()

# This is a raw getMessage library, not an engine Plugin.
# Refresh on every app build, including when only the library has changed.
add_custom_target(dylib_example_app_plugin
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGIN_DESTINATION}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:dylib_example_lib>" "${PLUGIN_DESTINATION}/$<TARGET_FILE_NAME:dylib_example_lib>"
    DEPENDS dylib_example_lib
    VERBATIM
)
add_dependencies(dylib_example_app dylib_example_lib dylib_example_app_plugin)
```

### Create the shared library

`add_library(dylib_example_lib SHARED plugin/getMessage.cpp)` asks CMake for a
loadable shared library. CMake chooses the platform prefix/suffix; typical names
are `libdylib_example_lib.so`, `libdylib_example_lib.dylib`, and
`dylib_example_lib.dll`. Do not hard-code these names in the application.

The three output properties place the library's build artifacts under the
example's binary-tree `plugin` directory. `RUNTIME_OUTPUT_DIRECTORY` covers the
Windows DLL; `LIBRARY_OUTPUT_DIRECTORY` covers Unix shared libraries;
`ARCHIVE_OUTPUT_DIRECTORY` covers artifacts such as the Windows import library.
Configuration subdirectories may be added by multi-configuration generators.
The host does not link the Windows import library: it resolves the export later.

### Create the engine application

`bundle_app_sdl` is a repository helper supplied by `cmake/utils.cmake`. Set
`APP_TARGET_NAME` as a convenient local target name; the helper uses its parsed
`TARGET_NAME` argument. It creates an executable from `src/*.cpp`, adds
engine include paths, links the engine, Vulkan, and SDL, and deploys engine
resources. On macOS it creates and populates an application bundle. The root
build includes these utilities before processing the examples; configure the
root project, not this example directory in isolation.

Linux and Windows pass `OUTPUT_DIRECTORY` to place this example in its own runtime
root before resource/runtime deployment targets are created. Other examples keep
their own plugin directories. Windows configuration subdirectories remain separate.
The shared helper stages the engine and required Windows DLLs beside the moved
executable and configures Linux runtime search paths. No special working directory
or library-path environment variable is required. macOS keeps its existing bundle
boundary and output location.

The [window tutorial's CMake section](../draw/01_window_ui.md#cmake-utilities-for-engine-applications)
has more detail on the application helpers. Everything needed for this example's
plugin build and loading is explained here.

`target_compile_definitions` supplies `DYLIB_EXAMPLE_FILENAME` as a C++ string
literal. `$<TARGET_FILE_NAME:dylib_example_lib>` is a CMake generator expression:
it resolves the correct filename for the chosen platform/configuration.
It supplies a filename, not an absolute build-machine path.

### Make building the application also build and copy the plugin

The custom target creates the destination directory and copies the library
with `copy_if_different`. `$<TARGET_FILE:...>` refers to the complete source
artifact path; `$<TARGET_FILE_DIR:dylib_example_app>` refers to the executable's
directory for the current configuration. `VERBATIM` preserves command arguments,
including paths containing spaces.

`DEPENDS dylib_example_lib` orders the copy after the library build.
`add_dependencies` orders the application after the library and copy target.
The copy target runs on each application build, so changing only the library
still refreshes the deployed file even when the executable does not relink.
These are build dependencies, not a link dependency on the plugin.

The deployed layout is:

```text
Linux:
  bin/linux/examples/dynamic_library/dylib_example_app
  bin/linux/examples/dynamic_library/plugins/<platform library filename>

Windows (example configuration):
  bin/windows/examples/dynamic_library/Debug/dylib_example_app.exe
  bin/windows/examples/dynamic_library/Debug/plugins/<platform library filename>

macOS bundle:
  dylib_example_app.app/Contents/
    MacOS/dylib_example_app
    Resources/plugins/<platform library filename>
```

The existing application helper handles the engine's own libraries/resources.
The engine-plugin `bg2e_add_plugin` helper is not used for this raw one-file
library. The engine target also links `${CMAKE_DL_LIBS}` for platforms where the POSIX loader needs a
separate library; the example does not need to repeat that linkage.

## 4. Understand the engine objects we will use

An engine window runs through `app::MainLoop`. An `app::Application` registers
three callback objects, called delegates:

| Object | Responsibility in this example |
|--------|--------------------------------|
| `draw::RenderLoopDelegate` | Record commands that fill the scene image with one fixed color. |
| `app::InputDelegate` | Supply the application's default input callbacks. |
| `ui::UserInterfaceDelegate` | Build the panel and respond to its button. |

The application stores delegates using `std::shared_ptr`, so they remain alive
while registered. `MainLoop::run` creates the native window and draw engine,
initializes the delegates, processes events, presents frames, and shuts down.
`draw::EngineConfig{}` selects Metal on macOS and Vulkan elsewhere.

`bg2e::ui` wraps the engine's UI implementation. Use these wrappers rather than
including ImGui headers or calling ImGui directly. The UI is composed over the
scene; no scene graph, camera, mesh, or custom shader is needed to clear a color.

## 5. Record the fixed scene background

`SceneDelegate::render` receives a borrowed `draw::FrameContext`. Its command
buffer is already recording, and `colorTarget` is the engine-owned retained
scene image:

```cpp
void render(const draw::FrameContext& context) override
{
    context.commandBuffer.beginRendering(&context.colorTarget);
    context.commandBuffer.clearColor(0, {0.12f, 0.18f, 0.28f, 1.0f});
    context.commandBuffer.endRendering();
}
```

`beginRendering` opens a rendering scope; `clearColor` fills attachment 0 with
RGBA values; `endRendering` closes that scope. The engine submits/presents the
commands. Do not store frame-context references or submit this command buffer
yourself. The scene image can be retained across presentation frames while the
UI continues to run. Since this color never changes, the button does not need
to request a new scene render.

## 6. Give the UI ownership of the loaded library

The UI delegate has three members:

```cpp
ui::Window _window;
base::DynamicLibrary _library;
std::string _message = "Click Load library to read the plugin message.";
```

The default `DynamicLibrary` is unloaded. Keeping it as a delegate member keeps
its native handle alive across `drawUI()` calls. A local owner inside the button
handler would unload at the end of that handler; storing function pointers for
later calls would then be unsafe.

The UI's `init(draw::Engine*, ui::UserInterface*)` callback sets the panel's
title, position, size, and `noClose` flag. The pointers are supplied by the
engine; this example does not need to use them. `drawUI()` calls
`_window.draw(lambda)` to emit its contents. `ui::Button::button("Load library")`
returns true when activated, and `ui::Text::text(_message)` displays the current
owned message string.

## 7. Find, load, and call the library

First, the handler checks `isLoaded()` so repeated clicks reuse the same owner.
`PlatformTools::pluginPath()` returns the executable-sibling `plugins` directory
on Linux/Windows, or `Contents/Resources/plugins` inside the macOS application
bundle. It does not depend on the shell's working directory. Append only the
CMake-supplied filename to match the deployment layout in step 3:

```cpp
_library = base::DynamicLibrary::load(
    base::PlatformTools::pluginPath() / DYLIB_EXAMPLE_FILENAME);
```

MainLoop automatically scans that directory before graphics startup. This raw
library exports `getMessage`, not the engine's `getPluginInstance` contract, so the
scan classifies it as `NotAPlugin`. That recoverable result does not register an
engine plugin or prevent the window from opening. The **Load library** button
still loads it independently through `base::DynamicLibrary` and resolves
`getMessage`. The automatic scanner's temporary handle is separate from the UI's
explicit library owner.

`load` returns an owning value. Its move assignment transfers the handle into
`_library`, leaving the temporary unloaded. Copying owners is disabled, which
prevents two copied objects from releasing the same loader reference.

Next, define and use the function pointer type:

```cpp
using GetMessage = const char* (*)();
auto getMessage = reinterpret_cast<GetMessage>(_library.getSymbol("getMessage"));
if (!getMessage)
{
    throw std::runtime_error("The getMessage symbol is null");
}
const char* message = getMessage();
_message = message ? message : "The plugin returned a null message";
```

Read `const char* (*)()` as “pointer to a function with no parameters returning
`const char*`.” `getSymbol` returns `void*`, so the host must convert the address
to the agreed type. The name is case-sensitive. Lookup cannot check a signature;
using the wrong type is invalid even if the name resolves successfully.

The host checks the function address, calls it while the library is loaded,
and handles a possible null string. Assigning a non-null C string to `_message`
copies its bytes. The resulting `std::string` belongs to the application and
can outlive the plugin.

The handler catches `std::exception` and displays its message. Missing library
files/dependencies and missing exports therefore appear in the panel. A failed
load leaves the owner unloaded and the next click can retry. A failed lookup
after a successful load leaves the library loaded; it is reused on the next
click. The default loader does not provide a hot-reload mechanism.

## 8. Understand unloading and pointer lifetime

The example lets the UI delegate's destruction unload its member automatically.
You may also call `_library.unload()` explicitly. It clears the owner's handle,
so `isLoaded()` becomes false and later `unload()` calls, including the destructor,
do not release it again.

A standalone ownership sequence looks like this:

```cpp
auto library = base::DynamicLibrary::load(pluginPath);
// Resolve and call functions, then copy any data you need to retain.
library.unload();
// Do not call previously resolved function pointers or read plugin-owned data.
```

A native module can remain mapped if other loader references exist; our owner
has still released its reference. Do not rely on incidental continued mapping.
Moving an owner transfers the lifetime obligation; move assignment also releases
the destination's previous library. No internal synchronization protects calls
against unloading from another thread. This example performs both on the UI
thread.

On macOS/Linux the implementation uses `dlopen` with immediate symbol binding
and local symbol visibility, `dlsym`, and `dlclose`. On Windows it uses
`LoadLibraryW`, `GetProcAddress`, and `FreeLibrary`. These platform headers stay
in the engine implementation. See the [API reference](../../api/base/DynamicLibrary.md)
for exception types, symbol-name constraints, and the nonthrowing unload contract.

## 9. Assemble the complete application

The following is the complete `src/main.cpp`, with its license comment omitted
here for readability:

```cpp
#include <bg2e/app/Application.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/base/DynamicLibrary.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/draw/RenderLoopDelegate.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Window.hpp>

#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
using namespace bg2e;

class SceneDelegate final : public draw::RenderLoopDelegate {
public:
    void render(const draw::FrameContext& context) override
    {
        context.commandBuffer.beginRendering(&context.colorTarget);
        context.commandBuffer.clearColor(0, {0.12f, 0.18f, 0.28f, 1.0f});
        context.commandBuffer.endRendering();
    }
};

class UiDelegate final : public ui::UserInterfaceDelegate {
public:
    void init(draw::Engine*, ui::UserInterface*) override
    {
        _window.setTitle("Dynamic library example");
        _window.setPosition(20, 20);
        _window.setSize(500, 200);
        _window.options.noClose = true;
    }

    void drawUI() override
    {
        _window.draw([this]
        {
            if (ui::Button::button("Load library"))
            {
                try
                {
                    if (!_library.isLoaded())
                    {
                        _library = base::DynamicLibrary::load(
                            base::PlatformTools::pluginPath() / DYLIB_EXAMPLE_FILENAME);
                    }
                    using GetMessage = const char* (*)();
                    auto getMessage = reinterpret_cast<GetMessage>(_library.getSymbol("getMessage"));
                    if (!getMessage)
                    {
                        throw std::runtime_error("The getMessage symbol is null");
                    }
                    const char* message = getMessage();
                    _message = message ? message : "The plugin returned a null message";
                }
                catch (const std::exception& error)
                {
                    _message = std::string("Error: ") + error.what();
                }
            }
            ui::Text::text(_message);
        });
    }

private:
    ui::Window _window;
    base::DynamicLibrary _library;
    std::string _message = "Click Load library to read the plugin message.";
};

class Application final : public app::Application {
public:
    void init(int, char**) override
    {
        setRenderDelegate(std::make_shared<SceneDelegate>());
        setInputDelegate(std::make_shared<app::InputDelegate>());
        setUiDelegate(std::make_shared<UiDelegate>());
    }
};

}

int main(int argc, char** argv)
{
    try
    {
        bg2e::app::MainLoop loop("org.bg2e.examples.plugins.dynamic-library");
        Application application;
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e dynamic library", 1280, 720));
        return loop.run(&application, bg2e::draw::EngineConfig{});
    }
    catch (const std::exception& error)
    {
        std::cerr << "dylib_example_app: " << error.what() << '\n';
        return 1;
    }
}
```

`Application::init` installs the three delegates. In `main`, construct the loop
before the application, give it an application identifier, register delegates,
and configure a 1280×720 window. Calling `run` enters the engine lifecycle;
the outer exception handler reports startup/runtime exceptions to standard
error. Plugin load errors are handled separately inside the panel.

The `namespace` block keeps the example's helper types local to this translation
unit. `using namespace bg2e` makes the shorter `app`, `draw`, `base`, and `ui`
names available there. The includes supply all engine and standard types used.

## 10. Configure, build, and run

Run commands from the repository root. If you already have a configured build
directory, keep its generator and SDK configuration and skip configuration.
For a fresh directory, choose the command for your platform and replace the SDK
path with your installation:

```sh
# Linux
cmake -S . -B build -G Ninja -DVULKAN_SDK=/path/to/vulkan/sdk -DBG2E_BUILD_EXAMPLES=ON

# macOS (Xcode is required for working native file dialogs)
cmake -S . -B build -G Xcode -DVULKAN_SDK=/path/to/vulkan/sdk -DBG2E_BUILD_EXAMPLES=ON
```

```powershell
# Windows
cmake -S . -B build -G "Visual Studio 17 2022" -DVULKAN_SDK="C:/path/to/VulkanSDK" -DBG2E_BUILD_EXAMPLES=ON
```

The root build requires the installed engine dependencies as well as the SDK;
see [build and distribution](../../architecture/build_and_distribution.md)
for environment-specific setup. The plugin itself has no external dependencies.
Do not reuse an existing build directory with a different generator.

Build the application target; its dependencies build and copy the plugin:

```sh
cmake --build build --target dylib_example_app --config Debug
```

With Ninja, select a build type at configure time using
`-DCMAKE_BUILD_TYPE=Debug`; `--config` selects configurations for Xcode/Visual
Studio and does not select the build type of a single-configuration Ninja tree.
To build only the library, use `--target dylib_example_lib`; that does not run
the application's copy target.

With the default `PRODUCT_DIR`, launch Linux with:

```sh
./bin/linux/examples/dynamic_library/dylib_example_app
```

For Xcode's Debug output on macOS:

```sh
./bin/macos/Debug/dylib_example_app.app/Contents/MacOS/dylib_example_app
```

For Visual Studio's Debug output on Windows:

```powershell
.\bin\windows\examples\dynamic_library\Debug\dylib_example_app.exe
```

If using another configuration, a custom output directory, or a macOS Ninja
build, adjust the directory accordingly. A single-configuration macOS build
normally puts the bundle directly under `bin/macos`.

Click **Load library**. The panel displays
`Hello from the bg2e dynamic library!`. Further clicks call the same loaded
function again. Closing the application releases the library automatically.
The application does not parse `--backend` options; it uses the default
`EngineConfig` backend described above.

## Remove the historical shared copy after migration

Earlier versions copied this example's raw library into the runtime directory
shared by ordinary examples. Rebuilding the isolated example does not remove an
old copy. With applications closed, locate and remove **only the exact historical
example library** if it exists:

| Platform | Historical file to remove manually (default output root) |
|----------|-----------------------------------------------------------|
| Linux | `bin/linux/plugins/libdylib_example_lib.so` |
| Windows, Debug | `bin/windows/Debug/plugins/dylib_example_lib.dll` |
| Windows, Release | `bin/windows/Release/plugins/dylib_example_lib.dll` |
| Windows, other existing configurations | `bin/windows/<configuration>/plugins/dylib_example_lib.dll` |
| Windows, an older flat output layout | `bin/windows/plugins/dylib_example_lib.dll` |

Use the previous `PRODUCT_DIR` instead of `bin/<platform>` for a custom output
root. Remove the individual file with the names above, preserving the directory
and every unrelated plugin. Do not recursively delete `plugins`, and do not remove
the new copies under `examples/dynamic_library` or `examples/tutorial_plugins`.
macOS has always used the example's private bundle and needs no shared-directory
migration. No configure-time cleanup or automatic binary deletion is performed.

## If loading fails

| Symptom | What to inspect |
|---------|-----------------|
| Library load error | Build the application target and check its deployed `plugins` folder; building only the library does not copy it. |
| File exists but loading fails | Read the native diagnostic; check architecture compatibility and any dependencies introduced into the library. |
| `getMessage` cannot be resolved | Check spelling, C linkage, and export visibility in the library source. |
| Old message after rebuilding | Close the running host, rebuild the application target, then restart; this example retains its loaded library. |
| Startup error before a panel appears | Inspect the console diagnostic and the normal engine graphics/runtime dependencies. |

## Next steps

- [DynamicLibrary](../../api/base/DynamicLibrary.md) — complete ownership and error contracts.
- [Base API](../../api/base/index.md) — related CPU data and platform services.
- [Window and UI](../draw/01_window_ui.md) — optional background controls, retained-scene invalidation, and backend selection.

This example establishes a single exported function contract. The engine's
[Plugin API](../../api/plugin/index.md) adds SDK/ABI validation, registration and
lifecycle management; that separate contract is demonstrated by the
[automatic component plugin tutorial](plugin.md). This raw example continues to use explicit
`base::DynamicLibrary` ownership and `getMessage`.
