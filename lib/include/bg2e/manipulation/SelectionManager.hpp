//
//  SelectionManager.hpp

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

namespace bg2e {
namespace manipulation {

struct SelectionItem {
    scene::Node * node;
    scene::DrawableComponent* drawable;
    scene::Drawable* mesh;
    uint32_t submesh;
};

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
        uint32_t y
    );

    inline bool pick(
        scene::Node * rootNode,
        scene::CameraComponent * camera,
        uint32_t x,
        uint32_t y
    ) {
        if (!camera->projection())
        {
            return false;
        }
        auto viewport = camera->projection()->viewport();
        auto projection = camera->projectionMatrix();
        auto viewMatrix = camera->ownerNode()->invertedWorldMatrix();
        return pick(rootNode, viewMatrix, projection, viewport, x, y);
    }

    inline bool pick(
        scene::Scene * scene,
        uint32_t x,
        uint32_t y
    ) {
        if (!scene->rootNode() || !scene->mainCamera())
        {
            return false;
        }
        auto rootNode = scene->rootNode();
        auto camera = scene->mainCamera();
        return pick(rootNode, camera, x, y);
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

    void deselect();

    // selectedItem returns the first item in the selection array, when multi select is enabled
    [[nodiscard]] inline std::shared_ptr<SelectionItem> selectedItem() const { return _selectedItem; }

    [[nodiscard]] inline scene::Node * selectedNode() const { return selectedItem() ? selectedItem()->node : nullptr; }
    [[nodiscard]] inline scene::DrawableComponent * selectedDrawable() const { return selectedItem() ? selectedItem()->drawable : nullptr; }
    [[nodiscard]] inline scene::Drawable * selectedMesh() const { return selectedItem() ? selectedItem()->mesh : nullptr; }
    [[nodiscard]] inline uint32_t selectedSubmesh() const { return selectedItem() ? selectedItem()->submesh : 0; }

    [[nodiscard]] inline bool multiSelection() const { return _multiSelection; }
    inline void setMultiSelectionEnabled(bool e) { _multiSelection = e; }
    [[nodiscard]] inline const std::vector<std::shared_ptr<SelectionItem>>& selectedItems() const { return _selectedItems; }

    bool isSelected(const scene::Node * node, const scene::DrawableComponent * drawable, uint32_t submesh);
    bool isSelected(const scene::Node * node, const scene::Drawable * drawable, uint32_t submesh);
    bool addToSelectedItems(scene::Node * node, scene::DrawableComponent * drawable, uint32_t submesh);
    void removeFromSelectedItems(scene::Node * node);
    void removeFromSelectedItems(scene::Node * node, uint32_t submesh);
    void removeFromSelectedItems(scene::DrawableComponent* drawable, uint32_t submesh);

    [[nodiscard]] inline bool clearSelectionOnEmptyPick() const { return _clearSelectionOnEmptyPick; }
    inline void setClearSelectionOnEmptyPick(bool e) { _clearSelectionOnEmptyPick = e; }

protected:
    render::Engine * _engine;
    
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    std::shared_ptr<render::vulkan::Image> _image;
    std::shared_ptr<PickSelectionVisitor> _pickVisitor;

    std::shared_ptr<SelectionItem> _selectedItem;

    bool _multiSelection = true;
    bool _clearSelectionOnEmptyPick = true;
    std::vector<std::shared_ptr<SelectionItem>> _selectedItems;

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
};

}
}
