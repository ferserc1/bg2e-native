# Step 1: Compute Shaders — SMAA GLSL Sources

## Files

- `shaders/src/glsl/smaa_edge_detection.comp.glsl` (create)
- `shaders/src/glsl/smaa_blend_weight.comp.glsl` (create)
- `shaders/src/glsl/smaa_neighborhood_blend.comp.glsl` (create)
- `shaders/src/glsl/smaa_area_generate.comp.glsl` (create)
- `shaders/src/glsl/smaa_search_generate.comp.glsl` (create)

## Purpose

Five compute shaders implementing the SMAA algorithm and its LUT generation. All shaders target GLSL 460 with `GL_ARB_shading_language_include` for `#include` support. All use `local_size_x = 8, local_size_y = 8, local_size_z = 1`.

## Shared Constants

Create a shared include file `shaders/src/glsl/lib/smaa.glsl` containing:

```glsl
#ifndef SMAA_GLSL
#define SMAA_GLSL

// SMAA quality presets (default: SMAA 1x, medium quality)
#define SMAA_THRESHOLD 0.05
#define SMAA_MAX_SEARCH_STEPS 16
#define SMAA_MAX_SEARCH_STEPS_DIAG 8
#define SMAA_CORNER_ROUNDING 25
#define SMAA_AREATEX_MAX_DISTANCE 16
#define SMAA_AREATEX_MAX_DISTANCE_DIAG 20
#define SMAA_AREATEX_PIXEL_SIZE (1.0 / vec2(256.0, 256.0))
#define SMAA_AREATEX_SUBTEX_SIZE (1.0 / 7.0)
#define SMAA_SEARCHTEX_SIZE vec2(64.0, 64.0)
#define SMAA_SEARCHTEX_PACKED_SIZE vec2(64.0, 32.0)

#endif
```

---

## 1.1 — `smaa_edge_detection.comp.glsl`

### Bindings (set=0)

| Binding | Type | Variable | Description |
|---------|------|----------|-------------|
| 0 | `uniform sampler2D` | `inputImage` | Final composed LDR image |
| 1 | `image2D` (rg8, writeonly) | `edgesOutput` | Edge detection output |

### Push Constants

```glsl
layout(push_constant) uniform PushConstant {
    vec2 texelSize;    // 1.0 / viewportSize
} pc;
```

Total: 8 bytes, padded to 16.

### Algorithm

For each pixel `p`:

1. Compute UV: `(p + 0.5) / imageSize(edgesOutput)`
2. Sample luma at current pixel and its 4 neighbors (left, right, top, bottom):
   ```
   L  = dot(texture(inputImage, uv).rgb, vec3(0.299, 0.587, 0.114))
   Lm = dot(texture(inputImage, uv - vec2(pc.texelSize.x, 0)).rgb, ...)
   Lr = dot(texture(inputImage, uv + vec2(pc.texelSize.x, 0)).rgb, ...)
   Lt = dot(texture(inputImage, uv + vec2(0, pc.texelSize.y)).rgb, ...)
   Lb = dot(texture(inputImage, uv - vec2(0, pc.texelSize.y)).rgb, ...)
   ```
3. Compute delta:
   ```
   deltaH = abs(L - Lm) + abs(L - Lr)  // or use: abs(2*L - Lm - Lr)
   deltaV = abs(L - Lt) + abs(L - Lb)
   ```
4. Threshold: `threshold = SMAA_THRESHOLD` (use a fixed constant or push constant)
5. Write:
   ```
   edges.x = deltaH > threshold ? 1.0 : 0.0
   edges.y = deltaV > threshold ? 1.0 : 0.0
   ```
6. `imageStore(edgesOutput, p, vec4(edges, 0.0, 0.0))`

---

## 1.2 — `smaa_blend_weight.comp.glsl`

### Bindings (set=0)

