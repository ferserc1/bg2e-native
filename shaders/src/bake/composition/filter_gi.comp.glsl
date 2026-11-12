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
layout(set=0,binding=3) uniform utexture2D input1;
layout(set=0,binding=4) uniform texture2D input2;
layout(set=0,binding=5) uniform texture2D input3;
layout(set=0,binding=6,rgba32f) uniform writeonly image2D output0;
#define READ0(p) texelFetch(sampler2D(input0,nearestSampler),p,0)
#define READ1(p) texelFetch(usampler2D(input1,nearestSampler),p,0)
#define READ2(p) texelFetch(sampler2D(input2,nearestSampler),p,0)
#define READ3(p) texelFetch(sampler2D(input3,nearestSampler),p,0)
#define WRITE0(v) imageStore(output0,p,v)
layout(local_size_x=16,local_size_y=16,local_size_z=1) in;
void main()
{
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    int width = imageSize(output0).x, height = imageSize(output0).y;
    if (p.x >= width || p.y >= height)
    {
        return;
    }
    // Filter only the completed estimate; raw sums and counts never feed back
    // filtered values. Chart zero is uncovered atlas space.
    uint chart = READ1(p).r;
    vec3 center = READ0(p).rgb;
    if (chart == 0u)
    {
        WRITE0(vec4(center,1.0));
        return;
    }
    vec3 normal = normalize(READ2(p).xyz);
    vec3 position = READ3(p).xyz;
    float footprint = 1e30;
    for (int axis = 0; axis < 4; ++axis)
    {
        ivec2 offset = axis == 0 ? ivec2(1,0) : axis == 1 ? ivec2(-1,0) :
            axis == 2 ? ivec2(0,1) : ivec2(0,-1);
        ivec2 q = p+offset;
        if (q.x >= 0 && q.y >= 0 && q.x < width && q.y < height && READ1(q).r == chart)
        {
            float distance = length(READ3(q).xyz-position);
            if (distance > 0.0)
            {
                footprint = min(footprint,distance);
            }
        }
    }
    // A singleton chart must not borrow a footprint from another island.
    if (footprint == 1e30)
    {
        WRITE0(vec4(center,1.0));
        return;
    }
    vec3 sum = center;
    float weights = 1.0;
    for (int y = -3; y <= 3; ++y)
    {
        for (int x = -3; x <= 3; ++x)
        {
            ivec2 q = p+ivec2(x,y);
            if ((x == 0 && y == 0) || q.x < 0 || q.y < 0 || q.x >= width || q.y >= height)
            {
                continue;
            }
            if (READ1(q).r != chart)
            {
                continue;
            }
            vec3 neighborNormal = normalize(READ2(q).xyz);
            vec3 delta = READ3(q).xyz-position;
            float alignment = dot(normal,neighborNormal);
            // Reject folds, detached surfaces and discontinuous UV mappings.
            if (alignment < 0.9 || length(delta) > footprint*6.0 ||
                max(abs(dot(delta,normal)),abs(dot(delta,neighborNormal))) > footprint*0.5)
            {
                continue;
            }
            vec3 value = READ0(q).rgb;
            float centerLuminance = dot(center,vec3(0.2126,0.7152,0.0722));
            float luminance = dot(value,vec3(0.2126,0.7152,0.0722));
            float difference = (luminance-centerLuminance)/max(0.25,0.5*centerLuminance);
            float weight = exp(-float(x*x+y*y)/8.0)*pow(max(alignment,0.0),32.0)*
                exp(-0.5*difference*difference);
            sum += value*weight;
            weights += weight;
        }
    }
    WRITE0(vec4(sum/weights,1.0));
}
