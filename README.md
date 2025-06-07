
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

## Configuration

### macOS

On macOS the configuration of Vulkan is a bit special, as on this platform there is no concept of a system or user environment variable. Environment variables only work for command line applications, so you have to set the paths to Vulkan manually.

However, if you install VulkanSDK in your user folder (which is the default installation location) and you install the exact version of VulkanSDK specified here, you won't have to make many changes.

Currently, the VulkanSDK version required form macOS is **1.4.313.1**

In the project configuration, check that the `VULKAN_SDK` setting corresponds with the location of your VulkanSDK installation. If you have instaled a different version of Vulkan, you can change here this setting:

- VULKAN_SDK Default value: `$(HOME)/VulkanSDK/1.4.313.1/macOS`

Note that if you change the version of the VulkanSDK, you may also need to modify your Xcode projects to copy the correct versions of the library into the application package. It is best to use the version of VulkanSDK specified here, in the case of macOS

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


