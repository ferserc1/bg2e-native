
![logo](bg2-logo-web.png)

# bg2 engine - C++ API

bg2e is a graphic engine oriented to the creation of graphic applications. It is designed to cover some features that graphic engines for videogames do not have:

- Native integration with operating system APIs, especially with user interface elements.
- Load and store scenes and 3D models at runtime: no need to pack the scenes together with the executable application.
- Lightweight libraries: you don't need to distribute hundreds of megabytes of resources with the application.

bg2 engine is available in different APIs:

- JavaScript/WebGL
- C++ (this repository)
- Unreal C++

Please note that bg2 engine is not suitable for all projects. To help you decide whether to continue or quit, here are some tips:

- High level API: bg2 engine is not as flexible as other Open Source graphics engines for applications. The characteristics of 3D models and materials are quite strict. In this sense it resembles more to other graphic engines for videogames. 3D models must have normal and one or two texture channels. The materials are based on the PBR model and are defined using scalar, color or texture values for the typical properties of these materials (albedo, normal, roughness, metallicity, ambient occlussion, displacement map).
- Integrated scene editor: one way or another, bg2 engine will always include an integrated scene editor to facilitate the creation of resources for loading 3D models. Currently, this editor is Open Source and has been created using the JavaScript/WebGL API through Electron.js. In the future there may be other native and commercial options, but the intention is that there is always at least one Open Source version. Obviously, the scene editor is created using bg2 engine, so if you dare you are free to create your own editor.
- Multiple APIs for each use: with bg2 engine you can create native applications for various environments, but keep in mind that each of them has its own API. You can't always program a single application and compile it on different platforms, and the APIs are designed to work in each environment following the recommendations of each language. For example, scene elements don't work the same way in the Swift API as they do in the JavaScript API. The advantage of this approach is that we get the maximum possible integration in each platform.
- Not all bg2 engine APIs have the same features: for example, the Swift APIs will have more possibilities to work with sound, because we can use the framework AVFoundation and Core Audio in a simple way. In the same way, with JavaScript APIs it is easier to use vector graphics as texture, since the web browser allows us to load SVG files, or even generate textures dynamically using a canvas. The basic features that can be created with the editor will be very similar, but when choosing the API you are going to use, you must take into account what you are going to use it for.

# The C++ API

This repository implements the C++ API for bg2 engine. Use this version of the engine if

- You want to make a multiplatform application with the maximum performance possible, especially in desktop environments (Windows, macOS or Linux).
- You need to use C++ in your application.
- It is not necessary that your application works on smartphones: it is possible to use this version as a base for an Android application (Android support is in the roadmap, but not yet tested), but if your target is iOS it will be much better to use the specific version for Swift.

## Installation

### Requirements and dependencies

bg2 engine is divided into different libraries, and not all of them have the same requirements, but basically you will need:

- VulkanSDK version 1.3
- GLFW: window system.
- GLM: math library
- Vk Bootstrap: utilities to create Vulkan objects
- VMA: Vulkan Memory Allocator. A library from AMD to allocate buffers.
- stb_image
- bg2io
- tiny_obj_loader

You only have to install VulkanSDK v1.3 from the official website. The rest of the dependencies are downloaded and configured with the installation script included for your platform.

### macOS Xcode
#### Setup script

Install Xcode Tools from the Mac App Store and Vulkan SDK version 1.3.x from the official Vulkan site. It is important that you note the location of the VulkanSDK installation to complete the installation:

