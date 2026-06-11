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
        _gizmoAndSelectionRenderer = std::make_unique<bg2e::manipulation::GizmoAndSelectionRenderer>();
        _gizmoAndSelectionRenderer->init(engine, _sampleCount);

    }

    createPipelines(engine, false, _pipelineLayout, _opaquePipeline, _transparentPipeline, _solidTransparentPipeline);
    if (_engine->rayTracingSupported())
    {
        createPipelines(engine, true, _pipelineLayoutRT, _opaquePipelineRT, _transparentPipelineRT, _solidTransparentPipelineRT);
    }
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
    bool useRT = _engine->rayTracingSupported() && tlas != VK_NULL_HANDLE;
    VkPipelineLayout activeLayout = useRT ? _pipelineLayoutRT : _pipelineLayout;
    VkPipeline activeOpaque = useRT ? _opaquePipelineRT : _opaquePipeline;
    VkPipeline activeTransparent = useRT ? _transparentPipelineRT : _transparentPipeline;
    VkPipeline activeSolidTransparent = useRT ? _solidTransparentPipelineRT : _solidTransparentPipeline;

    auto rtSceneDS = useRT ? _rtDataBinding->newDescriptorSet(frameResources, tlas) : VK_NULL_HANDLE;

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
        activeLayout,
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
        if (useRT)
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
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activeOpaque);
    _renderQueue.render(
        bg2e::render::RenderQueueType::Opaque,
        cmd,
        activeLayout,
        dsFunction,
        cameraWorldPos
    );
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activeTransparent);
    _renderQueue.render(
        bg2e::render::RenderQueueType::Transparent,
        cmd,
        activeLayout,
        dsFunction,
        cameraWorldPos
    );
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activeSolidTransparent);
    _renderQueue.render(
        bg2e::render::RenderQueueType::SolidTransparent,
        cmd,
        activeLayout,
        dsFunction,
        cameraWorldPos
    );

    if (!_isOffscreen)
    {
        _gizmoAndSelectionRenderer->draw(
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

void RendererBasicForward::createPipelines(
    bg2e::render::Engine* engine,
    bool useRT,
    VkPipelineLayout& outLayout,
    VkPipeline& outOpaque,
    VkPipeline& outTransparent,
    VkPipeline& outSolidTransparent
) {
    auto frameDSLayout = frameDataBinding()->createLayout();
    auto objectDSLayout = objectDataBinding()->createLayout();
    auto envDSLayout = environmentDataBinding()->createLayout();
    auto lightDSLayout = lightDataBinding()->createLayout();

    bg2e::render::vulkan::factory::PipelineLayout layoutFactory(engine);
    layoutFactory.addDescriptorSetLayout(frameDSLayout);
    layoutFactory.addDescriptorSetLayout(objectDSLayout);
    layoutFactory.addDescriptorSetLayout(envDSLayout);
    layoutFactory.addDescriptorSetLayout(lightDSLayout);
    if (useRT)
    {
        layoutFactory.addDescriptorSetLayout(rtDataBinding()->createLayout());
    }

    layoutFactory.addPushConstantRange(
        0,
        sizeof(PushConstants),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );
    outLayout = layoutFactory.build("RendererBasicForward::PipelineLayout");

    auto createPipeline = [&](bool isTransparent, bool isSolidTransparent) -> VkPipeline {
        bg2e::render::vulkan::factory::GraphicsPipeline plFactory(engine);

        plFactory.addShader("basic_forward.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        if (useRT)
        {
            plFactory.addShader("basic_forward_rt_shadows.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        }
        else
        {
            plFactory.addShader("basic_forward.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        plFactory.setInputState<bg2e::render::vulkan::geo::Mesh>();

        plFactory.setDepthFormat(_depthImageFormat);
        plFactory.enableDepthtest(!isTransparent, VK_COMPARE_OP_LESS);
        plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        if (isSolidTransparent)
        {
            plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        }
        else if (isTransparent)
        {
            plFactory.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        }
        else
        {
            plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        }
        _isOffscreen ? plFactory.disableMultisample() : plFactory.enableMultisample();
        plFactory.setColorAttachmentFormat(_colorImageFormat);
        if (isTransparent || isSolidTransparent)
        {
            plFactory.enableBlendingAlphablend();
        }
        auto result = plFactory.build(outLayout, "RendererBasicForward::Pipeline");

        engine->cleanupManager().push([&, result](VkDevice dev) {
            vkDestroyPipeline(dev, result, nullptr);
        });
        return result;
    };

    outOpaque = createPipeline(false, false);
    outTransparent = createPipeline(true, false);
    outSolidTransparent = createPipeline(false, true);

    engine->cleanupManager().push([&, frameDSLayout, objectDSLayout, envDSLayout, lightDSLayout](VkDevice dev) {
        vkDestroyPipelineLayout(dev, outLayout, nullptr);
    });
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
        _lightUniforms.lights[i].castShadows = comp->light().castShadows() ? 1 : 0;
        _lightUniforms.lights[i].sourceSize = comp->light().sourceSize();
        _lightUniforms.lights[i].shadowSamples = static_cast<int32_t>(comp->light().shadowSamples());
    }
}

}
