
#include <bg2e/render/SphereToCubemapRenderer.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/base/Texture.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/geo/sphere.hpp>
#include <bg2e/db/image.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>

#include <array>

namespace bg2e::render {

SphereToCubemapRenderer::SphereToCubemapRenderer(Vulkan * vulkan)
    :_vulkan(vulkan)
{

}

SphereToCubemapRenderer::~SphereToCubemapRenderer()
{
    cleanup();
}

void SphereToCubemapRenderer::initFrameResources(vulkan::DescriptorSetAllocator* frameAllocator)
{
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    });
}

void SphereToCubemapRenderer::build(
    const std::filesystem::path& imagePath,
    const std::string& vertexShader,
    const std::string& fragmentShader,
    VkExtent2D cubeImageSize
) {
    vulkan::factory::DescriptorSetLayout dsFactory;
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _dsLayout = dsFactory.build(
        _vulkan->device().handle(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    );
    
    _projectionData.view[0] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _projectionData.view[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _projectionData.view[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    _projectionData.view[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f));
    _projectionData.view[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, 0.0f,-1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _projectionData.view[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    _projectionData.proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, _sphereRadius * 10.0f);
    _projectionData.proj[1][1] *= -1.0f;
    _projectionData.proj[0][0] *= -1.0f;
    
    _projectionDataBuffer = std::unique_ptr<vulkan::Buffer>(vulkan::Buffer::createAllocatedBuffer(
        _vulkan,
        sizeof(ProjectionData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    ));
    
    auto projectionDataBufferPtr = reinterpret_cast<ProjectionData*>(_projectionDataBuffer->allocatedData());
    *projectionDataBufferPtr = _projectionData;
    
    updateImage(imagePath);
    initImages(cubeImageSize);
    initPipeline(vertexShader, fragmentShader);
    initGeometry();
}

void SphereToCubemapRenderer::build(
    std::shared_ptr<render::Texture> texture,
    const std::string& vertexShader,
    const std::string& fragmentShader,
    VkExtent2D cubeImageSize
) {
    vulkan::factory::DescriptorSetLayout dsFactory;
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _dsLayout = dsFactory.build(
        _vulkan->device().handle(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    );
    
    _projectionData.view[0] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _projectionData.view[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _projectionData.view[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    _projectionData.view[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f));
    _projectionData.view[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, 0.0f,-1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _projectionData.view[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    _projectionData.proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, _sphereRadius * 10.0f);
    _projectionData.proj[1][1] *= -1.0f;
    _projectionData.proj[0][0] *= -1.0f;
    
    _projectionDataBuffer = std::unique_ptr<vulkan::Buffer>(vulkan::Buffer::createAllocatedBuffer(
        _vulkan,
        sizeof(ProjectionData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    ));
    
    auto projectionDataBufferPtr = reinterpret_cast<ProjectionData*>(_projectionDataBuffer->allocatedData());
    *projectionDataBufferPtr = _projectionData;
    
    updateImage(texture);
    initImages(cubeImageSize);
    initPipeline(vertexShader, fragmentShader);
    initGeometry();
}

void SphereToCubemapRenderer::updateImage(const std::filesystem::path& imagePath)
{
    if (_skyTexture.get())
    {
        _skyTexture = nullptr;
    }
    
    auto texture = new bg2e::base::Texture(imagePath);
    texture->setMagFilter(bg2e::base::Texture::FilterLinear);
    texture->setMinFilter(bg2e::base::Texture::FilterLinear);

    _skyTexture = std::shared_ptr<bg2e::render::Texture>(new bg2e::render::Texture(
        _vulkan,
        texture
    ));
}

void SphereToCubemapRenderer::updateImage(std::shared_ptr<render::Texture> texture)
{
    if (_skyTexture.get())
    {
        _skyTexture.reset();
    }
    
    _skyTexture = texture;
}
    
void SphereToCubemapRenderer::update(VkCommandBuffer commandBuffer, vulkan::FrameResources& frameResources)
{
    VkClearColorValue clearValue = { { 0.5f, 0.5f, 0.5f, 1.0f } };
    vulkan::macros::cmdClearImageAndSetLayout(commandBuffer, _cubeMapImage.get(), clearValue);
    
    auto descriptorSet = frameResources.newDescriptorSet(_dsLayout);
    descriptorSet->beginUpdate();
    descriptorSet->addBuffer(
        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _projectionDataBuffer.get(),
        sizeof(ProjectionData), 0
    );
    descriptorSet->addImage(
        1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        _skyTexture->image()->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        _skyTexture->sampler()
    );
    descriptorSet->endUpdate();
    std::array<VkDescriptorSet, 1> descSets = {
        descriptorSet->descriptorSet()
    };
    
    for (auto i = 0; i < 6; ++i)
    {
        auto view = _cubeMapImageViews[i];
        auto colorAttachment = vulkan::Info::attachmentInfo(view, nullptr);
        auto renderInfo = vulkan::Info::renderingInfo(_cubeMapImage->extent2D(), &colorAttachment, nullptr);
        vulkan::cmdBeginRendering(commandBuffer, &renderInfo);
        
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
        
        vulkan::macros::cmdSetDefaultViewportAndScissor(commandBuffer, _cubeMapImage->extent2D());
        
        RenderSpherePushConstant pushConstants = { .currentFace = i };
        vkCmdPushConstants(
            commandBuffer,
            _pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(RenderSpherePushConstant),
            &pushConstants
        );
        
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _pipelineLayout, 0,
            uint32_t(descSets.size()),
            descSets.data(),
            0, nullptr
        );
        
        _sphere->draw(commandBuffer);
        
        vulkan::cmdEndRendering(commandBuffer);
    }
    
    vulkan::Image::cmdTransitionImage(
        commandBuffer,
        _cubeMapImage->handle(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

void SphereToCubemapRenderer::cleanup()
{
    if (_projectionDataBuffer)
    {
        _projectionDataBuffer->cleanup();
    }
    
    if (_sphere)
    {
        _sphere.reset();
    }
    
    vkDestroyPipeline(_vulkan->device().handle(), _pipeline, nullptr);
    vkDestroyPipelineLayout(_vulkan->device().handle(), _pipelineLayout, nullptr);
    
    for (int32_t i = 0; i < 6; ++i)
    {
        vkDestroyImageView(_vulkan->device().handle(), _cubeMapImageViews[i], nullptr);
    }
    _cubeMapImage->cleanup();
    _skyTexture = nullptr;
    vkDestroyDescriptorSetLayout(_vulkan->device().handle(), _dsLayout, nullptr);
}

void SphereToCubemapRenderer::initImages(VkExtent2D extent)
{
    _cubeMapImage = std::shared_ptr<bg2e::render::vulkan::Image>(bg2e::render::vulkan::Image::createAllocatedImage(
        _vulkan,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        extent,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        6
    ));
    
    auto viewInfo = bg2e::render::vulkan::Info::imageViewCreateInfo(
        VK_FORMAT_R16G16B16A16_SFLOAT,
        _cubeMapImage->handle(),
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    
    VkImageView imgView;
    for (int32_t i = 0; i < 6; ++i)
    {
        viewInfo.subresourceRange.baseArrayLayer = i;
        vkCreateImageView(_vulkan->device().handle(), &viewInfo, nullptr, &imgView);
        _cubeMapImageViews[i] = imgView;
    }
}

void SphereToCubemapRenderer::initPipeline(const std::string& vshaderFile, const std::string& fshaderFile)
{
    vulkan::factory::GraphicsPipeline plFactory(_vulkan);
    
    plFactory.addShader(vshaderFile, VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader(fshaderFile, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    plFactory.setInputState<vulkan::geo::MeshPU>();
    
    vulkan::factory::PipelineLayout layoutFactory(_vulkan);
    layoutFactory.addPushConstantRange(
        0,
        sizeof(RenderSpherePushConstant),
        VK_SHADER_STAGE_VERTEX_BIT
    );
    layoutFactory.addDescriptorSetLayout(_dsLayout);
    _pipelineLayout = layoutFactory.build();
    
    plFactory.setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
    plFactory.disableDepthtest();
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(true, VK_FRONT_FACE_CLOCKWISE);
    
    _pipeline = plFactory.build(_pipelineLayout);
}

void SphereToCubemapRenderer::initGeometry()
{
    auto mesh = std::unique_ptr<bg2e::geo::MeshPU>(
        bg2e::geo::createSpherePU(_sphereRadius, 20, 20, true)
    );
    
    _sphere = std::unique_ptr<vulkan::geo::MeshPU>(new vulkan::geo::MeshPU(_vulkan));
    _sphere->setMeshData(mesh.get());
    _sphere->build();
    
    
}

}
