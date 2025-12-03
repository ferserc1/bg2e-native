
#include <bg2e.hpp>
#include <numbers>

#include <DevSceneDelegate.hpp>
#include <SpheresSceneDelegate.hpp>
#include <LoadGltfSceneDelegate.hpp>

class MyApplication : public bg2e::app::Application {
public:
	void init(int, char**) override
	{
		//auto delegate = std::make_shared<SpheresSceneDelegate>();
        //auto delegate = std::make_shared<DevSceneDelegate>();
		auto delegate = std::make_shared<LoadGltfSceneDelegate>();
		setRenderDelegate(delegate);
		setInputDelegate(delegate);
		setUiDelegate(delegate);

        // Test app settings path
        auto settingsPath = bg2e::base::PlatformTools::settingsPath();
        std::cout << "Application settings path: " << settingsPath << std::endl;
        
        // Test application preferences
        {
            bg2e::app::Preferences appPrefs;
        
            appPrefs.set("testNumber", 10.4f);
            appPrefs.set("testString", "Hello, World!");
        }
        
        // Test applicaiton preferences load
        {
            bg2e::app::Preferences appPrefs;
            
            auto testNumber = appPrefs.get("testNumber", 0.0f);
            auto testString = appPrefs.get("testString", "");
            
            std::cout << "Test number: " << testNumber << std::endl;
            std::cout << "Test string: " << testString << std::endl;
        }
        
        // PreferencesStore: recommended method
        auto & prefs = bg2e::app::PreferencesStore::instance().preferences();
        
        auto testNumber2 = prefs.get("testNumber2", 0);
        
        std::cout << "testNumber2: " << testNumber2 << std::endl;
        
        prefs.set("testNumber2", 4);
        
        std::cout << "testNumber2: " << testNumber2 << std::endl;
        
        auto vecValue = prefs.get("testVector", glm::vec3{ 0.0f });
        
        std::cout << "vecValue: " << vecValue.x << ", " << vecValue.y << ", " << vecValue.z << std::endl;
        
        prefs.set("testVector", glm::vec3{ 1.0f, 2.0f, 3.0f });
        
        vecValue = prefs.get("testVector", glm::vec3{ 0.0f });
        
        std::cout << "vecValue: " << vecValue.x << ", " << vecValue.y << ", " << vecValue.z << std::endl;
	}
};

int main(int argc, char** argv) {

    bg2e::app::GPUSelectionDialog gpuSelection("bg2eExampleApp");
    
    auto result = gpuSelection.run();
    
    if (result.get())
    {
        std::cout << "Choosed device: " << result->name << std::endl;
    }
    
	bg2e::app::MainLoop mainLoop("bg2eExampleApp");
	MyApplication app;
	app.init(argc, argv);
	return mainLoop.run(&app);
}

