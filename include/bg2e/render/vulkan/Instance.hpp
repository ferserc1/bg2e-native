#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>

#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_API Instance {
public:
	Instance();

	inline void enableValidationLayers(bool value) { _enableValidationLayers = value; }
	inline bool isValidationLayersEnabled() const { return _enableValidationLayers; }
	inline void setApplicationName(const std::string& name) { _applicationName = name; }
	inline const std::string& applicationName() const { return _applicationName; }

	void create();

	inline VkInstance instance() const { return _instance; }

protected:
	VkInstance _instance = VK_NULL_HANDLE;

	bool _enableValidationLayers = false;
	std::string _applicationName = "bg2 engine Vulkan Application";

	std::vector<std::string> _availableInstanceExtensions;

	bool checkValidationLayerSupport();
};

}
}
}