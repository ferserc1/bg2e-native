

#include <bg2wnd/application.hpp>
#include <bg2wnd/window.hpp>
#include <bg2wnd/window_delegate.hpp>
#include <bg2render/vk_instance.hpp>
#include <bg2render/vk_device.hpp>
#include <bg2render/swap_chain.hpp>


#include <iostream>

#include <vulkan/vulkan.h>

#include <string.h>

class MyDelegate : public bg2wnd::WindowDelegate {
public:
    MyDelegate(bool enableValidation) :_enableValidationLayers(enableValidation) {}

    void init() {
		_instance = std::unique_ptr<bg2render::vk::Instance>(new bg2render::vk::Instance());
		_instance->configureAppName("bg2e vulkan test");
		if (_enableValidationLayers) {
			_instance->setDebugCallback([](
				VkDebugUtilsMessageSeverityFlagBitsEXT severity,
				VkDebugUtilsMessageTypeFlagsEXT type,
				const VkDebugUtilsMessengerCallbackDataEXT * pData) {
					if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
						std::cout << pData->pMessage << std::endl;
					}
				});
		}

		std::vector<VkExtensionProperties> extensionData;
		_instance->enumerateInstanceExtensionProperties(extensionData);
		std::cout << "Available extensions:" << std::endl;
		for (const auto& ext : extensionData) {
			std::cout << "\t" << ext.extensionName << std::endl;
		}

		std::vector<const char*> extensions;
		window()->getVulkanRequiredInstanceExtensions(extensions);
		_instance->configureRequiredExtensions(extensions);

		_instance->create();

		// It's important to link the window surface to vulkan instance BEFORE
		// choose physical device, if you want to get support for presentation queues
		_instance->setSurface(window()->createVulkanSurface(_instance->instance()));

		_instance->choosePhysicalDevices();

		auto queue = _instance->renderQueue();
		auto presentQueue = _instance->presentQueue();
        
		_swapChain = std::unique_ptr<bg2render::SwapChain>(new bg2render::SwapChain(
			_instance->renderPhysicalDevice(), 
			_instance->renderDevice(), 
			_instance->surface()));
		_swapChain->create(window()->size());


		std::cout << "Done" << std::endl;


    }

    void resize(const bg2math::int2 & size) {
        std::cout << "resize: " << size.x() << ", " << size.y() << std::endl;
    }

    void update(float delta) {
       // std::cout << "udpate: " << delta << std::endl;
    }

    void draw() {
       // std::cout << "draw" << std::endl;
    }

    void cleanup() {
		_swapChain = nullptr;
		_instance = nullptr;
    }

    void keyUp(const bg2wnd::KeyboardEvent & e) {}
    void keyDown(const bg2wnd::KeyboardEvent & e) {}
    void mouseMove(const bg2wnd::MouseEvent & e) {}
    void mouseDown(const bg2wnd::MouseEvent & e) {}
    void mouseUp(const bg2wnd::MouseEvent & e) {}
    void mouseWheel(const bg2wnd::MouseEvent & e) {}

private:
	std::unique_ptr<bg2render::vk::Instance> _instance;
	std::unique_ptr<bg2render::SwapChain> _swapChain;

	bool _enableValidationLayers;
};

int main(int argc, char ** argv) {
    bg2wnd::Application app;

    auto window = bg2wnd::Window::Create();
    window->setWindowDelegate(new MyDelegate(true));
    app.addWindow(window);
    app.run();

    return 0;
}
