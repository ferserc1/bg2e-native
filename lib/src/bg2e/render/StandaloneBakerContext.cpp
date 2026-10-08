#include <bg2e/render/StandaloneBakerContext.hpp>

#include <bg2e/render/EnvironmentResources.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/deferred/RTAmbientOcclusion.hpp>
#include <bg2e/render/deferred/RTGlobalIllumination.hpp>
#include <bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp>
#include <bg2e/render/vulkan/rt/ReflectionLightDataBinding.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/ResizeViewportVisitor.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/SkyDomeTextureGenerator.hpp>
#include <bg2e/scene/UpdateVisitor.hpp>
#include <bg2e/base/Texture.hpp>
#include <bg2e/render/Texture.hpp>
#include <bg2e/scene/vk/LightDataBinding.hpp>

#include "LightmapBakeExecutor.hpp"
#include "LightmapCompositionPass.hpp"

#include <cmath>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace bg2e {
namespace render {
namespace {

scene::Node* requireSceneRoot(const std::shared_ptr<scene::Scene>& scene)
{
    if (!scene)
    {
        throw std::invalid_argument("StandaloneBakerContext: scene must not be null");
    }
    auto* root = scene->rootNode();
    if (!root)
    {
        throw std::invalid_argument("StandaloneBakerContext: scene must have a root node");
    }
    return root;
}

std::shared_ptr<render::Texture> defaultEnvironmentTexture(Engine* engine)
{
    auto source = std::make_shared<base::Texture>();
    source->setProceduralGenerator(
        std::make_shared<scene::SkyDomeTextureGenerator>(2048, 1024, 4));
    source->setUseMipmaps(false);

    auto texture = std::make_shared<render::Texture>(engine);
    texture->load(source);
    return texture;
}

}

struct StandaloneBakerContext::BakeResources {
    std::shared_ptr<vulkan::DescriptorSetAllocator> descriptorAllocator;
    std::shared_ptr<vulkan::rt::RTMaterialDataBinding> materialDataBinding;
    std::shared_ptr<vulkan::rt::ReflectionLightDataBinding> lightDataBinding;
    std::shared_ptr<deferred::RTAmbientOcclusion> rtaoPass;
    std::shared_ptr<deferred::RTGlobalIllumination> rtgiPass;
    std::shared_ptr<LightmapCompositionPass> compositionPass;
    std::shared_ptr<EnvironmentResources> environmentResources;

