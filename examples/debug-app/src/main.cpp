/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <bg2e.hpp>
#include <numbers>

#include <DevSceneDelegate.hpp>
#include <SpheresSceneDelegate.hpp>
#include <LoadGltfSceneDelegate.hpp>
#include <PickSelectionDelegate.hpp>

class MyApplication : public bg2e::app::Application {
public:
	void init(int, char**) override
	{
		auto delegate = std::make_shared<SpheresSceneDelegate>();
        //auto delegate = std::make_shared<DevSceneDelegate>();
		//auto delegate = std::make_shared<LoadGltfSceneDelegate>();
	    //auto delegate = std::make_shared<PickSelectionDelegate>();
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

int main(int argc, char** argv)
{
    bg2e::app::MainLoop mainLoop("bg2eExampleApp");

    // bg2e::app::GPUSelectionDialog gpuSelection("bg2eExampleApp");
    //
    // auto result = gpuSelection.run();
    //
    // if (result.get())
    // {
    //     std::cout << "Choosed device: " << result->name << std::endl;
    // }
    

	MyApplication app;
	app.init(argc, argv);
    mainLoop.initWindowConfig({
        .persistentSize = true
    });
	return mainLoop.run(&app);
}

