/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/render/RendererBasicForward.hpp>
#include <bg2e/scene/SkyDomeTextureGenerator.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/base/PlatformTools.hpp>

#include "bg2e/render/vulkan/rt/RayTracingScene.hpp"

namespace bg2e::render {

void RendererBasicForward::build(
    bg2e::render::Engine* engine,
    VkExtent2D initialExtent,
    VkFormat colorImageFormat,
    VkFormat depthImageFormat,
    VkSampleCountFlagBits sampleCount,
    bool isOffscreen
) {
    _engine = engine;
    _viewportExtent = initialExtent;
    _colorImageFormat = colorImageFormat;
    _depthImageFormat = depthImageFormat;
    _sampleCount = sampleCount;
    _isOffscreen = isOffscreen;

    _frameDataBinding = std::make_unique<bg2e::scene::vk::FrameDataBinding>(_engine);
    _objectDataBinding = std::make_unique<bg2e::scene::vk::ObjectDataBinding>(_engine);
    _environmentDataBinding = std::make_unique<bg2e::scene::vk::EnvironmentDataBinding>(_engine);
    _lightDataBinding = std::make_unique<bg2e::scene::vk::LightDataBinding>(_engine);
    _rtDataBinding = std::make_unique<bg2e::render::vulkan::rt::RayTracingSceneDataBinding>(_engine);
    _environment = std::unique_ptr<bg2e::render::EnvironmentResources>(
        new bg2e::render::EnvironmentResources(
            _engine,
            { colorImageFormat },
            depthImageFormat,
            sampleCount
        )
    );

    if (!isOffscreen)
    {
        _selectionHighlight = std::make_unique<bg2e::manipulation::SelectionHighlight>();
        _selectionHighlight->init(engine);

    }

    createPipelines(engine);
}

void RendererBasicForward::initFrameResources(
    bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator
) {
    _frameDataBinding->initFrameResources(frameAllocator);
    _objectDataBinding->initFrameResources(frameAllocator);
    _environmentDataBinding->initFrameResources(frameAllocator);
    _lightDataBinding->initFrameResources(frameAllocator);
    _rtDataBinding->initFrameResources(frameAllocator);
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
    
    _scene->updateLights();

    // Call the resizeViewportVisitor to set the initial viewport size in cameras
    _resizeVisitor.resizeViewport(_scene->rootNode(), _viewportExtent);

    _engine->cleanupManager().push([&](VkDevice) {
        _scene.reset();
    });
}

void RendererBasicForward::resize(
    VkExtent2D newExtent
) {
    _viewportExtent = newExtent;
    _scene->willResize();

    _resizeVisitor.resizeViewport(_scene->rootNode(), newExtent);
    
    _scene->didResize();
}

void RendererBasicForward::update(
    float delta
) {
    updateScene(delta, BG2E_MAX_FORWARD_LIGHTS);
}

void RendererBasicForward::draw(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const bg2e::render::vulkan::Image* colorImage,
    [[maybe_unused]] const bg2e::render::vulkan::Image* depthImage,
    const bg2e::render::vulkan::Image* msaaDepthImage,
    bg2e::render::vulkan::FrameResources& frameResources,
    VkImageLayout & outColorImageLayout,
    VkImageLayout & outDepthImageLayout,
    VkImageLayout & outMsaaDepthImageLayout
)
{
    using namespace bg2e::render::vulkan;

    prepareSceneRender(cmd, currentFrame, frameResources);

    auto mainCamera = _scene->mainCamera();
    auto projMatrix = mainCamera->projectionMatrix();
    auto cameraWorldPos = mainCamera->ownerNode()->worldPosition();
    auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();

    VkClearColorValue clearValue{ { 0.0f, 0.0f, 0.0f, 1.0f } };
    macros::cmdClearImagesAndBeginRendering(
        cmd,
        { colorImage },
        clearValue, VK_IMAGE_LAYOUT_UNDEFINED,
        msaaDepthImage, 1.0f
    );

    macros::cmdSetDefaultViewportAndScissor(cmd, colorImage->extent2D());

    
    //viewMatrix = glm::lookAt(glm::vec3{ 0.0f, 0.0f, 10.0f }, glm::vec3{ 0.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f });
    auto sceneDS = _frameDataBinding->newDescriptorSet(
        frameResources,
        viewMatrix,
        projMatrix
    );

    if (drawSkybox()) {
        _environment->drawSkybox(cmd, currentFrame, frameResources);
    }

    auto envDS = _environmentDataBinding->newDescriptorSet(frameResources, _environment.get());
    
    auto lightDS = _lightDataBinding->newDescriptorSet(frameResources, _lightUniforms);

    auto tlas = frameResources.rayTracingScene->tlas();
    auto rtSceneDS =  tlas != nullptr ? _rtDataBinding->newDescriptorSet(
        frameResources,
        tlas
    ) : VK_NULL_HANDLE;

    static struct PushConstants pushConstants {
        .gamma = 2.2f,
        .brightness = 0.0,
        .contrast = 1.0f,
        .exposure = 1.0
    };
    pushConstants.brightness = _brightness;
    pushConstants.contrast = _contrast;
    pushConstants.exposure = _exposure;
    vkCmdPushConstants(
        cmd,
        _pipelineLayout,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushConstants),
        &pushConstants
    );

