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

#include <bg2e/geobuild/Recipe.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace bg2e::geobuild {

namespace {

bool validOperation(BooleanOperation operation)
{
    return operation == BooleanOperation::Union || operation == BooleanOperation::Subtract ||
        operation == BooleanOperation::Intersect;
}

bool validPrimitive(const Primitive& primitive)
{
    switch (primitive.kind)
    {
    case PrimitiveKind::Cube:
        break;
    case PrimitiveKind::Cylinder:
    case PrimitiveKind::Sphere:
        if (primitive.tessellation.radialSegments < 3 ||
            (primitive.kind == PrimitiveKind::Sphere && primitive.tessellation.latitudeSegments < 2))
        {
            return false;
        }
        break;
    default:
        return false;
    }

    const auto& matrix = primitive.localTransform;
    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            if (!std::isfinite(matrix[column][row]))
            {
                return false;
            }
        }
    }
    return matrix[0][3] == 0.0f && matrix[1][3] == 0.0f &&
        matrix[2][3] == 0.0f && matrix[3][3] == 1.0f &&
        glm::determinant(glm::dmat3(matrix)) != 0.0;
}

}

const std::vector<Operand>& Recipe::operands() const
{
    return _operands;
}

const Operand* Recipe::find(PrimitiveId id) const
{
    const auto operand = std::ranges::find(_operands, id, &Operand::id);
    return operand == _operands.end() ? nullptr : &*operand;
}

std::optional<PrimitiveId> Recipe::add(Primitive primitive, BooleanOperation operation)
{
    if (!validPrimitive(primitive) || !validOperation(operation) ||
        _lastId == std::numeric_limits<PrimitiveId>::max())
    {
        return std::nullopt;
    }

    const auto id = _lastId + 1;
    const auto storedOperation = _operands.empty() ? std::nullopt : std::optional{operation};
    _operands.push_back({id, std::move(primitive), storedOperation});
    _lastId = id;
    return id;
}

std::optional<PrimitiveId> Recipe::duplicate(PrimitiveId id)
{
    const auto* operand = find(id);
    if (!operand)
    {
        return std::nullopt;
    }
    return add(operand->primitive, operand->operation.value_or(BooleanOperation::Union));
}

bool Recipe::replace(PrimitiveId id, Primitive primitive)
{
    const auto operand = findMutable(id);
    if (operand == _operands.end() || !validPrimitive(primitive))
    {
        return false;
    }
    operand->primitive = std::move(primitive);
    return true;
}

bool Recipe::setOperation(PrimitiveId id, BooleanOperation operation)
{
    const auto operand = findMutable(id);
    if (operand == _operands.end() || operand == _operands.begin() || !validOperation(operation))
    {
        return false;
    }
    operand->operation = operation;
    return true;
}

bool Recipe::remove(PrimitiveId id)
{
    const auto operand = findMutable(id);
    if (operand == _operands.end())
    {
        return false;
    }
    _operands.erase(operand);
    normalizeSeed();
    return true;
}

bool Recipe::move(PrimitiveId id, std::size_t destination)
{
    const auto operand = findMutable(id);
    if (operand == _operands.end() || destination >= _operands.size())
    {
        return false;
    }
    const auto target = _operands.begin() + static_cast<std::ptrdiff_t>(destination);
    if (operand < target)
    {
        std::rotate(operand, operand + 1, target + 1);
    }
    else if (operand > target)
    {
        std::rotate(target, operand, operand + 1);
    }
    normalizeSeed();
    return true;
}

std::vector<Operand>::iterator Recipe::findMutable(PrimitiveId id)
{
    return std::ranges::find(_operands, id, &Operand::id);
}

void Recipe::normalizeSeed()
{
    if (_operands.empty())
    {
        return;
    }

    _operands.front().operation.reset();
    for (auto operand = _operands.begin() + 1; operand != _operands.end(); ++operand)
    {
        if (!operand->operation)
        {
            operand->operation = BooleanOperation::Union;
        }
    }
}

}
