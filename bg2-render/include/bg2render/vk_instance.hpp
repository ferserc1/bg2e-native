
#ifndef _bg2_render_vk_instance_hpp_
#define _bg2_render_vk_instance_hpp_

#include <string>
#include <vector>
#include <functional>

#include <vulkan/vulkan.h>

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

			void enumerateInstanceExtensionProperties(std::vector<VkExtensionProperties> & ext);

		protected:
			VkInstance _instance = VK_NULL_HANDLE;

			DebugCallback _debugCallback;
			VkDebugUtilsMessengerEXT _debugMessenger;
			bool _destroyDebugCallback = false;

			std::string _appName;
			uint32_t _version[3] = { 1, 0, 0 };
			std::vector<const char*> _requiredExtensions;
			std::vector<const char*> _requiredLayers;

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
