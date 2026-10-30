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

#include "GeometryCommands.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>
#include <glm/gtc/matrix_transform.hpp>

namespace bg2e_composer::command::geometry {
namespace {
using Mesh = bg2e::draw::Drawable::Mesh;

size_t add(size_t a, size_t b)
{
    if (b > std::numeric_limits<size_t>::max() - a)
    {
        throw std::length_error("Geometry history memory overflow");
    }
    return a + b;
}

size_t multiply(size_t a, size_t b)
{
    if (a && b > std::numeric_limits<size_t>::max() / a)
    {
        throw std::length_error("Geometry history memory overflow");
    }
    return a * b;
}

size_t bytes(const Mesh& mesh)
{
    return add(sizeof(Mesh), add(multiply(mesh.vertices.capacity(), sizeof(bg2e::geo::VertexPNUUT)),
        add(multiply(mesh.indices.capacity(), sizeof(uint32_t)), multiply(mesh.submeshes.capacity(), sizeof(bg2e::geo::Submesh)))));
}

bool finite(const glm::vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

void validateData(const Mesh& mesh)
{
    if (mesh.vertices.empty() || mesh.indices.empty() || mesh.submeshes.empty() ||
        mesh.vertices.size() > UINT32_MAX || mesh.indices.size() > UINT32_MAX)
    {
        throw std::invalid_argument("Geometry edit requires nonempty 32-bit triangle geometry");
    }
    for (const auto& vertex : mesh.vertices)
    {
        if (!finite(vertex.position) || !finite(vertex.normal) || !finite(vertex.tangent) ||
            !std::isfinite(vertex.texCoord0.x) || !std::isfinite(vertex.texCoord0.y) ||
            !std::isfinite(vertex.texCoord1.x) || !std::isfinite(vertex.texCoord1.y))
        {
            throw std::invalid_argument("Geometry edit requires finite vertex attributes");
        }
    }
    for (auto index : mesh.indices)
    {
        if (index >= mesh.vertices.size())
        {
            throw std::invalid_argument("Geometry index is out of range");
        }
    }
    for (const auto& submesh : mesh.submeshes)
    {
        if (!submesh.indexCount || submesh.indexCount % 3 || submesh.firstIndex > mesh.indices.size() ||
            submesh.indexCount > mesh.indices.size() - submesh.firstIndex)
        {
            throw std::invalid_argument("Invalid geometry submesh range");
        }
    }
}

bool equal(const Mesh& a, const Mesh& b)
{
    if (a.indices != b.indices || a.vertices.size() != b.vertices.size() || a.submeshes.size() != b.submeshes.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.submeshes.size(); ++i)
    {
        if (a.submeshes[i].firstIndex != b.submeshes[i].firstIndex || a.submeshes[i].indexCount != b.submeshes[i].indexCount)
        {
            return false;
        }
    }
    for (size_t i = 0; i < a.vertices.size(); ++i)
    {
        const auto& x = a.vertices[i];
        const auto& y = b.vertices[i];
        if (x.position != y.position || x.normal != y.normal || x.tangent != y.tangent ||
            x.texCoord0 != y.texCoord0 || x.texCoord1 != y.texCoord1)
        {
            return false;
        }
    }
    return true;
}

const char* name(Operation operation)
{
    switch (operation)
    {
    case Operation::ZUpToYUp: return "Convert geometry from Z up to Y up";
    case Operation::Center: return "Center geometry";
    case Operation::PutOnFloor: return "Put geometry on floor";
    case Operation::CentimetersToMeters: return "Convert geometry from centimeters to meters";
    case Operation::MetersToCentimeters: return "Convert geometry from meters to centimeters";
    }
    throw std::invalid_argument("Unknown geometry operation");
}
}

GeometryEdit::GeometryEdit(const std::shared_ptr<StageScene>& stage,
    const std::vector<bg2e::manipulation::SelectionItem>& targets, Operation operation, std::function<void()> stopProducers)
    : _stage(stage), _stopProducers(std::move(stopProducers)), _operation(operation), _info{name(operation), "geometry.transform"}
{
    captureTargets(stage, targets);
    prepareInitial();
}

GeometryEdit::GeometryEdit(const std::shared_ptr<StageScene>& stage,
    const bg2e::manipulation::SelectionItem& target, const Mesh& prepared, std::function<void()> stopProducers, std::function<bool()> sourceValid)
    : _stage(stage), _stopProducers(std::move(stopProducers)), _sourceValid(std::move(sourceValid)), _replacement(true), _info{"Replace prepared geometry", "geometry.replace"}
{
    validateData(prepared);
    captureTargets(stage, {target});
    if (_groups.size() != 1 || prepared.submeshes.size() != _groups.front().mesh.lock()->submeshes.size())
    {
        throw std::invalid_argument("Prepared geometry must preserve submesh membership");
    }
    _groups.front().after = std::make_shared<Mesh>(prepared);
    _groups.front().capacityBound = std::max(_groups.front().capacityBound, bytes(prepared));
    prepareInitial();
}

TransformGeometry::TransformGeometry(const std::shared_ptr<StageScene>& stage,
    const std::vector<bg2e::manipulation::SelectionItem>& targets, Operation operation, std::function<void()> stopProducers)
    : GeometryEdit(stage, targets, operation, std::move(stopProducers))
{
}

ReplaceGeometry::ReplaceGeometry(const std::shared_ptr<StageScene>& stage,
    const bg2e::manipulation::SelectionItem& target, const bg2e::draw::Drawable::Mesh& prepared, std::function<void()> stopProducers, std::function<bool()> sourceValid)
    : GeometryEdit(stage, target, prepared, std::move(stopProducers), std::move(sourceValid))
{
}

void GeometryEdit::captureTargets(const std::shared_ptr<StageScene>& stage,
    const std::vector<bg2e::manipulation::SelectionItem>& targets)
{
    if (!stage || !stage->document().root)
    {
        throw std::invalid_argument("Geometry editing requires an open document");
    }
    for (const auto& item : targets)
    {
        auto node = item.node.lock();
        auto drawable = item.lockMesh();
        if (!node || !stage->contains(node.get()) || !drawable || !drawable->mesh())
        {
            throw std::invalid_argument("Geometry target is stale");
        }
        const auto found = std::find_if(_groups.begin(), _groups.end(), [&](const auto& group)
        {
            return group.mesh.lock() == drawable->mesh();
        });
        if (found == _groups.end())
        {
            Group group;
            group.mesh = drawable->mesh();
            group.capacityBound = bytes(*drawable->mesh());
            _groups.push_back(std::move(group));
        }
    }
    std::set<bg2e::draw::Drawable*> seen;
    const auto visit = [&](const auto& self, const std::shared_ptr<bg2e::scene::Node>& node) -> void
    {
        auto drawable = node->drawable() ? node->drawable()->drawable() : nullptr;
        if (drawable)
        {
            for (size_t i = 0; i < _groups.size(); ++i)
            {
                if (_groups[i].mesh.lock() == drawable->mesh() && seen.insert(drawable.get()).second)
                {
                    _targets.push_back({node, drawable, i});
                }
            }
        }
        for (const auto& child : node->children())
        {
            self(self, child);
        }
    };
    visit(visit, stage->document().root);
    validate();
}

void GeometryEdit::validate() const
{
    auto stage = _stage.lock();
    if (!stage || _groups.empty() || _targets.empty())
    {
        throw std::logic_error("Geometry command has no live targets");
    }
    for (const auto& group : _groups)
    {
        auto mesh = group.mesh.lock();
        if (!mesh)
        {
            throw std::logic_error("Geometry storage identity is stale");
        }
        if (bytes(*mesh) > group.capacityBound)
        {
            throw std::logic_error("Geometry storage grew outside its admitted memory bound");
        }
        validateData(*mesh);
    }
    for (const auto& target : _targets)
    {
        auto node = target.node.lock();
        auto drawable = target.drawable.lock();
        if (!node || !stage->contains(node.get()) || !drawable || !node->drawable() ||
            node->drawable()->drawable() != drawable || drawable->mesh() != _groups[target.group].mesh.lock() ||
            !drawable->isLoaded() || !drawable->drawMesh() ||
            !equal(*drawable->mesh(), drawable->drawMesh()->meshData()))
        {
            throw std::logic_error("Geometry identity or raster realization changed");
        }
        const auto& tracing = drawable->rayTracingMeshes();
        if (tracing.size() != (drawable->rayTracingEnabled() ? drawable->submeshesCount() : 0))
        {
            throw std::logic_error("Geometry acceleration membership changed");
        }
        for (size_t sub = 0; sub < tracing.size(); ++sub)
        {
            const auto& blas = tracing[sub];
            const auto range = drawable->mesh()->submeshes[sub];
            if (!blas || !blas->isBuilt() || blas->device() != drawable->engine()->device() ||
                blas->description().vertexBuffer != drawable->drawMesh()->vertexBuffer() ||
                blas->description().indexBuffer != drawable->drawMesh()->indexBuffer() ||
                blas->description().firstIndex != range.firstIndex || blas->description().indexCount != range.indexCount)
            {
                throw std::logic_error("Geometry acceleration realization changed");
            }
        }
    }
    // New live aliases cannot silently keep an old raster representation.
    const auto visit = [&](const auto& self, const std::shared_ptr<bg2e::scene::Node>& node) -> void
    {
        auto drawable = node->drawable() ? node->drawable()->drawable() : nullptr;
        if (drawable)
        {
            for (const auto& group : _groups)
            {
                if (drawable->mesh() == group.mesh.lock() && std::none_of(_targets.begin(), _targets.end(), [&](const auto& target)
                    { return target.drawable.lock() == drawable; }))
                {
                    throw std::logic_error("Geometry acquired a new document raster owner");
                }
            }
        }
        for (const auto& child : node->children())
        {
            self(self, child);
        }
    };
    visit(visit, stage->document().root);
}

void GeometryEdit::prepareInitial()
{
    if (_stopProducers)
    {
        _stopProducers();
    }
    for (auto& group : _groups)
    {
        auto source = group.mesh.lock();
        group.before = std::make_shared<Mesh>(*source);
        if (!_replacement)
        {
            auto next = std::make_shared<Mesh>(*source);
            glm::mat4 matrix(1);
            glm::mat3 directions(1);
            if (_operation == Operation::Center || _operation == Operation::PutOnFloor)
            {
                bg2e::geo::AABoundingBox<Mesh> bounds(*source);
                const glm::vec3 offset = _operation == Operation::Center ? -bounds.center() : glm::vec3(0, -bounds.min().y, 0);
                matrix = glm::translate(matrix, offset);
            }
            else if (_operation == Operation::ZUpToYUp)
            {
                matrix[1] = glm::vec4(0, 0, 1, 0);
                matrix[2] = glm::vec4(0, -1, 0, 0);
                directions = glm::mat3(matrix);
            }
            else
            {
                const float scale = _operation == Operation::CentimetersToMeters ? 0.01f : 100.0f;
                matrix = glm::scale(matrix, glm::vec3(scale));
            }
            for (auto& vertex : next->vertices)
            {
                vertex.position = glm::vec3(matrix * glm::vec4(vertex.position, 1));
                vertex.normal = directions * vertex.normal;
                vertex.tangent = directions * vertex.tangent;
            }
            group.after = std::move(next);
        }
        validateData(*group.after);
        group.capacityBound = std::max({group.capacityBound, bytes(*group.before), bytes(*group.after)});
        _effective = _effective || !equal(*group.before, *group.after);
    }
    if (!_effective)
    {
        return;
    }
    for (auto& target : _targets)
    {
        target.alternate = target.drawable.lock()->prepareGeometry(*_groups[target.group].after);
    }
}


const bg2e::app::CommandInfo& GeometryEdit::info() const noexcept
{
    return _info;
}

bg2e::app::CommandMemory GeometryEdit::memory() const
{
    validate();
    size_t budget = add(sizeof(*this), add(multiply(_groups.capacity(), sizeof(Group)), multiply(_targets.capacity(), sizeof(Target))));
    for (const auto& group : _groups)
    {
        auto mesh = group.mesh.lock();
        size_t payload = group.capacityBound;
        if (group.before)
        {
            payload = std::max(payload, bytes(*group.before));
        }
        if (group.after)
        {
            payload = std::max(payload, bytes(*group.after));
        }
        // Includes retained fallback data, transient CPU candidates, old/new
        // raster CPU copies, uploads, native payload and recovery publications.
        budget = add(budget, multiply(payload, 8));
        for (const auto& target : _targets)
        {
            if (_groups[target.group].mesh.lock() == mesh)
            {
                budget = add(budget, add(65536, multiply(payload, 8)));
            }
        }
    }
    for (const auto& target : _targets)
    {
        budget = add(budget, target.alternate.accelerationBytes());
        if (auto drawable = target.drawable.lock())
        {
            for (const auto& blas : drawable->rayTracingMeshes())
            {
                budget = add(budget, add(65536, blas->retainedBytes()));
            }
        }
    }
    bg2e::app::CommandMemory result{budget, {}};
    for (const auto& target : _targets)
    {
        if (auto owner = target.alternate.resourceOwner())
        {
            // Native payload is reserved above. Register a resource identity so
            // Composer drains GPU work before destroying an evicted command.
            result.resources.push_back({std::move(owner), 0});
        }
    }
    return result;
}

bg2e::app::CommandResult GeometryEdit::execute() noexcept
{
    return apply(true, true);
}

bg2e::app::CommandResult GeometryEdit::undo() noexcept
{
    return apply(false, false);
}

bg2e::app::CommandResult GeometryEdit::redo() noexcept
{
    return apply(true, false);
}

bg2e::app::CommandResult GeometryEdit::apply(bool forward, bool initial) noexcept
{
    using namespace bg2e::app;
    if ((initial && _captured) || (!initial && (!_captured || _applied == forward)))
    {
        return {CommandStatus::Rejected, "Geometry command has no matching replay state"};
    }
    try
    {
        if (initial && _sourceValid && !_sourceValid())
        {
            return {CommandStatus::Rejected, "Prepared geometry job or source revision changed"};
        }
        validate();
        if (!_effective && initial)
        {
            return {};
        }
        std::vector<std::shared_ptr<Mesh>> sources;
        std::vector<Mesh> candidates;
        for (const auto& group : _groups)
        {
            auto source = group.mesh.lock();
            if (!equal(*source, *(_applied ? group.after : group.before)))
            {
                return {CommandStatus::Rejected, "Geometry contents changed outside this command"};
            }
            sources.push_back(source);
            candidates.push_back(*(forward ? group.after : group.before));
        }
        if (_stopProducers)
        {
            _stopProducers();
        }
        std::vector<std::shared_ptr<bg2e::draw::Drawable>> drawables;
        for (const auto& target : _targets)
        {
            auto drawable = target.drawable.lock();
            if (!drawable->canExchangeGeometry(target.alternate))
            {
                return {CommandStatus::Rejected, "Prepared geometry owner changed"};
            }
            drawables.push_back(drawable);
        }
        // Drain all readers of the old CPU/raster/BLAS version before exchange.
        drawables.front()->engine()->device()->waitIdleForResourceUpdate();
        // Every fallible allocation/upload has completed. Retain displaced CPU
        // data locally and complete GPU versions in the command throughout publication.
        for (size_t i = 0; i < sources.size(); ++i)
        {
            using std::swap;
            swap(*sources[i], candidates[i]);
        }
        for (size_t i = 0; i < drawables.size(); ++i)
        {
            if (!drawables[i]->exchangeGeometry(_targets[i].alternate))
            {
                bool recovered = true;
                for (size_t j = 0; j < i; ++j)
                {
                    recovered = drawables[j]->exchangeGeometry(_targets[j].alternate) && recovered;
                }
                for (size_t j = 0; j < sources.size(); ++j)
                {
                    using std::swap;
                    swap(*sources[j], candidates[j]);
                }
                return {recovered ? CommandStatus::Failed : CommandStatus::Faulted, "Geometry batch publication failed"};
            }
        }
        _captured = true;
        _applied = forward;
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        return {CommandStatus::Failed, "Geometry preparation failed; live data and bindings are unchanged"};
    }
}

}
