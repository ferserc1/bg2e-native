
#include <bg2render/offscreen_renderer.hpp>
#include <bg2render/buffer_utils.hpp>

#include <array>
#include <stdexcept>

namespace bg2render {

	OffscreenRenderer::OffscreenRenderer(vk::Instance* instance)
		:_instance(instance)
	{

	}

	OffscreenRenderer::~OffscreenRenderer() {

	}

	void OffscreenRenderer::create(VkFormat format, const bg2math::uint2& size) {
		_size = size;
		_colorFormat = format;

		_depthFormat = _instance->renderPhysicalDevice()->findDepthFormat();

		// Color image attachments
		_colorImage = std::make_unique<vk::Image>(_instance);
		_colorImage->create(_colorFormat, size, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		_colorImageMemory = std::make_unique<vk::DeviceMemory>(_instance);
		_colorImageMemory->allocate(_colorImage->memoryRequirements(),VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vkBindImageMemory(_instance->renderDevice()->device(), _colorImage->image(), _colorImageMemory->deviceMemory(), 0);
		_colorImageView = std::make_unique<vk::ImageView>(_instance);
		_colorImageView->create(_colorImage.get(), _colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		_colorSampler = std::make_unique<vk::Sampler>(_instance);
		_colorSampler->createInfo().addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		_colorSampler->createInfo().addressModeV = _colorSampler->createInfo().addressModeU;
		_colorSampler->createInfo().addressModeW = _colorSampler->createInfo().addressModeU;
		_colorSampler->createInfo().borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		_colorSampler->create();

		// Depth image attachment
		_depthImage = std::make_unique<vk::Image>(_instance);
		_depthImage->create(_depthFormat, size, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		_depthImageMemory = std::make_unique<vk::DeviceMemory>(_instance);
		_depthImageMemory->allocate(_depthImage->memoryRequirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vkBindImageMemory(_instance->renderDevice()->device(), _depthImage->image(), _depthImageMemory->deviceMemory(), 0);
		_depthImageView = std::make_unique<vk::ImageView>(_instance);
		_depthImageView->create(_depthImage.get(), _depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
		
		// Render pass
		_renderPass = std::make_unique<RenderPass>(_instance->renderDevice()->device());

		std::array<VkAttachmentDescription, 2> attachmentDescriptions = {};
		attachmentDescriptions[0].format = _colorFormat;
		attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachmentDescriptions[1].format = _depthFormat;
		attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pDepthStencilAttachment = &depthReference;

		std::array<VkSubpassDependency, 2> dependencies;
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		_renderPass->createInfo().attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		_renderPass->createInfo().pAttachments = attachmentDescriptions.data();
		_renderPass->createInfo().subpassCount = 1;
		_renderPass->createInfo().pSubpasses = &subpassDescription;
		_renderPass->createInfo().dependencyCount = static_cast<uint32_t>(dependencies.size());
		_renderPass->createInfo().pDependencies = dependencies.data();
		_renderPass->create();

		VkImageView attachments[] = {
			_colorImageView->imageView(),
			_depthImageView->imageView()
		};

		VkFramebufferCreateInfo fbCreateInfo = {};
		fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbCreateInfo.renderPass = _renderPass->renderPass();
		fbCreateInfo.attachmentCount = 2;
		fbCreateInfo.pAttachments = attachments;
		fbCreateInfo.width = _size.width();
		fbCreateInfo.height = _size.height();
		fbCreateInfo.layers = 1;
		if (vkCreateFramebuffer(_instance->renderDevice()->device(), &fbCreateInfo, nullptr, &_framebuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create offscreen render framebuffer");
		}

		descriptorCreateInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorCreateInfo.imageView = _colorImageView->imageView();
		descriptorCreateInfo.sampler = _colorSampler->sampler();

	}

}