    ~BakeResources()
    {
        if (rtaoPass) rtaoPass->cleanup();
        if (rtgiPass) rtgiPass->cleanup();
        if (compositionPass) compositionPass->cleanup();
        if (materialDataBinding) materialDataBinding->cleanup();
        if (lightDataBinding) lightDataBinding->cleanup();
        if (descriptorAllocator) descriptorAllocator->destroy();
    }
};

StandaloneLightmapBaker::StandaloneLightmapBaker(
    std::shared_ptr<BakerContext> context,
    std::shared_ptr<scene::Node> targetNode,
    LightmapSettings settings,
    uint64_t sceneGeneration)
    : LightmapBaker(std::move(context), std::move(targetNode), settings),
      _sceneGeneration(sceneGeneration)
{
    if (_settings.mode == LightmapMode::RTGI && _settings.giBounces == 0)
    {
        throw std::invalid_argument("StandaloneLightmapBaker: giBounces must be positive for RTGI");
    }
}

void StandaloneLightmapBaker::update()
{
    auto* context = static_cast<StandaloneBakerContext*>(_context.get());
    context->beginBakerUpdate(_sceneGeneration);
    try
    {
        if (_completedFrames >= _settings.accumulationFrames)
        {
            throw std::logic_error(
                "StandaloneLightmapBaker::update: accumulation is complete; reset before another update");
        }
        if (!context->_frameResources || !context->_bakeResources ||
            !context->_bakeResources->descriptorAllocator || !context->_rayTracingScene ||
            !context->_frameResources->descriptorAllocator)
        {
            throw std::logic_error("StandaloneLightmapBaker::update: standalone bake resources are unavailable");
        }

        auto& frame = *context->_frameResources;
        auto& resources = *context->_bakeResources;
        const VkDevice device = context->engine()->device().handle();
        constexpr uint64_t infiniteTimeout = std::numeric_limits<uint64_t>::max();

        VK_ASSERT(vkWaitForFences(device, 1, &frame.frameFence, VK_TRUE, infiniteTimeout));
        VK_ASSERT(vkResetCommandBuffer(frame.commandBuffer, 0));
        const auto commandBeginInfo = vulkan::Info::commandBufferBeginInfo(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_ASSERT(vkBeginCommandBuffer(frame.commandBuffer, &commandBeginInfo));

        const uint32_t frameNumber = context->engine()->currentFrame();
        LightmapBakeExecutor::recordSample(
            *this, frame.commandBuffer, frameNumber, 0, frame,
            *resources.descriptorAllocator, *context->_rayTracingScene,
            *resources.rtaoPass, *resources.rtgiPass, *resources.compositionPass,
            resources.environmentResources->irradianceMapImage().get(),
            resources.environmentResources->irradianceMapSampler(), context->_sceneLights);
        VK_ASSERT(vkEndCommandBuffer(frame.commandBuffer));

        VK_ASSERT(vkResetFences(device, 1, &frame.frameFence));
        auto commandInfo = vulkan::Info::commandBufferSubmitInfo(frame.commandBuffer);
        auto submitInfo = vulkan::Info::submitInfo(&commandInfo, nullptr, nullptr);
        const VkResult submitResult = vulkan::queueSubmit2(
            context->engine()->command().graphicsQueue(), 1, &submitInfo, frame.frameFence);
        if (submitResult != VK_SUCCESS)
        {
            vkDestroyFence(device, frame.frameFence, nullptr);
            frame.frameFence = VK_NULL_HANDLE;
            const auto fenceInfo = vulkan::Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
            VK_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &frame.frameFence));
            throw std::runtime_error(
                std::string("StandaloneLightmapBaker::update: sample submission failed: ") +
                string_VkResult(submitResult));
        }

        const VkResult waitResult = vkWaitForFences(
            device, 1, &frame.frameFence, VK_TRUE, infiniteTimeout);
        if (waitResult != VK_SUCCESS)
        {
            context->engine()->device().waitIdle();
            vkDestroyFence(device, frame.frameFence, nullptr);
            frame.frameFence = VK_NULL_HANDLE;
            const auto fenceInfo = vulkan::Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
            VK_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &frame.frameFence));
            throw std::runtime_error(
                std::string("StandaloneLightmapBaker::update: waiting for sample failed: ") +
                string_VkResult(waitResult));
        }

        frame.cleanupManager.flush(context->engine()->device());
        frame.descriptorAllocator->clearDescriptors();
        resources.descriptorAllocator->clearDescriptors();
        markResultImage(0);
        context->engine()->nextFrame();
        context->endBakerUpdate();
    }
    catch (...)
    {
        context->endBakerUpdate();
        throw;
    }
}

StandaloneBakerContext::StandaloneBakerContext(
    Engine* engine,
    std::shared_ptr<scene::Scene> scene)
    : BakerContext(engine, requireSceneRoot(scene)),
      _scene(std::move(scene))
{
}

StandaloneBakerContext::~StandaloneBakerContext()
{
    cleanup();
}

void StandaloneBakerContext::initialize(VkExtent2D extent)
{
    requireState(State::Created, "initialize");
    if (extent.width == 0 || extent.height == 0)
    {
        throw std::invalid_argument("StandaloneBakerContext::initialize: extent dimensions must be positive");
    }
    requireCurrentSceneRoot("initialize");

    auto frameResources = std::make_unique<vulkan::FrameResources>();
    auto rayTracingScene = std::make_unique<vulkan::rt::RayTracingScene>(engine());
    try
    {
        frameResources->init(engine(), &engine()->command());
        frameResources->descriptorAllocator->init(engine());
    }
    catch (...)
    {
        frameResources->cleanup();
        throw;
    }

    _frameResources = std::move(frameResources);
    _rayTracingScene = std::move(rayTracingScene);
    _extent = extent;
    try
    {
        initializeBakeResources();
    }
    catch (...)
    {
        if (_rayTracingScene)
        {
            _rayTracingScene->cleanup();
            _rayTracingScene.reset();
        }
        if (_frameResources)
        {
            _frameResources->cleanup();
            _frameResources.reset();
        }
        throw;
    }
    _state = State::Initialized;
}

