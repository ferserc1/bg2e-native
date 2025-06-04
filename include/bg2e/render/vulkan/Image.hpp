
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>

namespace bg2e {
namespace render {

class Engine;

namespace vulkan {

class Swapchain;

class BG2E_API Image {
public:
    struct TransitionInfo {
        TransitionInfo(
            VkImageAspectFlags    aspectMask = 0,
            uint32_t              mipLevel = 0,
            uint32_t              mipLevelsCount = VK_REMAINING_MIP_LEVELS,
            uint32_t			  baseArrayLayer = 0,
            uint32_t			  layerCount = VK_REMAINING_ARRAY_LAYERS,
            VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            VkAccessFlags2        srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            VkAccessFlags2        dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
        )
            :aspectMask { aspectMask }
            ,mipLevel { mipLevel }
            ,mipLevelsCount { mipLevelsCount }
            ,baseArrayLayer { baseArrayLayer }
            ,layerCount { layerCount }
            ,srcStageMask { srcStageMask }
            ,srcAccessMask { srcAccessMask }
            ,dstStageMask { dstStageMask }
            ,dstAccessMask { dstAccessMask }
        {}
        
        
        VkImageAspectFlags    aspectMask = 0;
        uint32_t              mipLevel = 0;
        uint32_t              mipLevelsCount = VK_REMAINING_MIP_LEVELS;
        uint32_t			  baseArrayLayer = 0;
        uint32_t			  layerCount = VK_REMAINING_ARRAY_LAYERS;
        VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        VkAccessFlags2        srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        VkAccessFlags2        dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    };
    
    /*
     *   Add an image transition command to the command buffer to switch from one layout
     *   to another.
     *
     *   The default configuration of stageMask and accessMask will always work, but will
     *   leave the queue stopped until the the queue stopped until the transition is complete.
     *
     *   It is recommended to fine-tune these parameters for the intended use of the image.
     */
    static void cmdTransitionImage(
        VkCommandBuffer       cmd,
        VkImage               image,
        VkImageLayout         oldLayout,
        VkImageLayout         newLayout,
        TransitionInfo        transitionInfo = TransitionInfo()
    );

    /*
	 * Execute a transition on an image blocking the thread. It works in the same way as the
	 * cmdTransitionImage function, but it blocks the thread until the transition is complete.
	 * Receives the vulkanData pointer to be able to create and execute the command buffer, instead
	 * of receiving the command buffer directly.
     */
    static void transitionImage(
        Engine*               engine,
        VkImage               image,
        VkImageLayout         oldLayout,
        VkImageLayout         newLayout,
        TransitionInfo        transitionInfo = TransitionInfo()
    );

    /*
     *  Returns a default initialised VkImageSubresourceRange structure for all mipmaps and layers.
     */
    static VkImageSubresourceRange subresourceRange(VkImageAspectFlags aspectMask);

	static uint32_t getMipLevels(VkExtent2D extent);

    static int32_t getMipLevelSize(int32_t baseMipLevelSize, uint32_t mipLevel) {
        return int32_t(baseMipLevelSize >> mipLevel);
    }
    
    /*
     *   Setup an image to image copy command
     */
    static void cmdCopy(
        VkCommandBuffer cmd,
        VkImage         srcImage,
        VkExtent2D      srcSize,
        VkImage         dstImage,
        VkExtent2D      dstSize
    );
    
    /*
     *  Setup an image to image copy command, but also adds the
     *  required transitions to the source and destination image.
     *  Optionally, the final layout of the images can also be
     *  configured if srcFinalLayout and dstFinalLayout are set
     *  to other layout other than VK_IMAGE_LAYOUT_UNDEFINED
     */
    static void cmdCopy(
        VkCommandBuffer cmd,
        VkImage         srcImage,
        VkExtent2D      srcSize,
        VkImageLayout 	srcInitialLayout,
        VkImage         dstImage,
        VkExtent2D      dstSize,
        VkImageLayout   dstInitialLayout,
        VkImageLayout 	srcFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        VkImageLayout   dstFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED
    );
    
    static Image* createAllocatedImage(
        Engine * vulkanData,
        VkFormat format,
        VkExtent2D extent,
        VkImageUsageFlags usage,
        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        uint32_t arrayLayers = 1,
        bool useMipmaps = false,
        uint32_t maxMipmapLevels = 20
    );
    
    static Image* createAllocatedImage(
        Engine * vulkanData,
        void* data,
        VkExtent2D extent,
        uint32_t dataBytesPerPixel,  // WARNING: for now, it only works with 4 bpp
        VkFormat imageFormat,
        VkImageUsageFlags usage,
        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        bool useMipmaps = false,
        uint32_t maxMipmapLevels = 20
    );

    static Image* wrapSwapchainImage(
        const Swapchain* swapchain,
        uint32_t swapchainImageIndex
    );
    
    virtual ~Image();
    
    void cleanup();

    inline VkImage handle() const { return _image; }
    inline VkImageView imageView() const { return _imageView; }
    inline VmaAllocation allocation() const { return _allocation; }
    inline const VkExtent3D& extent() const { return _extent; }
    inline const VkExtent2D extent2D() const { return VkExtent2D{ _extent.width, _extent.height }; }
    inline VkFormat format() const { return _format; }
	inline uint32_t mipLevels() const { return _mipLevels; }


protected:
    // Only allow create images using factory functions
    Image() = default;
    
    VkImage _image = VK_NULL_HANDLE;
    VkImageView _imageView = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
    VkExtent3D _extent = { 0, 0 };
    VkFormat _format = VK_FORMAT_R8G8B8A8_UNORM;
    uint32_t _mipLevels = 1;
    
    Engine * _engine = nullptr;
};

}
}
}

