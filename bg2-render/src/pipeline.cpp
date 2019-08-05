
#include <bg2render/pipeline.hpp>

#include <stdexcept>

namespace bg2render {

	Pipeline::Pipeline(vk::Instance* instance)
		:_instance(instance)
	{

	}

	Pipeline::~Pipeline() {
		if (_pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(_instance->renderDevice()->device(), _pipeline, nullptr);
		}
		_renderPass = nullptr;
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

	void Pipeline::createDefaultRenderPass(VkFormat format) {
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// Subpasses and attachment references
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;


		// Subpass dependencies
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		bg2render::RenderPass* rp = new bg2render::RenderPass(_instance->renderDevice());
		rp->createInfo().attachmentCount = 1;
		rp->createInfo().pAttachments = &colorAttachment;
		rp->createInfo().subpassCount = 1;
		rp->createInfo().pSubpasses = &subpass;
		rp->createInfo().dependencyCount = 1;
		rp->createInfo().pDependencies = &dependency;
		rp->create();

		setRenderPass(rp);
	}

	void Pipeline::create() {
		if (_renderPass == nullptr) {
			throw std::invalid_argument("Failed to create pipeline: invalid render pass");
		}

		if (pipelineLayout() == VK_NULL_HANDLE) {
			throw std::invalid_argument("Failed to create pipeline: invalid pipeline layout");
		}

		VkGraphicsPipelineCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		// Shaders
		createInfo.stageCount = static_cast<uint32_t>(shaderStages().size());
		createInfo.pStages = shaderStages().data();

		// Vertex input
		createInfo.pVertexInputState = &vertexInputInfo();

		// Input assembly
		createInfo.pInputAssemblyState = &inputAssemblyInfo();

		// Viewport and scissor
		createInfo.pViewportState = &viewportState();

		// Rasterizer
		createInfo.pRasterizationState = &rasterizationStateInfo();

		// Multisampling
		createInfo.pMultisampleState = &multisamplingStateInfo();

		// TODO: Implement depth stencil state
		createInfo.pDepthStencilState = nullptr;

		// Color blend
		createInfo.pColorBlendState = &colorBlendInfo();

		// Dynamic state
		createInfo.pDynamicState = &dynamicStateInfo();

		// Layout
		createInfo.layout = pipelineLayout();

		// Render pass
		createInfo.renderPass = _renderPass->renderPass();
		createInfo.subpass = _subpass;

		// TODO: base pipeline handle
		createInfo.basePipelineHandle = VK_NULL_HANDLE;
		createInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(_instance->renderDevice()->device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &_pipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphics pipeline");
		}
	}
}