void StandaloneBakerContext::initializeBakeResources()
{
    if (!_frameResources)
    {
        throw std::logic_error("StandaloneBakerContext: frame resources are unavailable");
    }

    auto resources = std::make_shared<BakeResources>();

    resources->descriptorAllocator = std::make_shared<vulkan::DescriptorSetAllocator>();
    resources->descriptorAllocator->init(engine());
    resources->materialDataBinding = std::make_shared<vulkan::rt::RTMaterialDataBinding>(engine());
    resources->materialDataBinding->createLayout(
        VK_SHADER_STAGE_FRAGMENT_BIT |
        VK_SHADER_STAGE_COMPUTE_BIT |
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    resources->lightDataBinding = std::make_shared<vulkan::rt::ReflectionLightDataBinding>(engine());
    resources->lightDataBinding->createLayout(
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT);
    resources->materialDataBinding->initFrameResources(resources->descriptorAllocator.get());
    resources->lightDataBinding->initFrameResources(resources->descriptorAllocator.get());
    resources->descriptorAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
    });
    resources->descriptorAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
    });
    resources->descriptorAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
    });
    resources->descriptorAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
    });

    resources->rtaoPass = std::make_shared<deferred::RTAmbientOcclusion>(engine());
    resources->rtaoPass->setMaterialDataBinding(resources->materialDataBinding.get());
    resources->rtaoPass->buildUv();
    resources->rtgiPass = std::make_shared<deferred::RTGlobalIllumination>(engine());
    resources->rtgiPass->setMaterialDataBinding(resources->materialDataBinding.get());
    resources->rtgiPass->setReflectionLightDataBinding(resources->lightDataBinding.get());
    resources->rtgiPass->buildUv();
    resources->compositionPass = std::make_shared<LightmapCompositionPass>(engine());

    // EnvironmentResources registers cleanup callbacks that capture its owner.
    // Keep it alive until those callbacks run during Engine::cleanup().
    auto retainedEnvironment = std::make_shared<std::shared_ptr<EnvironmentResources>>();
    engine()->cleanupManager().push([retainedEnvironment](VkDevice) {
        retainedEnvironment->reset();
    });
    resources->environmentResources = std::make_shared<EnvironmentResources>(engine());
    *retainedEnvironment = resources->environmentResources;
    resources->environmentResources->initFrameResources(_frameResources->descriptorAllocator);
    resources->descriptorAllocator->initPool();
    _frameResources->descriptorAllocator->initPool();

    auto* environment = _scene->mainEnvironment();
    if (environment && !environment->environmentImage().empty())
    {
        _environmentImage = environment->environmentImage();
        _environmentImageHash = environment->imgHash();
        resources->environmentResources->build(_environmentImage);
    }
    else
    {
        _environmentImage = "__bg2e_default_environment__";
        _environmentImageHash = 0;
        resources->environmentResources->build(defaultEnvironmentTexture(engine()));
    }

    _bakeResources = std::move(resources);
}

void StandaloneBakerContext::refreshEnvironmentResource()
{
    if (!_bakeResources || !_bakeResources->environmentResources)
    {
        throw std::logic_error("StandaloneBakerContext: environment bake resources are unavailable");
    }

    auto* environment = _scene->mainEnvironment();
    const bool hasEnvironment = environment && !environment->environmentImage().empty();
    const std::string imagePath = hasEnvironment
        ? environment->environmentImage()
        : "__bg2e_default_environment__";
    const size_t imageHash = hasEnvironment ? environment->imgHash() : 0;
    if (imagePath == _environmentImage && imageHash == _environmentImageHash)
    {
        return;
    }

    if (hasEnvironment)
    {
        _bakeResources->environmentResources->swapEnvironmentTexture(imagePath);
    }
    else
    {
        _bakeResources->environmentResources->swapEnvironmentTexture(defaultEnvironmentTexture(engine()));
    }
    _environmentImage = imagePath;
    _environmentImageHash = imageHash;
}

void StandaloneBakerContext::captureSceneLights()
{
    _sceneLights.clear();
    const auto lightComponents = _scene->lightComponents();
    const auto lightCount = std::min<size_t>(lightComponents.size(), BG2E_MAX_FORWARD_LIGHTS);
    _sceneLights.reserve(lightCount);
    for (size_t index = 0; index < lightCount; ++index)
    {
        const auto& component = lightComponents[index];
        base::LightData light{};
        light.type = component->light().type();
        light.color = component->light().color();
        light.intensity = component->light().intensity();
        light.position = component->position();
        light.direction = component->direction();
        light.spotAngle = component->light().spotAngle();
        light.spotCutoff = component->light().spotCutoff();
        light.castShadows = component->light().castShadows() ? 1 : 0;
        light.sourceSize = component->light().sourceSize();
        light.shadowSamples = static_cast<int32_t>(component->light().shadowSamples());
        light.affectsReflections = component->light().affectsReflections() ? 1 : 0;
        _sceneLights.push_back(light);
    }
}

