#include <bg2e/render/IntegratedBakerContext.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/EnvironmentResources.hpp>
#include <bg2e/render/deferred/RTAmbientOcclusion.hpp>
#include <bg2e/render/deferred/RTGlobalIllumination.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>
#include <bg2e/render/vulkan/rt/ReflectionLightDataBinding.hpp>
#include <bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp>
#include "UvSurfacePass.hpp"
#include "LightmapCompositionPass.hpp"
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/vk/LightDataBinding.hpp>

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bg2e {
namespace render {

struct IntegratedBakerContext::SceneBindingSnapshot
{
    scene::vk::LightDataBinding::LightUniforms lights{};
    std::string environmentImage;
    size_t environmentImageHash = 0;
};

IntegratedLightmapBaker::IntegratedLightmapBaker(
    std::shared_ptr<BakerContext> context,
    std::shared_ptr<scene::Node> targetNode,
    LightmapSettings settings)
    : LightmapBaker(std::move(context), std::move(targetNode), settings)
{
    if (_settings.mode == LightmapMode::RTGI && _settings.giBounces == 0)
    {
        throw std::invalid_argument("IntegratedLightmapBaker: giBounces must be positive for RTGI");
    }
}

void IntegratedLightmapBaker::update(VkCommandBuffer cmd, vulkan::FrameResources& frameResources)
{
    auto* context = static_cast<IntegratedBakerContext*>(_context.get());
    auto& rayTracingScene = context->requirePreparedFrame(cmd, frameResources);
    (void)rayTracingScene;

    const uint32_t frameNumber = _context->engine()->currentFrame();
    if (_hasUpdatedFrame && _lastUpdatedFrame == frameNumber)
    {
        throw std::logic_error("IntegratedLightmapBaker::update: baker already updated for this frame");
    }
    if (_completedFrames >= _settings.accumulationFrames)
    {
        throw std::logic_error("IntegratedLightmapBaker::update: accumulation is complete; reset before another update");
    }

    if (!_targetNode || _targetNode->sceneRoot() != context->rootNode())
    {
        throw std::invalid_argument("IntegratedLightmapBaker::update: target no longer belongs to the context root");
    }
    if (_settings.mode == LightmapMode::RTGI && !context->_environmentResources)
    {
        throw std::logic_error(
            "IntegratedLightmapBaker::update: RTGI requires the active renderer's EnvironmentResources; "
            "call IntegratedBakerContext::setEnvironmentResources before baking");
    }
    std::shared_ptr<vulkan::Image> irradianceMap;
    VkSampler irradianceSampler = VK_NULL_HANDLE;
    if (_settings.mode == LightmapMode::RTGI)
    {
        irradianceMap = context->_environmentResources->irradianceMapImage();
        irradianceSampler = context->_environmentResources->irradianceMapSampler();
        if (!irradianceMap || irradianceSampler == VK_NULL_HANDLE)
        {
            throw std::logic_error("IntegratedLightmapBaker::update: active environment has no usable irradiance map");
        }
    }

    const uint32_t frameSlot = _context->engine()->currentFrameResourcesIndex();
    recordUvSurface(cmd, frameSlot);
    context->initializeBakeDescriptorAllocator(frameSlot);
    auto& descriptorAllocator = context->bakeDescriptorAllocator(frameSlot);
    const auto& uvSurface = _uvSurfacePass->manager(frameSlot);
    const auto& sceneBindings = context->sceneBindingSnapshot();

    std::vector<base::LightData> giLights;
    giLights.reserve(sceneBindings.lights.lightCount);
    for (uint32_t lightIndex = 0; lightIndex < sceneBindings.lights.lightCount; ++lightIndex)
    {
        const auto& light = sceneBindings.lights.lights[lightIndex];
        if (light.affectsReflections != 0)
        {
            giLights.push_back(light);
        }
    }

    if (_settings.mode == LightmapMode::RTAO)
    {
        context->rtaoPass().renderUv(
            cmd, frameNumber, frameResources, descriptorAllocator,
            uvSurface, rayTracingScene, aoImage(frameSlot),
            _settings.samplesPerPixel, 0.1f);
        context->rtgiPass().clearUv(cmd, giImage(frameSlot));
    }
    else
    {
        context->rtaoPass().clearUvNeutral(cmd, aoImage(frameSlot));
        context->rtgiPass().renderUv(
            cmd, frameNumber, frameResources, descriptorAllocator,
            uvSurface, rayTracingScene, giImage(frameSlot),
            irradianceMap.get(), irradianceSampler,
            giLights, _settings);
    }

    context->compositionPass().render(
        cmd, descriptorAllocator, uvSurface,
        aoImage(frameSlot), giImage(frameSlot),
        resultImage(frameSlot), _settings.mode);
    recordAccumulation(cmd, descriptorAllocator, frameSlot);
    markResultImage(frameSlot);
    _lastUpdatedFrame = frameNumber;
    _hasUpdatedFrame = true;
}

IntegratedBakerContext::IntegratedBakerContext(Engine* engine, scene::Node* rootNode)
    : BakerContext(engine, rootNode)
{
    _sceneBindingSnapshot = std::make_unique<SceneBindingSnapshot>();
    _rtMaterialDataBinding = std::make_unique<vulkan::rt::RTMaterialDataBinding>(engine);
    _rtMaterialDataBinding->createLayout(
        VK_SHADER_STAGE_FRAGMENT_BIT |
        VK_SHADER_STAGE_COMPUTE_BIT |
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
    );
    _rtaoPass = std::make_unique<deferred::RTAmbientOcclusion>(engine);
    _rtaoPass->setMaterialDataBinding(_rtMaterialDataBinding.get());
    _rtaoPass->buildUv();

    _giLightDataBinding = std::make_unique<vulkan::rt::ReflectionLightDataBinding>(engine);
    _giLightDataBinding->createLayout(
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT);
    _rtgiPass = std::make_unique<deferred::RTGlobalIllumination>(engine);
    _rtgiPass->setMaterialDataBinding(_rtMaterialDataBinding.get());
    _rtgiPass->setReflectionLightDataBinding(_giLightDataBinding.get());
    _rtgiPass->buildUv();
    _compositionPass = std::make_unique<LightmapCompositionPass>(engine);

    const uint32_t frameSlotCount = engine->numImages();
    _bakeRayTracingScenes.resize(frameSlotCount);
    _bakeDescriptorAllocators.resize(frameSlotCount);
    _bakeDescriptorPoolsInitialized.resize(frameSlotCount, false);
    for (uint32_t slot = 0; slot < frameSlotCount; ++slot)
    {
        _bakeDescriptorAllocators[slot] = std::make_unique<vulkan::DescriptorSetAllocator>();
        _bakeDescriptorAllocators[slot]->init(engine);
        _rtMaterialDataBinding->initFrameResources(_bakeDescriptorAllocators[slot].get());
        _giLightDataBinding->initFrameResources(_bakeDescriptorAllocators[slot].get());
        _bakeDescriptorAllocators[slot]->requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
            { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
        });
        _bakeDescriptorAllocators[slot]->requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
            { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
        });
        _bakeDescriptorAllocators[slot]->requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
        });
        _bakeDescriptorAllocators[slot]->requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
        });
    }
}