    auto dsFunction = [&](bg2e::render::MaterialBase * mat, const glm::mat4& transform, uint32_t /*submesh*/) {
        auto modelDS = _objectDataBinding->newDescriptorSet(
            frameResources,
            mat,
            transform
        );
        if (rtSceneDS)
        {
            return std::vector<VkDescriptorSet> {
                sceneDS,
                modelDS,
                envDS,
                lightDS,
                rtSceneDS
            };
        }
        else
        {
            return std::vector<VkDescriptorSet> {
                sceneDS,
                modelDS,
                envDS,
                lightDS
            };
        }
    };
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _opaquePipeline);
    _renderQueue.render(
        bg2e::render::RenderQueueType::Opaque,
        cmd,
        _pipelineLayout,
        dsFunction,
        cameraWorldPos
    );
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _transparentPipeline);
    _renderQueue.render(
        bg2e::render::RenderQueueType::Transparent,
        cmd,
        _pipelineLayout,
        dsFunction,
        cameraWorldPos
    );
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _solidTransparentPipeline);
    _renderQueue.render(
        bg2e::render::RenderQueueType::SolidTransparent,
        cmd,
        _pipelineLayout,
        dsFunction,
        cameraWorldPos
    );

    if (!_isOffscreen)
    {
        _selectionHighlight->draw(
            _scene->rootNode(),
            viewMatrix,
            projMatrix,
            cmd
        );
    }

    vulkan::cmdEndRendering(cmd);

    endSceneRender();

    outColorImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    outDepthImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    outMsaaDepthImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
}

void RendererBasicForward::cleanup()
{
    _renderQueue.cleanup();
    frameDataBinding()->cleanup();
    objectDataBinding()->cleanup();
    environmentDataBinding()->cleanup();
    lightDataBinding()->cleanup();
    if (rtDataBinding())
    {
        rtDataBinding()->cleanup();
    }
}

void RendererBasicForward::createPipelines(bg2e::render::Engine* engine) {
    auto frameDSLayout = frameDataBinding()->createLayout();
    auto objectDSLayout = objectDataBinding()->createLayout();
    auto envDSLayout = environmentDataBinding()->createLayout();
    auto lightDSLayout = lightDataBinding()->createLayout();
    auto rtDSLayout = rtDataBinding()->createLayout();

    bg2e::render::vulkan::factory::PipelineLayout layoutFactory(engine);
    layoutFactory.addDescriptorSetLayout(frameDSLayout);
    layoutFactory.addDescriptorSetLayout(objectDSLayout);
    layoutFactory.addDescriptorSetLayout(envDSLayout);
    layoutFactory.addDescriptorSetLayout(lightDSLayout);
    if (rtDSLayout)
    {
        layoutFactory.addDescriptorSetLayout(rtDSLayout);
    }

    layoutFactory.addPushConstantRange(
        0,
        sizeof(PushConstants),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );
    _pipelineLayout = layoutFactory.build();
    
    _opaquePipeline = createOpaquePipeline(engine, _pipelineLayout);
    _transparentPipeline = createTransparentPipeline(engine, _pipelineLayout);
    _solidTransparentPipeline = createSolidTransparentPipeline(engine, _pipelineLayout);

    engine->cleanupManager().push([&, objectDSLayout, envDSLayout, frameDSLayout, lightDSLayout, rtDSLayout](VkDevice dev) {
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
    });
}

