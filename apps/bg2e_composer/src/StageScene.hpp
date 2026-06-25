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
    void openScene(const std::filesystem::path& path, bg2e::scene::SceneProgressCallback progressCallback);

    // Save the editable scene to disk
    void saveScene(const std::filesystem::path& path);

    // Import a .bg2 model created with model_edit as a new node in the scene
    void importModelBg2(const std::filesystem::path& path);

    // Scene menu actions. New nodes hang from the primary selected node (or from
    // the editable root if there is no selection) and are placed in front of the
    // camera, at _createNodeDistance units along the camera view direction.
    void addLightNode();
    void addCubeNode();
    void addSphereNode();
    void addEmptyNode();

    // Duplicates the primary selected node as a sibling (same parent). The copy is
    // a full deep copy: every component is cloned by value (drawables duplicate
    // their mesh, materials and GPU resources, never sharing them with the source)
    // and the whole child subtree is cloned recursively. The new node is named
    // "<source name> Copy" and every cloned drawable is renamed so saving never
    // overwrites the source's mesh asset. Does nothing if there is no selection or
    // the selection is the editable root.
    void duplicateSelectedNode();

    // Removes the primary selected node from the scene. Asks for confirmation
    // because the action cannot be undone. Does nothing if there is no selection.
    void removeSelectedNode();

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

    // Distance (in world units) from the camera at which newly created nodes are
    // placed, measured along the camera view direction.
    const float _createNodeDistance = 5.0f;

    std::shared_ptr<bg2e::scene::Node> buildDefaultScene();
    void setEditableRoot(std::shared_ptr<bg2e::scene::Node> newEditableRoot);

    // Node that newly created nodes hang from: the primary selected node, or the
    // editable root if nothing is selected.
    std::shared_ptr<bg2e::scene::Node> newNodeParent();

    // Local-space translation that places a node, child of parent, in front of
    // the camera at _createNodeDistance. Accounts for parent's world transform so
    // the node lands at the desired world position regardless of where parent is.
    glm::vec3 placementLocalPosition(bg2e::scene::Node* parent);

    // Adds the gizmo/selection components, inserts node under parent and refreshes
    // the scene inside a safe update, then marks the document as modified.
    void insertNewNode(std::shared_ptr<bg2e::scene::Node> node, std::shared_ptr<bg2e::scene::Node> parent);

    // If the scene's main camera has no projection, assign an optical projection
    // (50 mm film size, 55 mm lens) so the aspect ratio renders correctly
    void ensureMainCameraProjection(bg2e::scene::Scene * scene);

    // Adds GizmoComponent to every camera, light and environment node in root
    // that does not already have one
    void addGizmoComponents(bg2e::scene::Node * root);
};
