
#ifndef _bg2render_vk_image_hpp_
#define _bg2render_vk_image_hpp_

#include <bg2render/vk_instance.hpp>
#include <bg2math/vector.hpp>

namespace bg2render {
    namespace vk {

        class Image {
		public:
			Image(Instance*);
			virtual ~Image();

			inline VkImageCreateInfo& createInfo() { return _createInfo; }
			
			// The parameters in create() function are those of the createInfo structure
			// that must be specified obligatorily. Depending on the create function used,
			// the image type will be VK_IMAGE_TYPE_2D or VK_IMAGE_TYPE_3D. The rest of the
			// parameters can be stablished using the createInfo() accessor
			void create(VkFormat format, const bg2math::int2& extent, VkImageUsageFlags usage);
			void create(VkFormat format, const bg2math::int3& extent, VkImageUsageFlags usage);
			inline void create(VkFormat format, const bg2math::uint2& extent, VkImageUsageFlags usage) {
				create(format, bg2math::int2(static_cast<int32_t>(extent.x()), static_cast<int32_t>(extent.y())), usage);
			}
			inline void create(VkFormat format, const bg2math::uint3& extent, VkImageUsageFlags usage) {
				create(format, bg2math::int3(static_cast<int32_t>(extent.x()), static_cast<int32_t>(extent.y()), static_cast<int32_t>(extent.z())), usage);
			}

			inline VkImage image() const { return _image; }
			inline const VkMemoryRequirements& memoryRequirements() const { return _memoryRequirements; }
			inline const VkExtent3D& extent() const { return _createInfo.extent; }
			inline VkFormat format() const { return _createInfo.format; }

			// layout transition presets:
			//	oldLayout: VK_IMAGE_LAYOUT_UNDEFINED, newLayout: VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL > prepare to copy a buffer to the image
			//	oldLayout: VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, newLayout: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL > prepare to use the image in a shader
			//			   Use the shaderStages parameter to specify the shader stages that will use the image. Default: VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			//  oldLayout: VK_IMAGE_LAYOUT_UNDEFINED, newLayout: VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL > prepare image to use as depth buffer
			void layoutTransition(VkCommandPool commandPool, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags shaderStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		protected:

			Instance* _instance;

			VkImage _image = VK_NULL_HANDLE;
			VkImageCreateInfo _createInfo = {
				VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,		// VkStructureType          sType;
				nullptr,									// const void* pNext;
				0,											// VkImageCreateFlags       flags;
				VK_IMAGE_TYPE_2D,							// VkImageType              imageType;
				VK_FORMAT_R8G8B8A8_UNORM,					// VkFormat                 format;
				{ 0, 0, 1 },								// VkExtent3D               extent;
				1,											// uint32_t                 mipLevels;
				1,											// uint32_t                 arrayLayers;
				VK_SAMPLE_COUNT_1_BIT,						// VkSampleCountFlagBits    samples;
				VK_IMAGE_TILING_OPTIMAL,					// VkImageTiling            tiling;
				0,											// VkImageUsageFlags        usage;
				VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode            sharingMode;
				0,											// uint32_t                 queueFamilyIndexCount;
				nullptr,									// const uint32_t* pQueueFamilyIndices;
				VK_IMAGE_LAYOUT_UNDEFINED					// VkImageLayout            initialLayout;
			};
			VkMemoryRequirements _memoryRequirements;
        };

    }
}

#endif
