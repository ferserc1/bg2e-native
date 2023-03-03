
#include <bg2e/base/PolyList.hpp>

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
    // TODO: Implement this function
    /*
     
    //This is the equivalent code in JavaScript version of PolyList.buildTangents()
    
    plist._tangent = [];

    if (!plist.texCoord0 || !plist.texCoord0.length || !plist.vertex || !plist.vertex.length) return;

    
    const result = [];
    const generatedIndexes = {};
    let invalidUV = false;

    if (plist.index.length%3==0) {
        // Triangles
        for (let i=0; i<plist.index.length - 2; i+=3) {
            const v0i = plist.index[i] * 3;
            const v1i = plist.index[i + 1] * 3;
            const v2i = plist.index[i + 2] * 3;
            
            const t0i = plist.index[i] * 2;
            const t1i = plist.index[i + 1] * 2;
            const t2i = plist.index[i + 2] * 2;
            
            const v0 = new Vec(plist.vertex[v0i], plist.vertex[v0i + 1], plist.vertex[v0i + 2]);
            const v1 = new Vec(plist.vertex[v1i], plist.vertex[v1i + 1], plist.vertex[v1i + 2]);
            const v2 = new Vec(plist.vertex[v2i], plist.vertex[v2i + 1], plist.vertex[v2i + 2]);
            
            const t0 = new Vec(plist.texCoord0[t0i], plist.texCoord0[t0i + 1]);
            const t1 = new Vec(plist.texCoord0[t1i], plist.texCoord0[t1i + 1]);
            const t2 = new Vec(plist.texCoord0[t2i], plist.texCoord0[t2i + 1]);
            
            const edge1 = Vec.Sub(v1, v0);
            const edge2 = Vec.Sub(v2, v0);
            
            const deltaU1 = t1.x - t0.x;
            const deltaV1 = t1.y - t0.y;
            const deltaU2 = t2.x - t0.x;
            const deltaV2 = t2.y - t0.y;
            
            const den = (deltaU1 * deltaV2 - deltaU2 * deltaV1);
            let tangent = null;
            if (den==0) {
                const n = plist.normal.length === plist.vertex.length ?
                    new Vec(plist.normal[v0i], plist.normal[v0i + 1], plist.normal[v0i + 2]) :
                    new Vec(1, 0, 0);

                invalidUV = true;
                tangent = new Vec(n.y, n.z, n.x);
            }
            else {
                const f = 1 / den;
            
                tangent = new Vec(f * (deltaV2 * edge1.x - deltaV1 * edge2.x),
                                  f * (deltaV2 * edge1.y - deltaV1 * edge2.y),
                                  f * (deltaV2 * edge1.z - deltaV1 * edge2.z));
                tangent.normalize();
            }
            
            if (generatedIndexes[v0i]===undefined) {
                result.push(tangent.x);
                result.push(tangent.y);
                result.push(tangent.z);
                generatedIndexes[v0i] = tangent;
            }
            
            if (generatedIndexes[v1i]===undefined) {
                result.push(tangent.x);
                result.push(tangent.y);
                result.push(tangent.z);
                generatedIndexes[v1i] = tangent;
            }
            
            if (generatedIndexes[v2i]===undefined) {
                result.push(tangent.x);
                result.push(tangent.y);
                result.push(tangent.z);
                generatedIndexes[v2i] = tangent;
            }
        }
    }
    else {	// other draw modes: lines, line_strip
        for (let i=0; i<plist.vertex.length; i+=3) {
            result.push(0,0,1);
        }
    }

    if (invalidUV) {
        console.warn(`Invalid UV texture coords found in PolyList '${ plist.name }'. Some objects may present artifacts in the lighting, and not display textures properly.`)
    }

    plist._tangent = result;
    */
}

}
}
