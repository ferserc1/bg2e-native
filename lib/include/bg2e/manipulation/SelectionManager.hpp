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

class BG2E_API SelectionManager {
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

    void deselect();

    inline scene::Node * selectedNode() { return _selectedItem ? _selectedItem->node : nullptr; }
    inline scene::DrawableComponent * selectedDrawable() { return _selectedItem ? _selectedItem->drawable : nullptr; }
    inline scene::Drawable * selectedMesh() { return _selectedItem ? _selectedItem->mesh : nullptr; }
    inline uint32_t selectedSubmesh() { return _selectedItem ? _selectedItem->submesh : 0; }
    
protected:
    render::Engine * _engine;
    
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    std::shared_ptr<render::vulkan::Image> _image;
    std::shared_ptr<PickSelectionVisitor> _pickVisitor;

    std::shared_ptr<SelectionItem> _selectedItem;
    
    void createImage();
    void createPipeline();
    void cleanupImage();
};

}
}
