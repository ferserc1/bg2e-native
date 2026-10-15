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

#version 450
layout(set = 0, binding = 0) uniform texture2D image0;
layout(set = 0, binding = 8) uniform sampler sample0;
layout(set = 0, binding = 1) uniform texture2D image1;
layout(set = 0, binding = 9) uniform sampler sample1;
layout(set = 0, binding = 2) uniform texture2D image2;
layout(set = 0, binding = 10) uniform sampler sample2;
layout(set = 0, binding = 3) uniform texture2D image3;
layout(set = 0, binding = 11) uniform sampler sample3;
layout(set = 0, binding = 4) uniform texture2D image4;
layout(set = 0, binding = 12) uniform sampler sample4;
layout(set = 0, binding = 5) uniform texture2D image5;
layout(set = 0, binding = 13) uniform sampler sample5;
layout(set = 0, binding = 6) uniform texture2D image6;
layout(set = 0, binding = 14) uniform sampler sample6;
layout(set = 0, binding = 7) uniform texture2D image7;
layout(set = 0, binding = 15) uniform sampler sample7;
layout(push_constant) uniform FinalPush
{
    vec4 color;
    vec4 outputExtent;
    uvec4 display;
} settings;
layout(location = 0) out vec4 outColor;
vec4 fetchSource(vec2 uv)
{
    if (settings.display.x == 0u || settings.display.x == 8u)
    {
        return textureLod(sampler2D(image0, sample0), uv, 0.0);
    }
    if (settings.display.x == 1u)
    {
        return textureLod(sampler2D(image1, sample1), uv, 0.0);
    }
    if (settings.display.x == 2u)
    {
        return textureLod(sampler2D(image2, sample2), uv, 0.0);
    }
    if (settings.display.x == 3u)
    {
        return textureLod(sampler2D(image3, sample3), uv, 0.0);
    }
    if (settings.display.x == 4u)
    {
        return textureLod(sampler2D(image4, sample4), uv, 0.0);
    }
    if (settings.display.x == 5u)
    {
        return textureLod(sampler2D(image5, sample5), uv, 0.0);
    }
    if (settings.display.x == 6u)
    {
        return textureLod(sampler2D(image6, sample6), uv, 0.0);
    }
    return textureLod(sampler2D(image7, sample7), uv, 0.0);
}
vec4 processColor()
{
    vec2 uv = gl_FragCoord.xy / vec2(settings.outputExtent.xy);
    vec2 size = (settings.display.x == 0u || settings.display.x == 8u)
        ? vec2(textureSize(sampler2D(image0, sample0), 0))
        : vec2(textureSize(sampler2D(image1, sample1), 0));
    vec2 coordinate = uv * size - 0.5;
    vec2 base = floor(coordinate);
    vec2 weight = coordinate - base;
    vec2 uv00 = (base + 0.5) / size;
    vec2 uv10 = (base + vec2(1.5, 0.5)) / size;
    vec2 uv01 = (base + vec2(0.5, 1.5)) / size;
    vec2 uv11 = (base + 1.5) / size;
    vec4 result = mix(mix(fetchSource(uv00), fetchSource(uv10), weight.x), mix(fetchSource(uv01), fetchSource(uv11), weight.x), weight.y);
    if (settings.display.x == 0u)
    {
        result.rgb = 1.0 - exp(-max(result.rgb, vec3(0.0)) * exp2(settings.color.x));
        result.rgb = pow(result.rgb, vec3(1.0 / settings.color.w));
        result.rgb = clamp((result.rgb + settings.color.z - 0.5) * settings.color.y + 0.5, vec3(0.0), vec3(1.0));
    }
    else if (settings.display.x == 7u)
    {
        result.rgb = vec3(result.r);
    }
    // Convert the display-encoded value back to linear for native sRGB output.
    // The attachment encoder then produces the same display value as UNORM.
    if (settings.display.y != 0u)
    {
        vec3 c = max(result.rgb, vec3(0.0));
        vec3 lo = c / 12.92;
        vec3 hi = pow((c + 0.055) / 1.055, vec3(2.4));
        result.r = c.r <= 0.04045 ? lo.r : hi.r;
        result.g = c.g <= 0.04045 ? lo.g : hi.g;
        result.b = c.b <= 0.04045 ? lo.b : hi.b;
    }
    return vec4(result.rgb, 1.0);
}
void main()
{
    outColor = processColor();
}
