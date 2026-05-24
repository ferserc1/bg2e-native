# Phase 6: Composite Shader Modifications

## Files

- `shaders/src/deferred_composite.frag.glsl` (modify)
- `shaders/src/deferred_composite_rt.frag.glsl` (modify)

Both shaders receive the same change: add `g_AO` sampler at binding 7 and multiply the sampled AO value into the `ambientOcclussion` parameter of `calcAmbientLight`.

---

## Change 1: `deferred_composite.frag.glsl` — Add AO Sampler

### Location

After line 34 (`g_Depth` declaration)

### Add

```glsl
layout(set = 0, binding = 7) uniform sampler2D g_AO;
```

### Context (lines 28-36)

```glsl
// G-buffer samplers (set=0)
layout(set = 0, binding = 0) uniform sampler2D g_Albedo;
layout(set = 0, binding = 1) uniform sampler2D g_Normal;
layout(set = 0, binding = 2) uniform sampler2D g_Material;
layout(set = 0, binding = 3) uniform sampler2D g_FresnelFlags;
layout(set = 0, binding = 4) uniform sampler2D g_SheenColor;
layout(set = 0, binding = 5) uniform sampler2D g_InputImage;
layout(set = 0, binding = 6) uniform sampler2D g_Depth;
layout(set = 0, binding = 7) uniform sampler2D g_AO;  // ← NEW
```

---

## Change 2: `deferred_composite.frag.glsl` — Apply AO in calcAmbientLight

### Location

Lines 110-113 (the `calcAmbientLight` call)

### Before

```glsl
vec3 ambient = calcAmbientLight(gbuf.viewDir, gbuf.normal, gbuf.F0, gbuf.albedo.rgb,
                                gbuf.metallic, gbuf.roughness,
                                irradianceMap, prefilteredEnvMap, environmentData.maxReflectionLOD,
                                brdfLUT, gbuf.ao, gbuf.sheenIntensity, gbuf.sheenColor);
```

### After

```glsl
float rtAo = texture(g_AO, vTexcoord).r;
vec3 ambient = calcAmbientLight(gbuf.viewDir, gbuf.normal, gbuf.F0, gbuf.albedo.rgb,
                                gbuf.metallic, gbuf.roughness,
                                irradianceMap, prefilteredEnvMap, environmentData.maxReflectionLOD,
                                brdfLUT, gbuf.ao * rtAo, gbuf.sheenIntensity, gbuf.sheenColor);
```

**Note**: `gbuf.ao` is the material AO from the G-buffer (stored in `g_Material.b`). `rtAo` is the ray-traced AO from the compute shader. They are multiplied together, so the final AO is the product of both. When the fallback white image is used, `rtAo = 1.0`, so the result is just `gbuf.ao` (no change from original behavior).

---

## Change 3: `deferred_composite_rt.frag.glsl` — Add AO Sampler

### Location

After line 36 (`g_Depth` declaration)

### Add

```glsl
layout(set = 0, binding = 7) uniform sampler2D g_AO;
```

### Context (lines 30-38)

```glsl
// G-buffer samplers (set=0)
layout(set = 0, binding = 0) uniform sampler2D g_Albedo;
layout(set = 0, binding = 1) uniform sampler2D g_Normal;
layout(set = 0, binding = 2) uniform sampler2D g_Material;
layout(set = 0, binding = 3) uniform sampler2D g_FresnelFlags;
layout(set = 0, binding = 4) uniform sampler2D g_SheenColor;
layout(set = 0, binding = 5) uniform sampler2D g_InputImage;
layout(set = 0, binding = 6) uniform sampler2D g_Depth;
layout(set = 0, binding = 7) uniform sampler2D g_AO;  // ← NEW
```

---

## Change 4: `deferred_composite_rt.frag.glsl` — Apply AO in calcAmbientLight

### Location

Lines 118-121 (the `calcAmbientLight` call)

### Before

```glsl
vec3 ambient = calcAmbientLight(gbuf.viewDir, gbuf.normal, gbuf.F0, gbuf.albedo.rgb,
                                gbuf.metallic, gbuf.roughness,
                                irradianceMap, prefilteredEnvMap, environmentData.maxReflectionLOD,
                                brdfLUT, gbuf.ao, gbuf.sheenIntensity, gbuf.sheenColor);
```

### After

```glsl
float rtAo = texture(g_AO, vTexcoord).r;
vec3 ambient = calcAmbientLight(gbuf.viewDir, gbuf.normal, gbuf.F0, gbuf.albedo.rgb,
                                gbuf.metallic, gbuf.roughness,
                                irradianceMap, prefilteredEnvMap, environmentData.maxReflectionLOD,
                                brdfLUT, gbuf.ao * rtAo, gbuf.sheenIntensity, gbuf.sheenColor);
```

---

## How `ambientOcclussion` Is Used in `calcAmbientLight`

From `shaders/src/lib/pbr.glsl` (lines 254-257):

```glsl
vec3 base = (Kd * diffuse + specular) * ambientOcclussion;
vec3 sheen = calcSheen(normal, viewDir, sheenColor, sheenIntensity) * ambientOcclussion;

return base + sheen;
```

The `ambientOcclussion` parameter scales both the IBL specular+diffuse and the sheen contribution. By multiplying `gbuf.ao * rtAo`, the ray-traced AO darkens both ambient terms proportionally.

---

## Debug Visualization: `RTAmbientOcclusion` Case

In `DeferredLayer.cpp`, the `resolveDebugSource()` method returns the AO image when `_debugVisualization == DeferredDebugVisualization::RTAmbientOcclusion`. The debug blit pipeline (`deferred_debug_blit.frag.glsl`) will display it as a grayscale image (since it's R8, the single channel is replicated to RGB by the sampler).

No changes needed to the debug blit shader — it already handles single-channel textures correctly because Vulkan's R8_UNORM format returns `(r, 0, 0, 1)` and the shader displays `texture(tex, uv).rgb`, which shows red-channel values as grayscale if the shader outputs `vec4(sampled.rgb, 1.0)`.

**Verify**: Check `deferred_debug_blit.frag.glsl` to confirm it outputs the sampled color correctly for R8 textures.

---

## Reference: `deferred_debug_blit.frag.glsl`

Let's verify the debug shader handles R8 correctly:

```glsl
// Expected: outputs texture color as-is
outColor = texture(sourceImage, vTexcoord);
```

If the debug shader does `outColor = vec4(texture(sourceImage, vTexcoord).rgb, 1.0)`, then R8 will appear red. If it does `outColor = texture(sourceImage, vTexcoord)`, then R8 will appear red with alpha=1.

**Possible fix**: If the debug visualization shows red instead of grayscale, we may need to add a special case in the debug shader to replicate the red channel to green and blue:
```glsl
vec4 sampled = texture(sourceImage, vTexcoord);
if (sampled.g == 0.0 && sampled.b == 0.0) {
    // Single-channel texture (like AO)
    outColor = vec4(sampled.rrr, 1.0);
} else {
    outColor = sampled;
}
```

However, this can be deferred to a later refinement. The primary functionality (AO computation and composite integration) works correctly regardless of debug visualization color.
