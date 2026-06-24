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

#include <bg2e/manipulation/SelectionManager.hpp>

#include <bg2e/render/all.hpp>

#include "bg2e/base/Log.hpp"
#include "bg2e/manipulation/SelectableComponent.hpp"
#include "bg2e/manipulation/GizmoComponent.hpp"

namespace bg2e::manipulation {

SelectionManager::SelectionManager(render::Engine * engine)
    : _engine(engine)
{

}

void SelectionManager::init()
{
    createImage();
    createPipeline();
    
    _pickVisitor = std::make_shared<manipulation::PickSelectionVisitor>();
}

bool SelectionManager::pick(
    scene::Node * rootNode,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projMatrix,
    const math::Viewport & vp,
    uint32_t x,
    uint32_t y,
    bool additive
) {
    if (!additive)
    {
        deselect();
    }

    SelectionItem result {};
    if (pickObject(rootNode, viewMatrix, projMatrix, vp, x, y, &result))
    {
        auto nodePtr = result.nodePtr();

        // Transform gizmo parts are not selectable: manipulation is started on
        // mouse down (see mouseButtonDown), so ignore them here.
        if (result.kind == PickKind::TransformGizmo)
        {
            return true;
        }

        // Light/environment/camera gizmo: node-level selection (additive /
        // subtractive), same as selecting from the scene tree.
        if (result.kind == PickKind::Gizmo)
        {
            if (isSelected(nodePtr))
            {
                removeFromSelectedItems(nodePtr);
            }
            else
            {
                addToSelectedItems(nodePtr);
            }
            return true;
        }

        // Regular drawable submesh.
        auto drawablePtr = result.drawablePtr();
        if (isSelected(nodePtr, drawablePtr, result.submesh))
        {
            removeFromSelectedItems(nodePtr, result.submesh);
        }
        else
        {
            addToSelectedItems(nodePtr, drawablePtr, result.submesh);
        }
        return true;
    }
    else if (_clearSelectionOnEmptyPick)
    {
        deselect();
    }

    return false;
}

bool SelectionManager::mouseButtonDown(scene::Scene * scene, int button, int x, int y)
{
    _mouseDownX = x;
    _mouseDownY = y;

    // Only attempt a transform-handle interaction with the left button, when
    // manipulation is enabled and a transform gizmo is currently visible.
    if (button == 0 && _transformManipulationEnabled && GizmoComponent::currentTransformNode())
    {
        SelectionItem result {};
        if (pickObject(scene, x, y, &result) && result.kind == PickKind::TransformGizmo)
        {
            if (auto node = result.nodePtr())
            {
                if (auto gizmo = node->getComponent<GizmoComponent>())
                {
                    if (auto drw = gizmo->transformDrawable())
                    {
                        auto handle = GizmoComponent::handleForSubmesh(
                            drw->submeshName(result.submesh),
                            drw->submeshGroupName(result.submesh)
                        );
                        gizmo->beginTransform(handle, x, y);
                        _capturing = true;
                        return false; // captured: do not propagate to the scene graph
                    }
                }
            }
        }
    }

    return true; // not captured: propagate (camera controllers, etc.)
}

bool SelectionManager::mouseMove(scene::Scene* scene, int x, int y)
{
    if (_capturing)
    {
        if (auto node = GizmoComponent::currentTransformNode())
        {
            if (auto gizmo = node->getComponent<GizmoComponent>())
            {
                gizmo->updateTransform(scene, x, y);
            }
        }
        return false;
    }
    return true;
}

bool SelectionManager::mouseButtonUp(scene::Scene * scene, int button, int x, int y, bool additive)
{
    if (_capturing)
    {
        if (auto node = GizmoComponent::currentTransformNode())
        {
            if (auto gizmo = node->getComponent<GizmoComponent>())
            {
                gizmo->endTransform();
            }
        }
        _capturing = false;
        return false; // the press was captured; do not propagate the release
    }

    // Click selection only if the press did not move, so dragging the camera
    // does not select.
    if (button == 0 && x == _mouseDownX && y == _mouseDownY)
    {
        bg2e_log_debug << "Additive: " << additive << bg2e_log_end;
        pick(scene, x, y, additive);
    }

    return true; // propagate the release to the scene graph
}

bool SelectionManager::pickObject(
    scene::Node * rootNode,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projMatrix,
    const math::Viewport & vp,
    uint32_t x,
    uint32_t y,
    SelectionItem * result
) {
    if (!result)
    {
        throw std::runtime_error("SelectionManager::pickObject: result is null");
    }

    auto objectId = pickObjectId(rootNode, viewMatrix, projMatrix, vp, x, y);
    if (auto objectData = _pickVisitor->findObject(objectId))
    {
        result->node = objectData->node->weak_from_this();
        result->submesh = objectData->submeshIndex;
        result->kind = objectData->kind;

        // Only regular drawable picks carry drawable/mesh. Gizmo picks select
        // the node, and transform-gizmo picks are handled as interactions.
        if (objectData->kind == PickKind::Drawable)
        {
            auto drawableComp = objectData->node->getComponent<scene::DrawableComponent>();
            result->drawable = drawableComp ? std::weak_ptr<scene::DrawableComponent>(std::dynamic_pointer_cast<scene::DrawableComponent>(drawableComp->shared_from_this())) : std::weak_ptr<scene::DrawableComponent>{};
            result->mesh = drawableComp ? drawableComp->drawable() : std::shared_ptr<scene::Drawable>{};
        }
        else
        {
            result->drawable = std::weak_ptr<scene::DrawableComponent>{};
            result->mesh = std::weak_ptr<scene::Drawable>{};
        }
        return true;
    }

    return false;
}

void SelectionManager::deselect()
{
    for (const auto & item : _selectedItems)
    {
        if (auto node = item->node.lock())
        {
            if (const auto& sel = node->getComponent<SelectableComponent>())
            {
                sel->setSelected(item->submesh, false);
            }
        }
    }

    _selectedItems.clear();
    _selectedNodes.clear();
    callOnChange();
}

bool SelectionManager::isSelected(const scene::Node * node)
{
    if (!node)
    {
        return false;
    }

    return std::any_of(
        _selectedNodes.cbegin(),
        _selectedNodes.cend(),
        [node](const std::weak_ptr<scene::Node>& n) { return n.lock().get() == node; }
    );
}

bool SelectionManager::isSelected(const scene::Node * node, const scene::DrawableComponent * drawable, uint32_t submesh)
{
    return isSelected(node, drawable->drawable().get(), submesh);
}

bool SelectionManager::isSelected(const scene::Node * node, const scene::Drawable * drawable, uint32_t submesh)
{
    if (!node || !drawable)
    {
        return false;
    }

    return std::find_if(
        _selectedItems.cbegin(),
        _selectedItems.cend(),
        [node, drawable, submesh](const std::shared_ptr<SelectionItem>& curItem)
        {
            auto curNode = curItem->node.lock();
            auto curMesh = curItem->mesh.lock();
            return curNode.get() == node && curMesh.get() == drawable && submesh == curItem->submesh;
        }
    ) != _selectedItems.cend();
}

bool SelectionManager::addToSelectedItems(scene::Node * node, scene::DrawableComponent * drawable, uint32_t submesh)
{
    // Are the pointets valid?
    if (!node || !drawable || !drawable->drawable().get())
    {
        return false;
    }

    // Is the node selectable?
    auto selComponent = node->getComponent<SelectableComponent>();
    if (!selComponent)
    {
        return false;
    }

    // Is the submesh index valid?
    auto mesh = drawable->drawable();
    if (submesh >= mesh->submeshesCount())
    {
        return false;
    }

    // If the node is already selected, return true
    if (isSelected(node, drawable, submesh))
    {
        return true;
    }

    // If multi selection is disabled, deselect all items
    if (!multiSelection())
    {
        deselect();
    }

    // Build the SelectionItem class
    auto selectItem = std::make_shared<SelectionItem>();
    selectItem->node = node->weak_from_this();
    selectItem->drawable = std::weak_ptr<scene::DrawableComponent>(std::dynamic_pointer_cast<scene::DrawableComponent>(drawable->shared_from_this()));
    selectItem->mesh = mesh;
    selectItem->submesh = submesh;
    _selectedItems.push_back(selectItem);

    // Mark the submesh as selected
    selComponent->setSelected(submesh, true);

    // Keep the unique node list in sync
    addUniqueSelectedNode(node);
    callOnChange();

    return true;
}

bool SelectionManager::addToSelectedItems(scene::Node * node)
{
    if (!node)
    {
        return false;
    }

    // If the node owns a drawable, select it at submesh level (submesh 0) so it
    // also highlights in the 3D view, reusing all the validation above.
    if (auto * drawable = node->drawable())
    {
        return addToSelectedItems(node, drawable, 0);
    }

    // Node-only selection (groups, lights, cameras...). Only reachable from the
    // SceneTree for now.
    if (isSelected(node))
    {
        return true;
    }

    if (!multiSelection())
    {
        deselect();
    }

    addUniqueSelectedNode(node);
    callOnChange();

    return true;
}

void SelectionManager::removeFromSelectedItems(scene::Node * node)
{
    if (!node)
    {
        throw std::runtime_error("SelectionManager: invalid node pointer supplied calling removeFromSelectedItems()");
    }
    bool changed = isSelected(node);
    std::erase_if(
        _selectedItems,
        [&, node](const std::shared_ptr<SelectionItem>& curItem)
        {
            auto curNode = curItem->node.lock();
            if (!curNode || curNode.get() != node)
            {
                return false;
            }
            auto sel = curNode->getComponent<SelectableComponent>();
            if (!sel)
            {
                return false;
            }
            sel->setSelected(curItem->submesh, false);
            changed = true;
            return true;
        }
    );

    removeSelectedNode(node);

    if (changed)
    {
        callOnChange();
    }
}

void SelectionManager::removeFromSelectedItems(scene::Node * node, uint32_t submesh)
{
    const auto drawable = const_cast<scene::DrawableComponent*>(node->getComponent<scene::DrawableComponent>());
    return removeFromSelectedItems(drawable, submesh);
}

void SelectionManager::removeFromSelectedItems(scene::DrawableComponent* drawable, uint32_t submesh)
{
    if (!drawable || !drawable->ownerNode())
    {
        throw std::runtime_error("SelectionManager: invalid drawable pointer supplied calling removeFromSelectedItems()");
    }
    auto node = drawable->ownerNode();
    bool changed = false;
    std::erase_if(
        _selectedItems,
        [&, drawable, submesh](const std::shared_ptr<SelectionItem>& curItem)
        {
            auto curNode = curItem->node.lock();
            if (!curNode)
            {
                return false;
            }
            auto sel = curNode->getComponent<SelectableComponent>();
            if (!sel)
            {
                return false;
            }
            auto curDrawable = curItem->drawable.lock();
            if (curDrawable.get() == drawable && curItem->submesh == submesh)
            {
                sel->setSelected(curItem->submesh, false);
                changed = true;
                return true;
            }
            return false;
        }
    );

    // If the node has no remaining selected submeshes, drop it from the node list
    if (changed && !nodeHasSelectedItems(node))
    {
        removeSelectedNode(node);
    }

    if (changed)
    {
        callOnChange();
    }
}

uint32_t SelectionManager::pickObjectId(
    scene::Node * rootNode,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projMatrix,
    const math::Viewport & vp,
    uint32_t x,
    uint32_t y
) {
    using namespace render::vulkan;

    if (_image->extent2D().width != static_cast<uint32_t>(vp.width) ||
        _image->extent2D().height != static_cast<uint32_t>(vp.height)
    ) {
        cleanupImage();
        createImage();
    }

    _engine->command().immediateSubmit([&](VkCommandBuffer cmd) {
        Image::cmdTransitionImage(
            cmd,
            _image->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL
        );
        VkClearColorValue clearValue { { 0.0f, 0.0f, 0.0f, 0.0f } };
        auto clearRange = Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdClearColorImage(
            cmd,
            _image->handle(), VK_IMAGE_LAYOUT_GENERAL,
            &clearValue, 1, &clearRange
        );

        Image::cmdTransitionImage(
            cmd,
            _image->handle(),
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );

        auto colorAttachment = Info::attachmentInfo(_image->imageView(), nullptr);
        auto depthAttachment = Info::depthAttachmentInfo(_engine->swapchain().depthImage()->imageView());
        auto renderInfo = Info::renderingInfo(_image->extent2D(), &colorAttachment, &depthAttachment);

        cmdBeginRendering(cmd, &renderInfo);

        macros::cmdSetDefaultViewportAndScissor(cmd, _image->extent2D());

        // The visitor binds the pipelines itself: depth-tested for drawables and
        // the transform gizmo, depth-disabled for the type gizmos.
        _pickVisitor->pick(
            rootNode, viewMatrix, projMatrix, cmd, _pipelineLayout,
            _pipeline, _pipelineNoDepth, _image->extent2D(),
            _transformManipulationEnabled
        );

        cmdEndRendering(cmd);
    });

    // Read pixels in image
    std::vector<uint8_t> outData = { 0, 0, 0, 0 };
    Image::readPixelsRGBA8(
        _engine,
        _image.get(),
        x, y,
        1, 1,
        outData,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    return SelectableComponent::decodeObjectId(outData.data());
}

void SelectionManager::createImage()
{
    using namespace bg2e::render::vulkan;

    auto extent = _engine->swapchain().extent();
    _image = std::shared_ptr<render::vulkan::Image>(
        render::vulkan::Image::createAllocatedImage(
            _engine,
            "SelectionManager pick image buffer",
            VK_FORMAT_R8G8B8A8_UNORM,
            extent,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1, false, 20, VK_SAMPLE_COUNT_1_BIT
        )
    );
    
    _engine->cleanupManager().push([&](VkDevice) {
        _image.reset();
    });
}

void SelectionManager::createPipeline()
{
    using namespace bg2e::render::vulkan;
    factory::GraphicsPipeline plFactory(_engine);
    
    plFactory.addShader("pick_selection.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("pick_selection.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    render::vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addPushConstantRange(0, sizeof(PushConstantData), VK_SHADER_STAGE_VERTEX_BIT);
    _pipelineLayout = layoutFactory.build("SelectionManager::PipelineLayout");
    
    plFactory.setInputState<render::vulkan::geo::Mesh>();
    plFactory.setColorAttachmentFormat(_image->format());
    plFactory.setDepthFormat(_engine->swapchain().depthImageFormat());
    plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    _pipeline = plFactory.build(_pipelineLayout, "SelectionManager::Pipeline");

    // Depth-disabled variant for type gizmos, so they pick on top of the scene
    // (matching the visual gizmo renderer). Same layout and shaders.
    plFactory.disableDepthtest();
    _pipelineNoDepth = plFactory.build(_pipelineLayout, "SelectionManager::PipelineNoDepth");

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipeline(dev, _pipelineNoDepth, nullptr);
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
    });
}

void SelectionManager::cleanupImage()
{
    _image.reset();
}

void SelectionManager::callOnChange()
{
    prioritizeDrawableNode();

    // Point the transform gizmo at the primary selected node before notifying
    // listeners, so onSelect callbacks already observe the current transform.
    scene::Node * primary = _selectedNodes.empty() ? nullptr : _selectedNodes.front().lock().get();
    GizmoComponent::setCurrentTransform(primary);

    for (auto & cb : _onSelectCallbacks)
    {
        cb();
    }
}

void SelectionManager::addUniqueSelectedNode(scene::Node * node)
{
    if (!node)
    {
        return;
    }

    const bool exists = std::any_of(
        _selectedNodes.begin(), _selectedNodes.end(),
        [node](const std::weak_ptr<scene::Node>& n) { return n.lock().get() == node; }
    );

    if (!exists)
    {
        _selectedNodes.push_back(node->weak_from_this());
    }
}

void SelectionManager::removeSelectedNode(scene::Node * node)
{
    std::erase_if(
        _selectedNodes,
        [node](const std::weak_ptr<scene::Node>& n)
        {
            auto locked = n.lock();
            return !locked || locked.get() == node;
        }
    );
}

bool SelectionManager::nodeHasSelectedItems(const scene::Node * node) const
{
    return std::any_of(
        _selectedItems.cbegin(), _selectedItems.cend(),
        [node](const std::shared_ptr<SelectionItem>& item) { return item->node.lock().get() == node; }
    );
}

void SelectionManager::prioritizeDrawableNode()
{
    if (_selectedNodes.size() < 2)
    {
        return;
    }

    // If index 0 already owns a drawable, nothing to do
    if (auto front = _selectedNodes.front().lock())
    {
        if (front->drawable())
        {
            return;
        }
    }

    for (std::size_t i = 1; i < _selectedNodes.size(); ++i)
    {
        auto candidate = _selectedNodes[i].lock();
        if (candidate && candidate->drawable())
        {
            auto wp = _selectedNodes[i];
            _selectedNodes.erase(_selectedNodes.begin() + i);
            _selectedNodes.insert(_selectedNodes.begin(), wp);
            return;
        }
    }
}

}
