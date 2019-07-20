
#ifndef _bg2_render_vk_instance_hpp_
#define _bg2_render_vk_instance_hpp_

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include <vulkan/vulkan.h>

#include <bg2render/vk_definitions.hpp>
#include <bg2render/vk_physical_device.hpp>
#include <bg2render/vk_device.hpp>

namespace bg2render {

	namespace vk {

		class Instance {
		public:
			typedef std::function<void (VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*)> DebugCallback;
			Instance();
			virtual ~Instance();

			inline void setDebugCallback(DebugCallback callback) { _debugCallback = callback; }

			inline void configureAppName(const std::string& name) { _appName = name; }
			inline void configureAppVersion(int major, int minor, int rev) { _version[0] = major; _version[1] = minor; _version[2] = rev; }
			void configureRequiredExtensions(const std::vector<const char*>& ext);
			void configureRequiredLayers(const std::vector<const char*>& layers);

			void create();

			inline VkInstance instance() const { return _instance; }
			inline PhysicalDevice* renderPhysicalDevice() { return _renderPhysicalDevice.get(); }
			inline const PhysicalDevice* renderPhysicalDevice() const { return _renderPhysicalDevice.get(); }
			inline Device* renderDevice() { return _renderDevice.get(); }
			inline const Device* renderDevice() const { return _renderDevice.get(); }
			inline VkQueue renderQueue() const { return _renderDevice->graphicsQueue(); }


			void enumerateInstanceExtensionProperties(std::vector<VkExtensionProperties> & ext);

			// Option 1: automatically choose the best physical devices
			void choosePhysicalDevices();

			// Option 2: (TODO) create a new APi to manually choose a custom physical device for each task
			void enumeratePhysicalDevices(std::vector<std::shared_ptr<PhysicalDevice>>& devices);
			bool isDeviceSuitableForTask(const PhysicalDevice * dev, DeviceTask task);

		protected:
			VkInstance _instance = VK_NULL_HANDLE;

			DebugCallback _debugCallback;
			VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
			bool _destroyDebugCallback = false;

			std::string _appName;
			uint32_t _version[3] = { 1, 0, 0 };
			std::vector<const char*> _requiredExtensions;
			std::vector<const char*> _requiredLayers;

			// Devices
			std::shared_ptr<PhysicalDevice> _renderPhysicalDevice;
			std::shared_ptr<Device> _renderDevice;
			

			// Validation and debug
			static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageType,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* userData)
			{
				auto instance = reinterpret_cast<Instance*>(userData);
				if (instance && instance->_debugCallback) {
					instance->_debugCallback(messageSeverity, messageType, pCallbackData);
				}
				return VK_FALSE;
			}

			bool checkValidationLayerSupport();
			VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
			void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
		};
	}
}

#endif