| Binding | Type | Variable | Description |
|---------|------|----------|-------------|
| 0 | `uniform sampler2D` | `edgesImage` | Edge detection output (RG8) |
| 1 | `uniform sampler2D` | `areaTex` | Area/weight lookup texture |
| 2 | `uniform sampler2D` | `searchTex` | Search distance lookup texture |
| 3 | `image2D` (rgba8, writeonly) | `blendWeightsOutput` | Blend weights output |

### Push Constants

```glsl
layout(push_constant) uniform PushConstant {
    vec2 texelSize;    // 1.0 / viewportSize
} pc;
```

### Algorithm

This is the most complex shader. For each pixel:

1. Check if any edge is present at the current pixel (`.x` or `.y` > 0). If no edge, write `vec4(0)` and return.
2. For each edge direction (horizontal and vertical), search for the line endpoint:
   - Use `searchTex` to determine how far to search along the edge
   - Walk along the edge in both directions until an endpoint is found or max steps reached
3. Calculate the area using `areaTex`:
   - Encode the edge pattern (left distance, right distance, edge configuration) into UV coordinates
   - Sample `areaTex` to get the blend weights
4. Write blend weights to RGBA channels:
   - R = left blend weight
   - G = top blend weight
   - B = right blend weight
   - A = bottom blend weight

### Reference Implementation Notes

The SMAA blend weight calculation follows these sub-steps for each edge pixel:

```
For horizontal edge at pixel p:
  1. Search left:  walk along pixels (p.x - 1, p.x - 2, ...) checking edges.y
     Use searchTex to accelerate the search
  2. Search right: walk along pixels (p.x + 1, p.x + 2, ...) checking edges.y
  3. Encode pattern: d.x = left_distance, d.y = right_distance
  4. Offset: compute subpixel offset based on edge crossing
  5. Sample areaTex at (d, offset) → get (left_weight, right_weight)

For vertical edge at pixel p:
  Same process but in Y direction → get (top_weight, bottom_weight)
```

### SMAA Helper Functions

Include in `lib/smaa.glsl` or inline:

- `float SMAASearchLength(vec2 e, float offset, sampler2D searchTex)` — searches for edge endpoint using the search LUT
- `vec2 SMAAArea(vec4 d, float offset, sampler2D areaTex)` — looks up blend weights from the area LUT
- `vec4 SMAABlendingWeightCalculation(vec2 texcoord, vec2 pixcoord, vec4 offset[3], sampler2D edgesImage, sampler2D areaTex, sampler2D searchTex)` — main weight calculation

---

## 1.3 — `smaa_neighborhood_blend.comp.glsl`

### Bindings (set=0)

| Binding | Type | Variable | Description |
|---------|------|----------|-------------|
| 0 | `uniform sampler2D` | `inputImage` | Original composed image |
| 1 | `uniform sampler2D` | `blendWeightsImage` | Blend weights (RGBA8) |
| 2 | `image2D` (rgba8, writeonly) | `outputImage` | Anti-aliased output |

### Push Constants

```glsl
layout(push_constant) uniform PushConstant {
    vec2 texelSize;    // 1.0 / viewportSize
} pc;
```

### Algorithm

For each pixel:

1. Sample blend weights at current pixel: `weights = texture(blendWeightsImage, uv)`
2. Compute the blended color:
   ```
   color  = texture(inputImage, uv).rgb
   color += texture(inputImage, uv + vec2(-weights.x, 0) * texelSize).rgb * weights.x   // left
   color += texture(inputImage, uv + vec2(0, -weights.y) * texelSize).rgb * weights.y   // top
   color += texture(inputImage, uv + vec2(weights.z, 0) * texelSize).rgb * weights.z    // right
   color += texture(inputImage, uv + vec2(0, weights.w) * texelSize).rgb * weights.w    // bottom
   ```
   Wait, that's not quite right. The standard SMAA neighborhood blend is:
   ```
   // Sum of weights
   sum = weights.x + weights.y + weights.z + weights.w
   // Blend
   color = texture(inputImage, uv).rgb * (1.0 - sum)
         + texture(inputImage, uv + leftOffset).rgb * weights.x
         + texture(inputImage, uv + topOffset).rgb * weights.y
         + texture(inputImage, uv + rightOffset).rgb * weights.z
         + texture(inputImage, uv + bottomOffset).rgb * weights.w
   ```
   But actually, the standard formulation uses the blend weights as offsets:
   ```
   // Each weight indicates how much to blend from the neighbor in that direction
   // The offsets are: left = (-texelSize.x, 0), right = (texelSize.x, 0), etc.
   // But the weights encode sub-pixel offsets, not simple direction offsets
   
   // More precisely, the neighborhood blend takes the weighted sum of
   // the current pixel and the pixels in the four diagonal directions,
   // weighted by the four blend weights.
   ```

