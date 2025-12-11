//
//  StageScene.hpp
#pragma once

#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/scene/OrbitCameraComponent.hpp>

#include <Document.hpp>

#include <memory>
#include <filesystem>

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
    
    inline bg2e::scene::EnvironmentComponent * environment() { return _environment; }
    inline bg2e::scene::EnvironmentComponent * environment() const { return _environment; }
    
    inline bg2e::scene::OrbitCameraComponent * orbitCamera() { return _orbitCamera; }
    inline bg2e::scene::OrbitCameraComponent * orbitCamera() const { return _orbitCamera; }

    inline bool isModelValid() const { return _targetNode.get() != nullptr; }
    std::shared_ptr<bg2e::scene::Drawable> targetDrawable();

    void cleanup();

    bool checkUnsavedChanges();

    inline Document * document() { return _document.get(); }

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

    bg2e::scene::EnvironmentComponent * _environment;
    bg2e::scene::OrbitCameraComponent * _orbitCamera;
};