void StandaloneBakerContext::updateScene(float deltaSeconds)
{
    if (_state != State::Initialized && _state != State::SceneReady)
    {
        throw std::logic_error(
            "StandaloneBakerContext::updateScene: initialize the context before updating the scene");
    }
    if (_updateInProgress)
    {
        throw std::logic_error(
            "StandaloneBakerContext::updateScene: a baker update is currently in progress");
    }
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0f)
    {
        throw std::invalid_argument(
            "StandaloneBakerContext::updateScene: deltaSeconds must be finite and non-negative");
    }
    requireCurrentSceneRoot("updateScene");
    if (_sceneGeneration == std::numeric_limits<uint64_t>::max())
    {
        throw std::overflow_error("StandaloneBakerContext::updateScene: scene generation counter exhausted");
    }

    // Standalone work is synchronous; wait for any prior sample before recycling
    // descriptor pools or replacing the old TLAS.
    _state = State::Initialized;
    engine()->device().waitIdle();
    if (_bakeResources && _bakeResources->descriptorAllocator)
    {
        _bakeResources->descriptorAllocator->clearDescriptors();
    }
    if (_rayTracingScene)
    {
        _rayTracingScene->cleanup();
    }

    ++_sceneGeneration;
    invalidateBakersForSceneUpdate();

    try
    {
        resizeSceneOnce();

        _scene->willUpdate();
        try
        {
            scene::UpdateVisitor updateVisitor;
            updateVisitor.update(rootNode(), deltaSeconds);

            // updateAll() unconditionally resolves a main camera and throws for
            // camera-less bake scenes. Refresh only caches needed by baking.
            _scene->updateLights();
            _scene->updateEnvironment();
            if (scene::Scene::sceneHasCamera(rootNode()))
            {
                _scene->updateCamera();
            }
            requireCurrentSceneRoot("updateScene");
            refreshEnvironmentResource();
            captureSceneLights();
        }
        catch (...)
        {
            _scene->didUpdate();
            throw;
        }
        _scene->didUpdate();

        recordAndSubmitSceneTlas();
        _state = State::SceneReady;
    }
    catch (...)
    {
        _state = State::Initialized;
        if (_rayTracingScene)
        {
            _rayTracingScene->cleanup();
        }
        throw;
    }
}

void StandaloneBakerContext::invalidateBakersForSceneUpdate()
{
    auto baker = _bakers.begin();
    while (baker != _bakers.end())
    {
        if (auto liveBaker = baker->lock())
        {
            liveBaker->_sceneGeneration = _sceneGeneration;
            liveBaker->resetAccumulation();
            ++baker;
        }
        else
        {
            baker = _bakers.erase(baker);
        }
    }
}

void StandaloneBakerContext::resizeSceneOnce()
{
    if (_sceneResizeComplete)
    {
        return;
    }

    _scene->willResize();
    try
    {
        scene::ResizeViewportVisitor resizeVisitor;
        resizeVisitor.resizeViewport(rootNode(), _extent);
    }
    catch (...)
    {
        _scene->didResize();
        throw;
    }
    _scene->didResize();
    _sceneResizeComplete = true;
}

