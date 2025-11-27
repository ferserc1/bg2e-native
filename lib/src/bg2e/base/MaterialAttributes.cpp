
#include <bg2e/base/MaterialAttributes.hpp>

namespace bg2e {
namespace base {

MaterialAttributes::MaterialAttributes()
{
}

void MaterialAttributes::setUpdated()
{
    _albedoTextureUpdated = true;
    _metalnessTextureUpdated = true;
    _roughnessTextureUpdated = true;
    _normalTextureUpdated = true;
    _aoTextureUpdated = true;
}

}
}