IntegratedBakerContext::~IntegratedBakerContext()
{
    if (_bakeRayTracingScenes.empty() && _bakeDescriptorAllocators.empty() && !_rtMaterialDataBinding)
    {
        return;
    }

    struct DeferredResources
    {
        std::vector<std::unique_ptr<vulkan::rt::RayTracingScene>> scenes;
        std::vector<std::unique_ptr<vulkan::DescriptorSetAllocator>> descriptorAllocators;
        std::unique_ptr<vulkan::rt::RTMaterialDataBinding> materialDataBinding;
        std::unique_ptr<vulkan::rt::ReflectionLightDataBinding> giLightDataBinding;
        std::unique_ptr<deferred::RTAmbientOcclusion> rtaoPass;
        std::unique_ptr<deferred::RTGlobalIllumination> rtgiPass;
        std::unique_ptr<LightmapCompositionPass> compositionPass;
    };

    // A slot's TLAS and build buffers may still be referenced by a submitted
    // command buffer. Keep the TLASes, per-slot descriptor pools and their
    // shared material binding alive through the engine's deferred cleanup horizon.
    auto retained = std::make_shared<DeferredResources>();
    retained->scenes = std::move(_bakeRayTracingScenes);
    retained->descriptorAllocators = std::move(_bakeDescriptorAllocators);
    retained->materialDataBinding = std::move(_rtMaterialDataBinding);
    retained->giLightDataBinding = std::move(_giLightDataBinding);
    retained->rtaoPass = std::move(_rtaoPass);
    retained->rtgiPass = std::move(_rtgiPass);
    retained->compositionPass = std::move(_compositionPass);
    engine()->deferredExec([retained = std::move(retained)]() {
        retained->scenes.clear();
        retained->descriptorAllocators.clear();
        if (retained->rtgiPass)
        {
            retained->rtgiPass->cleanupUv();
            retained->rtgiPass.reset();
        }
        if (retained->compositionPass)
        {
            retained->compositionPass->cleanup();
            retained->compositionPass.reset();
        }
        if (retained->rtaoPass)
        {
            retained->rtaoPass->cleanupUv();
            retained->rtaoPass.reset();
        }
        if (retained->materialDataBinding)
        {
            retained->materialDataBinding->cleanup();
            retained->materialDataBinding.reset();
        }
        if (retained->giLightDataBinding)
        {
            retained->giLightDataBinding->cleanup();
            retained->giLightDataBinding.reset();
        }
    });
}

