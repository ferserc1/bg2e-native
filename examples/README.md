# How to create a project

## Requisites

The only requirement is to have the bg2 engine library compiled and to have the development tools to compile it. To compile the application you will need the bg2e.dll library (Windows) or libbg2e.a (macOS) and VulkanSDK. When installing VulkanSDK make sure to install SDL2, GLM and Vulkan Memory Allocator (VMA).

## Example file

Place the following file in a location that you can access from the Xcode and Visual Studio project:

**src/main.cpp**

```cpp
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/Application.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/base/Log.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>

#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Window.hpp>

class ClearScreenDelegate : public bg2e::render::RenderLoopDelegate,
	public bg2e::app::InputDelegate,
	public bg2e::ui::UserInterfaceDelegate
{
public:
	void init(bg2e::render::Vulkan* vulkan) override
	{
		using namespace bg2e::render::vulkan;
		RenderLoopDelegate::init(vulkan);

		createImage(vulkan->swapchain().extent());
  
        createPipeline();
	}

	void swapchainResized(VkExtent2D newExtent) override
	{
		_targetImage->cleanup();
		createImage(newExtent);
	}

	VkImageLayout render(
		VkCommandBuffer cmd,
		uint32_t currentFrame,
		const bg2e::render::vulkan::Image* colorImage,
		const bg2e::render::vulkan::Image* depthImage,
		bg2e::render::vulkan::FrameResources& frameResources
	) override {
		using namespace bg2e::render::vulkan;

		Image::cmdTransitionImage(
			cmd,
			_targetImage->image(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL
		);

		float flash = std::abs(std::sin(currentFrame / 120.0f));
		VkClearColorValue clearValue{ { 0.0f, 0.0f, flash, 1.0f } };
		auto clearRange = Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdClearColorImage(
			cmd,
			_targetImage->image(),
			VK_IMAGE_LAYOUT_GENERAL,
			&clearValue, 1, &clearRange
		);

		Image::cmdTransitionImage(
			cmd, _targetImage->image(),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		auto colorAttachment = Info::attachmentInfo(_targetImage->imageView(), nullptr);
		auto renderInfo = Info::renderingInfo(_targetImage->extent2D(), &colorAttachment, nullptr);
		cmdBeginRendering(cmd, &renderInfo);

		macros::cmdSetDefaultViewportAndScissor(cmd, _targetImage->extent2D());

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

		vkCmdDraw(cmd, 3, 1, 0, 0);

		bg2e::render::vulkan::cmdEndRendering(cmd);

		Image::cmdTransitionImage(
			cmd,
			_targetImage->image(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		);
		Image::cmdTransitionImage(
			cmd,
			colorImage->image(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		Image::cmdCopy(
			cmd,
			_targetImage->image(), _targetImage->extent2D(),
			colorImage->image(), colorImage->extent2D()
		);
  
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}

	// ============ User Interface Delegate Functions =========
	void init(bg2e::render::Vulkan*, bg2e::ui::UserInterface*) override {
		_window.setTitle("ImGui Wrapper Demo");
		_window.options.noClose = true;
        _window.options.minWidth = 100;
        _window.options.minHeight = 50;
        _window.setPosition(0, 0);
        _window.setSize(150, 90);
	}

	void drawUI() override
	{
		using namespace bg2e::ui;
		_window.draw([]() {
			BasicWidgets::text("Hello, world!");
		});
	}

protected:
	std::shared_ptr<bg2e::render::vulkan::Image> _targetImage;

	bg2e::ui::Window _window;
 
    VkPipelineLayout _layout;
    VkPipeline _pipeline;
    
    void createPipeline()
    {
        bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_vulkan);
        
        plFactory.addShader("test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        plFactory.addShader("test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        
        auto layoutInfo = bg2e::render::vulkan::Info::pipelineLayoutInfo();
        VK_ASSERT(vkCreatePipelineLayout(_vulkan->device().handle(), &layoutInfo, nullptr, &_layout));
        plFactory.setColorAttachmentFormat(_targetImage->format());
        _pipeline = plFactory.build(_layout);
        
        _vulkan->cleanupManager().push([&](VkDevice dev) {
            vkDestroyPipeline(dev, _pipeline, nullptr);
            vkDestroyPipelineLayout(dev, _layout, nullptr);
        });
    }

	void createImage(VkExtent2D extent)
	{
		using namespace bg2e::render::vulkan;
		auto vulkan = this->vulkan();
		_targetImage = std::shared_ptr<Image>(Image::createAllocatedImage(
			vulkan,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			extent,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT
		));
	}

	void cleanup() override
	{
		_targetImage->cleanup();
	}
};

class MyApplication : public bg2e::app::Application {
public:
	void init(int argc, char** argv) override
	{
		auto delegate = std::shared_ptr<ClearScreenDelegate>(new ClearScreenDelegate());
		setRenderDelegate(delegate);
		setInputDelegate(delegate);
		setUiDelegate(delegate);
	}
};

int main(int argc, char ** argv) {

    bg2e::app::MainLoop mainLoop;
	MyApplication app;
	app.init(argc, argv);
    return mainLoop.run(&app);
}
```

