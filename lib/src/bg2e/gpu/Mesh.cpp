/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/gpu/Mesh.hpp>

#include <cstddef>

namespace bg2e {
namespace gpu {

// ---------------------------------------------------------------------------
// MeshP
// ---------------------------------------------------------------------------
template <>
VertexBufferDescription MeshGeneric<bg2e::geo::MeshP>::vertexBufferDescription()
{
    using V = bg2e::geo::VertexP;
    VertexBufferDescription desc;
    desc.binding   = 0;
    desc.stride    = sizeof(V);
    desc.inputRate = VertexInputRate::Vertex;
    desc.attributes = {
        { 0, 0, VertexSemantic::Position, Format::R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(V, position)) }
    };
    return desc;
}

// ---------------------------------------------------------------------------
// MeshPN
// ---------------------------------------------------------------------------
template <>
VertexBufferDescription MeshGeneric<bg2e::geo::MeshPN>::vertexBufferDescription()
{
    using V = bg2e::geo::VertexPN;
    VertexBufferDescription desc;
    desc.binding   = 0;
    desc.stride    = sizeof(V);
    desc.inputRate = VertexInputRate::Vertex;
    desc.attributes = {
        { 0, 0, VertexSemantic::Position, Format::R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(V, position)) },
        { 1, 0, VertexSemantic::Normal,   Format::R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(V, normal))   }
    };
    return desc;
}

// ---------------------------------------------------------------------------
// MeshPC
// ---------------------------------------------------------------------------
template <>
VertexBufferDescription MeshGeneric<bg2e::geo::MeshPC>::vertexBufferDescription()
{
    using V = bg2e::geo::VertexPC;
    VertexBufferDescription desc;
    desc.binding   = 0;
    desc.stride    = sizeof(V);
    desc.inputRate = VertexInputRate::Vertex;
    desc.attributes = {
        { 0, 0, VertexSemantic::Position, Format::R32G32B32_SFLOAT,    static_cast<uint32_t>(offsetof(V, position)) },
        { 1, 0, VertexSemantic::Color,    Format::R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(V, color))    }
    };
    return desc;
}

// ---------------------------------------------------------------------------
// MeshPU
// ---------------------------------------------------------------------------
template <>
VertexBufferDescription MeshGeneric<bg2e::geo::MeshPU>::vertexBufferDescription()
{
    using V = bg2e::geo::VertexPU;
    VertexBufferDescription desc;
    desc.binding   = 0;
    desc.stride    = sizeof(V);
    desc.inputRate = VertexInputRate::Vertex;
    desc.attributes = {
        { 0, 0, VertexSemantic::Position,  Format::R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(V, position))  },
        { 1, 0, VertexSemantic::TexCoord0, Format::R32G32_SFLOAT,    static_cast<uint32_t>(offsetof(V, texCoord0)) }
    };
    return desc;
}

// ---------------------------------------------------------------------------
// MeshPNU
// ---------------------------------------------------------------------------
template <>
VertexBufferDescription MeshGeneric<bg2e::geo::MeshPNU>::vertexBufferDescription()
{
    using V = bg2e::geo::VertexPNU;
    VertexBufferDescription desc;
    desc.binding   = 0;
    desc.stride    = sizeof(V);
    desc.inputRate = VertexInputRate::Vertex;
    desc.attributes = {
        { 0, 0, VertexSemantic::Position,  Format::R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(V, position))  },
        { 1, 0, VertexSemantic::Normal,    Format::R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(V, normal))    },
        { 2, 0, VertexSemantic::TexCoord0, Format::R32G32_SFLOAT,    static_cast<uint32_t>(offsetof(V, texCoord0)) }
    };
    return desc;
}

// ---------------------------------------------------------------------------
// MeshPNC
// ---------------------------------------------------------------------------
template <>
VertexBufferDescription MeshGeneric<bg2e::geo::MeshPNC>::vertexBufferDescription()
{
    using V = bg2e::geo::VertexPNC;
    VertexBufferDescription desc;
    desc.binding   = 0;
    desc.stride    = sizeof(V);
    desc.inputRate = VertexInputRate::Vertex;
    desc.attributes = {
        { 0, 0, VertexSemantic::Position, Format::R32G32B32_SFLOAT,    static_cast<uint32_t>(offsetof(V, position)) },
        { 1, 0, VertexSemantic::Normal,   Format::R32G32B32_SFLOAT,    static_cast<uint32_t>(offsetof(V, normal))   },
        { 2, 0, VertexSemantic::Color,    Format::R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(V, color))    }
    };
    return desc;
}

// ---------------------------------------------------------------------------
// MeshPNUC
// ---------------------------------------------------------------------------
template <>
VertexBufferDescription MeshGeneric<bg2e::geo::MeshPNUC>::vertexBufferDescription()
{
    using V = bg2e::geo::VertexPNUC;
    VertexBufferDescription desc;
    desc.binding   = 0;
    desc.stride    = sizeof(V);
    desc.inputRate = VertexInputRate::Vertex;
    desc.attributes = {
        { 0, 0, VertexSemantic::Position,  Format::R32G32B32_SFLOAT,    static_cast<uint32_t>(offsetof(V, position))  },
        { 1, 0, VertexSemantic::Normal,    Format::R32G32B32_SFLOAT,    static_cast<uint32_t>(offsetof(V, normal))    },
        { 2, 0, VertexSemantic::TexCoord0, Format::R32G32_SFLOAT,       static_cast<uint32_t>(offsetof(V, texCoord0)) },
        { 3, 0, VertexSemantic::Color,     Format::R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(V, color))     }
    };
    return desc;
}

// ---------------------------------------------------------------------------
// MeshPNUT
// ---------------------------------------------------------------------------
template <>
VertexBufferDescription MeshGeneric<bg2e::geo::MeshPNUT>::vertexBufferDescription()
{
    using V = bg2e::geo::VertexPNUT;
    VertexBufferDescription desc;
    desc.binding   = 0;
    desc.stride    = sizeof(V);
    desc.inputRate = VertexInputRate::Vertex;
    desc.attributes = {
        { 0, 0, VertexSemantic::Position,  Format::R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(V, position))  },
        { 1, 0, VertexSemantic::Normal,    Format::R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(V, normal))    },
        { 2, 0, VertexSemantic::TexCoord0, Format::R32G32_SFLOAT,    static_cast<uint32_t>(offsetof(V, texCoord0)) },
        { 3, 0, VertexSemantic::Tangent,   Format::R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(V, tangent))   }
    };
    return desc;
}

// ---------------------------------------------------------------------------
// MeshPNUUT (Mesh)
// ---------------------------------------------------------------------------
template <>
VertexBufferDescription MeshGeneric<bg2e::geo::MeshPNUUT>::vertexBufferDescription()
{
    using V = bg2e::geo::VertexPNUUT;
    VertexBufferDescription desc;
    desc.binding   = 0;
    desc.stride    = sizeof(V);
    desc.inputRate = VertexInputRate::Vertex;
    desc.attributes = {
        { 0, 0, VertexSemantic::Position,  Format::R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(V, position))  },
        { 1, 0, VertexSemantic::Normal,    Format::R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(V, normal))    },
        { 2, 0, VertexSemantic::TexCoord0, Format::R32G32_SFLOAT,    static_cast<uint32_t>(offsetof(V, texCoord0)) },
        { 3, 0, VertexSemantic::TexCoord1, Format::R32G32_SFLOAT,    static_cast<uint32_t>(offsetof(V, texCoord1)) },
        { 4, 0, VertexSemantic::Tangent,   Format::R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(V, tangent))   }
    };
    return desc;
}

}
}