void IntegratedBakerContext::prepareFrame(
    VkCommandBuffer cmd,
    vulkan::FrameResources& frameResources)
{
    Engine* renderEngine = engine();
    if (cmd == VK_NULL_HANDLE)
    {
        throw std::invalid_argument("IntegratedBakerContext::prepareFrame: command buffer must not be null");
    }
    if (&frameResources != &renderEngine->currentFrameResources())
    {
        throw std::invalid_argument("IntegratedBakerContext::prepareFrame: frame resources are not the engine's current frame");
    }
    if (cmd != frameResources.commandBuffer)
    {
        throw std::invalid_argument("IntegratedBakerContext::prepareFrame: command buffer does not belong to the supplied frame");
    }
    if (!frameResources.descriptorAllocator)
    {
        throw std::logic_error("IntegratedBakerContext::prepareFrame: frame descriptor allocator is unavailable");
    }

    const uint32_t frameNumber = renderEngine->currentFrame();
    if (_hasPreparedAnyFrame && _lastPreparedFrame == frameNumber)
    {
        throw std::logic_error("IntegratedBakerContext::prepareFrame: context already prepared for this frame");
    }

    const uint32_t frameSlot = renderEngine->currentFrameResourcesIndex();
    if (frameSlot >= renderEngine->numImages())
    {
        throw std::out_of_range("IntegratedBakerContext::prepareFrame: engine returned an invalid frame slot");
    }
    if (!rootNode())
    {
        throw std::logic_error("IntegratedBakerContext::prepareFrame: context root is unavailable");
    }

    captureSceneBindings();

    _preparedFrame.valid = false;
    if (_bakeRayTracingScenes.size() < renderEngine->numImages())
    {
        _bakeRayTracingScenes.resize(renderEngine->numImages());
    }

    auto& scene = _bakeRayTracingScenes[frameSlot];
    _bakeDescriptorAllocators[frameSlot]->clearDescriptors();
    if (!scene)
    {
        scene = std::make_unique<vulkan::rt::RayTracingScene>(renderEngine);
    }
    else
    {
        // RenderLoop has waited for this slot's fence before recording. Only
        // this slot is safe to recycle; other slot TLASes may still be in use.
        scene->cleanup();
    }

    if (!scene->update(cmd, rootNode()))
    {
        throw std::runtime_error("IntegratedBakerContext::prepareFrame: failed to build the context bake TLAS");
    }
    if (scene->tlas() == VK_NULL_HANDLE || scene->objectInstances().empty())
    {
        scene->cleanup();
        throw std::runtime_error("IntegratedBakerContext::prepareFrame: context root contains no traceable instances");
    }

    _preparedFrame = { true, frameNumber, frameSlot, cmd };
    _lastPreparedFrame = frameNumber;
    _hasPreparedAnyFrame = true;
}

