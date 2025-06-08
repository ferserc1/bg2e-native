## macOS App Bundle

Packaging macOS applications with Xcode is a somewhat tedious process when using external Apple libraries. For one thing, we don't have a system-wide environment variable to indicate where we have installed the Vulkan SDK. For another, the normal way to indicate which libraries we want to link depends on the GUI, and to do that, we have to include a reference to the Vulkan SDK in the project. If we change the SDK version, those references are no longer valid: we have to correct them and reconfigure the libraries again. Finally, the linked libraries have to be copied to the application package, and have to be digitally signed to prevent a potential hacker from replacing one of these libraries with a modified version with malicious code. In addition to this, there are a number of files that have to be copied to the application package from the Vulkan SDK, in specific directories. For more information on this topic, please refer to the Vulkan SDK documentation on how to package applications on macOS.

[Gettin started with the macOS Vulkan SDK](https://vulkan.lunarg.com/doc/sdk/1.4.313.1/mac/getting_started.html)

Fortunately, I've prepared a script along with some simple setup instructions so you don't have to worry about any of this. In this section I'm going to show you how to set up an Xcode for macOS project from scratch with bg2-engine.

### Prerequisites

You must compile the `bg2e-native.a` library, either using the Xcode project, or using the `build_macos.sh` script. README.md file for more information on how to configure and build the engine.

Before you start creating your project, make sure you have your developer identity set up. To do this, you need to add your Apple developer account in the Xcode settings under Accounts. A free account is fine. By doing this, you will have all the digital signature part of the project set up without having to do any extra steps.

## Summary of steps

If you are familiar with Xcode, here is a summary list of the steps you need to do for the setup. If not, continue reading from the **Step 1: create the project** section.

1. Create a macOS App project, Objective-C. Rename the `main.m` file to `main.cpp`, and remove the rest of the files, except the entitlments and assets file. Check the **Step 1:create the project** section to get the full source of the example and paste it in the `main.cpp` file.
2. Copy the `assets`, `scripts`, `shaders` and `include` folder to the project WITHOUT checking the target. Copy them as folders, not groups. Also copy the `bin/Release/libbg2e-native.a` directly into the project, this time CHECKING the target, to add it in link library build phase.
3. Create the following copy files build phases:
    * Copy Assets - Directory: Resources, subpath: assets, files: `country_field_sun.jpg`, `two_submeshes.obj`, `two_submeshes_inner_albedo.jpg` and `two_submeshes_outer_albedo.jpg`
    * Copy Shaders - Directory: Resources, subpath: shaders, files: all files inside `shaders/spv` folder.
4. Add the `CoreFoundation.framework` framework to the `Link Binary With Libraries` phase. You should already have the `libbg2e-native.a` library there.
5. Create a run script build phase with the following script:

```sh
APP_BUNDLE=${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app
"${PROJECT_DIR}/scripts/package_vulkan_resources.sh" "${APP_BUNDLE}" ${CODE_SIGN_IDENTITY}
```

6. In target Build Settings, configure the following options:
    * Header Search Paths:
        + "$(PROJECT_DIR)/include"
        + $(VULKAN_SDK)include > recursive
    * Library Search Paths:
        + "$(PROJECT_DIR)"
        + $(VULKAN_SDK)/lib
    * Other Linker Flags:
        + `-lvulkan.$(VULKAN_LIB_VERSION)`
        + `-lSDL2-2.0.0`
7. Add the following user-defined settings:
    * `VULKAN_SDK`: `$(HOME)/VulkanSDK/1.4.313.1/macOS` > replace with your VulkanSDK version
    * `VULKAN_LIB_VERSION`: `1.4.313` > the version of the SDK, without the last digit
8. In the project Build Settings, disable the `User Script Sandboxing` option.

### Step 1: create the project

The first step is to create the project. Create a new Xcode project, of type **macOS** > **App**. You can use any name you want. In the application options, select the Objective-C language, don't select anything for Testing System or Storage, and as for Interface, it doesn't matter what you configure, because we are not going to use any of those. We will now add the initial configuration of the project:

- Delete all files in the project except the `*.entitlements` file, the `Assets.xcassets` file (we're not going to use it, but you can leave it because this is where you can later configure the application icon) and the `main.m` file.
- Rename the main code file: `main.m` => `main.cpp`
- Replace the `main.cpp` code with this:

**main.cpp**

```cpp
#include <bg2e.hpp>

class RotateCameraComponent : public bg2e::scene::Component {
public:
    void update(float delta) override
    {
        auto transform = ownerNode()->transform();
        
        if (transform)
        {
            transform->rotate(0.02f * delta / 10.0f, 0.0f, 1.0f, 0.0f);
        }
    }
};

class BasicSceneDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>,
	public bg2e::app::InputDelegate,
	public bg2e::ui::UserInterfaceDelegate
{
public:
 
	void init(bg2e::render::Engine*, bg2e::ui::UserInterface*) override {
		_window.setTitle("Basic forward renderer");
		_window.options.noClose = true;
		_window.options.minWidth = 190;
		_window.options.minHeight = 90;
		_window.setPosition(0, 0);
		_window.setSize(200, 100);
	}

	void drawUI() override
	{
        auto drawSkybox = renderer()->drawSkybox();
        _window.draw([&]() {
            bg2e::ui::BasicWidgets::checkBox("Draw Skybox", &drawSkybox);
        });
        renderer()->setDrawSkybox(drawSkybox);
	}

protected:
	bg2e::ui::Window _window;
    
    std::shared_ptr<bg2e::scene::Node> createScene() override
    {
        auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
        sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "country_field_sun.jpg"));
        
        auto anotherNode = new bg2e::scene::Node("Transform Node");
        anotherNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 1.0f, 0.0f));
        sceneRoot->addChild(anotherNode);
        
        auto drawable = std::shared_ptr<bg2e::scene::DrawableBase>(loadDrawable());
        auto drawableComponent = std::make_shared<bg2e::scene::DrawableComponent>(drawable);
        auto modelNode = std::make_shared<bg2e::scene::Node>("3D Model");
        modelNode->addComponent(drawableComponent);
        modelNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(2.0f, 0.0f, 0.0f));
        anotherNode->addChild(modelNode);
        
        auto secondModel = new bg2e::scene::Node("Second 3D model");
        auto anotherDrawable = new bg2e::scene::DrawableComponent(drawable);

        secondModel->addComponent(anotherDrawable);
        secondModel->addComponent(bg2e::scene::TransformComponent::makeTranslated(-2.0f, 0.0f, 0.0f ));
        sceneRoot->addChild(secondModel);
        
        auto cameraNode = std::shared_ptr<bg2e::scene::Node>(new bg2e::scene::Node("Camera"));
        cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 1.3f, -10.0f ));
        cameraNode->addComponent(new bg2e::scene::CameraComponent());
        auto projection = new bg2e::math::OpticalProjection();
        cameraNode->camera()->setProjection(projection);
        
        auto cameraRotation = new bg2e::scene::Node("Camera Rotation");
        cameraRotation->addComponent(new bg2e::scene::TransformComponent());
        cameraRotation->addComponent(new RotateCameraComponent());
        cameraRotation->addChild(cameraNode);
        sceneRoot->addChild(cameraRotation);
        
        return sceneRoot;
    }

	bg2e::scene::DrawableBase * loadDrawable()
	{  
        std::filesystem::path modelPath = bg2e::base::PlatformTools::assetPath();
        modelPath.append("two_submeshes.obj");
        
        auto innerAlbedoTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_inner_albedo.jpg"
        );
        
        auto outerAlbedoTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_outer_albedo.jpg"
        );
        
        auto drawable = new bg2e::scene::Drawable();
        drawable->setMesh(bg2e::db::loadMeshObj<bg2e::geo::Mesh>(modelPath));
        drawable->material(0).setAlbedo(outerAlbedoTexture);
        drawable->material(1).setAlbedo(innerAlbedoTexture);
        drawable->load(_engine);
        
        return drawable;
	}
};

class MyApplication : public bg2e::app::Application {
public:
	void init(int argc, char** argv) override
	{
		auto delegate = std::make_shared<BasicSceneDelegate>();
		setRenderDelegate(delegate);
		setInputDelegate(delegate);
		setUiDelegate(delegate);
	}
};

int main(int argc, char** argv) {
	bg2e::app::MainLoop mainLoop;
	MyApplication app;
	app.init(argc, argv);
	return mainLoop.run(&app);
}
```


### Step 2: Project resources

You will need some resources from the `assets` directory, plus the `scripts` directory, the `shaders` directory and the `libbg2e-native.a` file inside the `bin/Release` or `bin/Debug` directory (depending on whether you want to link to the debug version or the development version). You will also need the graphics engine header files, which are in the `include` folder.

- Drag the `assets` folder, from the `bg2e-native` repository to the project root in the Project Navigator tab, inside Xcode. Select **Action**: **Copy files to destination** and **Groups**: **Create folders**, and uncheck the target: we'll decide what resources to copy later.
- Repeat the same action with the `scripts`, `shaders` and `include` directories, selecting the same options.
- Copy the library from `bin/Release/libbg2e-native.a` to the Project Navigator. If you don't have the `bin/Release` directory, execute the `build_macos.sh` script. This script will generate the release version of the library. Select **Action**: **Copy files to destination**, and **CHECK** the target to link the executable with the bg2 engine librarys.

After completing these steps, your project directory will contain the following files:

```
My App
 |- assets
 |- include
 |- libbg2e-native.a
 |- My App
 |   |- Assets.xcassets
 |   |- Base.lproj
 |   |- main.cpp
 |   \- My_App.entitlements
 |- My App.xcodeproj
 |- scripts
 \- shaders
```

### Step 3: Project configuration

We now have all the files needed to configure the project. Now let's complete the configuration.

**Assets**

If you examine the example code, you will see that we will need the following files from the `assets` folder:

- `country_field_sun.jpg`
- `two_submeshes.obj`
- `two_submeshes_inner_albedo.jpg`
- `two_submeshes_outer_albedo.jpg`

Select the project in the Project Navigator, and in the properties panel, select the application target. In the **Build Phases** tab, click on the `+` button in the header of the panel, and select **New Copy Files Phase**. Click on the new phase you have added to rename it, and type `Copy Assets`. Expand the new phase, and select **Resources** for the **Destination** property. In the **Subpath** field, type **assets**. Then, click the `+` sign under the emtpy list of files, and select the above files from the `assets` folder. You can select more than one file pressing the command key.

**Shaders**

Within the same **Build Phases** tab, select the `+` button to the left of the tab header, and click **New Copy Files Phase**. A new phase will be added with the name **Copy Files**, which you can rename by double-clicking on it. Name it **Copy Shaders** and expand it.

In the options, make sure that **Resources** is selected in **Destination**. In **Subpath** type `shaders`. Add to the list of files all files inside the `shaders/spv` folder (no need to add any subdirectories, just files with a .spv extension).

**Libraries**

In the `Link Binary With Libraries` section you should have a single item: `libbg2e-native.a`. If you don't have it, it's because you didn't mark the target when you copied the file to the project, in that case add that file in the same way you added the assets and shaders.

In addition to this library, we need to add another system library. Click on the `+` button, find the `CoreFoundation.framework` library, select it and add it with the **Add** button.

**Vulkan Resources**

This is the most complex part of the configuration, which is why we have included the `scripts` directory in the project. The `package_vulkan_resources.sh` script will take care of adding all the files and libraries within the vulkan SDK to the application package, and digitally sign them with the identity we have configured.

Add a new build phase, in the same way as you added the file copy phases, but this time select **New Run Script Phase**. Change the name to **Package Vulkan resources**.

Expand the new compilation phase and enter the following script:

```sh
APP_BUNDLE=${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app
"${PROJECT_DIR}/scripts/package_vulkan_resources.sh" "${APP_BUNDLE}" ${CODE_SIGN_IDENTITY}
```

## Step 4: Configure target settings

Now we going to configure the target build settings. In the project settings panel, the same panel where you have configured the build phases, select the **Build Settings** tab. Within this tab, select the option `All`.

Using the search text box, search for the text `search path`. Locate the **Header Search Paths** and **Library Search Paths** options and configure them as follows:

**Header Search Paths:**

- "$(PROJECT_DIR)/include"   - non-recursive
- $(VULKAN_SDK)/include      - recursive

**Library Search Paths:**

- $(inherited)        - non-recursive
- "$(PROJECT_DIR)"    - non-recursive
- $(VULKAN_SDK)/lib   - non-recursive

Now search the `Other Linker Flags` option and set the following properties:

- `-lvulkan.$(VULKAN_LIB_VERSION)`
- `-lSDL2-2.0.0`

## Step 5: Configure the VULKAN_SDK variables

In macOS there is no concept of an environment variable for desktop applications, so Xcode does not know which version of Vulkan we have installed, nor the location of the SDK files. In the above configuration we have used the `VULKAN_SDK` and `VULKAN_LIB_VERSION` variables, which do not exist in Xcode, but the good thing is that we can define them manually in the project.

In the same project properties panel, select the project on the left side of the view (so far all the configuration has been done in the target). The project should have the same name as the target, but it will be at the top, under the title **PROJECT**. When we switch to the project, we will see that we only have three tabs: **Info**, **Build Settings** and **Package Dependencies**. If you have more options, you have probably selected the target instead of the project.

Select the **Build Settings** tab. At the bottom, to the left of **Basic**, there is a `+` button with which we can add new variables. Click on that button and create the variable `VULKAN_SDK` by selecting the `Add User-Defined Setting` option. As a value, enter the path to the Vulkan SDK folder. This will depend on the version of the SDK you have installed and the location you have chosen in the installation. By default it is installed in the user's home folder. Let's assume you have installed version `1.4.313.1`, in which case the configuration value would be as follows:

- `VULKAN_SDK`: `$(HOME)/VulkanSDK/1.4.313.1/macOS`

Repeat the previous step to create the `VULKAN_LIB_VERSION` variable. In this case, the value of the variable will be the version number of the vulkan SDK, with the last digit removed. That is, for SDK version `1.4.313.1`, the library version will be `1.4.313`:

- `VULKAN_LIB_VERSION`: `1.4.313`

To check that we have set the variables correctly, select the target properties again. Look for the `Header Search Paths` and `Library Search Paths` options. If all went well, you will see the full paths to the Vulkan SDK include and lib directories appear (will depend on the name of your user account on your Mac, in my case the name is `fernando`.)

- `Header Search Paths`: `/Users/fernando/VulkanSDK/1.4.313.1/macOS/include/**`
- `Library Search Paths`: `/Users/fernando/VulkanSDK/1.4.313.1/macOS/lib`

You can also check if the library version is set correctly by looking for the `Other Linker Flags` option:

- `Other Linker Flags`: `-lSDL2-2.0.0 -lvulkan.1.4.313`

If you don't see the version number and the path to the Vulkan SDK, check all the previous configuration. It is important that these variables are correct and with exactly these names, as they will also be used in the script to copy the Vulkan resources.

## Step 6: Script sandboxing

By default, Xcode has security restrictions on script execution. In order for the script that collects Vulkan resources and signs libraries to work, we need to disable this security mechanism. In the project options, under the **Build Settings** tab look for the `User Script Sandboxing` option and change the value to `No`.
