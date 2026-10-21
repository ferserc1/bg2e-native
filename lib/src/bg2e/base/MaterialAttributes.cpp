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

#include <bg2e/base/MaterialAttributes.hpp>

namespace bg2e {
namespace base {

MaterialAttributes::MaterialAttributes()
{
}

MaterialAttributes& MaterialAttributes::operator=(const MaterialAttributes& source)
{
    if (this != &source)
    {
        assign(_isTransparent, source._isTransparent);
        assign(_isSolid, source._isSolid);
        assign(_unlit, source._unlit);
        assign(_refractionFactor, source._refractionFactor);
        assign(_alphaCutoff, source._alphaCutoff);
        assign(_albedo, source._albedo);
        assign(_albedoTexture, source._albedoTexture);
        assign(_albedoScale, source._albedoScale);
        assign(_albedoUVSet, source._albedoUVSet);
        assign(_metalness, source._metalness);
        assign(_metalnessTexture, source._metalnessTexture);
        assign(_metalnessScale, source._metalnessScale);
        assign(_metalnessChannel, source._metalnessChannel);
        assign(_metalnessInvert, source._metalnessInvert);
        assign(_metalnessUVSet, source._metalnessUVSet);
        assign(_roughness, source._roughness);
        assign(_roughnessTexture, source._roughnessTexture);
        assign(_roughnessScale, source._roughnessScale);
        assign(_roughnessChannel, source._roughnessChannel);
        assign(_roughnessInvert, source._roughnessInvert);
        assign(_roughnessUVSet, source._roughnessUVSet);
        assign(_normalTexture, source._normalTexture);
        assign(_normalScale, source._normalScale);
        assign(_normalUVSet, source._normalUVSet);
        assign(_fresnelTint, source._fresnelTint);
        assign(_sheenIntensity, source._sheenIntensity);
        assign(_sheenColor, source._sheenColor);
        assign(_indirectTextureMode, source._indirectTextureMode);
        assign(_aoTexture, source._aoTexture);
        assign(_aoScale, source._aoScale);
        assign(_aoChannel, source._aoChannel);
        assign(_aoUVSet, source._aoUVSet);
        assign(_lightEmission, source._lightEmission);
        assign(_lightEmissionTexture, source._lightEmissionTexture);
        assign(_lightEmissionScale, source._lightEmissionScale);
        assign(_lightEmissionChannel, source._lightEmissionChannel);
        assign(_lightEmissionInvert, source._lightEmissionInvert);
        assign(_lightEmissionUVSet, source._lightEmissionUVSet);
        assign(_name, source._name);
        assign(_groupName, source._groupName);
        assign(_visible, source._visible);
        _albedoTextureUpdated = source._albedoTextureUpdated;
        _metalnessTextureUpdated = source._metalnessTextureUpdated;
        _roughnessTextureUpdated = source._roughnessTextureUpdated;
        _normalTextureUpdated = source._normalTextureUpdated;
        _aoTextureUpdated = source._aoTextureUpdated;
        _lightEmissionTextureUpdated = source._lightEmissionTextureUpdated;
    }
    return *this;
}

void MaterialAttributes::setUpdated()
{
    _albedoTextureUpdated = true;
    _metalnessTextureUpdated = true;
    _roughnessTextureUpdated = true;
    _normalTextureUpdated = true;
    _aoTextureUpdated = true;
    _lightEmissionTextureUpdated = true;
}

}
}