void IntegratedBakerContext::setEnvironmentResources(EnvironmentResources* environmentResources)
{
    _environmentResources = environmentResources;
}

vulkan::rt::RayTracingScene& IntegratedBakerContext::requirePreparedFrame(
    VkCommandBuffer cmd,
    vulkan::FrameResources& frameResources)
{
    Engine* renderEngine = engine();
    if (cmd == VK_NULL_HANDLE || cmd != frameResources.commandBuffer)
    {
        throw std::invalid_argument("IntegratedLightmapBaker::update: command buffer does not match the supplied frame");
    }
    if (&frameResources != &renderEngine->currentFrameResources())
    {
        throw std::invalid_argument("IntegratedLightmapBaker::update: frame resources are not the engine's current frame");
    }
    if (!frameResources.descriptorAllocator)
    {
        throw std::logic_error("IntegratedLightmapBaker::update: frame descriptor allocator is unavailable");
    }

    const uint32_t frameNumber = renderEngine->currentFrame();
    const uint32_t frameSlot = renderEngine->currentFrameResourcesIndex();
    if (!_preparedFrame.valid ||
        _preparedFrame.frameNumber != frameNumber ||
        _preparedFrame.frameSlot != frameSlot ||
        _preparedFrame.commandBuffer != cmd)
    {
        throw std::logic_error("IntegratedLightmapBaker::update: prepareFrame must succeed for this frame and command buffer first");
    }
    if (frameSlot >= _bakeRayTracingScenes.size() || !_bakeRayTracingScenes[frameSlot])
    {
        throw std::logic_error("IntegratedLightmapBaker::update: context bake TLAS is unavailable");
    }

    auto& scene = *_bakeRayTracingScenes[frameSlot];
    if (scene.tlas() == VK_NULL_HANDLE || scene.objectInstances().empty())
    {
        throw std::logic_error("IntegratedLightmapBaker::update: context bake TLAS is empty");
    }
    return scene;
}

vulkan::DescriptorSetAllocator& IntegratedBakerContext::bakeDescriptorAllocator(uint32_t frameSlot)
{
    if (frameSlot >= _bakeDescriptorAllocators.size() || !_bakeDescriptorAllocators[frameSlot])
    {
        throw std::out_of_range("IntegratedBakerContext: bake descriptor allocator slot is invalid");
    }
    return *_bakeDescriptorAllocators[frameSlot];
}

void IntegratedBakerContext::initializeBakeDescriptorAllocator(uint32_t frameSlot)
{
    auto& allocator = bakeDescriptorAllocator(frameSlot);
    if (!_bakeDescriptorPoolsInitialized[frameSlot])
    {
        allocator.initPool();
        _bakeDescriptorPoolsInitialized[frameSlot] = true;
    }
}

vulkan::rt::RTMaterialDataBinding& IntegratedBakerContext::rtMaterialDataBinding()
{
    if (!_rtMaterialDataBinding)
    {
        throw std::logic_error("IntegratedBakerContext: RT material data binding is unavailable");
    }
    return *_rtMaterialDataBinding;
}

deferred::RTAmbientOcclusion& IntegratedBakerContext::rtaoPass()
{
    if (!_rtaoPass)
    {
        throw std::logic_error("IntegratedBakerContext: UV RTAO pass is unavailable");
    }
    return *_rtaoPass;
}

