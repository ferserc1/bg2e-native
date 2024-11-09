#include <bg2e/render/vulkan/Instance.hpp>
#include <bg2e/base/Log.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

Instance::Instance()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

	bg2e_log_debug << "Available Vulkan extensions:" << bg2e_log_end;;
	for (const auto& extension : availableExtensions) {
		bg2e_log_debug << "\t" << extension.extensionName << bg2e_log_end;
		_availableInstanceExtensions.push_back(extension.extensionName);
	}
}

void Instance::create()
{

}

bool Instance::checkValidationLayerSupport()
{
	return false;
}

}
}
}
