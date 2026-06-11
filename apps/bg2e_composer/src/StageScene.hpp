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
#include <bg2e/render/Engine.hpp>
#include <bg2e/scene/OrbitCameraComponent.hpp>
#include <bg2e/app/MainLoop.hpp>

#include <Document.hpp>

#include <memory>
#include <filesystem>
#include <functional>

#include "bg2e/scene/Scene.hpp"

class AppDelegate;

class StageScene {
public:
    using OnSceneSwapCallback = std::function<void()>;

    StageScene(bg2e::render::Engine * engine, AppDelegate * appDelegate);

    // Builds containerRoot + default editable scene; returns containerRoot
    std::shared_ptr<bg2e::scene::Node> init();

    // Replace the whole editable scene with one loaded from disk
    void openScene(const std::filesystem::path& path);

    // Save the editable scene to disk
    void saveScene(const std::filesystem::path& path);

    // Import a .bg2 model created with model_edit as a new node in the scene
    void importModelBg2(const std::filesystem::path& path);

    // Empty the editable scene (deselect first)
    void close();

    void cleanup();

    bool checkUnsavedChanges();

    inline bool isSceneValid() const { return _editableRoot != nullptr; }

    inline std::shared_ptr<bg2e::scene::Node> sceneRoot() const { return _containerRoot; }
    // The editable subtree shown in the scene tree
    inline std::shared_ptr<bg2e::scene::Node> editableRoot() const { return _editableRoot; }

    // Resolved from the current scene content (not stored)
    bg2e::scene::OrbitCameraComponent * orbitCamera();
    bg2e::scene::CameraComponent * cameraComponent();

    inline Document * document() { return _document.get(); }
    inline bg2e::render::Engine * engine() const { return _engine; }

    inline void onSceneSwap(OnSceneSwapCallback cb) { _onSceneSwap = std::move(cb); }

protected:
    bg2e::render::Engine * _engine;
    AppDelegate * _appDelegate;
    std::unique_ptr<Document> _document;

    std::shared_ptr<bg2e::scene::Node> _containerRoot;
    std::shared_ptr<bg2e::scene::Node> _editableRoot;

    std::shared_ptr<bg2e::app::SafeUpdateToken> _swapToken;

    OnSceneSwapCallback _onSceneSwap;

    std::shared_ptr<bg2e::scene::Node> buildDefaultScene();
    void setEditableRoot(std::shared_ptr<bg2e::scene::Node> newEditableRoot);

    // If the scene's main camera has no projection, assign an optical projection
    // (50 mm film size, 55 mm lens) so the aspect ratio renders correctly
    void ensureMainCameraProjection(bg2e::scene::Scene * scene);

    // Adds GizmoComponent to every camera, light and environment node in root
    // that does not already have one
    void addGizmoComponents(bg2e::scene::Node * root);
};
