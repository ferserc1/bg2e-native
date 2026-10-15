#pragma once

#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/draw/RenderLoop.hpp>
#include <bg2e/scene/Node.hpp>

#include <stdexcept>
#include <vector>
#include <utility>

namespace bg2e::draw::tests {

struct LifecycleProbeState {
    std::shared_ptr<scene::Node> scene = std::make_shared<scene::Node>();
    std::vector<uint64_t> sceneNumbers;
    uint32_t initialized = 0;
    uint32_t cleaned = 0;
    uint32_t attached = 0;
    uint32_t detached = 0;
};

// Uses no shader/native renderer or future RendererDeferred implementation.
class LifecycleProbeRenderer : public Renderer {
  public:
    explicit LifecycleProbeRenderer(std::shared_ptr<LifecycleProbeState> state) : _state(std::move(state))
    {
    }
    const RendererConfiguration& configuration() const override
    {
        return _configuration;
    }
    void initialize(Engine&, RendererConfiguration configuration) override
    {
        Renderer::validateConfiguration(configuration);
        _configuration = std::move(configuration);
        ++_state->initialized;
    }
    void attachScene(std::shared_ptr<scene::Node> root) override
    {
        if (root != _state->scene)
        {
            throw std::runtime_error("Lifecycle probe lost the retained scene");
        }
    }
    void resize(gpu::Size2D output, gpu::Size2D internal) override
    {
        _configuration.outputExtent = output;
        _configuration.internalExtent = internal;
    }
    void update(const FrameContext&) override
    {
    }
    void record(const FrameContext& frame) override
    {
        if (frame.sceneFrameNumber != _state->sceneNumbers.size())
        {
            throw std::runtime_error("Lifecycle probe scene sequence advanced without scene rendering");
        }
        _state->sceneNumbers.push_back(frame.sceneFrameNumber);
        frame.commandBuffer.beginRendering(&frame.colorTarget);
        frame.commandBuffer.clearColor(0, {0.2f, 0.4f, 0.6f, 1.0f});
        frame.commandBuffer.endRendering();
    }
    void cleanup() override
    {
        if (_state->detached <= _state->cleaned)
        {
            throw std::runtime_error("Lifecycle probe cleanup preceded settings detachment");
        }
        ++_state->cleaned;
    }

  private:
    std::shared_ptr<LifecycleProbeState> _state;
    RendererConfiguration _configuration;
};

class LifecycleProbeDelegate : public DefaultRenderLoopDelegate {
  public:
    explicit LifecycleProbeDelegate(std::shared_ptr<LifecycleProbeState> state)
        : DefaultRenderLoopDelegate(std::make_unique<LifecycleProbeRenderer>(state)), _state(std::move(state))
    {
    }

  protected:
    std::shared_ptr<scene::Node> createScene() override
    {
        return _state->scene;
    }

  private:
    std::shared_ptr<LifecycleProbeState> _state;
};

// Call on the main thread with an initialized window Engine, outside any
// existing RenderLoop/frame. Visible, non-minimized presentation is required.
inline void runRendererLifecycleProbe(Engine& engine)
{
    auto state = std::make_shared<LifecycleProbeState>();
    auto delegate = std::make_shared<LifecycleProbeDelegate>(state);
    std::weak_ptr<LifecycleProbeDelegate> weak = delegate;
    delegate->setRendererChangeCallbacks([state]()
    {
        ++state->detached;
    }, [state](Renderer&, uint64_t)
    {
        ++state->attached;
    });
    RenderLoop loop;
    loop.setDelegate(delegate);
    loop.init(&engine);
    loop.initScene();
    const auto present = [&]()
    {
        const auto before = loop.presentedFrameCount();
        for (uint32_t attempt = 0; attempt < 60 && loop.presentedFrameCount() == before; ++attempt)
        {
            loop.frame(1.0f / 60.0f);
        }
        if (loop.presentedFrameCount() == before)
        {
            throw std::runtime_error("Lifecycle probe requires an available window drawable");
        }
    };
    present();
    const auto firstGeneration = delegate->rendererGeneration();
    const auto oldAction = [weak, firstGeneration]()
    {
        if (auto owner = weak.lock())
        {
            return owner->withRenderer(firstGeneration, [](Renderer&)
            {
            });
        }
        return false;
    };
    if (!oldAction())
    {
        throw std::runtime_error("Lifecycle probe initial renderer is not bound");
    }
    const auto firstCount = loop.renderedSceneCount();
    present(); // Retained-image/UI-only presentation must not update the scene.
    if (loop.renderedSceneCount() != firstCount)
    {
        throw std::runtime_error("Lifecycle probe rendered a clean scene");
    }
    bool queued = false;
    auto* original = delegate->renderer();
    loop.setUIFramePreparationCallback([&](gpu::CommandBuffer&, gpu::SurfaceFrame&)
    {
        if (!queued)
        {
            queued = true;
            delegate->setRenderer(std::make_unique<LifecycleProbeRenderer>(state));
            if (delegate->renderer() != original)
            {
                throw std::runtime_error("Lifecycle probe replaced a renderer during command recording");
            }
        }
    });
    loop.requestSceneFrame();
    present(); // UI queues replacement while this frame still uses the old owner.
    loop.setUIFramePreparationCallback(nullptr);
    present(); // Replacement commits before recording.
    if (oldAction() || state->initialized != 2 || state->cleaned != 1 || delegate->scene() != state->scene)
    {
        throw std::runtime_error("Lifecycle probe replacement/generation/lifetime mismatch");
    }
    loop.pauseScene();
    const auto pausedCount = loop.renderedSceneCount();
    loop.requestResize();
    present();
    delegate->setRenderer(std::make_unique<LifecycleProbeRenderer>(state));
    present(); // Lifecycle still progresses while scene recording is paused.
    if (loop.renderedSceneCount() != pausedCount || !loop.sceneDirty())
    {
        throw std::runtime_error("Lifecycle probe lost paused scene invalidation");
    }
    loop.resumeScene();
    present();
    delegate->setContinuousRendering(true);
    present();
    present();
    if (!loop.sceneDirty() || loop.renderedSceneCount() != pausedCount + 3)
    {
        throw std::runtime_error("Lifecycle probe did not keep animation frames dirty");
    }
    delegate->setContinuousRendering(false);
    loop.cleanup();
    if (oldAction() || state->cleaned != state->initialized || state->detached != state->attached)
    {
        throw std::runtime_error("Lifecycle probe cleanup retained settings targets");
    }
}

}
