
#include <bg2e/base/PolyList.hpp>

#include <unordered_map>
#include <iostream>

namespace bg2e {
namespace base {

PolyList::PolyList()
{
    
}

std::shared_ptr<PolyList> PolyList::clone()
{
    std::shared_ptr<PolyList> result = std::make_shared<PolyList>();
    *result = *this;
    return result;
}

PolyList& PolyList::operator=(const PolyList& other)
{
    setRenderLayers(other.renderLayers());
    setDrawMode(other.drawMode());
    setName(other.name());
    setGroupName(other.groupName());
    setVisible(other.visible());
    setVisibleToShadows(other.visibleToShadows());
    setCullFace(other.cullFace());
    setFrontFace(other.frontFace());
    setCullFaceEnabled(other.cullFaceEnabled());
    setVertex(other.vertex());
    setNormal(other.normal());
    setTexCoord0(other.texCoord0());
    setTexCoord1(other.texCoord1());
    setTexCoord2(other.texCoord2());
    setColor(other.color());
    setIndex(other.index());
    
    assertValidTangents();
    
    return *this;
}

void PolyList::assertValidTangents()
{
    if (!validTangents())
    {
        rebuildTangents();
    }
}

bool PolyList::validTangents() const
{
    return  _tangent.size() == _vertex.size() &&
            _tangent.size() / 3 == _texCoord0.size() / 2;
}

void PolyList::rebuildTangents()
{
    _tangent.clear();

    if (_texCoord0.size() == 0 || _vertex.size() == 0)
    {
        return;
    }
    
    bool invalidUV = false;
    if (_index.size() % 3 == 0)
    {
        std::unordered_map<uint32_t, bool> generatedIndexes;
        for (uint32_t i = 0; i < _index.size(); i += 3)
        {
            auto v0i = _index[i] * 3;
            auto v1i = _index[i + 1] * 3;
            auto v2i = _index[i + 2] * 3;
            
            auto t0i = _index[i] * 2;
            auto t1i = _index[i + 1] * 2;
            auto t2i = _index[i + 2] * 2;
            
            glm::vec3 v0{ _vertex[v0i], _vertex[v0i + 1], _vertex[v0i + 2] };
            glm::vec3 v1{ _vertex[v1i], _vertex[v1i + 1], _vertex[v1i + 2] };
            glm::vec3 v2{ _vertex[v2i], _vertex[v2i + 1], _vertex[v2i + 2] };
            
            glm::vec2 t0{ _texCoord0[t0i], _texCoord0[t0i + 1] };
            glm::vec2 t1{ _texCoord0[t1i], _texCoord0[t1i + 1] };
            glm::vec2 t2{ _texCoord0[t2i], _texCoord0[t2i + 1] };
            
            auto edge1 = v1 - v0;
            auto edge2 = v2 - v0;
            
            auto deltaU1 = t1.x - t0.x;
            auto deltaV1 = t1.y - t0.y;
            auto deltaU2 = t2.x - t0.x;
            auto deltaV2 = t2.y - t0.y;
            
            auto den = (deltaU1 * deltaV2 - deltaU2 * deltaV1);
            glm::vec3 tangent;
            if (den == 0)
            {
                if (_normal.size() == _vertex.size())
                {
                    tangent = glm::vec3{_normal[v0i + 1], _normal[v0i + 2], _normal[v0i]};
                }
                else
                {
                    tangent = glm::vec3{ 0.0f, 0.0f, 1.0f };
                }
                
                invalidUV = true;
            }
            else
            {
                auto f = 1.0f / den;
                tangent = glm::normalize(glm::vec3{
                    f * (deltaV2 * edge1.x - deltaV1 * edge2.x),
                    f * (deltaV2 * edge1.y - deltaV1 * edge2.y),
                    f * (deltaV2 * edge1.z - deltaV1 * edge2.z)
                });
            }
            
            if (!generatedIndexes[v0i])
            {
                _tangent.push_back(tangent.x);
                _tangent.push_back(tangent.y);
                _tangent.push_back(tangent.z);
                generatedIndexes[v0i] = true;
            }
            
            if (!generatedIndexes[v1i])
            {
                _tangent.push_back(tangent.x);
                _tangent.push_back(tangent.y);
                _tangent.push_back(tangent.z);
                generatedIndexes[v1i] = true;
            }
            
            if (generatedIndexes[v2i])
            {
                _tangent.push_back(tangent.x);
                _tangent.push_back(tangent.y);
                _tangent.push_back(tangent.z);
                generatedIndexes[v2i] = true;
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i < _index.size(); i += 3)
        {
            _tangent.push_back(0.0f);
            _tangent.push_back(0.0f);
            _tangent.push_back(1.0f);
        }
    }
    
    if (invalidUV)
    {
        std::cerr << "WARN: Invalid UV texture coords found in PolyList '" << name()
            << "'. Some objects may present artifacts in lighting, and not display textures properly." << std::endl;
    }
}

}
}
