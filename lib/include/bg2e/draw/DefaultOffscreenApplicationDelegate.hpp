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

#include <bg2e/app/DrawOffscreenApplicationDelegate.hpp>
#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/draw/RenderSettingsPreferences.hpp>
#include <bg2e/draw/pipeline/FrameAccumulator.hpp>

namespace bg2e::draw {

// Bridge the existing public renderer/scene lifecycle to the typed headless
// delegate. Renderer and scene have exactly the same owners as in the windowed
// delegate; no second traversal or display-loop coordinator is instantiated.
class BG2E_API DefaultOffscreenApplicationDelegate : public DefaultRenderLoopDelegate,
    public app::DrawOffscreenApplicationDelegate {
  public:
    explicit DefaultOffscreenApplicationDelegate(std::unique_ptr<Renderer> renderer,
        RendererConfiguration configuration = {});
    ~DefaultOffscreenApplicationDelegate() override = default;

    void configure(const app::DrawOffscreenApplicationConfig& configuration) override;

    // Also guard initialization through the inherited scene-delegate interface.
    void init(Engine* engine) override;
    void init(Engine* engine, gpu::OffscreenSurface* surface) override;
    void initScene() override;
    void resize(gpu::Size2D extent) override;

    void prepareFrame() override;
    using DefaultRenderLoopDelegate::update;
    void update(float deltaSeconds, uint64_t sceneFrameNumber) override;
    void render(const FrameContext& frame) override;
    bool continueRendering() const override;
    void completeFrame(uint64_t sceneFrameNumber) final;
    bool needsResolve() const final;
    void resolveFrame(const FrameContext& frame) final;
    void completeResolve() final;
    void finish() final;

    uint32_t completedFrames() const;
    bool hasResult() const;
    const SettingsResult& settingsResult() const;

    void cleanup() override;

    // Headless exports exclude editor overlays even during renderer replacement.
    void setRenderer(std::unique_ptr<Renderer> renderer, RendererConfiguration configuration = {});

  protected:
    // Explicit mappings for application-owned IDs such as exported lightPose.
    virtual void extendSettingsCatalog(Renderer& renderer, SettingsCatalog& catalog);

  private:
    static RendererConfiguration withoutOverlays(RendererConfiguration configuration);

    app::DrawOffscreenApplicationConfig _job;
    pipeline::FrameAccumulator _accumulator;
    std::unique_ptr<RenderSettingsPreferences> _settings;
    SettingsResult _settingsResult;
    std::shared_ptr<gpu::Image> _sample;
    std::string _settingsSnapshot;
    std::vector<uint8_t> _sceneSnapshot;
    uint64_t _generation = 0, _pendingSerial = 0;
    const scene::Node* _sceneIdentity = nullptr;
    bool _ready = false, _pending = false, _finalRecorded = false, _resolved = false;
};

}
