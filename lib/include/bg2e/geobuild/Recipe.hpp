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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/math/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace bg2e::geobuild {

using PrimitiveId = std::uint64_t;

enum class PrimitiveKind { Cube, Cylinder, Sphere };
enum class BooleanOperation { Union, Subtract, Intersect };

struct PrimitiveTessellation {
    std::uint32_t radialSegments = 32;
    std::uint32_t latitudeSegments = 16;
};

// All canonical primitives are centered and span one unit along each axis.
// The cylinder axis is Y. Placement, dimensions and shear live in the matrix.
struct Primitive {
    std::string label;
    PrimitiveKind kind = PrimitiveKind::Cube;
    PrimitiveTessellation tessellation;
    glm::mat4 localTransform{1.0f};
};

struct Operand {
    PrimitiveId id;
    Primitive primitive;
    // An empty operation denotes the seed, which is always the first operand.
    std::optional<BooleanOperation> operation;
};

class BG2E_API Recipe {
  public:
    const std::vector<Operand>& operands() const;
    const Operand* find(PrimitiveId id) const;

    // IDs start at one and are never recycled within a recipe. Invalid input,
    // a missing source, or ID exhaustion returns an empty optional.
    std::optional<PrimitiveId> add(Primitive primitive, BooleanOperation operation = BooleanOperation::Union);
    std::optional<PrimitiveId> duplicate(PrimitiveId id);
    bool replace(PrimitiveId id, Primitive primitive);
    bool setOperation(PrimitiveId id, BooleanOperation operation);
    bool remove(PrimitiveId id);
    // Destination is the final zero-based index.
    bool move(PrimitiveId id, std::size_t destination);

  private:
    std::vector<Operand>::iterator findMutable(PrimitiveId id);
    void normalizeSeed();

    std::vector<Operand> _operands;
    PrimitiveId _lastId = 0;
};

}
