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
#include "command/scene/MetadataCommands.hpp"
#include "command/geometry/GeometryCommands.hpp"
#include "command/scene/ImportCommand.hpp"
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/ui/DrawMaterialEditor.hpp>
#include "SceneImporter.hpp"
#include "ImportServer.hpp"
#include "ImportSettings.hpp"
#include <unordered_map>

#include <bg2e/app/InputDelegate.hpp>
#include <bg2e/app/Uv2SafeReload.hpp>
#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/draw/RenderSettingsPreferences.hpp>
#include <bg2e/bake/IntegratedBakerContext.hpp>
#include <bg2e/bake/LightmapPixels.hpp>
#include <bg2e/scene/InputVisitor.hpp>
#include <bg2e/ui/Loader.hpp>

#include <optional>
#include <exception>
#include <functional>
#include <utility>
#include <vector>
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
    void focusLost() override;
    void keyDown(const bg2e::app::KeyEvent& keyEvent) override;
    void keyUp(const bg2e::app::KeyEvent& keyEvent) override;

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
    void asyncLoadGuarded(std::function<void(bg2e::ui::Loader*)> loadFn,
        glm::vec4 clearColor = {0, 0, 0, 1},
        std::function<void(std::exception_ptr)> onComplete = {});

    bool sceneLoading() const
    {
        return _asyncLoadInProgress || _sceneResourceLoad;
    }
    void drawSceneLoader()
    {
        _sceneLoader.draw();
    }
    void importDocument(const std::filesystem::path& path, float unitsScale = 1.0f,
        bool sourceIsZUp = false);
    void exportSelectedModels();

    void cancelImports(bool remoteOnly = false);
    void setImportService(bool enabled, const std::string& port);
    bool importServiceRunning() const
    {
        return _importServer.isRunning();
    }
    uint32_t importPort() const
    {
        return _importSettings.port();
    }
    SceneImporter& imports()
    {
        return _imports;
    }
    const std::string& importStatus() const
    {
        return _importStatus;
    }
    bool confirmClose();
    void confirmQuitRequest();
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
    void initializeTransformInteractions();
    void finishTransformInteraction(bool commit);

    void setLogicalExtent(uint32_t width, uint32_t height);
    void selectNode(std::weak_ptr<bg2e::scene::Node> node, uint32_t submesh, bool additive);
    void selectTreeNode(std::weak_ptr<bg2e::scene::Node> node, bool additive);
    void reparentNode(std::weak_ptr<bg2e::scene::Node> node,
        std::weak_ptr<bg2e::scene::Node> parent);
    void editComponent(std::weak_ptr<bg2e::scene::Node> node,
        std::weak_ptr<bg2e::scene::Component> original,
        std::shared_ptr<bg2e::scene::Component> replacement);
    bool componentEditIntent(const std::shared_ptr<bg2e::scene::Component>& component,
        bg2e::ui::EditPhase phase, uint64_t interaction, const bg2e::ui::ReflectionEditIntent& intent);
    void componentAction(const std::shared_ptr<bg2e::scene::Component>& component,
        const bg2e::ui::ReflectionActionIntent& intent);
    bg2e::app::CommandResult editSubmeshMetadata(command::scene::MetadataRequest request);
    void toggleSelectionHistory();
    bool selectionHistoryEnabled() const;
    void selectPickedItem(bg2e::manipulation::SelectionItem item, bool additive);
    bg2e::app::CommandResult renameNode(std::weak_ptr<bg2e::scene::Node> node, const std::string& name);
    bool historyActionAvailable(bool redo) const;
    std::string historyActionLabel(bool redo) const;
    void editSceneSettings(std::shared_ptr<bg2e::draw::RenderSettingsPreferences> preferences,
        std::vector<bg2e::draw::SettingChange> changes, bg2e::draw::RenderSettingsPreferences::Completion completion);
    void undoEdit();
    void redoEdit();
    void setNodeEnabled(std::weak_ptr<bg2e::scene::Node> node, bool enabled);
    void addSceneItem(unsigned kind);
    void addPrimitive(unsigned kind);
    void addSceneComponent(std::weak_ptr<bg2e::scene::Node> node, const std::string& typeName);

    void editGeometry(command::geometry::Operation operation);
    void duplicateSelection();
    bool materialEditIntent(bg2e::ui::EditPhase phase, uint64_t interaction,
        const bg2e::ui::MaterialEditTargets& targets, const bg2e::ui::MaterialEditIntent& intent);
    void queueMaterialEdit(bg2e::ui::MaterialEditTargets targets, bg2e::ui::MaterialEditIntent intent,
        std::shared_ptr<bg2e::draw::MaterialBase> paste = {});
    bool valueEditAvailable() const;

    void copyMaterial();
    void pasteMaterial();
    void removeSelection();
    void applyMaterialPatch(const std::filesystem::path& path);
    void queueEnvironmentEdit(std::weak_ptr<bg2e::scene::EnvironmentComponent> target,
        bg2e::ui::ReflectionEditIntent intent);
    void environmentPreset(const std::filesystem::path& path, bool save);
    void setHighlight(float intensity);
    std::shared_ptr<bg2e::app::SafeUpdateToken> regenerateUv2(std::weak_ptr<bg2e::scene::Node> target,
        const bg2e::geo::Uv2AtlasOptions& options,
        bg2e::app::Uv2SafeReload::CompletionCallback completion);
    void startLightmapBake(std::weak_ptr<bg2e::scene::Node> target,
        const bg2e::bake::LightmapSettings& settings);
    void startLightmapBatch(bool wholeScene, const bg2e::bake::LightmapSettings& settings);
    void startLightmapTargets(std::vector<std::weak_ptr<bg2e::scene::Node>> targets,
        const bg2e::bake::LightmapSettings& settings);
    void cancelLightmapBake();
    void applyLightmapBake();
    bg2e::bake::BakeProgress lightmapBakeProgress() const;
    std::optional<bg2e::bake::LightmapResult> lightmapResult(
        const std::shared_ptr<bg2e::scene::Node>& target = {}) const;
    bool lightmapBakeActive() const;
    void onSelectionChanged(std::function<void()> callback)
    {
        _selectionChanged = std::move(callback);
    }
    void onSettleUiInteractions(std::function<bool(bool)> callback)
    {
        _settleUiInteractions = std::move(callback);
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
    struct DeferredRead {
        enum class Kind { CopyMaterial, SaveEnvironmentPreset };
        Kind kind;
        uint64_t generation = 0;
        bg2e::manipulation::SelectionItem target;
        std::filesystem::path path;
    };
    std::vector<DeferredRead> _deferredReads;
    void processReadActions();
    void retireBakeFrame();
    void stopLightmapBake(bool cancelled);
    void invalidateLightmapSources();

    std::function<bool(bool)> _settleUiInteractions;
    bool _settlingUiInteractions = false;
    uint64_t _transformTransaction = 0;
    std::vector<std::function<void()>> _resumeTransformWriters;
    std::shared_ptr<bg2e::manipulation::GizmoAndSelectionRenderer> _overlay;
    std::shared_ptr<bg2e::manipulation::SelectionManager> _selection;
    glm::uvec2 _logicalExtent{0};
    std::function<void()> _toolsCleanup;
    std::function<void()> _selectionChanged;
    std::shared_ptr<bg2e::draw::MaterialBase> _clipboard;
    void requestOpen(const std::filesystem::path& path);
    std::filesystem::path chooseSavePath();

    struct PendingIo {
        enum class Kind { Open, Save, New, Export };
        Kind kind = Kind::Open;
        std::filesystem::path path;
        uint64_t generation = 0;
        std::weak_ptr<bg2e::scene::Node> parent;
        std::vector<std::weak_ptr<bg2e::scene::Node>> targets;
        bool batch = false;
        bool destinationConfirmed = false;
        std::shared_ptr<bg2e::json::JsonNode> schema;
        bool replacementConfirmed = false;
        std::shared_ptr<bg2e::scene::Node> candidate;
        struct Resource {
            std::shared_ptr<bg2e::scene::Component> component;
            std::string nodeName;
        };
        std::vector<Resource> resources;
        size_t nextResource = 0;
    };
    void queueSceneCommand(std::function<std::unique_ptr<bg2e::app::Command>()> prepare,
        command::Effects effects = {.lightmapSources = true, .renderHistories = true});
    bg2e::app::CommandResult submitSceneCommand(std::unique_ptr<bg2e::app::Command> action,
        command::Effects effects = {}, command::Boundary boundary = command::Boundary::Conservative);
    std::vector<std::weak_ptr<bg2e::scene::Node>> selectedSceneBranches() const;
    std::shared_ptr<command::Dispatcher> makeCommandDispatcher(uint64_t generation);
    bg2e::app::CommandResult submitNodeCommand(std::weak_ptr<bg2e::scene::Node> node,
        std::unique_ptr<bg2e::app::Command> action, command::Boundary boundary, command::Effects effects);
    void applyCommandEffects(bg2e::draw::Renderer& renderer, const command::Effects& effects);
    struct ValueEdit {
        uint64_t interaction = 0;
        uint64_t generation = 0;
        uint64_t transaction = 0;
        bool ending = false;
        bool finished = false;
        bool failed = false;
        bool cancelling = false;
    };
    std::shared_ptr<ValueEdit> _valueEdit;

    bool valueEditIntent(bg2e::ui::EditPhase phase, uint64_t interaction,
        std::function<std::unique_ptr<bg2e::app::CommandTransaction>()> prepare, std::any value);
    std::function<void()> valueEditEffects();
    bool settleEditDrafts();
    bool flushDocumentCommands(bool commit, bool allowFault = false);
    void sampleNavigation(bool trackChanges);
    std::weak_ptr<bg2e::scene::OrbitCameraComponent> _observedOrbit;
    glm::vec2 _observedRotation{0};
    glm::vec3 _observedCenter{0};
    float _observedDistance = 0;
    bg2e::draw::Renderer* _commandRenderer = nullptr; // Borrowed only during a safe executor callback.

    bool commandFaulted() const
    {
        return document() && document()->commands && document()->commands->history().faulted();
    }
    void initializeImports();
    void processImports();
    void enqueueImport(ImportRequest request);
    void setImportPaused(bool paused);
    void invalidateImports();

    SceneImporter _imports;
    ImportServer _importServer;
    ImportSettings _importSettings;
    std::unordered_map<std::string, std::shared_ptr<command::scene::ImportRecord>> _imported;
    std::unordered_map<std::string, uint64_t> _latestImportTokens;
    bool _importPublicationPending = false;
    std::weak_ptr<SceneImporter::Job> _publishingImport;
    uint64_t _nextImportToken = 1;
    bool _importPaused = false;
    std::string _importStatus;

    void processDocumentIo();
    bool saveBeforeReplace();
    bool _preparing = false;
    bool _asyncLoadInProgress = false;
    bool _sceneResourceLoad = false;
    bg2e::ui::Loader _sceneLoader;
    bool _quitPending = false;
    bool _quitConfirmed = false;
    std::function<void()> _quit;
    std::optional<PendingIo> _pendingIo;
    std::string _status;
    std::string _lastEnvironmentError;
    std::string _viewpointStatus;

    std::shared_ptr<bg2e::bake::IntegratedBakerContext> _bakeContext;
    std::shared_ptr<bg2e::bake::IntegratedLightmapBaker> _baker;
    struct BakeSubmission {
        uint32_t slot = 0;
        uint64_t serial = 0;
    };
    std::optional<BakeSubmission> _bakeSubmission;
    std::weak_ptr<bg2e::scene::Node> _bakeTarget;
    struct BakeTarget {
        std::weak_ptr<bg2e::scene::Node> node;
        std::shared_ptr<bg2e::draw::Drawable> drawable;
        std::optional<bg2e::bake::LightmapResult> result;
        std::optional<bg2e::bake::LightmapPixels> pixels;
    };
    std::vector<BakeTarget> _bakeTargets;
    size_t _bakeTargetIndex = 0;
    bg2e::bake::LightmapSettings _bakeSettings;
    uint64_t _bakeDocumentGeneration = 0;
    uint64_t _bakeJobToken = 0;
    bg2e::app::CommandDocumentState _bakeSourceState;
    uint64_t _bakeRendererGeneration = 0;
    bool _bakeApplyQueued = false;
    bool _wasContinuousRendering = false;

    std::shared_ptr<StageScene> _stage;
    bg2e::scene::InputVisitor _input;
};

}
