
#include <bg2render/vk_sampler.hpp>

#include <stdexcept>

namespace bg2render {
	namespace vk {

		Sampler::Sampler(Instance* inst)
			:_instance(inst)
		{

		}

		Sampler::~Sampler() {
			if (_sampler != VK_NULL_HANDLE) {
				vkDestroySampler(_instance->renderDevice()->device(), _sampler, nullptr);
			}
		}

		void Sampler::create() {
			if (vkCreateSampler(_instance->renderDevice()->device(), &_createInfo, nullptr, &_sampler) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create sampler");
			}
		}
	}
}