Add the following shader files to your project directory:

**shaders/test.vert.glsl**

```glsl
#version 450

layout(location = 0) out vec4 outColor;

void main() {
    const vec3 positions[3] = vec3[3](
		vec3( 0.3f, 0.3f, 0.0f),
		vec3(-0.3f, 0.3f, 0.0f),
		vec3( 0.0f,-0.3f, 0.0f)
	);

	const vec4 colors[3] = vec4[3](
		vec4(1.0f, 0.0f, 0.0f, 1.0f),
		vec4(0.0f, 1.0f, 0.0f, 1.0f),
		vec4(00.f, 0.0f, 1.0f, 1.0f)
	);

	gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
	outColor = colors[gl_VertexIndex];
}
```

**shaders/test.frag.glsl**

```glsl
#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 inColor;

void main() {
    outColor = vec4(inColor.rgb, 1.0f);
}
```

## macOS (Xcode)

Create a macOS App project:

- Interface: XIB
- Language: Objective-C
- Testing System: none
- Storage: none

Remove the following files from the project:

- AppDelegate.h
- AppDelegate.m
- Assets.xcassets
- main.m
- MainMenu.xib

Get references for the following libraries to use them in the next phase:

VulkanSDK:

- libMoltenVK.dylib
- libSDL2-2.0.0.dylib
- libVkLayer_khronos_validation.dylib
- libvulkan.1.3.290.dylib

bg2 engine:

- libbg2e-native.a

Make a copy of the following files from VulkanSDK to your proyect directory:

- MoltenVK_icd.json
- VkLayer_khronos_validation.json

We will edit them later.

### Build settings

Create a custom build setting varialbe:

- VULKAN_SDK: `$(HOME)/VulkanSDK/1.3.290.0/macOS/` (replace 1.3.290 with the VulkanSDK version)

Set the following build settings:

- Header search paths:
    * $(VULKAN_SDK)/include **recursive**
    * Path to [bg2e-native/include] directory
- Library search paths:
    * $(VULKAN_SDK)/lib **recursive**
    * Path to `libbg2e.a` file
- Use script sandboxing: No

### Build phases

Create and/or configure the following build phases:

**Link binary with libraries:**

- CoreFoundation.framework (system)
- libbg2e-native.a
- libvulkan.1.3.290.dylib
- libSDL2-2.0.0.dylib

**Copy files**: copy libraries to Frameworks bundle directory.

- Destination: `Frameworks`
- Files:
    * libvulkan.1.3.290.dylib
    * libMoltenVK.dylib
    * libSDL2-2.0.0.dylib
    * libVkLayer_khronos_validation.dylib

**Run script script**: create a symbolic link to Vulkan library

```sh
ln -sf ${BUILT_PRODUCTS_DIR}/${FRAMEWORKS_FOLDER_PATH}/libvulkan.1.3.290.dylib ${BUILT_PRODUCTS_DIR}/${FRAMEWORKS_FOLDER_PATH}/libvulkan.1.dylib
```

**Copy files**: copy MoltenVK ICD settings

- Destination: `Resources`
- Subpath: `vulkan/icd.d`
- File: `MoltenVK_icd.json`

**Copy files**: Debug layers configuraton files

- Destination: `Resources`
- Subpath: `vulkan/explicit_layer.d`
- File: `VkLayer_khronos_validation.json`

**Copy files**: copy bg2 engine shaders

- Destination: `Resources`
- Subpath: `shaders`
- Files: All the files inside `shaders/spv` folder.

**Execute script**: compile shaders. Only if your application defines custom shaders.

This script uses the bg2 engine shader compilation script, located in the `shaders/build.sh` folder. It takes as first argument the location of the shaders we want to compile, and as second argument the path where we want to place the compiled shaders. You will have to modify the `INPUT_DIR` path and the path to the `shaders/build.sh` build script. The `OUTPUT_DIR` path will leave the compiled shaders in the `Resources/shaders` directory of the application package, which is the default location where **bg2 engine** looks for shaders.

```sh
INPUT_DIR=${PROJECT_DIR}/shaders
OUTPUT_DIR=${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/shaders

"${PROJECT_DIR}/../../shaders/build.sh" "${INPUT_DIR}" "${OUTPUT_DIR}"
```

### Set up the Vulkan layer configuration file

Edit the `layer/library_path` property in `VkLayer_khronos_validation.json` file. This property is a reference to the `libVkLayer_khronos_validation.dylib` file. By default, this file specify that the library must be placed in the `lib` folder inside the application bundle, but in our case we have put this file in the `Frameworks` directory:

**VkLayer_khronos_validation.json:**

```json
{
    "file_format_version": "1.2.0",
    "layer": {
        "name": "VK_LAYER_KHRONOS_validation",
        "type": "GLOBAL",
        "library_path": "../../../Frameworks/libVkLayer_khronos_validation.dylib",
        ... keep the rest of the file unchanged
}
```

