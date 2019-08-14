
#ifndef _bg2_render_vk_physical_device_hpp_
#define _bg2_render_vk_physical_device_hpp_

#include <vulkan/vulkan.h>

#include <vector>

namespace bg2render {
    namespace vk {

        class Instance;
        class PhysicalDevice {
        public:
			struct QueueFamilyIndices {
				int32_t graphicsFamily = -1;
				int32_t presentFamily = -1;

				inline bool isComplete() const {
					return graphicsFamily != -1 &&
							presentFamily != -1;
				}
			};

			struct SwapChainSupportDetails {
				VkSurfaceCapabilitiesKHR capabilities;
				std::vector<VkSurfaceFormatKHR> formats;
				std::vector<VkPresentModeKHR> presentModes;
			};

            PhysicalDevice(Instance * instance, VkPhysicalDevice dev, VkSurfaceKHR surface = VK_NULL_HANDLE);
            virtual ~PhysicalDevice();

			void getProperties(VkPhysicalDeviceProperties& properties) const;
			void getFeatures(VkPhysicalDeviceFeatures& features) const;
			bool checkExtensionSupport(const std::vector<const char*>& ext) const;
			void getExtensionProperties(std::vector<VkExtensionProperties>& ext) const;
			
			VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
			inline VkFormat findDepthFormat() {
				return findSupportedFormat(
					{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
					VK_IMAGE_TILING_OPTIMAL,
					VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
			}

            inline VkPhysicalDevice physicalDevice() const { return _physicalDevice; }

			inline const QueueFamilyIndices& queueIndices() const { return _queueIndices; }
			const SwapChainSupportDetails& getSwapChainSupport();
			uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

        protected:
            Instance * _instance = nullptr;

			VkSurfaceKHR _surface = VK_NULL_HANDLE;
            VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
			QueueFamilyIndices _queueIndices;
			SwapChainSupportDetails _swapChainSupportDetails;
        };

    }
}

#endif
