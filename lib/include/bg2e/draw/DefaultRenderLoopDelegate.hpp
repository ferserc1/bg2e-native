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

#pragma once

#include <bg2e/draw/Renderer.hpp>
#include <bg2e/draw/RenderLoopDelegate.hpp>

#include <functional>
#include <memory>
#include <optional>

namespace bg2e::draw {

// Main-thread scene delegate. Renderer ownership is exclusive; scene ownership
// is shared and survives renderer replacement. Setters queue work, including
// when invoked from UI/update/record callbacks; prepareFrame commits it.
class BG2E_API DefaultRenderLoopDelegate : public RenderLoopDelegate {
  public:
    explicit DefaultRenderLoopDelegate(std::unique_ptr<Renderer> renderer,
                                      RendererConfiguration configuration = {});
    ~DefaultRenderLoopDelegate() override = default;

    void init(Engine* engine) override;
    void initScene() override;
    void prepareFrame() override;
    void surfaceChanged(gpu::Size2D extent, gpu::PixelFormat format) override;
    void resize(gpu::Size2D extent) override;
    void update(const FrameContext& frame) override;
    void render(const FrameContext& frame) override;
    void cleanup() override;

    // Non-null replacement. Only the latest queued replacement is retained.
    // Zero internalExtent follows output; an explicit internalExtent is fixed
    // across output resizes. Output extent/format come from the retained target.
    void setRenderer(std::unique_ptr<Renderer> renderer, RendererConfiguration configuration = {});
    void setScene(std::shared_ptr<scene::Node> sceneRoot);
    std::shared_ptr<scene::Node> scene() const;

    // Borrowed active renderer, possibly not initialized before first resize.
    // Never retain it in a queued callback. Use weak delegate ownership and a
    // generation check through withRenderer for delayed UI/settings actions.
    Renderer* renderer() const;
    uint64_t rendererGeneration() const;
    bool withRenderer(uint64_t generation, const std::function<void(Renderer&)>& operation);

    void enqueueRendererUpdate(uint64_t generation, std::function<void(Renderer&)> operation,
        std::function<void()> stale = {});

    using RendererAttachedCallback = std::function<void(Renderer&, uint64_t)>;
    // Configure before first binding (e.g. during application/UI initialization).
    // Detach cancels settings targets/callbacks before old resources are freed;
    // attach receives the initialized renderer after scene attachment/resize.
    void setRendererChangeCallbacks(std::function<void()> detach, RendererAttachedCallback attach);

    void setContinuousRendering(bool enabled);
    bool continuousRendering() const;

  protected:
    // Called once by initScene unless setScene supplied an explicit root.
    // The default creates an empty root; subclasses may construct their scene.
    virtual std::shared_ptr<scene::Node> createScene();

  private:
    struct PendingRenderer {
        std::unique_ptr<Renderer> renderer;
        RendererConfiguration configuration;
    };
    struct PendingUpdate {
        uint64_t generation;
        std::function<void(Renderer&)> operation;
        std::function<void()> stale;
    };
    void applyUpdates(std::vector<PendingUpdate> updates);
    std::vector<PendingUpdate> _updates;
    void initializeRenderer();
    void reconfigureRenderer();
    gpu::Size2D _fixedInternalExtent;
    void detachTargets();
    void attachTargets();
    void releaseRenderer();

    std::unique_ptr<Renderer> _renderer;
    std::optional<PendingRenderer> _pendingRenderer;
    std::shared_ptr<scene::Node> _scene;
    std::shared_ptr<scene::Node> _pendingScene;
    bool _scenePending = false;
    bool _sceneReady = false;
    bool _rendererInitialized = false;
    bool _targetsBound = false;
    bool _followOutputExtent = true;
    bool _continuousRendering = false;
    uint64_t _rendererGeneration = 0;
    gpu::Size2D _outputExtent;
    gpu::PixelFormat _outputFormat = gpu::PixelFormat::Undefined;
    RendererConfiguration _initialConfiguration;
    std::function<void()> _detach;
    RendererAttachedCallback _attach;
};

}
