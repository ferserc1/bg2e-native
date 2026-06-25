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

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/math/projections.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/manipulation/PickSelectionVisitor.hpp>
#include <bg2e/render/vulkan/Image.hpp>

#include <memory>
#include <vector>
#include <functional>

namespace bg2e {
namespace manipulation {

struct SelectionItem {
    std::weak_ptr<scene::Node> node;
    std::weak_ptr<scene::DrawableComponent> drawable;
    std::weak_ptr<scene::Drawable> mesh;
    uint32_t submesh;
    PickKind kind = PickKind::Drawable;

    [[nodiscard]] scene::Node * nodePtr() const { return node.lock().get(); }
    [[nodiscard]] scene::DrawableComponent * drawablePtr() const { return drawable.lock().get(); }
    [[nodiscard]] scene::Drawable * meshPtr() const { return mesh.lock().get(); }
    [[nodiscard]] bool expired() const { return node.expired() || drawable.expired() || mesh.expired(); }
};

typedef std::function<void()> OnSelectCallback;

class BG2E_API SelectionManager
{
public:
    struct PushConstantData {
        glm::mat4 mvp;
        uint32_t identifier;
    };

    SelectionManager(render::Engine * engine);
    virtual ~SelectionManager() = default;

    void init();

    bool pick(
        scene::Node * rootNode,
        const glm::mat4 & viewMatrix,
        const glm::mat4 & projMatrix,
        const math::Viewport & vp,
        uint32_t x,
        uint32_t y,
        bool additive = false
    );

    inline bool pick(
        scene::Node * rootNode,
        scene::CameraComponent * camera,
        uint32_t x,
        uint32_t y,
        bool additive = false
    ) {
        if (!camera->projection())
        {
            return false;
        }
        auto viewport = camera->projection()->viewport();
        auto projection = camera->projectionMatrix();
        auto viewMatrix = camera->ownerNode()->invertedWorldMatrix();
        return pick(rootNode, viewMatrix, projection, viewport, x, y, additive);
    }

    inline bool pick(
        scene::Scene * scene,
        uint32_t x,
        uint32_t y,
        bool additive = false
    ) {
        if (!scene->rootNode() || !scene->mainCamera())
        {
            return false;
        }
        auto rootNode = scene->rootNode();
        auto camera = scene->mainCamera();
        return pick(rootNode, camera, x, y, additive);
    }

    // This functions does not modify the selection
    bool pickObject(
        scene::Node * rootNode,
        const glm::mat4 & viewMatrix,
        const glm::mat4 & projMatrix,
        const math::Viewport & vp,
        uint32_t x,
        uint32_t y,
        SelectionItem * result
    );

    inline bool pickObject(
        scene::Node * rootNode,
        scene::CameraComponent * camera,
        uint32_t x,
        uint32_t y,
        SelectionItem * result
    ) {
        if (!camera->projection())
        {
            return false;
        }
        auto viewport = camera->projection()->viewport();
        auto projection = camera->projectionMatrix();
        auto viewMatrix = camera->ownerNode()->invertedWorldMatrix();
        return pickObject(rootNode, viewMatrix, projection, viewport, x, y, result);
    }

    inline bool pickObject(
        scene::Scene * scene,
        uint32_t x,
        uint32_t y,
        SelectionItem * result
    ) {
        if (!scene->rootNode() || !scene->mainCamera())
        {
            return false;
        }
        auto rootNode = scene->rootNode();
        auto camera = scene->mainCamera();
        return pickObject(rootNode, camera, x, y, result);
    }

    // --- Input handling ----------------------------------------------------
    // The app routes raw mouse events through these. The return value is true if
    // the event should still be propagated to the scene graph (camera
    // controllers), false if the SelectionManager captured it (transform gizmo
    // manipulation in progress). mouseButtonUp performs the click selection, but
    // only when the press did not move, so dragging the camera does not select.
    bool mouseButtonDown(scene::Scene * scene, int button, int x, int y);
    bool mouseMove(scene::Scene * scene, int x, int y);
    bool mouseButtonUp(scene::Scene * scene, int button, int x, int y, bool additive = false);

    // True while a transform gizmo handle is being manipulated; the app must not
    // forward input to the scene graph while this is true.
    [[nodiscard]] inline bool sceneInputBlocked() const { return _capturing; }

    // Enables/disables manipulation of the transform gizmo through the viewport.
    // When disabled the transform gizmo is purely visual and clicks pass through
    // to the objects behind it.
    [[nodiscard]] inline bool transformManipulationEnabled() const { return _transformManipulationEnabled; }
    inline void setTransformManipulationEnabled(bool e) { _transformManipulationEnabled = e; }