VkPipeline RendererBasicForward::createOpaquePipeline(
    bg2e::render::Engine * engine,
    VkPipelineLayout //layout
) {
    bg2e::render::vulkan::factory::GraphicsPipeline plFactory(engine);

    plFactory.addShader("basic_forward.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    if (_engine->rayTracingSupported())
    {
        plFactory.addShader("basic_forward_rt_shadows.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    else
    {
        plFactory.addShader("basic_forward.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }


    plFactory.setInputState<bg2e::render::vulkan::geo::Mesh>();

    plFactory.setDepthFormat(_depthImageFormat);
    plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    _isOffscreen ? plFactory.disableMultisample() : plFactory.enableMultisample();
    plFactory.setColorAttachmentFormat(_colorImageFormat);
    auto result = plFactory.build(_pipelineLayout);

    engine->cleanupManager().push([&, result](VkDevice dev) {
        vkDestroyPipeline(dev, result, nullptr);
    });
    return result;
}

VkPipeline RendererBasicForward::createTransparentPipeline(
    bg2e::render::Engine * engine,
    VkPipelineLayout //layout
) {
    bg2e::render::vulkan::factory::GraphicsPipeline plFactory(engine);

    plFactory.addShader("basic_forward.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    if (_engine->rayTracingSupported())
    {
        plFactory.addShader("basic_forward_rt_shadows.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    else
    {
        plFactory.addShader("basic_forward.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    plFactory.setInputState<bg2e::render::vulkan::geo::Mesh>();

    plFactory.setDepthFormat(_depthImageFormat);
    plFactory.enableDepthtest(
        false,  // disable depth write for transparent objects
        VK_COMPARE_OP_LESS
    );
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    _isOffscreen ? plFactory.disableMultisample() : plFactory.enableMultisample();
    plFactory.setColorAttachmentFormat(_colorImageFormat);
    plFactory.enableBlendingAlphablend();
    auto result = plFactory.build(_pipelineLayout);

    engine->cleanupManager().push([&, result](VkDevice dev) {
        vkDestroyPipeline(dev, result, nullptr);
    });
    return result;
}

VkPipeline RendererBasicForward::createSolidTransparentPipeline(
    bg2e::render::Engine * engine,
    VkPipelineLayout //layout
) {
    bg2e::render::vulkan::factory::GraphicsPipeline plFactory(engine);

    plFactory.addShader("basic_forward.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    if (_engine->rayTracingSupported())
    {
        plFactory.addShader("basic_forward_rt_shadows.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    else
    {
        plFactory.addShader("basic_forward.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    plFactory.setInputState<bg2e::render::vulkan::geo::Mesh>();

    plFactory.setDepthFormat(_depthImageFormat);
    plFactory.enableDepthtest(
        false,  // disable depth write for solid transparent objects
        VK_COMPARE_OP_LESS
    );
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    // Back face culling enabled for solid transparent objects
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    _isOffscreen ? plFactory.disableMultisample() : plFactory.enableMultisample();
    plFactory.setColorAttachmentFormat(_colorImageFormat);
    plFactory.enableBlendingAlphablend();
    auto result = plFactory.build(_pipelineLayout);

    engine->cleanupManager().push([&, result](VkDevice dev) {
        vkDestroyPipeline(dev, result, nullptr);
    });
    return result;
}

void RendererBasicForward::updateLights(const std::vector<std::shared_ptr<bg2e::scene::LightComponent>>& lightComponents, uint32_t maxLights)
{
    auto lights = static_cast<uint32_t>(lightComponents.size() < maxLights
        ? lightComponents.size()
        : maxLights);
    _lightUniforms.lightCount = lights;
    for (uint32_t i = 0; i < lights; ++i)
    {
        auto comp = lightComponents[i];
        _lightUniforms.lights[i].type = comp->light().type();
        _lightUniforms.lights[i].color = comp->light().color();
        _lightUniforms.lights[i].intensity = comp->light().intensity();
        _lightUniforms.lights[i].position = comp->position();
        _lightUniforms.lights[i].direction = comp->direction();
        _lightUniforms.lights[i].spotAngle = comp->light().spotAngle();
        _lightUniforms.lights[i].spotCutoff = comp->light().spotCutoff();
    }
}

}
