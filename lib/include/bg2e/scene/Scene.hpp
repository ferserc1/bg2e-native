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

#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/base/TextFont.hpp>

#include <vector>
#include <memory>
#include <map>
#include <unordered_set>

namespace bg2e {

namespace scene {

class BG2E_API Scene {
  public:
    ~Scene();

    static std::shared_ptr<Scene> deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath,
        SceneLoadProgress* progress = nullptr);
    static std::shared_ptr<Scene> deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath,
        draw::Engine& engine, SceneLoadProgress* progress = nullptr);

    void setSceneRoot(std::shared_ptr<Node> scene);
    void setMainCamera(CameraComponent* camera);

    // nullptr explicitly selects the renderer default, not another Environment.
    // In-place edits preserve selection; component edits notify the owning Scene.
    void setMainEnvironment(EnvironmentComponent* environment);
    void resetMainEnvironmentSelection();
    bool hasExplicitMainEnvironment() const
    {
        return _explicitEnvironment;
    }

    // Detaches the complete root for adoption by another Scene, clearing its
    // back-pointer and this Scene's cached component references.
    std::shared_ptr<Node> releaseSceneRoot();

    // Non-throwing scene validation. Each rule is a separate static function so
    // new checks can be added independently without touching existing ones.
    // validateScene() is the aggregate entry point used by file-open flows.
    static bool validateScene(Node* rootNode);
    static bool sceneHasCamera(Node* rootNode);

    inline Node* rootNode()
    {
        return _sceneRoot.get();
    }

    CameraComponent* mainCamera();
    EnvironmentComponent* mainEnvironment();

    // Call this once in the update() function every time a light is added
    // or removed from the scene
    void updateLights();

    // Call this once when the environment is updated, added or removed from the scene
    void updateEnvironment();

    // Call this when you the camera is updated in the scene
    void updateCamera();

    // Call this when you want to update all the scene object references (ligths, environment, camera)
    void updateAll();

    // CPU resource boundary, before any text realization. No per-frame file I/O.
    // Successful preparation publishes the complete scene-wide registry at once.
    // Unchanged declarations reuse immutable resources; explicit reload rereads.
    void prepareTextFonts() noexcept;
    void reloadTextFonts() noexcept;
    void invalidateTextFonts() noexcept;

    // Dirty/failed/missing/ambiguous entries warn once per generation and return
    // nullptr, never stale handles or exceptions. There is no fallback font.
    // Existing shared handles are immutable snapshots, not live subscriptions.
    // Text consumers must track BOTH scene identity and textFontVersion().
    std::shared_ptr<const base::TextFont> textFont(const std::string& identifier) const noexcept;
    // Preparation completed, not a promise that every font source is available.
    bool textFontsReady() const;
    // Quiet inspection of the prepared registry; does not load or log.
    bool textFontAvailable(const std::string& identifier) const noexcept;
    uint64_t textFontVersion() const;

    [[nodiscard]] const std::vector<std::shared_ptr<LightComponent>>& lightComponents() const
    {
        return _lightComponents;
    }
    [[nodiscard]] const std::vector<base::Light*> lights() const
    {
        return _lights;
    }

    // Indicates whether the lights have changed since the last frame. This flag is set to false in didDraw().
    inline bool lightsChanged() const
    {
        return _lightsChanged;
    }

    // Render loop events
    void willResize();
    void didResize();
    void willUpdate();
    void didUpdate();
    void willDraw();
    void didDraw();

  protected:
    std::shared_ptr<Node> _sceneRoot;
    Node* _mainCameraNode = nullptr;
    Node* _mainEnvironment = nullptr;
    bool _explicitEnvironment = false;
    std::weak_ptr<Node> _selectedEnvironmentNode;
    std::weak_ptr<EnvironmentComponent> _selectedEnvironment;
    std::vector<std::shared_ptr<LightComponent>> _lightComponents;
    std::vector<base::Light*> _lights;

    bool _lightsChanged = false;

  private:
    struct FontResource {
        std::filesystem::path path;
        uint32_t faceIndex = 0;
        std::shared_ptr<const base::TextFont> font;
        bool ambiguous = false;
        bool retryOnChange = false;
    };
    std::map<std::string, FontResource> _textFonts;
    uint64_t _textFontVersion = 1;
    bool _textFontsDirty = true;
    bool _reloadTextFonts = false;
    mutable std::unordered_set<std::string> _textFontWarnings;
};

}
}
