
![logo](bg2-logo-web.png)

# bg2 engine - C++ API

bg2e is a graphic engine oriented to the creation of graphic applications. It is designed to cover some features that graphic engines for videogames do not have:

- Native integration with operating system APIs, especially with user interface elements.
- Load and store scenes and 3D models at runtime: no need to pack the scenes together with the executable application.
- Lightweight libraries: you don't need to distribute hundreds of megabytes of resources with the application.

## Requirements

You need to install Vulkan SDK, and during the installation, make sure you also install the following packages:

- GLM
- SDL2
- Vulkan Memory Allocator

> Note: If you are using macOS, see the specific notes about macOS configuration. It's important that you install the exact version of VulkanSDK described in macOS configuration notes.

If you have not installed Vulkan with these options, please reinstall.

> Note: for now, the Linux ARM64 binaries of Vulkan SDK are not available to download. You need to build it from source, using the script provided in the VulkanSDK download from the website. In this case, you only need to install SDL2, because GLM is a dependency needed to build VulkanSDK. You can use your package manager (dnf, apt...) or build it from sources. For example:

```sh
$ sudo dnf install SDL2-devel
```

The minimum Vulkan version required is 1.3.290. If you are using macOS, pay special attention to the Vulkan configuration part.

You will also need the development tools specific for your operating system:

- macOS: Xcode 15.0 or higher
- Windows: Visual Studio 2022 with the C++ development tools.

## File format

The native bg2 engine file format is implemented in the bg2-io project, that it is available as an external library. It has been implemented in a separate repository so that it can be easily integrated into other projects. For example, it is used in the JavaScript version of the bg2 engine through a WebAssembly module. The project also includes other implementations of the format, such as the C# implementation that can be used in engines such as Unity or Godot. The C and C++ part works simply by adding the source code files to the compilation and has no external dependencies. This makes it particularly convenient to integrate, for example, as a C++ plugin for Unreal.

bg2-io is included as a Git submodule. If you did not use the `--recursive` option when downloading the repository, you will need to run the following command to download the submodule:

```sh
$ git submodule update --init
```

If you want to update the submodule, you can run:

```sh
$ git submodule update --remoate
```

## Configuration

### macOS

On macOS the configuration of Vulkan is a bit special, as on this platform there is no concept of a system or user environment variable. Environment variables only work for command line applications, so you have to set the paths to Vulkan manually.

Fortunately, inside Xcode it is possible to add environment variables, so all you have to do to compile the macOS version of the bg2 engine is to modify two variables in the project. This also applies to the example projects, which are inside the `examples` directory and are independent: to compile the examples, you will have to set the environment variables beforehand.

However, you can skip this step by installing the preconfigured Vulkan SDK version in **your home directory**, which is currently **1.4.313.1**.

The environment variables that you have to set, if you install a different version of the SDK, or if you install it in a different location, are defined in the project:

1. Select the `bg2e` project in the **project navigator**.
2. In the central panel, select the **bg2e** project to change the environment variables for all targets in the project.
3. In the **Build Settings** panel, type the search string `VULKAN`. Modify the following properties:
    * `VULKAN_SDK`: Set the path to the Vulkan SDK `macOS` folder here. For SDK version `1.4.313.1` this would be: `~/VulkanSDK/1.4.313.1/macOS`.
    * VULKAN_LIB_VERSION: This is the version of the `libvulkan.1.x.xxx.dylib` library. Look for this library in the Vulkan SDK `lib` directory to make sure what the name is, but if nothing changes in the future it should correspond to the name of the SDK folder, with the last digit removed. For version `1.4.313.1` it would be `1.4.313`.

With this set up, you can now compile the project. The `libbg2e-native.a` library will be generated in the `bin/Debug` or `bin/Release` folder, depending on the configuration. If you want to compile the library from the terminal, you can use the `build_macos.sh` script located in the root of the repository.

> Note: The `build_macos.sh` script compiles to the **Release** version of the Xcode project, so for the compilation to work you have to take into account the previous Vulkan SDK version settings.

## macOS Package bundle

In the document [macOS Package Bundle](macos-app-bundle.md) you have detailed instructions on how to create an Xcode project for a bg2 engine application, with all the necessary resources to distribute the application as a bundle.

### Windows

On Windows there is no special configuration to do, apart from installing Visual Studio 2022 and Vulkan SDK. But to run the code from Visual Studio you have to modify the working directory in the project options (Debugging > Workind Directory) to be the output directory (`$(OutDir)`)

![Visual Studio 2022 configuration](doc/images/vs_2022_settings.jpg)

### Linux

On Linux we will install the dependencies in one way or another depending on the distro we are using. The other dependencies we will need are GLM, SDL2 and Vulkan Memory Allocator (vma). The GLM library is header-based, but it is also available in apt, so if you are using Debian you can also install it as a package.

```sh
$ sudo apt install libsdl2-2.0-0 libsdl2-dev libglm-dev
```

The Vulkan Memory Allocator library is installed together with Vulkan, so you don't need to do anything else.

Following the package-based installation process, the Vulkan header files, SDL2, GLM and Vulkan Memory Allocator, are placed in the system header files directory (`/usr/include`). Vulkan layers configuration files are installed in `/usr/share/vulkan`. Vulkan executables are in `/usr/bin`. The vulkan libraries and layers are installed in `/usr/lib/x86_64` In this case, since the libraries are installed on the system, we don't have to set any environment variables like `$VULKAN_SDK`.


To generate the library and the main example application, simply use Make:

```sh
$ make -j$(grep -c processor /proc/cpuinfo)
```

To execute the example application, go to the `bin` directory and run:

```sh
$ LD_LIBRARY_PATH=LD_LIBRARY_PATH:. ./bg2e-example-app
```