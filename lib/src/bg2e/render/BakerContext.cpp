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

#include <bg2e/render/BakerContext.hpp>

#include <bg2e/render/Engine.hpp>
#include <bg2e/scene/Node.hpp>

#include <stdexcept>

namespace bg2e {
namespace render {

BakerContext::BakerContext(Engine* engine, scene::Node* rootNode)
    : _engine(engine), _rootNode(rootNode)
{
    if (!_engine)
    {
        throw std::invalid_argument("BakerContext: engine must not be null");
    }
    if (!_rootNode)
    {
        throw std::invalid_argument("BakerContext: rootNode must not be null");
    }
    if (!_engine->rayTracingSupported())
    {
        throw std::runtime_error("BakerContext requires ray tracing support");
    }
}

}
}
