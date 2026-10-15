#include <bg2e/manipulation/SelectionManager.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/manipulation/GizmoComponent.hpp>
#include <bg2e/manipulation/GizmoAndSelectionRenderer.hpp>
#include <bg2e/geo/cube.hpp>
#include <stdexcept>

using namespace bg2e;

void require(bool value)
{
    if (!value)
    {
        throw std::runtime_error("Dual-mode selection check failed");
    }
}

int main()
{
    // CPU-only contract checks: no Engine::init, window, GPU or MainLoop.
    draw::Engine engine;
    manipulation::SelectionManager manager(&engine);
    manager.init();
    require(manager.executionMode() == manipulation::ToolExecutionMode::Draw);
    auto geometry = std::make_shared<draw::Drawable>();
    geometry->setMesh(geo::createCube(1.0f, 1.0f, 1.0f));
    auto node = std::make_shared<scene::Node>();
    auto drawable = std::make_shared<scene::DrawableComponent>(geometry);
    auto selectable = std::make_shared<manipulation::SelectableComponent>();
    node->addComponent(drawable);
    node->addComponent(selectable);
    int changes = 0;
    manager.onSelect([&changes]()
    {
        ++changes;
    });
    require(manager.addToSelectedItems(node.get(), drawable.get(), 0));
    require(manager.addToSelectedItems(node.get(), drawable.get(), 1));
    require(manager.selectedNodes().size() == 1);
    require(manager.selectedMesh() == nullptr);
    require(manager.selectedDrawMesh() == geometry.get());
    require(manager.selectedItem()->lockDrawMesh() == geometry);
    require(selectable->isSelected(0) && selectable->isSelected(1));
    bool pickRejected = false;
    try
    {
        manager.pick(node.get(), glm::mat4{1}, glm::mat4{1}, math::Viewport{800, 600}, 0, 0);
    }
    catch (const std::logic_error&)
    {
        pickRejected = true;
    }
    require(pickRejected && manager.selectedItems().size() == 2);
    auto first = manager.selectedItem();
    const auto oldId = selectable->identifier(0);
    require(oldId && changes == 2);
    require(manager.addToSelectedItems(node.get(), drawable.get(), 0));
    require(changes == 2); // Duplicate selection never notifies.

    // Same-owner geometry replacement with the same submesh count expires IDs.
    geometry->setMesh(geo::createCube(1.0f, 1.0f, 1.0f));
    require(first->expired());
    manager.refreshSelection();
    require(manager.selectedItems().empty() && manager.selectedNodes().empty());
    require(changes == 3 && selectable->identifier(0) != oldId);
    require(!selectable->isSelected(0));
    require(!selectable->isSelected(999) && selectable->identifier(999) == 0);

    require(manager.addToSelectedItems(node.get(), drawable.get(), 0));
    auto second = manager.selectedItem();
    node->removeComponent(drawable); // External owners deliberately keep it alive.
    require(second->expired() && !second->lockDrawMesh());
    require(manager.selectedNode() == nullptr);
    manager.refreshSelection();
    require(manager.selectedNodes().empty());

    // CPU serialization retains intent/common attributes, never runtime IDs.
    selectable->setPriority(17);
    auto document = selectable->serializeReferences({});
    manipulation::SelectableComponent restored;
    restored.deserialize(document, {});
    require(restored.priority() == 17 && restored.submeshCount() == 0);
    auto cloned = std::dynamic_pointer_cast<manipulation::SelectableComponent>(selectable->clone());
    require(cloned && cloned->priority() == 17 && cloned->gizmoIdentifier() != selectable->gizmoIdentifier());

    node->addComponent(drawable);
    require(manager.addToSelectedItems(node.get(), drawable.get(), 0));
    auto third = manager.selectedItem();
    node.reset();
    require(third->expired());
    manager.refreshSelection();
    require(!manager.selectedNode());

    auto group = std::make_shared<scene::Node>();
    require(manager.addToSelectedItems(group.get()));
    require(manager.addToSelectedItems(group.get()));
    require(manager.selectedNodes().size() == 1);
    group.reset();
    manager.refreshSelection();
    require(manager.selectedNodes().empty());

    // Selecting a production resource through a draw manager is rejected.
    auto legacyNode = std::make_shared<scene::Node>();
    auto legacyResource = std::make_shared<scene::Drawable>();
    legacyResource->setMesh(geo::createCube(1.0f, 1.0f, 1.0f));
    auto legacyComponent = std::make_shared<scene::DrawableComponent>(legacyResource);
    legacyNode->addComponent(legacyComponent);
    legacyNode->addComponent(std::make_shared<manipulation::SelectableComponent>());
    bool crossModeRejected = false;
    try
    {
        manager.addToSelectedItems(legacyNode.get(), legacyComponent.get(), 0);
    }
    catch (const std::logic_error&)
    {
        crossModeRejected = true;
    }
    require(crossModeRejected);

    manipulation::GizmoComponent gizmo(&engine);
    auto gizmoClone = std::dynamic_pointer_cast<manipulation::GizmoComponent>(gizmo.clone());
    require(gizmoClone && gizmoClone->manipulationMode() == manipulation::ToolExecutionMode::Draw);
    require(!gizmo.transformDrawDrawable());
    bool rejected = false;
    try
    {
        (void)gizmo.transformDrawable();
    }
    catch (const std::logic_error&)
    {
        rejected = true;
    }
    require(rejected);
    manipulation::GizmoAndSelectionRenderer overlay;
    overlay.init(&engine);
    require(overlay.executionMode() == manipulation::ToolExecutionMode::Draw);
    return 0;
}
