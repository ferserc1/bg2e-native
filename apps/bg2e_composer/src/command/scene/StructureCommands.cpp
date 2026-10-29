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

#include "StructureCommands.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace bg2e_composer::command::scene {
using namespace bg2e::app;
using Node = bg2e::scene::Node;

EditNodes::EditNodes(std::shared_ptr<StageScene> stage,
    std::shared_ptr<bg2e::manipulation::SelectionManager> selection, NodeEdit operation,
    const std::vector<std::shared_ptr<Node>>& targets, std::weak_ptr<Node> destination, std::string label)
    : _stage(stage), _selection(selection), _operation(operation), _destination(std::move(destination)),
      _info{label.empty() ? "Edit scene structure" : std::move(label), "scene.structure"}
{
    std::set<Node*> seen;
    for (const auto& node : targets)
    {
        if (!node || !seen.insert(node.get()).second)
        {
            continue;
        }
        if (std::any_of(targets.begin(), targets.end(), [&](const auto& other)
        {
            for (auto* parent = node->parent(); parent; parent = parent->parent())
            {
                if (parent == other.get())
                {
                    return true;
                }
            }
            return false;
        }))
        {
            continue; // One user action, only topmost selected branches.
        }
        _targets.push_back(stage->retainForCommand(node));
    }
}

const CommandInfo& EditNodes::info() const noexcept
{
    return _info;
}

CommandMemory EditNodes::memory() const
{
    CommandMemory result;
    result.localBytes = sizeof(*this) + 16384;
    if (auto stage = _stage.lock())
    {
        result.localBytes += 4 * stage->editingStateBytes();
    }
    std::set<Node*> parents;
    for (const auto& target : _targets)
    {
        result.resources.push_back({target, target->bytes});
        if (target->node->parent())
        {
            parents.insert(target->node->parent());
        }
    }
    if (auto destination = _destination.lock())
    {
        parents.insert(destination.get());
    }
    for (auto* parent : parents)
    {
        result.localBytes += 4 * (parent->children().size() + _targets.size() + 1) * sizeof(std::weak_ptr<Node>);
    }
    for (const auto* state : {&_before, &_after})
    {
        for (const auto& entry : *state)
        {
            result.localBytes += entry.children.capacity() * sizeof(std::weak_ptr<Node>);
        }
    }
    if (auto selection = _selection.lock())
    {
        result.localBytes += 4 * (selection->selectedItems().size() + selection->selectedNodes().size() + 1) *
            (sizeof(bg2e::manipulation::SelectionItem) + sizeof(std::weak_ptr<Node>));
    }
    return result;
}

void EditNodes::publish(const std::vector<Placement>& placements)
{
    std::vector<Node::ChildList> lists;
    auto stage = _stage.lock();
    for (const auto& placement : placements)
    {
        auto parent = placement.parent.lock();
        if (!stage || !parent || !stage->contains(parent.get()))
        {
            throw std::logic_error("Structural parent is no longer in the document");
        }
        Node::ChildList list{parent, {}};
        for (const auto& weak : placement.children)
        {
            auto child = weak.lock();
            if (!child)
            {
                throw std::logic_error("A structural sibling identity expired");
            }
            list.children.push_back(std::move(child));
        }
        lists.push_back(std::move(list));
    }
    Node::setChildLists(std::move(lists));
}

