
#include <bg2render/pipeline.hpp>

#include <stdexcept>

namespace bg2render {

	Pipeline::Pipeline(vk::Instance* instance)
		:_instance(instance)
	{

	}

	Pipeline::~Pipeline() {
		for (auto& shaderData : _shaderStagesData) {
			vkDestroyShaderModule(_instance->renderDevice()->device(), shaderData.module, nullptr);
		}
		_shaderStagesData.clear();
		_shaderStages.clear();
	}

	void Pipeline::addShader(const std::vector<char>& buffer, VkShaderStageFlagBits type, const std::string& mainFunction) {
		VkShaderModule shaderModule;
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = buffer.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

		if (vkCreateShaderModule(_instance->renderDevice()->device(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Error creating shader module");
		}

		ShaderData data;
		data.module = shaderModule;
		data.mainFunction = std::vector<char>(mainFunction.c_str(), mainFunction.c_str() + mainFunction.size() + 1);
		_shaderStagesData.push_back(data);
		auto& dataPtr = _shaderStagesData.back();
	
		_shaderStages.push_back({
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr,		// pNext
			0,	// flags
			type,
			shaderModule,
			dataPtr.mainFunction.data(),
			nullptr	// pSpecializationInfo
		});
	}

}