[https://www.lunarg.com/vulkan-sdk/](https://www.lunarg.com/vulkan-sdk/)

To use the configuration script, you must supply the path to the particular version of VulkanSDK. It is possible to have several SDKs installed simultaneously, so each SDK is installed in a location in the following form:

`/installation/dir/VulkanSDK/1.3.xxx.x`

To run the dependency installation script, just pass as a parameter the path to the folder of the version you want to use. For example, if you install versions `1.3.204.1` and `1.3.239.0` in your user folder (which is the default location in the installer), and you want to use the latter, just run:

```sh
% ./setup-deps-mac.sh ~/VulkanSDK/1.3.239.0
```

#### Clean script

You can clean the dependencies folder using the following script:

```sh
% ./clean-deps.sh
```

#### Manual setup

Basically, the dependency installation script executes the following commands, after checking that each of the dependencies are not configured:

```sh
% VK_SDK_PATH=path to the VulkanSDK version directory
% ln -s $VK_SDK_PATH deps/VulkanSDK
% git clone https://github.com/ferserc1/bg2-io
% https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.MACOS.zip --output deps/glfw.zip
% unzip deps/glfw.zip -d deps
% mv deps/glfw-3.3.8.bin.MACOS deps/glfw
% rm deps/glfw.zip
```

Note about GLFW headers: To avoid glfw documentation warning like `Empty paragraph passed to '@sa' command`, disable `Documentation comments` warnings in `Build Settings`

### Windows Visual Studio
#### Vulkan SDK

You need to download and install VulkanSDK for Windows 64 bits, version 1.3 or higher. In Windows platform, the VulkanSDK installer configures the `VK_SDK_PATH` environment variable to be accessible from Visual Studio, so you don't need to do anything else to setup Vulkan.

#### Setup script

To setup the dependencies using the PowerShell script, you must to meet the following requirements:

- You must to allow the execution of PowerShell scripts.
- You need `git` access from PowerShell.

You can enable script execution by opening a PowerShell window with administrator privileges and executing the following line:

```ps
Set-ExecutionPolicy -ExecutionPolicy Bypass
```

After configuring the dependencies, it is recommended to leave script execution disabled:

```ps
Set-ExecutionPolicy -ExecutionPolicy Restricted
```

After that, you can use the dependency download script in a PowerShell console:

```ps
cd path\to\bg2e-cpp
.\setup-deps-win.ps1
```

You can clean the dependencies with the `clean-deps.ps1` script if you need it, but the downloaded dependencies are set up in the `.gitignore` file.

#### Manual setup

1. Download GLFW binaries for Windows 64 bits, extract it and put it in `deps/glfw` directory.
2. Clone or download `bg2-io` from the repository [https://github.com/ferserc1/bg2-io](https://github.com/ferserc1/bg2-io) and put it in `deps/bg2-io` directory.


## Usage

### macOS

To use the bg2 engine you only have to link to the libbg2e.dylib library, but you have to distribute the other libraries it depends on as well. You also need to specify the search directories for the header files and the bg2e library. From Xcode, the steps are as follows:

- Create a macOS application project, in Objective-C language.
- Remove all the files provided by default, and add the C++ files of our application.
- In the target configuration, add the search paths for the bg2 engine header files and for the `libbg2e.dylib` library (this step will depend on how you have organised your source code).
- In the `Link Binary With Libraries` compilation phase, add `libg2e.dylib`.
- Add a copy files build phase to the `Frameworks` directory and copy the following files:
    + `libbg2e.dylib`
    + `libMoltenVK.dylib`: found in the Vulkan SDK, in the [VulkanSDK/version]/MoltenVK/dylib/macOS folder.
    + `libvulkan.1.dylib`: found in the Vulkan SDK, in the [VulkanSDK/version]/macOS/lib folder. This is a symbolic link, but Xcode will copy the original library file with the correct name `libvulkan.1.dylib`.
    + `libVkLayer_khronos_validation.dylib`: found in the Vulkan SDK, in [VulkanSDK/version]/macOS/lib.
    + `libVkLayer_api_dump.dylib`: found in the Vulkan SDK, in [VulkanSDK/version]/macOS/lib.
    + `libVkLayer_khronos_synchronization2.dylib`: found in the Vulkan SDK, in [VulkanSDK/version]/macOS/lib.
    + `libVkLayer_khronos_profiles.dylib`: found in the Vulkan SDK, in [VulkanSDK/version]/macOS/lib.
- Add a copy files build phase to the `Resources` directory, setting the subpath to `vulkan/icd.d` and copy the following files, located at the `vulkan_resources/macOS/icd.d` directory in this repository:
    + `MoltenVK_icd.json`
- Add a copy files build phase to the `Resources` directory, setting the subpath to `vulkan/explicit_layer.d` and copy the following files, located at the `vulkan_resources/macOS/explicit_layer.d` directory in this repository:
    + `VkLayer_api_dump.json`
    + `VkLayer_khronos_profiles.json`
    + `VkLayer_khronos_synchronization2.json`
    + `VkLayer_khronos_validation.json`

Note that the files included in `vulkan_resource` directory are modified versions extracted from the VulkanSDK. You can't use the original VulkanSDK files because the modified versions are designed to allow the Vulkan Loader to load the extensions and layers from the macOS application bundle. If you  want to create your own versions of these files, you can do it modifying the `library_path` attribute in the json files. Specifically, these files have been modified so that the MoltenVK dynamic libraries and Vulkan layers can be installed in the `Frameworks` folder of the application package.


### Windows

TODO: complete this