CommandResult EditNodes::execute() noexcept
{
    auto stage = _stage.lock();
    auto selection = _selection.lock();
    if (!stage || !selection || _targets.empty())
    {
        return {CommandStatus::NoOp, {}};
    }
    bool published = false;
    try
    {
        auto destination = _destination.lock();
        if (_operation != NodeEdit::Remove && (!destination || !stage->contains(destination.get())))
        {
            return {CommandStatus::Rejected, "Choose a current scene destination"};
        }
        std::vector<std::shared_ptr<Node>> parents;
        const auto remember = [&](const std::shared_ptr<Node>& parent)
        {
            if (std::find(parents.begin(), parents.end(), parent) == parents.end())
            {
                parents.push_back(parent);
            }
        };
        for (const auto& target : _targets)
        {
            auto node = target->node;
            if (_operation == NodeEdit::Insert)
            {
                if (node->parent() || node->scene())
                {
                    return {CommandStatus::Rejected, "Insertion requires detached candidates"};
                }
            }
            else
            {
                if (!stage->contains(node.get()) || node == stage->document().root || !node->parent())
                {
                    return {CommandStatus::Rejected, "Keep the document root and choose current scene branches"};
                }
                if (_operation == NodeEdit::Remove)
                {
                    for (auto* camera = stage->camera()->ownerNode(); camera; camera = camera->parent())
                    {
                        if (camera == node.get())
                        {
                            return {CommandStatus::Rejected, "Cannot remove the active camera branch"};
                        }
                    }
                }
                remember(node->parent()->shared_from_this());
            }
            if (_operation == NodeEdit::Reparent)
            {
                for (auto* ancestor = destination.get(); ancestor; ancestor = ancestor->parent())
                {
                    if (ancestor == node.get())
                    {
                        return {CommandStatus::Rejected, "Cannot reparent into the edited subtree"};
                    }
                }
            }
        }
        if (_operation == NodeEdit::Reparent && std::all_of(_targets.begin(), _targets.end(), [&](const auto& target)
        {
            return target->node->parent() == destination.get();
        }))
        {
            return {CommandStatus::NoOp, {}};
        }
        if (destination)
        {
            remember(destination);
        }
        _before.clear();
        for (const auto& parent : parents)
        {
            Placement placement{parent, {}};
            for (const auto& child : parent->children())
            {
                placement.children.push_back(child);
            }
            _before.push_back(std::move(placement));
        }
        _after = _before;
        for (auto& placement : _after)
        {
            std::erase_if(placement.children, [&](const auto& child)
            {
                return std::any_of(_targets.begin(), _targets.end(), [&](const auto& target)
                {
                    return child.lock() == target->node;
                });
            });
            if (_operation != NodeEdit::Remove && placement.parent.lock() == destination)
            {
                for (const auto& target : _targets)
                {
                    placement.children.push_back(target->node);
                }
            }
        }
        _beforeState = stage->editingState();
        _beforeSelection = selection->captureSelection();
        publish(_after);
        published = true;
        stage->validateEditingGraph(false);
        stage->refreshCamera(true);
        _afterState = stage->editingState();
        selection->restoreSelection(_beforeSelection, [stage](auto* node)
        {
            return stage->contains(node) || node == stage->fillLightNode().get();
        });
        _afterSelection = selection->captureSelection();
        _captured = true;
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        if (published)
        {
            try
            {
                publish(_before);
                stage->restoreEditingState(_beforeState);
                selection->restoreSelection(_beforeSelection, [stage](auto* node)
                {
                    return stage->contains(node) || node == stage->fillLightNode().get();
                });
            }
            catch (...)
            {
                return {CommandStatus::Faulted, "Structural edit recovery failed"};
            }
        }
        return {CommandStatus::Failed, "Structural edit rejected; original placement retained"};
    }
}

CommandResult EditNodes::restore(bool forward) noexcept
{
    auto stage = _stage.lock();
    auto selection = _selection.lock();
    if (!_captured || !stage || !selection)
    {
        return {CommandStatus::Rejected, "Structural snapshot is stale"};
    }
    bool published = false;
    try
    {
        publish(forward ? _after : _before);
        published = true;
        stage->validateEditingGraph(false);
        stage->restoreEditingState(forward ? _afterState : _beforeState);
        selection->restoreSelection(forward ? _afterSelection : _beforeSelection,
            [stage](auto* node)
            {
                return stage->contains(node) || node == stage->fillLightNode().get();
            });
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        if (published)
        {
            try
            {
                publish(forward ? _before : _after);
                stage->restoreEditingState(forward ? _beforeState : _afterState);
                selection->restoreSelection(forward ? _beforeSelection : _afterSelection,
                    [stage](auto* node)
                    {
                        return stage->contains(node) || node == stage->fillLightNode().get();
                    });
            }
            catch (...)
            {
                return {CommandStatus::Faulted, "Structural replay recovery failed"};
            }
        }
        return {CommandStatus::Failed, "Could not restore structural placement"};
    }
}

CommandResult EditNodes::undo() noexcept
{
    return restore(false);
}

CommandResult EditNodes::redo() noexcept
{
    return restore(true);
}

}
