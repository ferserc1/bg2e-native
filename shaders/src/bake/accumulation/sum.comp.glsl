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
layout(set=0,binding=0,std140) uniform Parameters { uvec4 parameters; };
layout(set=0,binding=1) uniform sampler nearestSampler;
layout(set=0,binding=2) uniform texture2D input0;
#define READ0(p) texelFetch(sampler2D(input0,nearestSampler),p,0)
layout(set=0,binding=3) uniform texture2D input1;
#define READ1(p) texelFetch(sampler2D(input1,nearestSampler),p,0)
layout(set=0,binding=4) uniform utexture2D input2;
#define READ2(p) texelFetch(usampler2D(input2,nearestSampler),p,0)
layout(set=0,binding=5) uniform texture2D input3;
#define READ3(p) texelFetch(sampler2D(input3,nearestSampler),p,0)
layout(set=0,binding=6) uniform utexture2D input4;
#define READ4(p) texelFetch(usampler2D(input4,nearestSampler),p,0)
layout(set=0,binding=7) uniform utexture2D input5;
#define READ5(p) texelFetch(usampler2D(input5,nearestSampler),p,0)
layout(set=0,binding=8,rgba32f) uniform writeonly image2D output0;
#define WRITE0(v) imageStore(output0,p,v)
layout(set=0,binding=9,r32ui) uniform writeonly uimage2D output1;
#define WRITE1(v) imageStore(output1,p,v)
bool finite3(vec3 v)
{
    return !any(isnan(v)) && !any(isinf(v));
}
layout(local_size_x=16,local_size_y=16,local_size_z=1) in;
void main()
{
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    int width = imageSize(output0).x, height = imageSize(output0).y;
    if (p.x >= width || p.y >= height)
    {
        return;
    }
    vec4 oldSum = parameters.y != 0u ? READ1(p) : vec4(0.0);
    uint oldCount = parameters.y != 0u ? READ2(p).r : 0u;
    uint chart = READ4(p).r;
    bool covered = READ3(p).r > 0.5;
    float error = oldSum.a;
    if (covered != (chart != 0u) || (parameters.y != 0u && READ5(p).r != chart))
    {
        error = 1.0;
    }
    vec3 sum = oldSum.rgb;
    uint count = oldCount;
    if (covered)
    {
        vec4 raw = READ0(p);
        bool valid = finite3(raw.rgb);
        vec3 value = parameters.x == 0u ? vec3(raw.r) : raw.rgb;
        if (parameters.x == 0u)
        {
            valid = valid && raw.r >= 0.0 && raw.r <= 1.0;
        }
        else
        {
            // Public GI uses a half-float target and saturates at 65504.
            // Conservatively reject the saturation boundary, never accept clipped HDR.
            valid = valid && raw.a == 1.0 && min(value.x,min(value.y,value.z)) >= 0.0 &&
                max(value.x,max(value.y,value.z)) < 65504.0;
        }
        if (valid)
        {
            sum += value;
            count += 1u;
        }
        else
        {
            error = 1.0;
        }
        if (!finite3(sum))
        {
            error = 1.0;
        }
    }
    else
    {
        if (oldCount != 0u)
        {
            error = 1.0;
        }
        sum = vec3(0.0);
        count = 0u;
    }
    WRITE0(vec4(sum,error));
    WRITE1(uvec4(count,0u,0u,0u));
}
