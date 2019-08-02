
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
		if (_pipelineLayout != VK_NULL_HANDLE && _destroyLayout) {
			vkDestroyPipelineLayout(_instance->renderDevice()->device(), _pipelineLayout, nullptr);
		}
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

	void Pipeline::addColorBlendAttachment(const VkPipelineColorBlendAttachmentState& att) {
		_colorBlendAttachments.push_back({
				att.blendEnable,
				att.srcColorBlendFactor,
				att.dstColorBlendFactor,
				att.colorBlendOp,
				att.srcAlphaBlendFactor,
				att.dstAlphaBlendFactor,
				att.alphaBlendOp,
				att.colorWriteMask
			});
		_colorBlendInfo.attachmentCount = static_cast<uint32_t>(_colorBlendAttachments.size());
		_colorBlendInfo.pAttachments = _colorBlendAttachments.data();
	}

	void Pipeline::loadDefaultBlendAttachments() {
		addColorBlendAttachment({
			VK_FALSE,				// blendEnable;
			VK_BLEND_FACTOR_ONE,	// srcColorBlendFactor;
			VK_BLEND_FACTOR_ZERO,	// dstColorBlendFactor;
			VK_BLEND_OP_ADD,		// colorBlendOp;
			VK_BLEND_FACTOR_ONE,		// srcAlphaBlendFactor;
			VK_BLEND_FACTOR_ZERO,	// dstAlphaBlendFactor;
			VK_BLEND_OP_ADD,	// alphaBlendOp;
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT	// colorWriteMask;
			});
	}

	void Pipeline::createDefaultLayout() {
		if (_destroyLayout && _pipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(_instance->renderDevice()->device(), _pipelineLayout, nullptr);
		}

		VkPipelineLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.setLayoutCount = 0;
		createInfo.pSetLayouts = nullptr;
		createInfo.pushConstantRangeCount = 0;
		createInfo.pPushConstantRanges = nullptr;
		if (vkCreatePipelineLayout(_instance->renderDevice()->device(), &createInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Error creating pipeline layout");
		}
		_destroyLayout = true;
	}

	void Pipeline::setPipelineLayout(VkPipelineLayout lo) {
		if (_destroyLayout && _pipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(_instance->renderDevice()->device(), _pipelineLayout, nullptr);
		}
		_pipelineLayout = lo;
		_destroyLayout = false;
	}
}
