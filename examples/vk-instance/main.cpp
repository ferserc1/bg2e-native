

#include <bg2engine.hpp>

#include <iostream>
#include <array>
#include <chrono>
#include <thread>

class MyWindowDelegate : public bg2wnd::WindowDelegate {
public:
	MyWindowDelegate() {}

	void init() {
        // Simplified method: creates an instance with debug messages, if compiled in DEBUG mode
        // _instance = std::shared_ptr<bg2render::vk::Instance>(bg2render::vk::Instance::CreateDefault(window(), "bg2 vulkan test"));

        // Manual method: this sequence does the same as the simplified method
        _instance = std::make_shared<bg2render::vk::Instance>();

        _instance->configureAppName("bg2 render vulkan instance example");
        #ifdef _DEBUG

            _instance->setDebugCallback([](
                VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                VkDebugUtilsMessageTypeFlagsEXT type,
                const VkDebugUtilsMessengerCallbackDataEXT* pData) {
                    if (severity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
                        return;
                    }

                    if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                        std::cerr << pData->pMessage << std::endl;
                    }
                    else {
                        std::cout << pData->pMessage << std::endl;
                    }
                });

            std::vector<VkExtensionProperties> extensionData;
            _instance->enumerateInstanceExtensionProperties(extensionData);
            std::cout << "Available Vulkan instance extensions: " << std::endl;
            for (const auto & ext : extensionData) {
                std::cout << "\t" << ext.extensionName << std::endl;
            }
        #endif

        std::vector<const char*> extensions;
        window()->getVulkanRequiredInstanceExtensions(extensions);
        _instance->configureRequiredExtensions(extensions);
        _instance->create();

        _instance->setSurface(window()->createVulkanSurface(_instance->instance()));

        _instance->choosePhysicalDevices();
    }

    void resize(const bg2math::int2 & size) {}

    void update(float delta) {}

    void draw() {}

    void cleanup() {
        std::cout << window()->title() << ": Cleanup" << std::endl;
        _instance = nullptr;
    }

    void keyUp(const bg2wnd::KeyboardEvent & e) {
        if (e.keyCode() == bg2wnd::KeyCode::KeyESCAPE) {
            window()->close();
        }
    }

protected:
    std::shared_ptr<bg2render::vk::Instance> _instance;

};

int main(int argc, char ** argv) {
	bg2wnd::Application * app = bg2wnd::Application::Get();

    auto window = bg2wnd::Window::New();
    window->setWindowDelegate(new MyWindowDelegate());
    window->setTitle("vulkan instance");
    window->setPosition({ 40, 40, });
    window->setSize({ 800, 600 });
    app->addWindow(window);

    return app->run();
}
