
![logo](bg2-logo-web.png)

# bg2 engine - C++ API

bg2e is a graphic engine oriented to the creation of graphic applications. It is designed to cover some features that graphic engines for videogames do not have:

- Native integration with operating system APIs, especially with user interface elements.
- Load and store scenes and 3D models at runtime: no need to pack the scenes together with the executable application.
- Lightweight libraries: you don't need to distribute hundreds of megabytes of resources with the application.

## Requirements

You need to install Vulkan SDK. In macOS and Windows, during the VulkanSDK installation, Ensure you also install the following packages:

- GLM
- SDL2
- Vulkan Memory Allocator


On Linux, install the SDL2 development packages using your distro's package manager, for example, for Debian:

```sh
$ sudo apt update
$ sudo apt install libsdl2-dev libgtk-3-dev libglm-dev
```

The minimum Vulkan version required is 1.3.290.

You will also need the development tools specific for your operating system:

- macOS: Xcode 15.0 or higher
- Windows: Visual Studio 2022 with the C++ development tools.
- Linux: build essentials. Ensure you install clang and lldb.

Lastly, for all platforms, you need to install CMake 4.0 or newer

> If you want to use the same IDE for all platforms, you can install CLion from JetBrains (I do not receive any funding from them, but I personally like it and recommend it). Is free for non-commercial and open source projects.

## File format

The native bg2 engine file format is implemented in the bg2-io project, that it is available as an external library. It has been implemented in a separate repository so that it can be easily integrated into other projects. For example, it is used in the JavaScript version of the bg2 engine through a WebAssembly module. The project also includes other implementations of the format, such as the C# implementation that can be used in engines such as Unity or Godot. The C and C++ part works simply by adding the source code files to the compilation and has no external dependencies. This makes it particularly convenient to integrate, for example, as a C++ plugin for Unreal.

bg2-io is included as a Git submodule. If you did not use the `--recursive` option when downloading the repository, you will need to run the following command to download the submodule:

```sh
$ git submodule update --init
```

If you want to update the submodule, you can run:

```sh
$ git submodule update --remote
```

## Build

bg2 engine utiliza CMake para generar los proyectos. The recommended configuration for generating projects is
:

- Windows: Visual Studio 17
- macOS: Xcode
- Linux: Ninja

> Important: On macOS, you can generate the project with Ninja, but some features of the graphics engine, such as the open and save file dialogs, do not work unless Xcode is used as the project.

> **DO NOT USE NINJA** generator in **Windows**. You can try using other versions of Visual Studio as a
generator, as long as they support C++20, but if you use Ninja as a generator, **the project will not compile**.

In some scenarios, you must manually specify the VULKAN_SDK variable:

- On macOS, if the VULKAN_SDK environment variable has not been configured from the terminal
- On macOS and Linux, if you are using CLion (because it is not able to read environment variables from the IDE)

If this is one of the cases, you can manually set the VULKAN_SDK variable:

```sh
$ cd bg2e-native
$ cmake -G "Xcode" -DVULKAN_SDK="/Users/myuser/VulkanSDK/1.4.313.1/macOS" -S . -B cmake-build-debug
```

> Note: The repository's .gitignore file includes the default file generation directory for CLion, which is `cmake-build-debug`.


If you use CMake GUI or an IDE such as CLion, you can manually set the VulkanSDK directory after the first configuration attempt (it will fail if it cannot find the VulkanSDK installation).

In CLion, you can manually configure the variables after the first configuration attempt in `Settings > Build,
Execution, Deployment > CMake`.

![cmake CLion variables](doc/images/clion-cmake-settings.png)

After configuring VULKAN_SDK, you should now be able to generate the project.

Asegúrate de indicar la ruta correcta a la carpeta del SDK de Vulkan:

- On Windows: you should not need to do anything, as if the VulkanSDK installation has been followed correctly, the 
environment variable will be registered. However, the folder in this case is the folder for the version of VulkanSDK, for example `C:\VulkanSDK\1.4.313.1`.
- On macOS, the VulkanSDK path is the `macOS` directory inside the vulkan version directory, for example 
`/Users/my_user/VulkanSDK/1.4.313.1/macOS`
- On Linux, the VulkanSDK path is the architecture directory inside the vulkan version directory, for example 
`/home/my_user/VulkanSDK/1.4.313.1/x86_64`

Note that if you want to use some specific version of VulkanSDK, you only need to set the VULKAN_SDK variable in 
CMake and regenerate the project files.

