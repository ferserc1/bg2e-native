#include <bg2e/render/RendererBasicForward.hpp>
#include <bg2e/scene/SkyDomeTextureGenerator.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/base/PlatformTools.hpp>

namespace bg2e::render {

void RendererBasicForward::build(
    bg2e::render::Engine* engine,
    VkFormat colorAttachmentFormat,
    VkFormat depthAttachmentFormat
) {
    _engine = engine;
    _colorAttachmentFormat = colorAttachmentFormat;
    _depthAttachmentFormat = depthAttachmentFormat;

    _colorAttachments = std::unique_ptr<bg2e::render::ColorAttachments>(
        new bg2e::render::ColorAttachments(_engine, {
            _colorAttachmentFormat
        })
    );

    _frameDataBinding = std::make_unique<bg2e::scene::vk::FrameDataBinding>(_engine);
    _objectDataBinding = std::make_unique<bg2e::scene::vk::ObjectDataBinding>(_engine);
    _environmentDataBinding = std::make_unique<bg2e::scene::vk::EnvironmentDataBinding>(_engine);
    _lightDataBinding = std::make_unique<bg2e::scene::vk::LightDataBinding>(_engine);
    _environment = std::make_unique<bg2e::render::EnvironmentResources>(
        _engine,
        _colorAttachments->attachmentFormats(),
        _engine->swapchain().depthImageFormat()
    );

    createPipeline(engine);
}

void RendererBasicForward::initFrameResources(
    bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator
) {
    _frameDataBinding->initFrameResources(frameAllocator);
    _objectDataBinding->initFrameResources(frameAllocator);
    _environmentDataBinding->initFrameResources(frameAllocator);
    _lightDataBinding->initFrameResources(frameAllocator);
    _environment->initFrameResources(frameAllocator);
}

void RendererBasicForward::initScene(
    std::shared_ptr<bg2e::scene::Node> sceneRoot
) {
    _scene = std::make_unique<bg2e::scene::Scene>();
    _scene->setSceneRoot(sceneRoot);

    auto skyDomeTexture = std::make_shared<bg2e::base::Texture>();
    auto skyDomeGenerator = new bg2e::scene::SkyDomeTextureGenerator(2048, 1024, 4);
    skyDomeTexture->setProceduralGenerator(skyDomeGenerator);
    skyDomeTexture->setUseMipmaps(false);
    auto envTexture = std::make_shared<bg2e::render::Texture>(_engine);
    envTexture->load(skyDomeTexture);
    _environment->build(
        envTexture,         // Equirectangular texture
        { 2048, 2048 },     // Cube map size
        { 32, 32 },         // Irradiance map size
        { 1024, 1024 }      // Specular reflection map size
    );
    _colorAttachments->build(_engine->swapchain().extent());
    
    _scene->updateLights();

    // Call the resizeViewportVisitor to set the initial viewport size in cameras
    _resizeVisitor.resizeViewport(_scene->rootNode(), _engine->swapchain().extent());

    _engine->cleanupManager().push([&](VkDevice) {
        _scene.reset();
    });
}

void RendererBasicForward::resize(
    VkExtent2D newExtent
) {
    _scene->willResize();
    
    // This function releases all previous resources before recreating the images
    _colorAttachments->build(newExtent);

    _resizeVisitor.resizeViewport(_scene->rootNode(), newExtent);
    
    _scene->didResize();
}

void RendererBasicForward::update(
    float delta
) {
    _scene->willUpdate();
    
    _updateVisitor.update(_scene->rootNode(), delta);

    if(_scene->mainEnvironment() && _scene->mainEnvironment()->imgHash() != _skyImageHash) {
        _environment->swapEnvironmentTexture(_scene->mainEnvironment()->environmentImage());
        _skyImageHash = _scene->mainEnvironment()->imgHash();
    }
    
    
    // Update light resources
    auto lightComponents = _scene->lights();
    uint32_t lights = static_cast<uint32_t>(lightComponents.size() < BG2E_MAX_FORWARD_LIGHTS
        ? lightComponents.size()
        : BG2E_MAX_FORWARD_LIGHTS);
    _lightUniforms.lightCount = lights;
    for (uint32_t i = 0; i < lights; ++i)
    {
        auto comp = lightComponents[i];
        _lightUniforms.lights[i].type = comp->light().type();
        _lightUniforms.lights[i].color = comp->light().color();
        _lightUniforms.lights[i].intensity = comp->light().intensity();
        _lightUniforms.lights[i].position = comp->position();
        _lightUniforms.lights[i].direction = comp->direction();
    }
    
    _scene->didUpdate();
}

void RendererBasicForward::draw(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const bg2e::render::vulkan::Image* depthImage,
    bg2e::render::vulkan::FrameResources& frameResources
) {
    using namespace bg2e::render::vulkan;
    _scene->willDraw();
    
    _environment->update(cmd, currentFrame, frameResources);

    VkClearColorValue clearValue{ { 0.0f, 0.0f, 0.0f, 1.0f } };
    macros::cmdClearImagesAndBeginRendering(
        cmd,
        _colorAttachments->images(),
        clearValue, VK_IMAGE_LAYOUT_UNDEFINED,
        depthImage, 1.0f
    );

    macros::cmdSetDefaultViewportAndScissor(cmd, _colorAttachments->extent());
    auto mainCamera = _scene->mainCamera();
    auto projMatrix = mainCamera->projectionMatrix();
    auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();    
    
    //viewMatrix = glm::lookAt(glm::vec3{ 0.0f, 0.0f, 10.0f }, glm::vec3{ 0.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f });
    auto sceneDS = _frameDataBinding->newDescriptorSet(
        frameResources,
        viewMatrix,
        projMatrix
    );

    if (_drawSkybox) {
        _environment->updateSkybox(viewMatrix, projMatrix);
        _environment->drawSkybox(cmd, currentFrame, frameResources);
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

    auto envDS = _environmentDataBinding->newDescriptorSet(frameResources, _environment.get());
    
    auto lightDS = _lightDataBinding->newDescriptorSet(frameResources, _lightUniforms);

    static struct PushConstants pushConstants {
    #if BG2E_IS_MAC
        .gamma = 2.2f // Default gamma value, can be changed later
    #else
        .gamma = 1.1f
    #endif
    };
    vkCmdPushConstants(
        cmd,
        _pipelineLayout,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushConstants),
        &pushConstants
    );
    _drawVisitor.draw(
        _scene->rootNode(),
        cmd,
        _pipelineLayout,
        [&](bg2e::render::MaterialBase* mat, const glm::mat4& transform, uint32_t submesh) {
            auto modelDS = _objectDataBinding->newDescriptorSet(
                frameResources,
                mat,
                transform
            );
            return std::vector<VkDescriptorSet>{
                sceneDS,
                modelDS,
                envDS,
                lightDS
            };
        }
    );

    vulkan::cmdEndRendering(cmd);
    _scene->didDraw();
}

void RendererBasicForward::cleanup() {
    _colorAttachments->cleanup();
}

bg2e::scene::Scene* RendererBasicForward::scene()
{
    return _scene.get();
}

void RendererBasicForward::createPipeline(bg2e::render::Engine* engine) {
    bg2e::render::vulkan::factory::GraphicsPipeline plFactory(engine);

    plFactory.addShader("basic_forward.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("basic_forward.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    plFactory.setInputState<bg2e::render::vulkan::geo::Mesh>();

    auto frameDSLayout = frameDataBinding()->createLayout();
    auto objectDSLayout = objectDataBinding()->createLayout();
    auto envDSLayout = environmentDataBinding()->createLayout();
    auto lightDSLayout = lightDataBinding()->createLayout();

    bg2e::render::vulkan::factory::PipelineLayout layoutFactory(engine);
    layoutFactory.addDescriptorSetLayout(frameDSLayout);
    layoutFactory.addDescriptorSetLayout(objectDSLayout);
    layoutFactory.addDescriptorSetLayout(envDSLayout);
    layoutFactory.addDescriptorSetLayout(lightDSLayout);
    layoutFactory.addPushConstantRange(
        0,
        sizeof(PushConstants),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );
    _pipelineLayout = layoutFactory.build();

    plFactory.setDepthFormat(engine->swapchain().depthImageFormat());
    plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    plFactory.setColorAttachmentFormat(colorAttachments()->attachmentFormats());
    _pipeline = plFactory.build(_pipelineLayout);

    engine->cleanupManager().push([&, objectDSLayout, envDSLayout, frameDSLayout, lightDSLayout](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, objectDSLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, envDSLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, frameDSLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, lightDSLayout, nullptr);
    });
}

}
