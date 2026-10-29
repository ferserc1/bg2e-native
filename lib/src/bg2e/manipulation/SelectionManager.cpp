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

#include <bg2e/base/Log.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/manipulation/GizmoComponent.hpp>

namespace bg2e::manipulation {

bool SelectionItem::expired() const
{
    auto n = node.lock();
    if (!n)
    {
        return true;
    }
    if (kind != PickKind::Drawable)
    {
        return false;
    }
    auto component = drawable.lock();
    auto resource = mesh.lock();
    if (!component || component->ownerNode() != n.get() || n->drawable() != component.get() ||
        !resource || component->drawable() != resource)
    {
        return true;
    }
    auto selectable = n->getComponent<SelectableComponent>();
    if (!selectable || submesh >= resource->submeshesCount())
    {
        return true;
    }
    selectable->update(0);
    return selectionIdentifier && selectable->identifier(submesh) != selectionIdentifier;
}

std::shared_ptr<draw::Drawable> SelectionItem::lockMesh() const
{
    return !expired() ? mesh.lock() : nullptr;
}

scene::Node* SelectionItem::nodePtr() const
{
    return !expired() ? node.lock().get() : nullptr;
}

scene::DrawableComponent* SelectionItem::drawablePtr() const
{
    return !expired() ? drawable.lock().get() : nullptr;
}

draw::Drawable* SelectionItem::meshPtr() const
{
    return lockMesh().get();
}

SelectionState SelectionManager::captureSelection() const
{
    SelectionState result;
    result.nodes = _selectedNodes;
    result.items.reserve(_selectedItems.size());
    for (const auto& item : _selectedItems)
    {
        result.items.push_back(*item);
    }
    return result;
}


void SelectionManager::restoreSelection(const SelectionState& state,
    const std::function<bool(scene::Node*)>& belongs)
{
    std::vector<std::shared_ptr<SelectionItem>> items;
    std::vector<std::weak_ptr<scene::Node>> nodes;
    for (const auto& weak : state.nodes)
    {
        if (auto node = weak.lock(); node && (!belongs || belongs(node.get())))
        {
            const bool hadGeometry = std::any_of(state.items.begin(), state.items.end(), [&](const auto& item)
            {
                return item.node.lock() == node;
            });
            const bool hasGeometry = std::any_of(state.items.begin(), state.items.end(), [&](const auto& item)
            {
                return item.node.lock() == node && bool(item.lockMesh());
            });
            if (hadGeometry && !hasGeometry)
            {
                continue; // Stale geometry must not become a node-only selection.
            }
            if (std::none_of(nodes.begin(), nodes.end(), [&](const auto& entry)
            {
                return entry.lock() == node;
            }))
            {
                nodes.push_back(node);
            }
        }
    }
    for (const auto& item : state.items)
    {
        auto node = item.node.lock();
        if (node && (!belongs || belongs(node.get())) && item.lockMesh())
        {
            items.push_back(std::make_shared<SelectionItem>(item));
        }
    }
    cancelTransform();
    // Allocate snapshot storage before changing highlight state or membership.
    for (const auto& item : _selectedItems)
    {
        if (auto node = item->node.lock())
        {
            if (auto selectable = node->getComponent<SelectableComponent>())
            {
                selectable->setSelected(item->submesh, false);
            }
        }
    }
    _selectedItems.swap(items);
    _selectedNodes.swap(nodes);
    for (const auto& item : _selectedItems)
    {
        if (auto node = item->node.lock())
        {
            if (auto selectable = node->getComponent<SelectableComponent>())
            {
                selectable->setSelected(item->submesh, true);
            }
        }
    }
    callOnChange();
}


void SelectionManager::refreshSelection()
{
    std::vector<scene::Node*> affected;
    const auto removed = std::erase_if(_selectedItems, [&](const std::shared_ptr<SelectionItem>& item)
    {
        if (!item || item->expired())
        {
            if (item)
            {
                if (auto n = item->node.lock())
                {
                    if (auto selectable = n->getComponent<SelectableComponent>())
                    {
                        selectable->update(0);
                    }
                    affected.push_back(n.get());
                }
            }
            return true;
        }
        return false;
    });
    const auto removedNodes = std::erase_if(_selectedNodes, [&](const std::weak_ptr<scene::Node>& weak)
    {
        auto n = weak.lock();
        return !n || (std::find(affected.begin(), affected.end(), n.get()) != affected.end() && !nodeHasSelectedItems(n.get()));
    });
    if (removed || removedNodes)
    {
        callOnChange();
    }
}

bool SelectionManager::isSelected(const scene::Node* node, const draw::Drawable* drawable, uint32_t submesh)
{
    refreshSelection();
    if (!node || !drawable)
    {
        return false;
    }
    return std::any_of(_selectedItems.begin(), _selectedItems.end(), [&](const std::shared_ptr<SelectionItem>& item)
    {
        return item->node.lock().get() == node && item->lockMesh().get() == drawable && item->submesh == submesh;
    });
}

SelectionManager::SelectionManager(draw::Engine* engine)
    : _drawEngine(engine)
{
    if (!engine)
    {
        throw std::invalid_argument("SelectionManager requires a draw engine");
    }
}

void SelectionManager::init()
{
    // GPU picking resources are invocation-local; CPU selection needs no device.
}

bool SelectionManager::pick(
    scene::Node* rootNode,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix,
    const math::Viewport& vp,
    uint32_t x,
    uint32_t y,
    bool additive)
{
    SelectionItem result{};
    if (pickObject(rootNode, viewMatrix, projMatrix, vp, x, y, &result))
    {
        auto nodePtr = result.nodePtr();

        // Transform gizmo parts are not selectable: manipulation is started on
        // mouse down (see mouseButtonDown), so ignore them here.
        if (result.kind == PickKind::TransformGizmo)
        {
            return true;
        }

        if (_pickRequested)
        {
            _pickRequested(result, additive);
            return true;
        }
        if (!additive)
        {
            deselect();
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
        if (_pickRequested)
        {
            _pickRequested({}, false);
        }
        else
        {
            deselect();
        }
    }

    return false;
}

bool SelectionManager::mouseButtonDown(scene::Scene* scene, int button, int x, int y)
{
    _mouseDownX = x;
    _mouseDownY = y;
    if (button == 0)
    {
        _leftPressed = true;
    }

    // Only attempt a transform-handle interaction with the left button, when
    // manipulation is enabled and a transform gizmo is currently visible.
    if (button == 0 && _transformManipulationEnabled && GizmoComponent::currentTransformNode())
    {
        SelectionItem result{};
        if (pickObject(scene, x, y, &result) && result.kind == PickKind::TransformGizmo)
        {
            if (auto node = result.nodePtr())
            {
                if (auto gizmo = node->getComponent<GizmoComponent>())
                {
                    const auto modern = gizmo->transformDrawable();
                    if (modern)
                    {
                        auto handle = GizmoComponent::handleForSubmesh(
                            modern->submeshName(result.submesh),
                            modern->submeshGroupName(result.submesh));
                        gizmo->setTransformEditObserver(_transformEditObserver);
                        gizmo->beginTransform(handle, x, y);
                        _capturing = gizmo->isTransforming();
                        if (_capturing)
                        {
                            _capturedGizmo = std::static_pointer_cast<GizmoComponent>(gizmo->shared_from_this());
                        }
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
        if (_capturedGizmo)
        {
            _capturedGizmo->updateTransform(scene, x, y);
            if (!_capturedGizmo->isTransforming())
            {
                _capturedGizmo.reset();
                _capturing = false;
            }
        }
        return false;
    }
    return true;
}

bool SelectionManager::mouseButtonUp(scene::Scene* scene, int button, int x, int y, bool additive)
{
    const bool clickAllowed = _leftPressed;
    if (button == 0)
    {
        _leftPressed = false;
    }
    if (_capturing && button != 0)
    {
        return false;
    }
    if (_capturing)
    {
        if (_capturedGizmo)
        {
            _capturedGizmo->endTransform();
            _capturedGizmo.reset();
        }
        _capturing = false;
        return false; // the press was captured; do not propagate the release
    }

    // Click selection only if the press did not move, so dragging the camera
    // does not select.
    if (button == 0 && clickAllowed && x == _mouseDownX && y == _mouseDownY)
    {
        bg2e_log_debug << "Additive: " << additive << bg2e_log_end;
        pick(scene, x, y, additive);
    }

    return true; // propagate the release to the scene graph
}

bool SelectionManager::pickObject(
    scene::Node* rootNode,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix,
    const math::Viewport& vp,
    uint32_t x,
    uint32_t y,
    SelectionItem* result)
{
    if (!result)
    {
        throw std::runtime_error("SelectionManager::pickObject: result is null");
    }

    *result = SelectionItem{};
    return pickDrawObject(rootNode, viewMatrix, projMatrix, vp, x, y, result);
}

void SelectionManager::setTransformEditObserver(TransformEditObserver observer)
{
    cancelTransform();
    _transformEditObserver = std::move(observer);
}


void SelectionManager::commitTransform()
{
    if (_capturedGizmo)
    {
        _capturedGizmo->endTransform();
        _capturedGizmo.reset();
    }
    _capturing = false;
    _leftPressed = false;
}


void SelectionManager::cancelTransform()
{
    if (_capturedGizmo)
    {
        _capturedGizmo->cancelTransform();
        _capturedGizmo.reset();
    }
    _capturing = false;
    _leftPressed = false;
}


void SelectionManager::deselect()
{
    _leftPressed = false;
    if (_capturing)
    {
        cancelTransform();
    }
    for (const auto& item : _selectedItems)
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

bool SelectionManager::isSelected(const scene::Node* node)
{
    if (!node)
    {
        return false;
    }

    refreshSelection();
    return std::any_of(
        _selectedNodes.cbegin(),
        _selectedNodes.cend(),
        [node](const std::weak_ptr<scene::Node>& n)
        {
            return n.lock().get() == node;
        });
}

bool SelectionManager::isSelected(const scene::Node* node, const scene::DrawableComponent* drawable, uint32_t submesh)
{
    refreshSelection();
    return drawable && isSelected(node, drawable->drawable().get(), submesh);
}

bool SelectionManager::addToSelectedItems(scene::Node* node, scene::DrawableComponent* drawable, uint32_t submesh)
{
    refreshSelection();
    if (!node || !drawable || drawable->ownerNode() != node || node->drawable() != drawable)
    {
        return false;
    }

    // Is the node selectable?
    auto selComponent = node->getComponent<SelectableComponent>();
    if (!selComponent)
    {
        return false;
    }

    auto mesh = drawable->drawable();
    if (mesh && mesh->engine() && mesh->engine() != _drawEngine)
    {
        throw std::logic_error("SelectionManager cannot select geometry from another draw engine");
    }
    const auto count = mesh ? mesh->submeshesCount() : 0;
    if (submesh >= count)
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
    selComponent->update(0);
    selectItem->selectionIdentifier = selComponent->identifier(submesh);
    selectItem->submesh = submesh;
    _selectedItems.push_back(selectItem);

    // Mark the submesh as selected
    selComponent->setSelected(submesh, true);

    // Keep the unique node list in sync
    addUniqueSelectedNode(node);
    callOnChange();

    return true;
}

bool SelectionManager::addToSelectedItems(scene::Node* node)
{
    if (!node)
    {
        return false;
    }

    // If the node owns a drawable, select it at submesh level (submesh 0) so it
    // also highlights in the 3D view, reusing all the validation above.
    if (auto* drawable = node->drawable())
    {
        return addToSelectedItems(node, drawable, 0);
    }

    return addNodeToSelection(node);
}

bool SelectionManager::addNodeToSelection(scene::Node* node)
{
    if (!node)
    {
        return false;
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

void SelectionManager::removeFromSelectedItems(scene::Node* node)
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
        });

    removeSelectedNode(node);

    if (changed)
    {
        callOnChange();
    }
}

void SelectionManager::removeFromSelectedItems(scene::Node* node, uint32_t submesh)
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
        });

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

void SelectionManager::callOnChange()
{
    prioritizeDrawableNode();

    // Point the transform gizmo at the primary selected node before notifying
    // listeners, so onSelect callbacks already observe the current transform.
    scene::Node* primary = _selectedNodes.empty() ? nullptr : _selectedNodes.front().lock().get();
    GizmoComponent::setCurrentTransform(primary);

    for (auto& cb : _onSelectCallbacks)
    {
        cb();
    }
}

void SelectionManager::addUniqueSelectedNode(scene::Node* node)
{
    if (!node)
    {
        return;
    }

    const bool exists = std::any_of(
        _selectedNodes.begin(), _selectedNodes.end(),
        [node](const std::weak_ptr<scene::Node>& n)
        {
            return n.lock().get() == node;
        });

    if (!exists)
    {
        _selectedNodes.push_back(node->weak_from_this());
    }
}

void SelectionManager::removeSelectedNode(scene::Node* node)
{
    std::erase_if(
        _selectedNodes,
        [node](const std::weak_ptr<scene::Node>& n)
        {
            auto locked = n.lock();
            return !locked || locked.get() == node;
        });
}

bool SelectionManager::nodeHasSelectedItems(const scene::Node* node) const
{
    return std::any_of(
        _selectedItems.cbegin(), _selectedItems.cend(),
        [node](const std::shared_ptr<SelectionItem>& item)
        {
            return item->node.lock().get() == node;
        });
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
