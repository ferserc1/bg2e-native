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

#include "MetadataCommands.hpp"
#include <set>
#include <algorithm>
#include <stdexcept>
#include <cmath>

namespace bg2e_composer::command::scene {
using namespace bg2e::app;

EditMetadata::EditMetadata(std::weak_ptr<StageScene> stage, MetadataRequest request)
    : _stage(std::move(stage)), _request(std::move(request))
{
    if (_request.field == MetadataField::SubmeshTransform || _request.field == MetadataField::DrawableTransform)
    {
        for (int c = 0; c < 4; ++c)
        {
            for (int r = 0; r < 4; ++r)
            {
                if (!std::isfinite(_request.transform[c][r]))
                {
                    throw std::invalid_argument("Instance transform must be finite");
                }
            }
        }
        const auto determinant = glm::determinant(_request.transform);
        if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f || _request.transform[0][3] != 0 ||
            _request.transform[1][3] != 0 || _request.transform[2][3] != 0 || _request.transform[3][3] != 1)
        {
            throw std::invalid_argument("Instance transform must be affine and invertible");
        }
    }
    std::set<std::pair<const bg2e::scene::DrawableRenderComponent*, uint32_t>> seen;
    std::erase_if(_request.targets, [&](const auto& target)
    {
        return !seen.emplace(target.drawable.lock().get(), target.submesh).second;
    });
}

const CommandInfo& EditMetadata::info() const noexcept
{
    return _info;
}

CommandMemory EditMetadata::memory() const
{
    size_t bytes = sizeof(*this) + 1024 + _request.targets.capacity() * sizeof(bg2e::manipulation::SelectionItem);
    for (const auto& target : _request.targets)
    {
        const auto value = read(target);
        bytes += 8 * (sizeof(Value) + value.text.capacity() + _request.text.capacity());
    }
    return {bytes, {}};
}

EditMetadata::Value EditMetadata::read(const bg2e::manipulation::SelectionItem& item) const
{
    auto stage = _stage.lock();
    auto node = item.node.lock();
    auto mesh = item.lockRenderable();
    if (!stage || !node || !stage->contains(node.get()) || !mesh)
    {
        throw std::logic_error("Submesh metadata identity is stale");
    }
    if (_request.field == MetadataField::SubmeshTransform || _request.field == MetadataField::DrawableTransform)
    {
        const auto instance = std::dynamic_pointer_cast<bg2e::scene::DrawableInstanceComponent>(mesh);
        if (!instance)
        {
            throw std::logic_error("Instance transform editing requires an instance receiver");
        }
        return {{}, true, _request.field == MetadataField::SubmeshTransform ? instance->localSubmeshTransform(item.submesh) : instance->transform()};
    }
    if (_request.field == MetadataField::Visibility)
    {
        return {{}, mesh->submeshVisibility(item.submesh)};
    }
    return {_request.field == MetadataField::Name ? mesh->submeshName(item.submesh) : mesh->submeshGroupName(item.submesh), true};
}

void EditMetadata::write(const bg2e::manipulation::SelectionItem& item, const Value& value) const
{
    const auto component = item.lockRenderable();
    if (const auto instance = std::dynamic_pointer_cast<bg2e::scene::DrawableInstanceComponent>(component))
    {
        if (_request.field == MetadataField::SubmeshTransform)
        {
            instance->setSubmeshTransform(value.transform, item.submesh);
        }
        else if (_request.field == MetadataField::DrawableTransform)
        {
            instance->setTransform(value.transform);
        }
        else if (_request.field == MetadataField::Visibility)
        {
            instance->setSubmeshVisibility(value.visible, item.submesh);
        }
        else if (_request.field == MetadataField::Name)
        {
            instance->setSubmeshName(value.text, item.submesh);
        }
        else
        {
            instance->setSubmeshGroupName(value.text, item.submesh);
        }
        return;
    }
    auto mesh = item.lockMesh();
    if (!mesh)
    {
        throw std::logic_error("Submesh metadata target changed");
    }
    if (_request.field == MetadataField::Visibility)
    {
        mesh->setSubmeshVisibility(value.visible, item.submesh);
    }
    else if (_request.field == MetadataField::Name)
    {
        mesh->setSubmeshName(value.text, item.submesh);
    }
    else
    {
        mesh->setSubmeshGroupName(value.text, item.submesh);
    }
}

CommandResult EditMetadata::assign(const std::vector<Value>& values) noexcept
{
    std::vector<Value> recovery;
    try
    {
        for (const auto& target : _request.targets)
        {
            recovery.push_back(read(target));
        }
    }
    catch (...)
    {
        return {CommandStatus::Rejected, "Metadata target changed or recovery allocation failed"};
    }
    try
    {
        for (size_t i = 0; i < _request.targets.size(); ++i)
        {
            write(_request.targets[i], values[i]);
        }
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        try
        {
            for (size_t i = 0; i < _request.targets.size(); ++i)
            {
                write(_request.targets[i], recovery[i]);
            }
        }
        catch (...)
        {
            return {CommandStatus::Faulted, "Metadata recovery failed"};
        }
        return {CommandStatus::Failed, "Metadata edit restored after failure"};
    }
}

CommandResult EditMetadata::execute() noexcept
{
    try
    {
        bool changed = false;
        for (const auto& target : _request.targets)
        {
            auto before = read(target);
            Value after{_request.text, _request.visible, _request.transform};
            const bool matrix = _request.field == MetadataField::SubmeshTransform || _request.field == MetadataField::DrawableTransform;
            changed = changed || (matrix ? before.transform != after.transform :
                _request.field == MetadataField::Visibility ? before.visible != after.visible : before.text != after.text);
            _before.push_back(std::move(before));
            _after.push_back(std::move(after));
        }
        if (!changed)
        {
            return {};
        }
        auto result = assign(_after);
        _captured = result.accepted();
        return result;
    }
    catch (...)
    {
        return {CommandStatus::Failed, "Could not prepare metadata values"};
    }
}

CommandResult EditMetadata::undo() noexcept
{
    return _captured ? assign(_before) : CommandResult{CommandStatus::Rejected, "No metadata snapshot"};
}

CommandResult EditMetadata::redo() noexcept
{
    return _captured ? assign(_after) : CommandResult{CommandStatus::Rejected, "No metadata snapshot"};
}

}
