
#ifndef _bg2_render_swap_chain_hpp_
#define _bg2_render_swap_chain_hpp_

#include <vulkan/vulkan.h>

#include <bg2render/vk_physical_device.hpp>
#include <bg2render/vk_device.hpp>
#include <bg2math/vector.hpp>

#include <vector>

namespace bg2render {

    class SwapChain {
	public:
		SwapChain(vk::PhysicalDevice* physicalDevice, vk::Device* device, VkSurfaceKHR surface);
		virtual ~SwapChain();

		inline void create(const bg2math::int2& size) { create(size.x(), size.y()); }
		void create(uint32_t width, uint32_t height);

		inline const std::vector<VkImage>& images() const { return _images; }
		inline VkFormat format() const { return _format; }
		inline const VkExtent2D& extent() const { return _extent; }

	protected:
		vk::PhysicalDevice* _physicalDevice;
		vk::Device* _device;
		VkSurfaceKHR _surface;
		VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
		std::vector<VkImage> _images;
		VkFormat _format;
		VkExtent2D _extent;

		VkSurfaceFormatKHR chooseSwapSurfaceFormat();
		VkPresentModeKHR chooseSwapPresentMode();
		VkExtent2D chooseSwapExtent(uint32_t w, uint32_t h);
    };

}

#endif