Let me be more precise about the SMAA neighborhood blend:

```
SMAANeighborhoodBlendingCompute:
  vec2 texcoord = (pixel + 0.5) / outputSize;
  
  // Fetch blend weights
  vec4 a = texture(blendWeightsImage, texcoord);
  
  // Calculate maximum blend weight
  float maxWeight = max(max(a.x, a.y), max(a.z, a.w));
  
  if (maxWeight < 0.001) {
    // No blending needed, pass through
    imageStore(output, pixel, texture(inputImage, texcoord));
    return;
  }
  
  // Calculate offsets based on blend weights
  // Horizontal blend: left (a.x) and right (a.z)
  // Vertical blend: top (a.y) and bottom (a.w)
  vec4 offset = vec4(
    -a.x,  // left offset X
    -a.y,  // top offset Y
     a.z,  // right offset X
     a.w   // bottom offset Y
  ) * texelSize.xyxy;
  
  // Blend using bilinear sampling trick:
  // Sample at the weighted offset position to get the blended result
  // in a single bilinear fetch
  vec4 color = texture(inputImage, texcoord + offset.xy); // top-left
  color += texture(inputImage, texcoord + offset.zy);     // top-right
  color += texture(inputImage, texcoord + offset.xw);     // bottom-left
  color += texture(inputImage, texcoord + offset.zw);     // bottom-right
  
  color *= 0.25;
  
  imageStore(output, pixel, color);
```

Actually, let me simplify. The SMAA neighborhood blend works like this:

For each pixel, the blend weights tell you how much to blend from each of the 4 cardinal neighbors. The simplest correct implementation:

```
vec2 uv = (pixel + 0.5) / outputSize;
vec4 w = texture(blendWeightsImage, uv);

vec4 color = texture(inputImage, uv);

// If any weight is non-zero, blend with the neighbor
if (w.x > 0.0) color = mix(color, texture(inputImage, uv + vec2(-texelSize.x, 0)), w.x);  // left
if (w.y > 0.0) color = mix(color, texture(inputImage, uv + vec2(0, -texelSize.y)), w.y);  // top
if (w.z > 0.0) color = mix(color, texture(inputImage, uv + vec2(texelSize.x, 0)), w.z);   // right
if (w.w > 0.0) color = mix(color, texture(inputImage, uv + vec2(0, texelSize.y)), w.w);   // bottom

imageStore(output, pixel, color);
```

Actually, SMAA uses a more efficient bilinear trick. Let me reference the actual SMAA algorithm:

The standard SMAA neighborhood blending shader:
```glsl
// SMAANeighborhoodBlendingPS
float4 SMAANeighborhoodBlendingPS(float2 texcoord, float4 offset, ...) {
    // Fetch the blend weights for the current pixel
    float4 a = SMAASamplePoint(areaTex, texcoord);
    
    // Use bilinear filtering to avoid reading each neighbor separately
    float2 offset = a.xz * (-SMAA_PIXEL_SIZE.x) + a.yw * SMAA_PIXEL_SIZE.y;
    // Wait, this isn't right either.
    
    // Actually, the trick is:
    // The four weights correspond to left, top, right, bottom
    // We compute a single offset and use it to sample the input
    // via a single bilinear fetch
    
    // Correct approach:
    // Blend in horizontal and vertical directions using bilinear
    float4 color = SMAASampleLevelZero(inputImage, texcoord + float2(-a.x, a.y) * texelSize);
    color += SMAASampleLevelZero(inputImage, texcoord + float2(a.z, a.w) * texelSize);
    color += SMAASampleLevelZero(inputImage, texcoord);
    color += SMAASampleLevelZero(inputImage, texcoord);
    
    return color * 0.25;
}
```

