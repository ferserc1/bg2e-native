
#ifndef _bg2_render_vk_instance_hpp_
#define _bg2_render_vk_instance_hpp_

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace bg2render {

	namespace vk {
		class Instance {
		public:
			Instance();
			virtual ~Instance();

			inline void configureAppName(const std::string& name) { _appName = name; }
			inline void configureAppVersion(int major, int minor, int rev) { _version[0] = major; _version[1] = minor; _version[2] = rev; }
			void configureRequiredExtensions(const std::vector<const char*>& ext);

			void create();

			inline VkInstance instance() const { return _instance; }

			void enumerateInstanceExtensionProperties(std::vector<VkExtensionProperties> & ext);

		protected:
			VkInstance _instance = VK_NULL_HANDLE;

			std::string _appName;
			uint32_t _version[3] = { 1, 0, 0 };
			std::vector<const char*> _requiredExtensions;

		};
	}
}

#endif
