
#include <bg2e/base/light.hpp>

#include <algorithm>

namespace bg2e {
namespace base {

	Light::LightRegistry Light::s_lightRegistry;

	void Light::ActivateLight(Light * light) {
		if (std::find(s_lightRegistry.begin(), s_lightRegistry.end(), light) == s_lightRegistry.end()) {
			s_lightRegistry.push_back(light);
		}
	}

	void Light::DeactivateLight(Light * light) {
		auto it = std::find(s_lightRegistry.begin(), s_lightRegistry.end(), light);
		if (it != s_lightRegistry.end()) {
			s_lightRegistry.erase(it);
		}
	}

	const Light::LightRegistry& Light::ActiveLights() {
		return s_lightRegistry;
	}

	Light::Light(bgfx::ViewId viewId)
		:_viewId(viewId)
	{

	}

	Light::~Light() {
		DeactivateLight(this);
	}

}
}
