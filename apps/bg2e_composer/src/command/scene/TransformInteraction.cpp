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

#include "TransformInteraction.hpp"

#include <utility>

namespace bg2e_composer::command::scene {
using namespace bg2e::app;

TransformInteraction::TransformInteraction(std::weak_ptr<bg2e::scene::Node> target,
    std::function<bool()> valid, std::function<void()> effects)
    : _target(std::move(target)), _valid(std::move(valid)), _effects(std::move(effects))
{
}


const CommandInfo& TransformInteraction::info() const noexcept
{
    return _info;
}


CommandMemory TransformInteraction::memory() const
{
    // The two fixed Composer callbacks retain weak identities only. Include
    // their callable storage and the fixed command label/category allocations.
    return {sizeof(*this) + 1024, {}};
}


CommandMemory TransformInteraction::memoryForUpdate(const std::any&) const
{
    return memory();
}


CommandResult TransformInteraction::begin() noexcept
{
    try
    {
        auto node = _target.lock();
        if (!_valid() || !node || !node->transform())
        {
            return {CommandStatus::Rejected, "Transform target is stale"};
        }
        _transform = std::dynamic_pointer_cast<bg2e::scene::TransformComponent>(node->transform()->shared_from_this());
        _before = _after = node->transform()->matrix();
        return {};
    }
    catch (...)
    {
        return {CommandStatus::Rejected, "Cannot capture transform interaction"};
    }
}


CommandResult TransformInteraction::update(const std::any&) noexcept
{
    try
    {
        auto node = _target.lock();
        auto transform = _transform.lock();
        if (!_valid() || !node || !transform || node->transform() != transform.get())
        {
            return {CommandStatus::Rejected, "Transform target changed"};
        }
        const auto previous = _after;
        const auto next = transform->matrix();
        try
        {
            _effects();
        }
        catch (...)
        {
            transform->setMatrix(previous);
            try
            {
                _effects();
            }
            catch (...)
            {
                return {CommandStatus::Faulted, "Transform preview recovery failed"};
            }
            return {CommandStatus::Failed, "Transform preview restored"};
        }
        _after = next;
        return {next == previous ? CommandStatus::NoOp : CommandStatus::Applied, {}};
    }
    catch (...)
    {
        return {CommandStatus::Rejected, "Transform preview validation failed"};
    }
}


bool TransformInteraction::changed() const noexcept
{
    return _before != _after;
}


CommandResult TransformInteraction::execute() noexcept
{
    return {CommandStatus::Rejected, "Transform interactions must be adopted"};
}


CommandResult TransformInteraction::undo() noexcept
{
    return assign(_before);
}


CommandResult TransformInteraction::redo() noexcept
{
    return assign(_after);
}


CommandResult TransformInteraction::assign(const glm::mat4& matrix) noexcept
{
    try
    {
        auto node = _target.lock();
        auto transform = _transform.lock();
        if (!_valid() || !node || !transform || node->transform() != transform.get())
        {
            return {CommandStatus::Rejected, "Transform target changed"};
        }
        const auto previous = transform->matrix();
        transform->setMatrix(matrix);
        try
        {
            _effects();
        }
        catch (...)
        {
            transform->setMatrix(previous);
            try
            {
                _effects();
            }
            catch (...)
            {
                return {CommandStatus::Faulted, "Transform restoration recovery failed"};
            }
            return {CommandStatus::Failed, "Transform restoration failed; previous value retained"};
        }
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        return {CommandStatus::Rejected, "Transform restoration validation failed"};
    }
}

}