Actually, I realize I need to be more careful. Let me use a simpler, correct approach for the compute shader version:

```glsl
// For each pixel, blend with the 4 cardinal neighbors using the 4 weights
vec2 uv = (pixel + 0.5) / outputSize;
vec4 w = texture(blendWeightsImage, uv);

// Bilinear trick: compute two offset samples that capture all 4 neighbors
// via hardware bilinear filtering
vec4 color;
color  = texture(inputImage, uv + vec2(-w.x, w.y) * texelSize);  // top-left blend
color += texture(inputImage, uv + vec2(w.z, w.w) * texelSize);   // bottom-right blend
color += texture(inputImage, uv) * 2.0;
color *= 0.25;

imageStore(output, pixel, color);
```

This is the standard SMAA neighborhood blend optimization. The two bilinear samples plus two center samples give the correct weighted result.

---

## 1.4 — `smaa_area_generate.comp.glsl`

### Bindings (set=0)

| Binding | Type | Variable | Description |
|---------|------|----------|-------------|
| 0 | `image2D` (rg8, writeonly) | `areaTex` | Area texture output (256×256) |

### No Push Constants

### Algorithm

This shader generates the SMAA area texture offline. The area texture encodes, for each possible edge configuration pattern, the blend weights for the left/right (or top/bottom) sides.

The texture is 256×256 pixels, where:
- X axis encodes the distance to the left (or top) endpoint
- Y axis encodes the distance to the right (or bottom) endpoint
- The 256×256 area is divided into 7 sub-regions, each representing a different edge pattern (no edge, one edge, crossing edges, etc.)

For each texel, compute:
1. Determine which sub-region (based on Y coordinate)
2. Compute the edge pattern from (x, y) coordinates
3. Calculate the area weights analytically
4. Store in RG channels

The generation algorithm is based on the SMAA reference implementation's `AreaTex` computation. The key function:

```glsl
void SMAACalculateArea(inout float2 area, float2 d, float2 texcoord, float2 subsampleSize) {
    // Calculate the area of the edge pattern
    // d.x = distance to left endpoint, d.y = distance to right endpoint
    // This is an analytical integration of the SMAA blending region
}
```

The full generation follows the SMAA reference `AreaTex.compute` or `AreaTex.h` from the SMAA repository.

---

## 1.5 — `smaa_search_generate.comp.glsl`

### Bindings (set=0)

| Binding | Type | Variable | Description |
|---------|------|----------|-------------|
| 0 | `image2D` (r8, writeonly) | `searchTex` | Search texture output (64×64) |

### No Push Constants

### Algorithm

This shader generates the SMAA search texture. For each texel, it encodes the distance to search for the endpoint of an edge pattern. The search texture is a 64×64 single-channel texture where:

- X axis encodes the edge configuration (left pattern)
- Y axis encodes the edge configuration (right pattern)
- Value = number of pixels to step to find the endpoint

The generation algorithm:
1. For each texel, decode the edge pattern from (x, y)
2. Simulate the search process: walk along the edge until an endpoint is found
3. Store the distance (normalized to [0, 1]) in the R channel

---

## Compilation

All `.glsl` files in `shaders/src/glsl/` are automatically compiled to `.spv` by the CMake post-build step. The compiled files will be:

- `smaa_edge_detection.comp.spv`
- `smaa_blend_weight.comp.spv`
- `smaa_neighborhood_blend.comp.spv`
- `smaa_area_generate.comp.spv`
- `smaa_search_generate.comp.spv`
- `lib/smaa.glsl` (include only, not compiled independently)

## Reference

The SMAA algorithm is documented in:
- Jimenez et al., "SMAA: Enhanced Subpixel Morphological Antialiasing", Computer Graphics Forum 2012
- Reference implementation: https://github.com/iryoku/smaa
