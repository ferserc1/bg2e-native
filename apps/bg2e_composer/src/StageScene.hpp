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
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/scene/OrbitCameraComponent.hpp>
#include <bg2e/app/MainLoop.hpp>

#include <Document.hpp>

#include <memory>
#include <filesystem>

#include "bg2e/scene/Scene.hpp"

class AppDelegate;

class StageScene {
public:
    StageScene(bg2e::render::Engine * engine, AppDelegate * appDelegate);
    
    std::shared_ptr<bg2e::scene::Node> init();
    
    void loadModel(const std::filesystem::path& path);
    
    void saveModel(const std::filesystem::path& path);

    void close();

    // Throws exception if model is invalid or format unsupported
    void importModel(const std::filesystem::path& path);

    void importObj(const std::filesystem::path& path);

    void importGltf(const std::filesystem::path& path);

    inline bool multiMeshScene() const { return _targetNames.size() > 0; }
    inline const std::vector<std::string>& targetNames() const { return _targetNames; }
    void selectTargetNode(uint32_t index);
    inline uint32_t selectedTargetNodeIndex() const { return _selectedTargetNode; }
    
    inline std::shared_ptr<bg2e::scene::Node> sceneRoot() const { return _sceneRoot; }
    
    inline bg2e::scene::EnvironmentComponent * environment() { return _environment.lock().get(); }
    inline bg2e::scene::EnvironmentComponent * environment() const { return _environment.lock().get(); }
    
    inline bg2e::scene::OrbitCameraComponent * orbitCamera() { return _orbitCamera; }
    inline bg2e::scene::OrbitCameraComponent * orbitCamera() const { return _orbitCamera; }

    bg2e::scene::CameraComponent * cameraComponent();
    bg2e::scene::CameraComponent * cameraComponent() const;

    inline bool isModelValid() const { return _targetNode.get() != nullptr; }
    std::shared_ptr<bg2e::scene::Drawable> targetDrawable();

    void cleanup();

    bool checkUnsavedChanges();

    inline Document * document() { return _document.get(); }

    const std::vector<std::shared_ptr<bg2e::scene::LightComponent>>& lights() const
    {
        return _sceneRoot->scene()->lightComponents();
    }
    void iterateLights(std::function<void(std::shared_ptr<bg2e::scene::LightComponent>)> cb);
    [[nodiscard]] uint32_t maxLights() const { return 5; }

    void addLight();
    void removeLight(uint32_t index);

    void showFloor(bool show);
    [[nodiscard]] bool isFloorVisible() const { return _showFloor; }

    void setFlorHeight(float h);
    [[nodiscard]] float floorHeight() const { return _floorHeight; }

    void saveEnvironmentSettings();
    void saveEnvironmentSettings(const std::filesystem::path& path);
    void restoreEnvironmentSettings();
    void restoreEnvironmentSettings(const std::filesystem::path& path);

    void updateLightMesh(bg2e::scene::Node* lightNode);

protected:
    bg2e::render::Engine * _engine;

    AppDelegate * _appDelegate;

    std::unique_ptr<Document> _document;

    std::shared_ptr<bg2e::scene::Node> _sceneRoot;
    
    // This is the node where the loaded model is placed
    std::shared_ptr<bg2e::scene::Node> _targetNode = nullptr;

    std::shared_ptr<bg2e::scene::Drawable> _targetDrawable = nullptr;

    // Some file formats stores a scene (for example, GLTF). If you open a scene file, this
    // vector will contain all the nodes that contains a DrawableComponent. You can use the
    // targetDrawable utility functions to set the _targetDrawable from this list. When you open
    // a model file (for example, .bg2) this array will be empty
    std::vector<std::shared_ptr<bg2e::scene::Drawable>> _targetDrawables;
    uint32_t _selectedTargetNode = 0;

    // This contains the loaded scene when the file is a scene file (for example, GLTF).
    // This scene is not visible, is stored only for debugging purposes
    std::shared_ptr<bg2e::scene::Node> _targetScene = nullptr;

    // This vector stores the drawable nodes names when the loaded file is a model file.
    // Otherwise is empty
    std::vector<std::string> _targetNames;

    // Lighting setup
    std::shared_ptr<bg2e::scene::Node> _lightsNode;

    // Floor node
    std::shared_ptr<bg2e::scene::Node> _floorNode;
    bool _showFloor = true;
    float _floorHeight = 0.0f;

    std::weak_ptr<bg2e::scene::EnvironmentComponent> _environment;
    bg2e::scene::OrbitCameraComponent * _orbitCamera = nullptr;
    std::shared_ptr<bg2e::scene::Node> _environmentNode;
    bool _restoringEnvironment = false;
    std::shared_ptr<bg2e::app::SafeUpdateToken> _restoreToken;

// The "name" parameter is mandatory if the scene is not build yet
    std::shared_ptr<bg2e::scene::Node> createLightNode(
        bg2e::base::Light::LightType type,
        float azimuth,
        float elevation,
        float distance = 2.0f,
        const bg2e::base::Color& color = bg2e::base::Color::White(),
        const std::string & name = ""
    );

    std::shared_ptr<bg2e::geo::Mesh> _pointLightMesh;
    std::shared_ptr<bg2e::geo::Mesh> _spotLightMesh;
    std::shared_ptr<bg2e::geo::Mesh> _directionalLightMesh;

    std::shared_ptr<bg2e::geo::Mesh> getLightMesh(bg2e::base::Light::LightType type);

    std::shared_ptr<bg2e::scene::Node> createFloorNode();

    void instantUpdateLightMesh(bg2e::scene::Node* lightNode);
};

