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

layout(set = 0, binding = 0) uniform texture2D sourceImage;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(push_constant) uniform BloomPush
{
    vec4 extent;
    vec4 parameters;
} settings;
layout(location = 0) out vec4 outColor;

vec3 sampleSource(vec2 uv)
{
    return textureLod(sampler2D(sourceImage, sourceSampler), uv, 0.0).rgb;
}

vec3 extractBright(ivec2 pixel)
{
    ivec2 size = textureSize(sampler2D(sourceImage, sourceSampler), 0);
    vec3 c = texelFetch(sampler2D(sourceImage, sourceSampler), clamp(pixel, ivec2(0), size - 1), 0).rgb;
    // Prevent nonfinite highlights from contaminating the complete pyramid.
    c = vec3(isnan(c.r) ? 0.0 : c.r, isnan(c.g) ? 0.0 : c.g, isnan(c.b) ? 0.0 : c.b);
    c = clamp(c, vec3(0.0), vec3(65504.0));
    float brightness = max(c.r, max(c.g, c.b));
    return c * (max(brightness - settings.parameters.x, 0.0) / max(brightness, 1e-6));
}

void main()
{
    vec2 uv = gl_FragCoord.xy / settings.extent.xy;
    vec2 texel = 1.0 / vec2(textureSize(sampler2D(sourceImage, sourceSampler), 0));
    vec3 color = vec3(0.0);
    if (settings.extent.w < 0.5)
    {
        // Threshold individual HDR texels BEFORE averaging: a small saturated
        // highlight must not disappear into its darker neighbours at quarter size.
        int divisor = int(settings.extent.z);
        ivec2 base = ivec2(gl_FragCoord.xy) * divisor;
        for (int y = 0; y < divisor; ++y)
        {
            for (int x = 0; x < divisor; ++x)
            {
                color += extractBright(base + ivec2(x, y)) / float(divisor * divisor);
            }
        }
    }
    else if (settings.extent.w < 1.5)
    {
        // Dual downsample: centre and four diagonals, using bilinear filtering.
        color = sampleSource(uv) * 0.5;
        color += sampleSource(uv + texel * vec2(-1.0, -1.0)) * 0.125;
        color += sampleSource(uv + texel * vec2( 1.0, -1.0)) * 0.125;
        color += sampleSource(uv + texel * vec2(-1.0,  1.0)) * 0.125;
        color += sampleSource(uv + texel * vec2( 1.0,  1.0)) * 0.125;
    }
    else
    {
        // Dual upsample: eight bilinear taps. Fixed-function alpha blending
        // combines this coarse level with the retained fine level, without feedback.
        color = sampleSource(uv + texel * vec2(-1.0,  0.0)) / 12.0;
        color += sampleSource(uv + texel * vec2( 1.0,  0.0)) / 12.0;
        color += sampleSource(uv + texel * vec2( 0.0, -1.0)) / 12.0;
        color += sampleSource(uv + texel * vec2( 0.0,  1.0)) / 12.0;
        color += sampleSource(uv + texel * vec2(-0.5, -0.5)) / 6.0;
        color += sampleSource(uv + texel * vec2( 0.5, -0.5)) / 6.0;
        color += sampleSource(uv + texel * vec2(-0.5,  0.5)) / 6.0;
        color += sampleSource(uv + texel * vec2( 0.5,  0.5)) / 6.0;
    }
    outColor = vec4(color, settings.extent.w > 1.5 ? settings.parameters.y : 1.0);
}
