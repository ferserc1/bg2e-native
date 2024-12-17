
#include <bg2e/render/SkyboxRenderer.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/render/vulkan/macros/descriptor_set.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>

namespace bg2e::render {

SkyboxRenderer::SkyboxRenderer(Vulkan * vulkan)
    :_vulkan { vulkan }
{

}
    
void SkyboxRenderer::initFrameResources(vulkan::DescriptorSetAllocator* allocator)
{
    allocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    });
}

void SkyboxRenderer::build(
    std::shared_ptr<Texture>&& skyTexture,
    VkFormat colorAttachmentFormat,
    const std::string& vshaderFile,
    const std::string& fshaderFile
) {
    _skyTexture = skyTexture;
    
    auto cubeMesh = std::unique_ptr<bg2e::geo::MeshP>(
        bg2e::geo::createCubeP(_cubeSize, _cubeSize, _cubeSize, true)
    );
    
    _cube = std::shared_ptr<vulkan::geo::MeshP>(new vulkan::geo::MeshP(_vulkan));
    _cube->setMeshData(cubeMesh.get());
    _cube->build();
    
    vulkan::factory::DescriptorSetLayout dsFactory;
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _dsLayout = dsFactory.build(
        _vulkan->device().handle(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    );
    
    vulkan::factory::PipelineLayout layoutFactory(_vulkan);
    layoutFactory.addDescriptorSetLayout(_dsLayout);
    _pipelineLayout = layoutFactory.build();
    
    vulkan::factory::GraphicsPipeline plFactory(_vulkan);
    plFactory.addShader(vshaderFile, VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader(fshaderFile, VK_SHADER_STAGE_FRAGMENT_BIT);
    plFactory.setInputState<vulkan::geo::MeshP>();
    plFactory.setColorAttachmentFormat(colorAttachmentFormat);
    plFactory.disableDepthtest();
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(true, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    _pipeline = plFactory.build(_pipelineLayout);
}

void SkyboxRenderer::update(const glm::mat4& view, const glm::mat4& proj)
{
    _skyData.view = view;
    _skyData.proj = proj;
}

void SkyboxRenderer::draw(
    VkCommandBuffer commandBuffer,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources
) {
    auto skyDataBuffer = vulkan::macros::createBuffer(_vulkan, frameResources, _skyData);
    
    auto descriptorSet = frameResources.newDescriptorSet(_dsLayout);
    descriptorSet->beginUpdate();
        descriptorSet->addBuffer(
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            skyDataBuffer, sizeof(SkyData), 0
        );
        descriptorSet->addImage(
            1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            _skyTexture->image(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            _skyTexture->sampler()
        );
    descriptorSet->endUpdate();
    std::array<VkDescriptorSet, 1> ds = { descriptorSet->descriptorSet() };
    
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pipelineLayout,
        0,
        uint32_t(ds.size()),
        ds.data(),
        0, nullptr
    );
    
    _cube->draw(commandBuffer);
}

void SkyboxRenderer::cleanup()
{
    vkDestroyPipeline(_vulkan->device().handle(), _pipeline, nullptr);
    vkDestroyPipelineLayout(_vulkan->device().handle(), _pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(_vulkan->device().handle(), _dsLayout, nullptr);
    _cube->cleanup();
}
    

}
