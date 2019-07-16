
#include <bg2render/vk_instance.hpp>

#include <bg2e-config.hpp>

#include <stdexcept>

namespace bg2render {
	namespace vk {

		Instance::Instance()
		{

		}

		Instance::~Instance() {
			if (_instance != VK_NULL_HANDLE) {
				vkDestroyInstance(_instance, nullptr);
			}
		}

		void Instance::configureRequiredExtensions(const std::vector<const char*>& ext) {
			for (auto& e : ext) {
				_requiredExtensions.push_back(e);
			}
		}

		void Instance::create() {
			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pApplicationName = _appName.c_str();
			appInfo.applicationVersion = VK_MAKE_VERSION(_version[0], _version[1], _version[2]);
			appInfo.pEngineName = "bg2 engine";
			appInfo.engineVersion = VK_MAKE_VERSION(bg2e_MAJOR_VERSION, bg2e_MINOR_VERSION, bg2e_REV_VERSION);
			appInfo.apiVersion = VK_API_VERSION_1_0;

			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(_requiredExtensions.size());
			if (_requiredExtensions.size() > 0) {
				createInfo.ppEnabledExtensionNames = _requiredExtensions.data();
			}

			// TODO: layers
			createInfo.enabledLayerCount = 0;

			if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
				throw std::runtime_error("Could not create Vulkan instance");
			}
		}

		void Instance::enumerateInstanceExtensionProperties(std::vector<VkExtensionProperties>& ext) {
			uint32_t extensionCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
			ext.resize(extensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, ext.data());
		}
	}
}