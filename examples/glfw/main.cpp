

#include <bg2wnd/application.hpp>
#include <bg2wnd/window.hpp>
#include <bg2wnd/window_delegate.hpp>
#include <bg2render/vk_instance.hpp>
#include <iostream>

#include <vulkan/vulkan.h>

#include <string.h>

class MyDelegate : public bg2wnd::WindowDelegate {
public:
    MyDelegate(bool enableValidation) :_enableValidationLayers(enableValidation) {}

    void init() {
		_instance = std::unique_ptr<bg2render::vk::Instance>(new bg2render::vk::Instance());
		_instance->configureAppName("bg2e vulkan test");

		std::vector<const char*> extensions;
		window()->getVulkanRequiredInstanceExtensions(extensions);
		_instance->configureRequiredExtensions(extensions);

		std::vector<VkExtensionProperties> extensionData;
		_instance->enumerateInstanceExtensionProperties(extensionData);
		std::cout << "Available extensions:" << std::endl;
		for (const auto& ext : extensionData) {
			std::cout << "\t" << ext.extensionName << std::endl;
		}

		if (_enableValidationLayers) {
            if (!checkValidationLayerSupport()) {
                std::runtime_error("Validation layers requested, but not available.");
            }


		}

		_instance->create();
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
        std::cout << "cleanup" << std::endl;
    }

    void keyUp(const bg2wnd::KeyboardEvent & e) {
        if (e.shiftEnabled() && e.keyCode() == bg2wnd::KeyboardEvent::KeyE) {
            std::cout << "Shift + E up" << std::endl;
        }
    }

    void keyDown(const bg2wnd::KeyboardEvent & e) {
        if (e.shiftEnabled() && e.keyCode() == bg2wnd::KeyboardEvent::KeyE) {
            std::cout << "Shift + E down" << std::endl;
        }
    }


    void mouseMove(const bg2wnd::MouseEvent & e) {
        std::cout << "Mouse move, x=" << e.posX() << ", y=" << e.posY() << std::endl;
    }

    void mouseDown(const bg2wnd::MouseEvent & e) {
        std::cout << "MouseDown: B1=" << e.button(1) <<
            ", B2=" << e.button(2) <<
            ", B3=" << e.button(3) << std::endl;
    }

    void mouseUp(const bg2wnd::MouseEvent & e) {
        std::cout << "MouseUp: B1=" << e.button(1) <<
            ", B2=" << e.button(2) <<
            ", B3=" << e.button(3) << std::endl;
    }

    void mouseWheel(const bg2wnd::MouseEvent & e) {
        std::cout << "Mouse wheel: x=" << e.wheelDeltaX() << ", y=" <<
            e.wheelDeltaY() << std::endl;
    }

private:
	std::unique_ptr<bg2render::vk::Instance> _instance;
	const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
	};
	bool _enableValidationLayers;

	bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount,availableLayers.data());

        for (const char * layerName : validationLayers) {
            bool layerFound = false;
            for (const auto & layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }
        return true;
	}
};

int main(int argc, char ** argv) {
    bg2wnd::Application app;

    auto window = bg2wnd::Window::Create();
    window->setWindowDelegate(new MyDelegate(true));
    app.addWindow(window);
    app.run();

    return 0;
}
