#include <bg2e/render/RendererBasicForward.hpp>
#include <bg2e/scene/SkyDomeTextureGenerator.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>

namespace bg2e::render {

void RendererBasicForward::build(
    bg2e::render::Engine* engine
) {
    _engine = engine;

    _colorAttachments = std::unique_ptr<bg2e::render::ColorAttachments>(
        new bg2e::render::ColorAttachments(_engine, {
            _colorAttachmentFormat
        })
    );

    _frameDataBinding = std::make_unique<bg2e::scene::vk::FrameDataBinding>(_engine);
    _objectDataBinding = std::make_unique<bg2e::scene::vk::ObjectDataBinding>(_engine);
    _environmentDataBinding = std::make_unique<bg2e::scene::vk::EnvironmentDataBinding>(_engine);
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

    // Call the resizeViewportVisitor to set the initial viewport size in cameras
    _resizeVisitor.resizeViewport(_scene->rootNode(), _engine->swapchain().extent());

    _engine->cleanupManager().push([&](VkDevice) {
        _scene.reset();
    });
}

void RendererBasicForward::resize(
    VkExtent2D newExtent
) {
    // This function releases all previous resources before recreating the images
    _colorAttachments->build(newExtent);

    _resizeVisitor.resizeViewport(_scene->rootNode(), newExtent);
}

void RendererBasicForward::update(
    float delta
) {
    _updateVisitor.update(_scene->rootNode(), delta);

    if(_scene->mainEnvironment() && _scene->mainEnvironment()->imgHash() != _skyImageHash) {
        _environment->swapEnvironmentTexture(_scene->mainEnvironment()->environmentImage());
        _skyImageHash = _scene->mainEnvironment()->imgHash();
    }
}

void RendererBasicForward::draw(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const bg2e::render::vulkan::Image* depthImage,
    bg2e::render::vulkan::FrameResources& frameResources
) {
    using namespace bg2e::render::vulkan;
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
    projMatrix[1][1] *= -1.0f; // Flip Vulkan Y coord
    auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();
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
                envDS
            };
        }
    );

    vulkan::cmdEndRendering(cmd);
}

void RendererBasicForward::cleanup() {
    _colorAttachments->cleanup();
}

void RendererBasicForward::createPipeline(bg2e::render::Engine* engine) {
    bg2e::render::vulkan::factory::GraphicsPipeline plFactory(engine);

    plFactory.addShader("test/texture_gi.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("test/texture_gi.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    plFactory.setInputState<bg2e::render::vulkan::geo::MeshPNU>();

    auto frameDSLayout = frameDataBinding()->createLayout();
    auto objectDSLayout = objectDataBinding()->createLayout();
    auto envDSLayout = environmentDataBinding()->createLayout();

    bg2e::render::vulkan::factory::PipelineLayout layoutFactory(engine);
    layoutFactory.addDescriptorSetLayout(frameDSLayout);
    layoutFactory.addDescriptorSetLayout(objectDSLayout);
    layoutFactory.addDescriptorSetLayout(envDSLayout);
    _pipelineLayout = layoutFactory.build();

    plFactory.setDepthFormat(engine->swapchain().depthImageFormat());
    plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(false, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    plFactory.setColorAttachmentFormat(colorAttachments()->attachmentFormats());
    _pipeline = plFactory.build(_pipelineLayout);

    engine->cleanupManager().push([&, objectDSLayout, envDSLayout, frameDSLayout](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, objectDSLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, envDSLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, frameDSLayout, nullptr);
    });
}

}
