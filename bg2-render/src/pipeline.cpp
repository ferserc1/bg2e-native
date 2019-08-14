
#include <bg2render/pipeline.hpp>

#include <stdexcept>
#include <array>

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

		_pipelineLayout = nullptr;
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

	void Pipeline::createDefaultRenderPass(VkFormat format, bool supportDepth) {
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = _instance->renderPhysicalDevice()->findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Subpasses and attachment references
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		if (supportDepth) {
			subpass.pDepthStencilAttachment = &depthAttachmentRef;
		}

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
		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		if (supportDepth) {
			rp->createInfo().attachmentCount = static_cast<uint32_t>(attachments.size());
			rp->createInfo().pAttachments = attachments.data();
		}
		else {
			rp->createInfo().attachmentCount = 1;
			rp->createInfo().pAttachments = &colorAttachment;
		}
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

		// Depth stencil state
		createInfo.pDepthStencilState = &depthStencilInfo();

		// Color blend
		createInfo.pColorBlendState = &colorBlendInfo();

		// Dynamic state
		createInfo.pDynamicState = &dynamicStateInfo();

		// Layout
		createInfo.layout = pipelineLayout()->pipelineLayout();

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

	void Pipeline::resize(const bg2math::int2& size) {
		if (_pipeline == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed to resize pipeline: the pipeline is not created.");
		}

		vkDestroyPipeline(_instance->renderDevice()->device(), _pipeline, nullptr);

		if (_scissor.extent.width == static_cast<uint32_t>(_viewport.width) &&
			_scissor.extent.height == static_cast<uint32_t>(_viewport.height)) {
			_scissor.extent.width = size.x();
			_scissor.extent.height = size.y();
		}
		_viewport.width = static_cast<float>(size.x());
		_viewport.height = static_cast<float>(size.y());

		create();
	}
}
