
#include <bg2render/swap_chain.hpp>
#include <bg2render/vk_physical_device.hpp>
#include <bg2render/vk_instance.hpp>

#include <algorithm>
#include <stdexcept>

namespace bg2render {

	SwapChain::SwapChain(vk::PhysicalDevice* physicalDevice, vk::Device* device, VkSurfaceKHR surface)
		:_physicalDevice(physicalDevice)
		,_device(device)
		,_surface(surface)
	{

	}

	SwapChain::~SwapChain() {
		release();
	}

	void SwapChain::create(uint32_t width, uint32_t height) {
		VkSurfaceFormatKHR format = chooseSwapSurfaceFormat();
		VkPresentModeKHR presentMode = chooseSwapPresentMode();
		VkExtent2D extent = chooseSwapExtent(width, height);

		const auto& capabilities = _physicalDevice->getSwapChainSupport().capabilities;
		uint32_t imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
			imageCount = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = _surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = format.format;
		createInfo.imageColorSpace = format.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		
		const auto& indices = _physicalDevice->queueIndices();
		uint32_t queueFamilyIndices[] = { 
			static_cast<uint32_t>(indices.graphicsFamily), 
			static_cast<uint32_t>(indices.presentFamily) 
		};

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(_device->device(), &createInfo, nullptr, &_swapChain) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan swap chain");
		}

		imageCount = 0;
		vkGetSwapchainImagesKHR(_device->device(), _swapChain, &imageCount, nullptr);
		_images.resize(imageCount);
		vkGetSwapchainImagesKHR(_device->device(), _swapChain, &imageCount, _images.data());

		_format = format.format;
		_extent = extent;

		for (size_t i = 0; i < _images.size(); ++i) {
			vk::ImageView* imgView = new vk::ImageView(_device);
			imgView->create(_images[i], _format);
			_imageViews.push_back(std::shared_ptr<vk::ImageView>(imgView));
		}
	}

	void SwapChain::createFramebuffers(VkRenderPass renderPass, vk::ImageView* depthImageView) {
		_renderPass = renderPass;
		_framebuffers.resize(_imageViews.size());

		for (size_t i = 0; i < _imageViews.size(); ++i) {
			std::vector<VkImageView> attachments;
			attachments.push_back(_imageViews[i]->imageView());
			if (depthImageView) {
				attachments.push_back(depthImageView->imageView());
			}

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = _renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = _extent.width;
			framebufferInfo.height = _extent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(_device->device(), &framebufferInfo, nullptr, &_framebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create framebuffer");
			}
		}
	}

	void SwapChain::resize(uint32_t width, uint32_t height, RenderPass * renderPass, vk::ImageView* depthImageView) {
		release();
		create(width, height);
		createFramebuffers(renderPass->renderPass(), depthImageView);
	}

	VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat() {
		for (const auto& format : _physicalDevice->getSwapChainSupport().formats) {
			if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return format;
			}
		}

		return _physicalDevice->getSwapChainSupport().formats[0];
	}

	VkPresentModeKHR SwapChain::chooseSwapPresentMode() {
		for (const auto& presentMode : _physicalDevice->getSwapChainSupport().presentModes) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return presentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D SwapChain::chooseSwapExtent(uint32_t w, uint32_t h) {
		const auto& capabilities = _physicalDevice->getSwapChainSupport().capabilities;
		VkExtent2D actualExtent = { w, h };

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}

	void SwapChain::release() {
		_images.clear();
		if (_framebuffers.size()) {
			for (auto fb : _framebuffers) {
				vkDestroyFramebuffer(_device->device(), fb, nullptr);
			}
			_framebuffers.clear();
		}

		if (_swapChain != VK_NULL_HANDLE) {
			_imageViews.clear();
			vkDestroySwapchainKHR(_device->device(), _swapChain, nullptr);
		}
	}
}
