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
layout(set=0,binding=0) uniform texture2D sourceImage;
layout(set=0,binding=1) uniform sampler sourceSampler;
layout(push_constant) uniform DebugPush
{
    mat4 inverseProjection;
    vec4 display;
    vec4 rangeMode;
} pc;
layout(location=0) out vec4 outColor;
void main()
{
    vec2 uv = (gl_FragCoord.xy/pc.display.xy-0.5)/pc.display.w+0.5;
    ivec2 size = textureSize(sampler2D(sourceImage,sourceSampler),0);
    ivec2 pixel = clamp(ivec2(uv*vec2(size)),ivec2(0),size-1);
    vec4 value = texelFetch(sampler2D(sourceImage,sourceSampler),pixel,0);
    uint mode = uint(pc.rangeMode.z);
    if (mode == 1u)
    {
        value.rgb = vec3(value.r);
    }
    if (mode == 2u)
    {
        value.rgb = vec3(value.g);
    }
    if (mode == 3u)
    {
        value.rgb = vec3(value.b);
    }
    if (mode == 4u)
    {
        value.rgb = vec3(value.a);
    }
    if (mode == 5u)
    {
        vec2 center = (vec2(pixel)+0.5)/vec2(size);
        vec4 view = pc.inverseProjection*vec4(center.x*2.0-1.0,1.0-center.y*2.0,value.r,1);
        float depth = abs(view.z/max(abs(view.w),1e-8));
        value.rgb = vec3(clamp((depth-pc.rangeMode.x)/(pc.rangeMode.y-pc.rangeMode.x),0.0,1.0));
    }
    if (pc.rangeMode.w >= 0.0)
    {
        value.rgb = clamp(value.rgb*exp2(pc.display.z),vec3(0),vec3(1));
    }
    if (pc.rangeMode.w > 0.5)
    {
        vec3 lo = value.rgb/12.92;
        vec3 hi = pow((value.rgb+0.055)/1.055,vec3(2.4));
        value.rgb = mix(hi,lo,lessThanEqual(value.rgb,vec3(0.04045)));
    }
    outColor = pc.rangeMode.w < 0.0 ? value : vec4(value.rgb,1);
}
