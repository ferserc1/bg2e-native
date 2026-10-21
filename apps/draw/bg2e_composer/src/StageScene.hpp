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

#include "Document.hpp"
#include <bg2e/db/scene_gltf.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/db/mesh_obj.hpp>

#include <bg2e/draw/Drawable.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/OrbitCameraComponent.hpp>
#include <bg2e/utils/ViewpointAnalyzer.hpp>

#include <memory>
#include <functional>
#include <vector>
#include <map>

namespace bg2e_composer {

class StageScene {
  public:
    explicit StageScene(bg2e::draw::Engine* engine);

    std::shared_ptr<bg2e::scene::Node> create();
    void cleanup();

    void resetCamera();
    bool frameNode(bg2e::scene::Node* node);
    std::vector<bg2e::utils::ViewSample> analyzeBestView(bg2e::scene::Node* node);
    void applyViewpoint(const bg2e::utils::CameraCandidate& candidate);
    bg2e::scene::OrbitCameraComponent* orbitCamera() const;
    const std::vector<bg2e::utils::AnalyzedSubmesh>& analyzedSubmeshes() const;
    std::shared_ptr<bg2e::scene::Node> load(const std::filesystem::path& path,
        std::shared_ptr<bg2e::json::JsonNode> schema = {}, bool realizeResources = true);
    void adopt(std::shared_ptr<bg2e::scene::Node> candidate, const std::filesystem::path& source);
    bool save(const std::filesystem::path& path, bool destinationConfirmed = false);

    // Call only at a stopped renderer boundary. Candidates stay detached until
    // publication; imports preserve the current document and its save path.
    std::shared_ptr<bg2e::scene::Node> newDocumentRoot() const;
    std::shared_ptr<bg2e::scene::Node> prepareImport(const std::filesystem::path& path,
        float unitsScale = 1.0f, bool sourceIsZUp = false,
        bg2e::scene::SceneProgressCallback progress = nullptr,
        const bg2e::db::GltfDocument* gltf = nullptr,
        std::shared_ptr<bg2e::json::JsonNode> schema = {},
        const bg2e::db::Bg2Mesh* bg2 = nullptr, const bg2e::db::ObjMesh* obj = nullptr);
    void publishImport(const std::shared_ptr<bg2e::scene::Node>& candidate,
        const std::shared_ptr<bg2e::scene::Node>& parent,
        const std::shared_ptr<bg2e::scene::Node>& previous = {},
        std::function<void(bool restoring)> rebind = {});
    void insert(std::shared_ptr<bg2e::scene::Node> candidate,
        const std::shared_ptr<bg2e::scene::Node>& parent);
    bool exportModels(const std::vector<std::shared_ptr<bg2e::scene::Node>>& nodes,
        const std::filesystem::path& destination, bool batch);
    void refreshCamera();
    void validateEditingGraph();
    void discardCandidate(const std::shared_ptr<bg2e::scene::Node>& candidate) noexcept;
    void releaseRetired();
    void prepareTools(std::shared_ptr<bg2e::scene::Node> root = {});
    std::shared_ptr<bg2e::scene::Node> createPrimitive(unsigned kind);
    bool contains(bg2e::scene::Node* node) const;
    void duplicate(const std::shared_ptr<bg2e::scene::Node>& node);
    void remove(const std::shared_ptr<bg2e::scene::Node>& node);
    void setFloor(bool visible, float height);
    void setFillLight(bool enabled);
    std::shared_ptr<bg2e::scene::Node> fillLightNode() const
    {
        return _fillLight;
    }

    bool fillLightEnabled() const
    {
        return _fillLight && _fillLight->light() && _fillLight->enabled();
    }
    bool floorVisible() const
    {
        return _floor && _floor->enabled();
    }
    float floorHeight() const;
    std::shared_ptr<bg2e::scene::Node> environmentNode() const;
    // Inspector clone replacement is an edit, not a newly added candidate.
    void environmentComponentReplaced(bg2e::scene::Node& node);
    std::shared_ptr<bg2e::scene::Node> environmentPreset() const;
    void validateEnvironmentPreset(const std::shared_ptr<bg2e::scene::Node>& preset) const;
    void restoreEnvironment(const std::shared_ptr<bg2e::scene::Node>& preset);
    void adoptEnvironment(const std::shared_ptr<bg2e::scene::Node>& preset);

    bg2e::scene::Scene* sceneObject() const
    {
        return _scene.get();
    }
    bg2e::scene::CameraComponent* camera() const
    {
        return _camera ? _camera->camera() : nullptr;
    }

    const Document& document() const
    {
        return _document;
    }

    void markModified()
    {
        _document.markModified();
    }

  private:
    std::shared_ptr<bg2e::scene::Node> prepareScene(const std::filesystem::path& path,
        bool requireCamera, bg2e::scene::SceneProgressCallback progress,
        std::shared_ptr<bg2e::json::JsonNode> schema = {}, bool realizeResources = true);
    void prepareCameras(const std::shared_ptr<bg2e::scene::Node>& root) const;
    void refreshEnvironment(bool reset = false);

    void addDrawable(const char* name, std::shared_ptr<bg2e::draw::Drawable> drawable,
        const glm::vec3& position);

    std::shared_ptr<bg2e::scene::Scene> _scene;
    std::shared_ptr<bg2e::scene::Node> _floor;
    std::shared_ptr<bg2e::scene::Node> _fillLight;
    Document _document;
    std::shared_ptr<bg2e::scene::Node> _model;
    std::vector<std::shared_ptr<bg2e::scene::Node>> _retired;
    bg2e::draw::Engine* _engine;
    std::shared_ptr<bg2e::scene::Node> _root;
    std::shared_ptr<bg2e::scene::Node> _camera;
    std::map<std::string, std::weak_ptr<bg2e::scene::EnvironmentComponent>> _knownEnvironmentNodes;
    std::vector<std::shared_ptr<bg2e::draw::Drawable>> _drawables;
    std::unique_ptr<bg2e::utils::ViewpointAnalyzer> _viewpointAnalyzer;
};

}
