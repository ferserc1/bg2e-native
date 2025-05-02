
#include <bg2e/render/CubemapRenderer.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/render/vulkan/macros/descriptor_set.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>

#include <array>

namespace bg2e::render {

CubemapRenderer::CubemapRenderer(Vulkan * vulkan)
    :_vulkan { vulkan }
{

}
    
void CubemapRenderer::initFrameResources(vulkan::DescriptorSetAllocator* allocator)
{
    allocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    });
}

void CubemapRenderer::build(
    std::shared_ptr<vulkan::Image> inputSkyBox,
    const std::string& vshaderFile,
    const std::string& fshaderFile,
    VkExtent2D cubeImageSize,
    bool useMipmaps,
    uint32_t maxMipmapLevels
) {
    _inputSkybox = inputSkyBox;
    
    initImages(cubeImageSize, true, maxMipmapLevels);
    
    vulkan::factory::Sampler samplerFactory(_vulkan);
    _skyImageSampler = samplerFactory.build(
        VK_FILTER_LINEAR,
        VK_FILTER_LINEAR
    );
    
    _vulkan->cleanupManager().push([&](VkDevice device) {
        vkDestroySampler(device, _skyImageSampler, nullptr);
    });
    
    initPipeline(vshaderFile, fshaderFile);
    
    initGeometry();
}

void CubemapRenderer::update(
    VkCommandBuffer commandBuffer,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources
) {
    auto transitionInfo = vulkan::Image::TransitionInfo();
    transitionInfo.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    transitionInfo.mipLevelsCount = _cubeMapImage->mipLevels();
    vulkan::Image::cmdTransitionImage(
        commandBuffer,
        _cubeMapImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        transitionInfo
    );
    
    VkClearColorValue clearValue {{ 0.5f, 0.5f, 0.5f, 1.0f }};
    auto clearRange = vulkan::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(
        commandBuffer,
        _cubeMapImage->handle(),
        VK_IMAGE_LAYOUT_GENERAL,
        &clearValue,
        1,
        &clearRange
    );
    
    vulkan::Image::cmdTransitionImage(
        commandBuffer,
        _cubeMapImage->handle(),
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        transitionInfo
    );
    
    auto descriptorSet = frameResources.newDescriptorSet(_descriptorSetLayout);
    descriptorSet->beginUpdate();
        descriptorSet->addBuffer(
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            _projectionDataBuffer->handle(), sizeof(ProjectionData), 0
        );
        descriptorSet->addImage(
            1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            _inputSkybox->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            _skyImageSampler
        );
    descriptorSet->endUpdate();
    std::array<VkDescriptorSet, 1> descSets = {
        descriptorSet->descriptorSet()
    };
    
    auto mipLevels = cubeMapImage()->mipLevels();
    for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
    {
        // Draw geometry
        for (auto i = 0; i < 6; ++i)
        {
            auto view = _cubeMapImageViews[mipLevel].imageViews[i];
            auto colorAttachment = vulkan::Info::attachmentInfo(view, nullptr);
            VkExtent2D extent{
                uint32_t(vulkan::Image::getMipLevelSize(_cubeMapImage->extent2D().width, mipLevel)),
                uint32_t(vulkan::Image::getMipLevelSize(_cubeMapImage->extent2D().height, mipLevel))
            };
            auto renderInfo = vulkan::Info::renderingInfo(extent, &colorAttachment, nullptr);
            vulkan::cmdBeginRendering(commandBuffer, &renderInfo);
            
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
            
            vulkan::macros::cmdSetDefaultViewportAndScissor(commandBuffer, extent);
            
            // Draw the sky mesh
            SkyPushConstants pushConstants;
            pushConstants.currentFace = i;
            pushConstants.currentMipLevel = mipLevel;
            pushConstants.totalMipLevels = mipLevels;
            vkCmdPushConstants(
                commandBuffer,
                _layout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(SkyPushConstants),
                &pushConstants
            );
            
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                _layout, 0,
                uint32_t(descSets.size()), descSets.data(),
                0, nullptr
            );
            
            _cube->draw(commandBuffer);
            
            vulkan::cmdEndRendering(commandBuffer);
        }
    }
    
    vulkan::Image::cmdTransitionImage(
        commandBuffer,
        _cubeMapImage->handle(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        transitionInfo
    );
}

