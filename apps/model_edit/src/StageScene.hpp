//
//  StageScene.hpp
#pragma once

#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/scene/OrbitCameraComponent.hpp>

#include <memory>
#include <filesystem>

class StageScene {
public:
    StageScene(bg2e::render::Engine * engine);
    
    std::shared_ptr<bg2e::scene::Node> init();
    
    void loadModel(const std::filesystem::path& path);
    
    inline std::shared_ptr<bg2e::scene::Node> sceneRoot() const { return _sceneRoot; }
    
    inline bg2e::scene::EnvironmentComponent * environment() { return _environment; }
    inline bg2e::scene::EnvironmentComponent * environment() const { return _environment; }
    
    inline bg2e::scene::OrbitCameraComponent * orbitCamera() { return _orbitCamera; }
    inline bg2e::scene::OrbitCameraComponent * orbitCamera() const { return _orbitCamera; }
    
protected:
    bg2e::render::Engine * _engine;
    
    std::shared_ptr<bg2e::scene::Node> _sceneRoot;
    
    // This is the node where the loaded model is placed
    std::shared_ptr<bg2e::scene::Node> _targetNode;
    
    bg2e::scene::EnvironmentComponent * _environment;
    bg2e::scene::OrbitCameraComponent * _orbitCamera;
};