    // IMPORTANT: Be sure to deselect the elements you want to remove from the scene before deleting them.
    void deselect();

    // selectedItem returns the first item in the selection array, when multi select is enabled
    [[nodiscard]] inline std::shared_ptr<SelectionItem> selectedItem() const {
        return _selectedItems.empty() ? nullptr : _selectedItems.front();
    }

    [[nodiscard]] inline scene::Node * selectedNode() const
    {
        return selectedItem()
            ? selectedItem()->nodePtr()
            : (_selectedNodes.empty() ? nullptr : _selectedNodes.front().lock().get());
    }
    [[nodiscard]] inline scene::DrawableComponent * selectedDrawable() const { return selectedItem() ? selectedItem()->drawablePtr() : nullptr; }
    [[nodiscard]] inline scene::Drawable * selectedMesh() const { return selectedItem() ? selectedItem()->meshPtr() : nullptr; }
    [[nodiscard]] inline uint32_t selectedSubmesh() const { return selectedItem() ? selectedItem()->submesh : 0; }

    [[nodiscard]] inline bool multiSelection() const { return _multiSelection; }
    inline void setMultiSelectionEnabled(bool e) { _multiSelection = e; }
    [[nodiscard]] inline const std::vector<std::shared_ptr<SelectionItem>>& selectedItems() const { return _selectedItems; }
    [[nodiscard]] inline const std::vector<std::weak_ptr<scene::Node>>& selectedNodes() const { return _selectedNodes; }

    // Node-level selection. Returns true if the node appears in the unique
    // selected-nodes list (regardless of which/how many submeshes are selected).
    bool isSelected(const scene::Node * node);
    bool isSelected(const scene::Node * node, const scene::DrawableComponent * drawable, uint32_t submesh);
    bool isSelected(const scene::Node * node, const scene::Drawable * drawable, uint32_t submesh);

    bool addToSelectedItems(scene::Node * node, scene::DrawableComponent * drawable, uint32_t submesh);
    // Node-level selection overload. If the node owns a DrawableComponent it is
    // selected at submesh level (submesh 0) so it also highlights in the 3D view.
    // Otherwise the node is registered as a node-only selection. Node-only objects
    // (groups, lights, cameras...) can currently only be selected through the
    // SceneTree; a future refactor will allow picking them through their gizmo.
    bool addToSelectedItems(scene::Node * node);
    void removeFromSelectedItems(scene::Node * node);
    void removeFromSelectedItems(scene::Node * node, uint32_t submesh);
    void removeFromSelectedItems(scene::DrawableComponent* drawable, uint32_t submesh);

    [[nodiscard]] inline bool clearSelectionOnEmptyPick() const { return _clearSelectionOnEmptyPick; }
    inline void setClearSelectionOnEmptyPick(bool e) { _clearSelectionOnEmptyPick = e; }

    inline void onSelect(OnSelectCallback&& cb)
    {
        _onSelectCallbacks.push_back(std::move(cb));
    }

protected:
    render::Engine * _engine;
    
    VkPipeline _pipeline = VK_NULL_HANDLE;        // depth-tested (drawables, transform gizmo)
    VkPipeline _pipelineNoDepth = VK_NULL_HANDLE; // depth-disabled (type gizmos, on top)
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    std::shared_ptr<render::vulkan::Image> _image;
    std::shared_ptr<PickSelectionVisitor> _pickVisitor;

    bool _multiSelection = true;
    bool _clearSelectionOnEmptyPick = true;

    // Transform gizmo manipulation / input capture state.
    bool _transformManipulationEnabled = true;
    bool _capturing = false;
    int _mouseDownX = 0;
    int _mouseDownY = 0;
    std::vector<std::shared_ptr<SelectionItem>> _selectedItems;
    std::vector<std::weak_ptr<scene::Node>> _selectedNodes;

    uint32_t pickObjectId(
        scene::Node * rootNode,
        const glm::mat4 & viewMatrix,
        const glm::mat4 & projMatrix,
        const math::Viewport & vp,
        uint32_t x,
        uint32_t y
    );
    void createImage();
    void createPipeline();
    void cleanupImage();

    std::vector<OnSelectCallback> _onSelectCallbacks;
    void callOnChange();

    // _selectedNodes maintenance helpers. _selectedNodes always holds the unique
    // set of currently selected nodes and is kept in sync by every mutation.
    void addUniqueSelectedNode(scene::Node * node);
    void removeSelectedNode(scene::Node * node);
    bool nodeHasSelectedItems(const scene::Node * node) const;
    // If a node owning a drawable is selected but is not at index 0, move it there.
    void prioritizeDrawableNode();
};

}
}