void CubemapRenderer::initImages(
    VkExtent2D cubeImageSize,
    bool useMipmaps,
    uint32_t maxMipmapLevels
) {
    // TODO: The format is hardcoded and should be configurable
    
    _cubeMapImage = std::shared_ptr<vulkan::Image>(vulkan::Image::createAllocatedImage(
       _vulkan,
       VK_FORMAT_R16G16B16A16_SFLOAT,
       cubeImageSize,
       VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
       VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
       VK_IMAGE_ASPECT_COLOR_BIT,
       6, // 6 layers. When specify this parameter, the image is created as a cube map compatible image with 6 layers
       useMipmaps,
       maxMipmapLevels
    ));
    
    // Initialize the imag layout for all the mipmap levels
    vulkan::Image::TransitionInfo info;
    info.mipLevelsCount = _cubeMapImage->mipLevels();
    _vulkan->command().immediateSubmit([&](VkCommandBuffer cmd) {
        vulkan::Image::cmdTransitionImage(
            cmd,
            _cubeMapImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL,
            info
        );
    });
    
    auto viewInfo = vulkan::Info::imageViewCreateInfo(
        VK_FORMAT_R16G16B16A16_SFLOAT,
        _cubeMapImage->handle(),
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    VkImageView imgView;
    auto mipLevels = _cubeMapImage->mipLevels();
    for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
    {
        _cubeMapImageViews.push_back({ VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE });
        viewInfo.subresourceRange.baseMipLevel = mipLevel;
        for (int i = 0; i < 6; ++i)
        {
            viewInfo.subresourceRange.baseArrayLayer = i;
            vkCreateImageView(_vulkan->device().handle(), &viewInfo, nullptr, &imgView);
            _cubeMapImageViews[mipLevel].imageViews[i] = imgView;
        }
    }
    
    _vulkan->cleanupManager().push([&](VkDevice dev) {
        auto mipLevels = _cubeMapImage->mipLevels();
        for (size_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
        {
            for (int i = 0; i < 6; ++i)
            {
                vkDestroyImageView(dev, _cubeMapImageViews[mipLevel].imageViews[i], nullptr);
            }
        }
        
        _cubeMapImage->cleanup();
    });
    
    vulkan::Image::transitionImage(
        _vulkan,
        _cubeMapImage->handle(),
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

void CubemapRenderer::initPipeline(
    const std::string &vshaderFile,
    const std::string &fshaderFile
) {
    vulkan::factory::GraphicsPipeline plFactory(_vulkan);
    
    plFactory.addShader(vshaderFile, VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader(fshaderFile, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    vulkan::factory::DescriptorSetLayout dsFactory;
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);         // Projection data buffer
    dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // Input skybox image
    _descriptorSetLayout = dsFactory.build(
        _vulkan->device().handle(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    );
    
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SkyPushConstants);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    auto layoutInfo = vulkan::Info::pipelineLayoutInfo();
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    layoutInfo.pushConstantRangeCount = 1;
    
    std::vector<VkDescriptorSetLayout> layouts = {
        _descriptorSetLayout
    };
    layoutInfo.pSetLayouts = layouts.data();
    layoutInfo.setLayoutCount = uint32_t(layouts.size());
    
    VK_ASSERT(vkCreatePipelineLayout(_vulkan->device().handle(), &layoutInfo, nullptr, &_layout));
    
    plFactory.setInputState<vulkan::geo::MeshP>();
    plFactory.setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
    plFactory.disableDepthtest();
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(false, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    
    _pipeline = plFactory.build(_layout);
    
    _vulkan->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipelineLayout(dev, _layout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _descriptorSetLayout, nullptr);
    });
}
    
void CubemapRenderer::initGeometry()
{
    _projectionData.view[0] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _projectionData.view[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _projectionData.view[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    _projectionData.view[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f));
    _projectionData.view[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, 0.0f,-1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _projectionData.view[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	_projectionData.proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);
    _projectionData.proj[1][1] *= -1.0f;
    _projectionData.proj[0][0] *= -1.0f;
   
    _projectionDataBuffer = std::unique_ptr<vulkan::Buffer>(vulkan::Buffer::createAllocatedBuffer(
        _vulkan,
        sizeof(ProjectionData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    ));
    
    ProjectionData* projectionDataPtr = reinterpret_cast<ProjectionData*>(_projectionDataBuffer->allocatedData());
    *projectionDataPtr = _projectionData;
    
    auto cubeMesh = std::unique_ptr<bg2e::geo::MeshP>(
        bg2e::geo::createCubeP(10.0f, 10.0f, 10.0f, true)
    );
    
    _cube = std::unique_ptr<vulkan::geo::MeshP>(new vulkan::geo::MeshP(_vulkan));
    _cube->setMeshData(cubeMesh.get());
    _cube->build();
    
    _vulkan->cleanupManager().push([&](VkDevice) {
        _projectionDataBuffer->cleanup();
        _cube->cleanup();
    });
}

}
