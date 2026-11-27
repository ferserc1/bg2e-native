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

#include <limits>
#include <stdexcept>

namespace {

void require(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("Geometry recipe contract failed");
    }
}

}

int main()
{
    using namespace bg2e::geobuild;
    Recipe recipe;
    require(recipe.operands().empty());
    const auto cube = recipe.add({.label = "Cube"}, BooleanOperation::Subtract).value();
    const auto cylinder = recipe.add({.label = "Cylinder", .kind = PrimitiveKind::Cylinder},
        BooleanOperation::Subtract).value();
    const auto copy = recipe.duplicate(cylinder).value();
    require(cube < cylinder && cylinder < copy);
    require(!recipe.find(cube)->operation && !recipe.setOperation(cube, BooleanOperation::Intersect));
    require(recipe.find(copy)->operation == BooleanOperation::Subtract);

    require(recipe.move(cylinder, 0));
    require(recipe.operands()[0].id == cylinder && !recipe.find(cylinder)->operation);
    require(recipe.find(cube)->operation == BooleanOperation::Union);
    require(recipe.find(copy)->operation == BooleanOperation::Subtract);
    require(recipe.move(cylinder, 2));
    require(recipe.find(cylinder)->operation == BooleanOperation::Union);
    require(recipe.remove(cube) && !recipe.find(copy)->operation);
    require(!recipe.find(cube) && !recipe.duplicate(cube));

    Primitive transformed{.kind = PrimitiveKind::Sphere};
    transformed.localTransform[0][0] = -2.0f;
    transformed.localTransform[1][1] = 3.0f;
    transformed.localTransform[2][2] = 4.0f;
    transformed.localTransform[1][0] = 0.75f;
    const auto sphere = recipe.add(transformed, BooleanOperation::Intersect).value();
    require(sphere > copy && recipe.find(sphere)->primitive.localTransform == transformed.localTransform);
    auto invalid = transformed;
    invalid.localTransform[2][2] = 0.0f;
    require(!recipe.replace(sphere, invalid));
    invalid = transformed;
    invalid.localTransform[3][0] = std::numeric_limits<float>::infinity();
    require(!recipe.add(invalid));
    invalid = transformed;
    invalid.localTransform[0][3] = 0.5f;
    require(!recipe.add(invalid));
    require(recipe.find(sphere)->operation == BooleanOperation::Intersect);

    require(recipe.remove(copy) && recipe.remove(cylinder) && recipe.remove(sphere));
    require(recipe.operands().empty());
    require(recipe.add({}).value() > sphere);
}
