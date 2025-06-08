# How to create a project

## Requisites

The only requirement is to have the bg2 engine library compiled and to have the development tools to compile it. To compile the application you will need the bg2e.dll library (Windows) or libbg2e.a (macOS) and VulkanSDK. When installing VulkanSDK make sure to install SDL2, GLM and Vulkan Memory Allocator (VMA).

## macOS Xcode project

Follow the instructions of the [document about creating a macOS App bundle, at the root of the repository](../macos-app-bundle.md)

## Windows (Visual Studio 2022)

### Configure project

Create an empty C++ application project and add the `main.cpp` file indicated at the beginning of the document.

You can configure the project as you wish, but you will have to change at least the following project properties:

- C++ Language Standard: ISO C++20
- VC++ Directories
	* Include Directories: Add the following paths:
		- $(VULKAN_SDK)\Include
		- $(VULKAN_SDK)\Include\vma
		- Path to the include directory of the `bg2e-native` repository.
	* Library Directories: Add the following paths:
		- $(VULKAN_SDK)\Lib
		- Path to the `bg2e.lib` library.
- Linker > Input
	* Additional Dependencies: Add the following:
		- vulkan-1.lib (from VulkanSDK)
		- SDL2.lib (from VulkanSDK)
		- SDL2main.lib (from VulkanSDK)
		- bg2e.lib (from bg2e-native built code)

If you want to use the debugger, you may add the `d` suffix to some libraries (`bg2ed.lib`, `SDL2d.lib`, `SDL2maind.lib`) and provide the paths to the pbd files.

### Copy bg2e.dll

To run the application you will need to provide access to the `bg2e.dll` file. You also have to do this for the Vulkan and SDL dll, but when installing VulkanSDK the necessary path is already included in the path, so you will only have to distribute these last DLLs together with your application to execute it in other PCs, in the same way that you have to include the DLLs of the C++ runtime. To give access to `bg2e.dll` at debug time, you can add a script execution phase that makes the copy of the DLL. You have to copy it to the working directory for debugging. What I usually do is to define the working directory in the output directory of the application binary in the project properties:

- Debugging > Working Directory: `$(OutDir)`

You can then use this script in the compilation events:

- Build Events > Post-Build Event:

```
xcopy "$(path to bg2e repository)\bin\*.dll" "$(OutDir)"
```

### Compile shaders

You can use the shader build script included with bg2e-native. It is in the `shaders/build.bat` folder. The script receives two parameters:

- Path to the shaders source code folder: the script will go through all files with `glsl` extension and compile them.
- Output path: the path where the `spv` files will be placed. The name of the files will be the same as the source file, but with `spv` extension. For example:
	* `test.vert.glsl` > `test.vert.spv`.
	* `test.frag.glsl` > `test.frag.spv`.

You can create a `shaders` folder next to the Visual Studio project folder with the following structure:

- `shaders`
    |- `src`: `.glsl` files
	|- `build.bat`: a copy of the `bg2e-native/shaders/build.bat` file

Then, modify the post-build event script to build the shaders:

```
xcopy "$(path to bg2e repository)\bin\*.dll" "$(OutDir)"
cd "$(SolutionDir)..\shaders"
call build.bat src $(OutDir)\bin\shaders
```

Note that the `build.bat` file will take care of creating the output directory for the shaders if it does not exist.

## The easyest way to create the project

The easiest way to create a cross-platform project for Windows and macOS is to use the `setup` project folder as a template, and then modify the Visual Studio and Xcode projects to fit your application.

To change the name of the executable you have to make some changes in the projects that we will see below. First of all, copy the `setup` folder with all its contents and set another name (for example, `vertex_buffer`)

### Xcode project

- Rename the `Setup.xcodeproj` file to `vertex_buffer.xcodeproj`.
- Open the project, select it in the project navigator panel, and rename the `Setup` target to `vertex_buffer`.
- In the Xcode window, unfold the `Setup` build scheme list and select `Manage Schemes...`
- Remove the `Setup` scheme and then press the button `Autocreate Schemes Now`


### Visual Studio project

We are going to modify the Visual Studio project files with a plain text editor manually, which is much easier than doing it from the Visual Studio interface.

First rename the project files:

- `Setup.sln` > `vertex_buffer.sln`
- `Setup.vcxproj` > `vertex_buffer.vcxproj`
- `Setup.vcxproj.filters` > `vertex_buffer.vcxproj.filters`

Remove the rest of the files in the `Visual Studio` folder.

Edit the project files. Search the text `Setup` and replace it with `vertex_buffer` in the following files:

- `vertex_buffer.sln`
- `vertex_buffer.vcxproj`

Set the `Debugging > Working Directory` property to `$(OutDir)`. You will have to do this whenever you open the project on another PC, as this setting is saved in the user properties file (`vertex_buffer.vcxproj.user`) and this file should never be distributed, because it may include details about the user that we do not want to broadcast.

