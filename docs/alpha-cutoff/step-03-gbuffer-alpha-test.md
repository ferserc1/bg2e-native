# Step 03 — Alpha test in the G-buffer pass (rasterization)

## Goal

Resolve the alpha test during the G-buffer fill: discard fragments whose final
albedo alpha is below `alphaCutoff`, pack the new material flag bits into the
fresnel+flags attachment, and force albedo alpha to 1.0 for opaque cutout
materials so the composite blends them fully.

No G-buffer channel is consumed. The sheen attachment alpha already carries
`refractionFactor` (`deferred_gbuffer.frag.glsl:73`) and the normal attachment
alpha carries light emission — both stay untouched.

## Files

| File | Action |
|---|---|
| `shaders/src/glsl/deferred_gbuffer.frag.glsl` | Modify |
| `shaders/src/glsl/lib/deferred_utils.glsl` | Modify |

Depends on: step 02 (uniform fields and flag constants).

## 3.1 `deferred_gbuffer.frag.glsl`

Current relevant code (lines 49-74):

```glsl
void main() {
    PBRMaterialData mat = objectData.material;

    // Albedo (sRGB to linear)
    vec4 albedo = sampleAlbedo(albedoTex, inUV0, inUV1, mat, 2.2);
    g_Albedo = albedo;
    ...
    // Fresnel color + flags
    float unlitFlag = (mat.unlit & MATERIAL_FLAG_UNLIT) != 0u ? 1.0 : 0.0;
    g_FresnelColorFlags = vec4(mat.fresnelTint.rgb, unlitFlag);
    ...
}
```

New version:

```glsl
void main() {
    PBRMaterialData mat = objectData.material;

    // Albedo (sRGB to linear). Note: sampleAlbedo applies the sRGB->linear
    // conversion to RGB only; the alpha channel stays linear, so it can be
    // compared directly against alphaCutoff.
    vec4 albedo = sampleAlbedo(albedoTex, inUV0, inUV1, mat, 2.2);

    bool alphaTest   = (mat.unlit & MATERIAL_FLAG_ALPHA_TEST)  != 0u;
    bool transparent = (mat.unlit & MATERIAL_FLAG_TRANSPARENT) != 0u;

    // Alpha test (cutout): applies to opaque and transparent materials.
    // Discarded fragments write nothing — neither color nor depth — so the
    // composite and all downstream passes need no special handling.
    if (alphaTest && albedo.a < mat.alphaCutoff)
    {
        discard;
    }

    // Opaque cutout: force alpha to 1.0 so the composite's
    // mix(inputColor, outColor, albedoAlpha) blends fully.
    // Transparent cutout: keep the sampled alpha for blending.
    g_Albedo = vec4(albedo.rgb, (alphaTest && !transparent) ? 1.0 : albedo.a);

    // Normal (world space, mapped to 0-1)
    vec3 normal = sampleNormal(normalTex, inUV0, inUV1, mat, inTBN);
    float emission = sampleLightEmission(lightEmissionTex, inUV0, inUV1, mat);
    g_Normal = vec4(normal * 0.5 + 0.5, emission);

    // Material properties
    float metallic = sampleMetallic(metallicTex, inUV0, inUV1, mat);
    float roughness = sampleRoughness(roughnessTex, inUV0, inUV1, mat);
    float ao = sampleAmbientOcclussion(aoTex, inUV0, inUV1, mat);
    float sheen = mat.sheenIntensity;
    g_Material = vec4(metallic, roughness, ao, sheen);

    // Fresnel color + flags bitfield (bit 0 = unlit, bit 1 = alphaTest,
    // bit 2 = transparent). Stored as value/255 in an R8G8B8A8_UNORM channel;
    // decoded with uint(round(a * 255.0)).
    uint flags = mat.unlit & (MATERIAL_FLAG_UNLIT |
                              MATERIAL_FLAG_ALPHA_TEST |
                              MATERIAL_FLAG_TRANSPARENT);
    g_FresnelColorFlags = vec4(mat.fresnelTint.rgb, float(flags) / 255.0);

    // Sheen color (RGB), refraction factor packed in the reserved alpha channel
    g_SheenColor = vec4(mat.sheenColor.rgb, mat.refractionFactor);
}
```

Key points:

- This shader runs for **both** deferred layers (opaque and transparent), so
  the discard automatically applies to transparent materials as agreed.
- `sampleAlbedo()` multiplies the texture by `mat.albedo` (the material's base
  color factor), so the test matches glTF semantics:
  `alpha = baseColorFactor.a * texture.a`.
- Empty-pixel detection (`albedo.a == 0` in the composite) is unaffected:
  discarded fragments leave the cleared `{0,0,0,0}` value.

## 3.2 `deferred_utils.glsl`

Extend `DeferredGBufferData` and `setupDeferredGBuffer()` to decode the new
bits (available for future consumers; the composite does not need them today):

```glsl
struct DeferredGBufferData {
    ...
    bool unlit;
    bool alphaTest;
    bool transparent;
    float refractionFactor;
    float lightEmission;
};
```

In `setupDeferredGBuffer()` (around line 86):

```glsl
    uint materialFlags = uint(round(fresnelFlags.a * 255.0));
    gbuf.unlit       = (materialFlags & MATERIAL_FLAG_UNLIT)       != 0u;
    gbuf.alphaTest   = (materialFlags & MATERIAL_FLAG_ALPHA_TEST)  != 0u;
    gbuf.transparent = (materialFlags & MATERIAL_FLAG_TRANSPARENT) != 0u;
```

## Behavior notes

- **Default cutoff 0.5**: any existing material (opaque or BLEND) whose final
  alpha drops below 0.5 will now discard. This is the requested semantics.
- A cutoff of exactly 0.0 disables the test (the `ALPHA_TEST` flag is not set
  in `PBRMaterialData::operator=`, step 02).
- Transparent queue ordering and blending are unchanged; only sub-cutoff
  fragments disappear.
- Motion vectors: discarded fragments leave depth cleared (1.0 in the opaque
  layer), so they are treated as sky — correct, since the surface is not
  visible.

## Verification

- Plane with an alpha-textured material (e.g. foliage) at cutoff 0.5: holes
  must show the background through, with correct depth (objects behind are
  visible and correctly occluded).
- `GBufferFresnelFlags` debug view with channel mode 4 (alpha as gray) shows
  the flag values (1/255 ≈ dark for unlit, 6/255 for transparent alpha-test).
- Opaque cutout pixels must composite fully (no halo from the previous layer);
  transparent cutout pixels must still blend.
