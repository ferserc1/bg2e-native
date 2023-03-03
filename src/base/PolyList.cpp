
#include <bg2e/base/PolyList.hpp>

namespace bg2e {
namespace base {

void buildTangents(PolyList* pl)
{
    
}

PolyList::PolyList()
{
    
}

const std::vector<float>& PolyList::tangent()
{
    if (!validTangents())
    {
        buildTangents(this);
    }
    return _tangent;
}

bool PolyList::validTangents()
{
    return  _tangent.size() == _vertex.size() &&
            _tangent.size() / 3 == _texCoord0.size() / 2;
}

void PolyList::rebuildTangents()
{
    buildTangents(this);
}

}
}