deferred::RTGlobalIllumination& IntegratedBakerContext::rtgiPass()
{
    if (!_rtgiPass)
    {
        throw std::logic_error("IntegratedBakerContext: UV RTGI pass is unavailable");
    }
    return *_rtgiPass;
}

LightmapCompositionPass& IntegratedBakerContext::compositionPass()
{
    if (!_compositionPass)
    {
        throw std::logic_error("IntegratedBakerContext: lightmap composition pass is unavailable");
    }
    return *_compositionPass;
}

vulkan::rt::ReflectionLightDataBinding& IntegratedBakerContext::giLightDataBinding()
{
    if (!_giLightDataBinding)
    {
        throw std::logic_error("IntegratedBakerContext: GI light data binding is unavailable");
    }
    return *_giLightDataBinding;
}

void IntegratedBakerContext::captureSceneBindings()
{
    if (!_sceneBindingSnapshot)
    {
        throw std::logic_error("IntegratedBakerContext: scene binding snapshot is unavailable");
    }

    auto& snapshot = *_sceneBindingSnapshot;
    snapshot.lights = {};
    snapshot.environmentImage.clear();
    snapshot.environmentImageHash = 0;

    std::vector<std::shared_ptr<scene::LightComponent>> lightComponents;
    scene::EnvironmentComponent* environment = nullptr;
    if (auto* sceneData = rootNode()->scene())
    {
        lightComponents = sceneData->lightComponents();
        environment = sceneData->mainEnvironment();
    }
    else
    {
        std::function<void(scene::Node*)> collect = [&](scene::Node* node) {
            if (!node || !node->enabled())
            {
                return;
            }

            if (auto* light = node->light())
            {
                auto component = std::dynamic_pointer_cast<scene::LightComponent>(light->shared_from_this());
                if (component)
                {
                    lightComponents.push_back(std::move(component));
                }
            }
            if (!environment && node->environment())
            {
                environment = node->environment();
            }
            for (const auto& child : node->children())
            {
                collect(child.get());
            }
        };
        collect(rootNode());
    }

    const uint32_t lightCount = static_cast<uint32_t>(std::min<size_t>(
        lightComponents.size(), BG2E_MAX_FORWARD_LIGHTS));
    snapshot.lights.lightCount = lightCount;
    for (uint32_t index = 0; index < lightCount; ++index)
    {
        const auto& component = lightComponents[index];
        auto& output = snapshot.lights.lights[index];
        output.type = component->light().type();
        output.color = component->light().color();
        output.intensity = component->light().intensity();
        output.position = component->position();
        output.direction = component->direction();
        output.spotAngle = component->light().spotAngle();
        output.spotCutoff = component->light().spotCutoff();
        output.castShadows = component->light().castShadows() ? 1 : 0;
        output.sourceSize = component->light().sourceSize();
        output.shadowSamples = static_cast<int32_t>(component->light().shadowSamples());
        output.affectsReflections = component->light().affectsReflections() ? 1 : 0;
    }

    if (environment)
    {
        snapshot.environmentImage = environment->environmentImage();
        snapshot.environmentImageHash = environment->imgHash();
    }
}

const IntegratedBakerContext::SceneBindingSnapshot& IntegratedBakerContext::sceneBindingSnapshot() const
{
    if (!_sceneBindingSnapshot)
    {
        throw std::logic_error("IntegratedBakerContext: scene binding snapshot is unavailable");
    }
    return *_sceneBindingSnapshot;
}

std::unique_ptr<IntegratedLightmapBaker> IntegratedBakerContext::createBaker(
    std::shared_ptr<scene::Node> targetNode,
    LightmapSettings settings)
{
    return std::unique_ptr<IntegratedLightmapBaker>(
        new IntegratedLightmapBaker(shared_from_this(), std::move(targetNode), settings));
}

}
}
