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

#include "StageScene.hpp"

#include <bg2e/app/InputDelegate.hpp>
#include <bg2e/app/Uv2SafeReload.hpp>
#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/bake/IntegratedBakerContext.hpp>
#include <bg2e/scene/InputVisitor.hpp>

#include <optional>
#include <functional>
#include <bg2e/manipulation/SelectionManager.hpp>
#include <bg2e/manipulation/GizmoAndSelectionRenderer.hpp>

namespace bg2e_composer {

class AppDelegate final : public bg2e::draw::DefaultRenderLoopDelegate,
    public bg2e::app::InputDelegate, public std::enable_shared_from_this<AppDelegate> {
  public:
    AppDelegate();

    void cleanup() override;
    void prepareFrame() override;
    void render(const bg2e::draw::FrameContext& frame) override;
    void fileDropped(const std::filesystem::path& path) override;

    bool mouseInputCaptured() const override
    {
        return _selection && _selection->sceneInputBlocked();
    }
    void mouseMove(int x, int y) override;
    void mouseButtonDown(int button, int x, int y) override;
    void mouseButtonUp(int button, int x, int y) override;
    void mouseWheel(int deltaX, int deltaY) override;

    void resetCamera();
    void frameSelection();
    void analyzeBestView();
    void openDocument();
    void openDocument(const std::filesystem::path& path);
    void newDocument();
    void importDocument(const std::filesystem::path& path, float unitsScale = 1.0f,
        bool sourceIsZUp = false);
    void exportSelectedModels();
    bool confirmClose();
    void requestQuit()
    {
        _quitPending = true;
        requestSceneFrame();
    }
    void setQuitCallback(std::function<void()> callback)
    {
        _quit = std::move(callback);
    }
    void saveDocument(bool saveAs = false);

    std::shared_ptr<bg2e::manipulation::SelectionManager> selection() const
    {
        return _selection;
    }
    std::shared_ptr<StageScene> stage() const
    {
        return _stage;
    }
    void setAdditiveSelection(bool enabled)
    {
        _additiveSelection = enabled;
    }
    void setLogicalExtent(uint32_t width, uint32_t height);
    void selectNode(std::weak_ptr<bg2e::scene::Node> node, uint32_t submesh, bool additive);
    void queueNodeEdit(std::weak_ptr<bg2e::scene::Node> node,
        std::function<void(bg2e::scene::Node&)> operation, bool rebuild = false);
    void editGeometry(unsigned operation);
    void duplicateSelection();
    void copyMaterial();
    void pasteMaterial();
    void removeSelection();
    void applyMaterialPatch(const std::filesystem::path& path);
    void setFloor(bool visible, float height);
    void setFillLight(bool enabled);
    void editEnvironment(std::function<void(bg2e::scene::EnvironmentComponent&)> operation);
    void environmentPreset(const std::filesystem::path& path, bool save);
    void setCameraIntrinsics(float fov, float nearPlane, float farPlane);
    void setHighlight(float intensity);
    void notifyEdited(bg2e::draw::Renderer& renderer);
    std::shared_ptr<bg2e::app::SafeUpdateToken> regenerateUv2(std::weak_ptr<bg2e::scene::Node> target,
        const bg2e::geo::Uv2AtlasOptions& options,
        bg2e::app::Uv2SafeReload::CompletionCallback completion);
    void startLightmapBake(std::weak_ptr<bg2e::scene::Node> target,
        const bg2e::bake::LightmapSettings& settings);
    void cancelLightmapBake();
    void applyLightmapBake();
    bg2e::bake::BakeProgress lightmapBakeProgress() const;
    std::optional<bg2e::bake::LightmapResult> lightmapResult() const;
    bool lightmapBakeActive() const;
    void onSelectionChanged(std::function<void()> callback)
    {
        _selectionChanged = std::move(callback);
    }
    void onToolsCleanup(std::function<void()> callback)
    {
        _toolsCleanup = std::move(callback);
    }

    const Document* document() const
    {
        return _stage ? &_stage->document() : nullptr;
    }
    const std::string& status() const
    {
        return _status;
    }
    const std::string& viewpointStatus() const
    {
        return _viewpointStatus;
    }

  protected:
    std::shared_ptr<bg2e::scene::Node> createScene() override;

  private:
    explicit AppDelegate(std::shared_ptr<bg2e::manipulation::GizmoAndSelectionRenderer> overlay);
    static bg2e::draw::RendererConfiguration configuration(
        const std::shared_ptr<bg2e::manipulation::GizmoAndSelectionRenderer>& overlay);
    void queueInput(std::function<void()> event);
    glm::ivec2 inputPosition(int x, int y) const;
    void retireBakeFrame();
    void stopLightmapBake(bool cancelled);
    void invalidateLightmapSources();

    std::shared_ptr<bg2e::manipulation::GizmoAndSelectionRenderer> _overlay;
    std::shared_ptr<bg2e::manipulation::SelectionManager> _selection;
    glm::uvec2 _logicalExtent{0};
    std::function<void()> _toolsCleanup;
    std::function<void()> _selectionChanged;
    std::shared_ptr<bg2e::draw::MaterialBase> _clipboard;
    void requestOpen(const std::filesystem::path& path);
    std::filesystem::path chooseSavePath();

    struct PendingIo {
        enum class Kind { Open, Save, New, Import, Export };
        Kind kind = Kind::Open;
        std::filesystem::path path;
        uint64_t generation = 0;
        std::weak_ptr<bg2e::scene::Node> parent;
        std::vector<std::weak_ptr<bg2e::scene::Node>> targets;
        float unitsScale = 1.0f;
        bool sourceIsZUp = false;
        bool batch = false;
        bool destinationConfirmed = false;
    };
    void processDocumentIo();
    bool saveBeforeReplace();
    bool _additiveSelection = false;
    bool _preparing = false;
    bool _quitPending = false;
    std::function<void()> _quit;
    std::optional<PendingIo> _pendingIo;
    std::string _status;
    std::string _viewpointStatus;

    std::shared_ptr<bg2e::bake::IntegratedBakerContext> _bakeContext;
    std::shared_ptr<bg2e::bake::IntegratedLightmapBaker> _baker;
    struct BakeSubmission {
        uint32_t slot = 0;
        uint64_t serial = 0;
    };
    std::optional<BakeSubmission> _bakeSubmission;
    std::weak_ptr<bg2e::scene::Node> _bakeTarget;
    uint64_t _bakeDocumentGeneration = 0;
    std::optional<bg2e::bake::LightmapResult> _completedLightmap;
    uint64_t _completedLightmapDocumentGeneration = 0;
    bool _bakeApplyQueued = false;
    bool _wasContinuousRendering = false;

    std::shared_ptr<StageScene> _stage;
    bg2e::scene::InputVisitor _input;
};

}