void StandaloneBakerContext::recordAndSubmitSceneTlas()
{
    if (!_frameResources || !_rayTracingScene || !_bakeResources ||
        !_bakeResources->environmentResources)
    {
        throw std::logic_error(
            "StandaloneBakerContext::updateScene: standalone frame, environment or TLAS resources are unavailable");
    }

    auto& frame = *_frameResources;
    const VkDevice device = engine()->device().handle();
    const uint64_t infiniteTimeout = std::numeric_limits<uint64_t>::max();
    VK_ASSERT(vkWaitForFences(device, 1, &frame.frameFence, VK_TRUE, infiniteTimeout));
    VK_ASSERT(vkResetCommandBuffer(frame.commandBuffer, 0));

    auto commandBeginInfo = vulkan::Info::commandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_ASSERT(vkBeginCommandBuffer(frame.commandBuffer, &commandBeginInfo));

    try
    {
        _bakeResources->environmentResources->update(
            frame.commandBuffer, engine()->currentFrame(), frame);
        if (!_rayTracingScene->update(frame.commandBuffer, rootNode()))
        {
            throw std::runtime_error("StandaloneBakerContext::updateScene: failed to build the context bake TLAS");
        }
        if (_rayTracingScene->tlas() == VK_NULL_HANDLE ||
            _rayTracingScene->objectInstances().empty())
        {
            throw std::runtime_error(
                "StandaloneBakerContext::updateScene: scene root contains no traceable instances");
        }
        VK_ASSERT(vkEndCommandBuffer(frame.commandBuffer));
    }
    catch (...)
    {
        _rayTracingScene->cleanup();
        throw;
    }

    VK_ASSERT(vkResetFences(device, 1, &frame.frameFence));
    auto commandInfo = vulkan::Info::commandBufferSubmitInfo(frame.commandBuffer);
    auto submitInfo = vulkan::Info::submitInfo(&commandInfo, nullptr, nullptr);
    const VkResult submitResult = vulkan::queueSubmit2(
        engine()->command().graphicsQueue(), 1, &submitInfo, frame.frameFence);
    if (submitResult != VK_SUCCESS)
    {
        vkDestroyFence(device, frame.frameFence, nullptr);
        frame.frameFence = VK_NULL_HANDLE;
        auto fenceInfo = vulkan::Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        VK_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &frame.frameFence));
        throw std::runtime_error(
            std::string("StandaloneBakerContext::updateScene: TLAS submission failed: ") +
            string_VkResult(submitResult));
    }

    const VkResult waitResult = vkWaitForFences(
        device, 1, &frame.frameFence, VK_TRUE, infiniteTimeout);
    if (waitResult != VK_SUCCESS)
    {
        engine()->device().waitIdle();
        vkDestroyFence(device, frame.frameFence, nullptr);
        frame.frameFence = VK_NULL_HANDLE;
        auto fenceInfo = vulkan::Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        VK_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &frame.frameFence));
        throw std::runtime_error(
            std::string("StandaloneBakerContext::updateScene: waiting for TLAS build failed: ") +
                string_VkResult(waitResult));
    }

    frame.cleanupManager.flush(engine()->device());
    if (frame.descriptorAllocator)
    {
        frame.descriptorAllocator->clearDescriptors();
    }
    if (_bakeResources->descriptorAllocator)
    {
        _bakeResources->descriptorAllocator->clearDescriptors();
    }
}

std::shared_ptr<StandaloneLightmapBaker> StandaloneBakerContext::createBaker(
    std::shared_ptr<scene::Node> targetNode,
    LightmapSettings settings)
{
    requireState(State::SceneReady, "createBaker");
    requireCurrentSceneRoot("createBaker");
    if (_updateInProgress)
    {
        throw std::logic_error("StandaloneBakerContext::createBaker: a baker update is currently in progress");
    }

    auto self = std::static_pointer_cast<StandaloneBakerContext>(shared_from_this());
    auto baker = std::shared_ptr<StandaloneLightmapBaker>(new StandaloneLightmapBaker(
        std::move(self), std::move(targetNode), settings, _sceneGeneration));
    _bakers.emplace_back(baker);
    return baker;
}

void StandaloneBakerContext::cleanup()
{
    if (_state == State::Cleaned)
    {
        return;
    }
    if (_updateInProgress)
    {
        throw std::logic_error("StandaloneBakerContext::cleanup: cannot clean up during a baker update");
    }

    if (_frameResources || _bakeResources || _rayTracingScene)
    {
        engine()->device().waitIdle();
    }
    if (_rayTracingScene)
    {
        _rayTracingScene->cleanup();
        _rayTracingScene.reset();
    }
    // Environment resources must outlive their Engine cleanup callbacks. Other
    // standalone bake resources are released now after the device-idle wait.
    _bakeResources.reset();
    if (_frameResources)
    {
        _frameResources->cleanup();
        _frameResources.reset();
    }

    _state = State::Cleaned;
}

void StandaloneBakerContext::requireState(State expected, const char* operation) const
{
    if (_state != expected)
    {
        throw std::logic_error(std::string("StandaloneBakerContext::") + operation +
            ": operation is invalid for the current lifecycle state");
    }
}

void StandaloneBakerContext::requireCurrentSceneRoot(const char* operation) const
{
    if (!_scene || _scene->rootNode() != rootNode())
    {
        throw std::logic_error(std::string("StandaloneBakerContext::") + operation +
            ": scene root changed after context construction; create a new context");
    }
}

void StandaloneBakerContext::beginBakerUpdate(uint64_t bakerGeneration)
{
    requireState(State::SceneReady, "StandaloneLightmapBaker::update");
    requireCurrentSceneRoot("StandaloneLightmapBaker::update");
    if (bakerGeneration != _sceneGeneration)
    {
        throw std::logic_error(
            "StandaloneLightmapBaker::update: baker belongs to a stale scene generation; call updateScene first");
    }
    if (_updateInProgress)
    {
        throw std::logic_error("StandaloneLightmapBaker::update: concurrent baker updates are unsupported");
    }
    _updateInProgress = true;
}

void StandaloneBakerContext::endBakerUpdate() noexcept
{
    _updateInProgress = false;
}

}
}
