
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/Device.hpp>

#include <bg2e/render/Vulkan.hpp>


#include <algorithm>

namespace bg2e {
namespace render {
namespace vulkan {

void Image::cmdTransitionImage(
    VkCommandBuffer       cmd,
    VkImage               image,
    VkImageLayout         oldLayout,
    VkImageLayout         newLayout,
    TransitionInfo        transitionInfo
    /*
    VkImageAspectFlags    aspectMask,
    uint32_t              mipLevel,
    uint32_t              mipLevelsCount,
	uint32_t			  baseArrayLayer,
	uint32_t			  layerCount,
    VkPipelineStageFlags2 srcStageMask,
    VkAccessFlags2        srcAccessMask,
    VkPipelineStageFlags2 dstStageMask,
    VkAccessFlags2        dstAccessMask
    */
) {
    VkImageAspectFlags    aspectMask = transitionInfo.aspectMask;
    uint32_t              mipLevel = transitionInfo.mipLevel;
    uint32_t              mipLevelsCount = transitionInfo.mipLevelsCount;
    uint32_t			  baseArrayLayer = transitionInfo.baseArrayLayer;
    uint32_t			  layerCount = transitionInfo.layerCount;
    VkPipelineStageFlags2 srcStageMask = transitionInfo.srcStageMask;
    VkAccessFlags2        srcAccessMask = transitionInfo.srcAccessMask;
    VkPipelineStageFlags2 dstStageMask = transitionInfo.dstStageMask;
    //VkAccessFlags2        dstAccessMask = transitionInfo.dstAccessMask;

    VkImageMemoryBarrier2 imageBarrier = {};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.srcStageMask = srcStageMask;
    imageBarrier.srcAccessMask = srcAccessMask;
    imageBarrier.dstStageMask = dstStageMask;
    imageBarrier.dstAccessMask = dstStageMask; 
    imageBarrier.oldLayout = oldLayout;
    imageBarrier.newLayout = newLayout;

    if (aspectMask == 0) {
        aspectMask = newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ?
            VK_IMAGE_ASPECT_DEPTH_BIT :
            VK_IMAGE_ASPECT_COLOR_BIT;
    }
    imageBarrier.subresourceRange = Image::subresourceRange(aspectMask);
	imageBarrier.subresourceRange.baseMipLevel = mipLevel;
    imageBarrier.subresourceRange.levelCount = mipLevelsCount;
	imageBarrier.subresourceRange.layerCount = layerCount;
	imageBarrier.subresourceRange.baseArrayLayer = baseArrayLayer;
    imageBarrier.image = image;

    VkDependencyInfo dependencies = {};
    dependencies.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencies.imageMemoryBarrierCount = 1;
    dependencies.pImageMemoryBarriers = &imageBarrier;
    
    cmdPipelineBarrier2(cmd, &dependencies);
}

void Image::transitionImage(
    Vulkan*               vulkan,
    VkImage               image,
    VkImageLayout         oldLayout,
    VkImageLayout         newLayout,
    TransitionInfo        transitionInfo
    
    /*
    VkImageAspectFlags    aspectMask,
    uint32_t              mipLevel,
    uint32_t              mipLevelsCount,
    uint32_t			  baseArrayLayer,
    uint32_t			  layerCount,
    VkPipelineStageFlags2 srcStageMask,
    VkAccessFlags2        srcAccessMask,
    VkPipelineStageFlags2 dstStageMask,
    VkAccessFlags2        dstAccessMask
    */
) {
	vulkan->command().immediateSubmit([&](VkCommandBuffer cmd) {
		Image::cmdTransitionImage(
			cmd,
			image,
			oldLayout,
			newLayout,
            transitionInfo
		);
	});
}

VkImageSubresourceRange Image::subresourceRange(VkImageAspectFlags aspectMask)
{
    VkImageSubresourceRange range {};
    range.aspectMask = aspectMask;
    range.baseMipLevel = 0;
    range.levelCount = VK_REMAINING_MIP_LEVELS;
    range.baseArrayLayer = 0;
    range.layerCount = VK_REMAINING_ARRAY_LAYERS;
    return range;
}

void Image::cmdCopy(
    VkCommandBuffer cmd,
    VkImage srcImage,
    VkExtent2D srcSize,
    VkImage dstImage,
    VkExtent2D dstSize
)
{
    VkImageBlit2 blitRegion = {};
    blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;
	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo = {};
    blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    
	blitInfo.dstImage = dstImage;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = srcImage;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

    cmdBlitImage2(cmd, &blitInfo);
}

void Image::cmdCopy(
    VkCommandBuffer cmd,
    VkImage         srcImage,
    VkExtent2D      srcSize,
    VkImageLayout 	srcInitialLayout,
    VkImage         dstImage,
    VkExtent2D      dstSize,
    VkImageLayout   dstInitialLayout,
    VkImageLayout 	srcFinalLayout,
    VkImageLayout   dstFinalLayout
)
{
    Image::cmdTransitionImage(
        cmd,
        srcImage,
        srcInitialLayout,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );
    Image::cmdTransitionImage(
        cmd,
        dstImage,
        dstInitialLayout,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    
    VkImageBlit2 blitRegion = {};
    blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;
	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo = {};
    blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    
	blitInfo.dstImage = dstImage;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = srcImage;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

    cmdBlitImage2(cmd, &blitInfo);
    
    if (srcFinalLayout != VK_IMAGE_LAYOUT_UNDEFINED)
    {
        Image::cmdTransitionImage(
            cmd,
            srcImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            srcFinalLayout
        );
    }
    
    if (dstFinalLayout != VK_IMAGE_LAYOUT_UNDEFINED)
    {
        Image::cmdTransitionImage(
            cmd,
            dstImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            dstFinalLayout
        );
    }
}

uint32_t Image::getMipLevels(VkExtent2D extent)
{
    return uint32_t(floor(log2(std::max(extent.width, extent.height))) + 1);
}

Image* Image::createAllocatedImage(
    Vulkan * vulkan,
    VkFormat format,
    VkExtent2D extent,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspectFlags,
    uint32_t arrayLayers,
	bool useMipmaps,
    uint32_t maxMipmapLevels
)
{
    auto result = new Image();
    result->_vulkan = vulkan;
    result->_extent = { extent.width, extent.height, 1 };
    result->_format = format;
    
    auto imgInfo = Info::imageCreateInfo(
        result->_format,
        usage,
        result->_extent,
        arrayLayers
    );

    if (useMipmaps)
    {
        imgInfo.mipLevels = std::min(Image::getMipLevels(extent), maxMipmapLevels);
		result->_mipLevels = imgInfo.mipLevels;
    }

    if (arrayLayers == 6)
    {
        imgInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
    
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    VmaAllocator alloc = vulkan->allocator();
    vmaCreateImage(
        alloc,
        &imgInfo,
        &allocInfo,
        &result->_image,
        &result->_allocation,
        nullptr
    );
    
    auto imgViewInfo = Info::imageViewCreateInfo(format, result->_image, aspectFlags);
    if (arrayLayers == 6)
    {
        imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        imgViewInfo.subresourceRange.layerCount = 6;
    }
    if (useMipmaps)
    {
        imgViewInfo.subresourceRange.levelCount = result->_mipLevels;
    }
    VK_ASSERT(vkCreateImageView(vulkan->device().handle(), &imgViewInfo, nullptr, &result->_imageView));
    
    return result;
}

Image* Image::createAllocatedImage(
    Vulkan * vulkan,
    void* data,
    VkExtent2D extent,
    uint32_t dataBytesPerPixel,  // WARNING: for now, it only works with 4 bpp
    VkFormat imageFormat,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspectFlags,
    bool useMipmaps,
    uint32_t maxMipmapLevels
) {
    size_t dataSize = extent.width * extent.height * dataBytesPerPixel;
    auto uploadBuffer = std::unique_ptr<Buffer>(
        Buffer::createAllocatedBuffer(
            vulkan,
            dataSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        )
    );
    
    auto uploadData = uploadBuffer->allocatedData();
    memcpy(uploadData, data, dataSize);
    
    if (useMipmaps)
	{
		usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

    auto image = createAllocatedImage(
        vulkan,
        imageFormat,
        extent,
        usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        aspectFlags,
        1,
        useMipmaps,
        maxMipmapLevels
    );
 
    vulkan->command().immediateSubmit([&](VkCommandBuffer cmd) {
        Image::cmdTransitionImage(
            cmd,
            image->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );
        
        VkBufferImageCopy copyRgn = {};
        copyRgn.imageSubresource.aspectMask = aspectFlags;
        copyRgn.imageSubresource.mipLevel = 0;
        copyRgn.imageSubresource.baseArrayLayer = 0;
        copyRgn.imageSubresource.layerCount = 1;
        copyRgn.imageExtent = VkExtent3D { extent.width, extent.height, 1 };
        
        vkCmdCopyBufferToImage(
            cmd,
            uploadBuffer->handle(),
            image->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRgn
        );

        if (useMipmaps)
        {
            Image::cmdTransitionImage(
                cmd,
                image->handle(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            );

            for (uint32_t i = 1; i < image->mipLevels(); ++i)
            {
                VkImageBlit imageBlit{};
                imageBlit.srcSubresource.aspectMask = aspectFlags;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
                imageBlit.srcOffsets[1].x = Image::getMipLevelSize(image->extent2D().width, i - 1);
                imageBlit.srcOffsets[1].y = Image::getMipLevelSize(image->extent2D().height, i - 1);
				imageBlit.srcOffsets[1].z = 1;

                imageBlit.dstSubresource.aspectMask = aspectFlags;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstOffsets[1].x = Image::getMipLevelSize(image->extent2D().width, i);
				imageBlit.dstOffsets[1].y = Image::getMipLevelSize(image->extent2D().height, i);
                imageBlit.dstOffsets[1].z = 1;

                Image::cmdTransitionImage(
                    cmd,
                    image->handle(),
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    Image::TransitionInfo( aspectFlags, i, 1 )
                );

				vkCmdBlitImage(
					cmd,
					image->handle(),
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					image->handle(),
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&imageBlit,
					VK_FILTER_LINEAR
				); 

                Image::cmdTransitionImage(
                    cmd,
                    image->handle(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    Image::TransitionInfo( aspectFlags, i, 1 )
                );
            }

            Image::cmdTransitionImage(
                cmd,
                image->handle(),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                Image::TransitionInfo( aspectFlags, 0, image->mipLevels() )
            );
        }
        else
        {
            Image::cmdTransitionImage(
                cmd,
                image->handle(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                aspectFlags
            );
        }
    });
    
    uploadBuffer->cleanup();
    
    return image;
}

Image* Image::wrapSwapchainImage(
    const Swapchain* swapchain,
    uint32_t swapchainImageIndex
) {
    auto result = new Image();
    
    result->_image = swapchain->image(swapchainImageIndex);
    result->_imageView = swapchain->imageView(swapchainImageIndex);
    result->_format = swapchain->imageFormat();
    result->_extent = VkExtent3D{
        swapchain->extent().width,
        swapchain->extent().height,
        1
    };

    return result;
}

void Image::cleanup()
{
    vkDestroyImageView(_vulkan->device().handle(), _imageView, nullptr);
    vmaDestroyImage(_vulkan->allocator(), _image, _allocation);
}

}
}
}

