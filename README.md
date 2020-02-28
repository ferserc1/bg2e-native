
![logo](bg2-logo-web.png)

# bg2 engine - C++ API

bg2e is a graphic engine oriented to the creation of graphic applications. It is designed to cover some features that graphic engines for videogames do not have:

- Native integration with operating system APIs, especially with user interface elements.
- Load and store scenes and 3D models at runtime: no need to pack the scenes together with the executable application.
- Lightweight libraries: you don't need to distribute hundreds of megabytes of resources with the application.

bg2 engine is available in different APIs:

- JavaScript/WebGL
- C++/Vulkan (this repository)
- Swift/Metal

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

- Vulkan SDK
- CMake: to generate the project files.

### macOS

- Install Vulkan SDK from the website: [https://www.lunarg.com/vulkan-sdk/](https://www.lunarg.com/vulkan-sdk). If you want CMake to automatically detect the location of Vulkan, unzip the tar file in the same directory where you downloaded the bg2e-cpp repository, and rename it `vulkansdk`.

Then, generate the xcode project using cmake

#### Use Xcode beta

If you want to use the beta version of Xcode, you need to set the active Xcode version using the `xcode-select` command:

```bash
$ sudo xcode-select -s /Applications/Xcode-beta.app
```

Note that if you have already generated the Xcode project with the previous version, you'll need to remove the build

### Windows

- Install Vulkan SDK from the website: [https://www.lunarg.com/vulkan-sdk/](https://www.lunarg.com/vulkan-sdk). You can install it in any location, but make sure that the environment variable of the VulkanSDK location is registered.

### Linux

- Install Vulkan SDK following the instructions from the website: [https://www.lunarg.com/vulkan-sdk/](https://www.lunarg.com/vulkan-sdk). If you want CMake to automatically detect the location of Vulkan, unzip the tar file in the same directory where you downloaded the bg2e-cpp repository, and rename it to `vulkansdk`. Make sure you compile and run the Vulkan examples to check that everything is working correctly. [here](https://vulkan-tutorial.com/Development_environment#page_Linux) you can get help on installing Vulkan SDK on Linux.
- Install CMake
- Install xorg-dev package (required by GLFW3)
- Install GLFW3 compiling form source:

To install GLFW 3, download glfw from web or clone from git repository, and then exeucte:

```bash
$ cd [path to the glfw3 directory you just downlaoded]
$ mkdir build
$ cd build
$ cmake ..
$ make
$ sudo make install
```



#### Debugging on Linux

The best option I have currently found to debug on Linux with CMake is to use Visual Studio Code with CMakeTools (these instructions are for Ubuntu based distributions):

- Install build_essentials
- Install instalar gdb
- Install Visual Studio Code, and within it, install the extensions:
    - C/C++ (Microsoft)
    - CMake Tools (Microsoft)

Once installed, open the `bg2e-cpp` directory in Visual Studio Code. In the status bar:

- Select the kit, for example, GCC 7.4.0
- Select the configuration (where it says CMake:), for example, Debug
- Click on the sprocket to compile

To debug, click on the bug icon and select the application you want to debug. You can use breakpoints and inspect variables in the same way as you can do it in Visual Studio 2019 or Xcode.

If it is not possible to debug, check that the file `.vscode/launch.json' exists, although with CMake Tools it is not essential to have this file:

```json
{ 
  "version": "0.2.0", 
  "configurations": [ 
  { 
    "name": "g++-7 build and debug active file", 
    "type": "cppdbg", 
    "request": "launch", 
    "program": "${command:cmake.launchTargetPath}", 
    "args": [], 
    "stopAtEntry": false, 
    "cwd": "${workspaceFolder}", 
    "environment": [], 
    "externalConsole": false, 
    "MIMode": "gdb", 
    "setupCommands": [ 
      { 
        "description": "Habilitar la impresión con sangría para gdb", 
        "text": "-enable-pretty-printing", 
        "ignoreFailures": true 
      } 
    ], 
    "preLaunchTask": "g++-7 build active file", 
    "miDebuggerPath": "/usr/bin/gdb" 
  }]
} 
``` 
This setting can be used in a similar way for debugging with Windows or macOS.