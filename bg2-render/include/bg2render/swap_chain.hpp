
#ifndef _bg2_render_swap_chain_hpp_
#define _bg2_render_swap_chain_hpp_

#include <vulkan/vulkan.h>

#include <bg2render/vk_physical_device.hpp>
#include <bg2render/vk_device.hpp>
#include <bg2render/render_pass.hpp>
#include <bg2render/vk_image_view.hpp>
#include <bg2math/vector.hpp>

#include <vector>

namespace bg2render {

    class SwapChain {
	public:
		SwapChain(vk::PhysicalDevice* physicalDevice, vk::Device* device, VkSurfaceKHR surface);
		virtual ~SwapChain();

		inline void create(const bg2math::int2& size) { create(size.x(), size.y()); }
		void create(uint32_t width, uint32_t height);

		inline void createFramebuffers(RenderPass* renderPass, vk::ImageView* dephtImageView = nullptr) { createFramebuffers(renderPass->renderPass(), dephtImageView); }
		void createFramebuffers(VkRenderPass renderPass, vk::ImageView* depthImageView = nullptr);

		inline void resize(const bg2math::int2& size, RenderPass * renderPass, vk::ImageView* depthImageView = nullptr) { resize(size.x(), size.y(), renderPass, depthImageView); }
		void resize(uint32_t width, uint32_t height, RenderPass * renderPass, vk::ImageView* depthImageView = nullptr);

		inline VkSwapchainKHR swapchain() const { return _swapChain; }

		inline const std::vector<VkImage>& images() const { return _images; }
		inline VkFormat format() const { return _format; }
		inline const VkExtent2D& extent() const { return _extent; }
		inline const std::vector<std::shared_ptr<vk::ImageView>>& imageViews() const { return _imageViews; }
		inline const std::vector<VkFramebuffer>& framebuffers() const { return _framebuffers; }

	protected:
		vk::PhysicalDevice* _physicalDevice;
		vk::Device* _device;
		VkSurfaceKHR _surface;
		VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
		std::vector<VkImage> _images;
		VkFormat _format;
		VkExtent2D _extent;
		std::vector<std::shared_ptr<vk::ImageView>> _imageViews;
		VkRenderPass _renderPass;
		std::vector<VkFramebuffer> _framebuffers;

		VkSurfaceFormatKHR chooseSwapSurfaceFormat();
		VkPresentModeKHR chooseSwapPresentMode();
		VkExtent2D chooseSwapExtent(uint32_t w, uint32_t h);

		void release();
    };

}

#endif
