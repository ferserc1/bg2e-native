# Phase 01 Alpha classification

## Outcome

Implementation status: implemented, statically checked; native entry validation and runtime acceptance pending. No builds, shader compilation, or application execution were requested or performed.

Materials with an authored cutoff below `0.02` stop using alpha test in rasterization and tracing. Their admitted RT instances use opaque intersection behavior. Raster transparency remains independent.

## Entry and implementation

First complete the existing shadow v3 and indexed-UV validation described in the [summary](summary.md). Fix any regression before using that implementation as the baseline.

1. Define `AlphaTestCutoffThreshold = 0.02f` in the draw shader-data contract, with a comment explaining tolerance for designer sliders close to zero. Centralize classification in a helper; compare using `>=`. Preserve finite/range validation before classification.
2. Keep authored cutoff unchanged. Pack an effective cutoff of zero and clear `MaterialAlphaTest` below the threshold. Above it, preserve the authored cutoff and set the flag. Use the same result for shadow metadata and TLAS opacity.
3. Update existing draw raster, query, and RT shader helpers on both backends to consume that effective cutoff and flag consistently. Disabled alpha test must return acceptance before fetching triangle UVs or sampling an alpha texture. Audit all variants and standalone preparation paths. Avoid shader copies of the numeric threshold: the CPU-packed classification is the shader contract.
4. Change the shared `base::MaterialAttributes` default to zero. Audit construction, clones, omitted serialized values, and supported loaders for assumptions about the previous default. This requested shared-default change must not become a refactor of other rendering paths.
5. Set glTF `MASK` cutoff to exactly `0.5`; set `OPAQUE` and `BLEND` cutoff to zero. Preserve blending flags. Document the deliberate override of an authored glTF mask cutoff.
6. Retain instance-level `ForceOpaque`/`ForceNonOpaque` selection. The current BLAS defaults already permit this path; do not rebuild geometry solely because cutoff changes. Preserve default transparent exclusion from tracing and verify explicitly admitted transparent instances separately.

## Verification and exit

- Check zero, small positive values, just below the constant, equality, and just above it. Reject invalid/nonfinite cutoffs as before. Temporarily changing the constant must change all effective classifications consistently.
- Verify textured alpha holes, scalar alpha, UV0/UV1, texture overflow, and transitions across the threshold. Below it, even zero-alpha texels must cease to be discarded by alpha test; blending still follows its separate rules.
- Check all combinations of raster transparency and effective alpha test. Explicitly admitted transparent objects with disabled alpha test must use opaque RT instances.
- Check glTF modes with missing, default, and custom file cutoffs; default-created materials; clones; and round-trip object independence.
- Compare opaque-instance and alpha-tested-instance counts. Demonstrate skipped alpha filtering for near-zero cutoffs without changing geometry build counts.

Exit with a working renderer and matching CPU/shader interfaces. Native validation requires an explicitly requested build and execution; otherwise record it as pending.

## Implementation record

- `draw/ShaderData.hpp` defines `AlphaTestCutoffThreshold`. `effectiveAlphaTestCutoff` rejects nonfinite and out-of-range authored values before classification with `>=`. Packing uses its result for both `emissionAlpha.y` and `MaterialAlphaTest`; the source attributes are unchanged.
- The shared material default is zero. glTF initializes cutoff to zero and deliberately overrides every MASK cutoff to exactly `0.5`, including custom file values. BLEND flags are preserved. The shared loader applies this policy to both draw and legacy render imports; no legacy renderer implementation was changed.
- Existing GLSL and Metal G-buffer shaders already gate discard with the packed flag and compare against the packed cutoff. Shared tracing helpers already accept disabled alpha test before fetching triangle UVs or sampling textures; AO, GI, reflection, and probe variants consume those helpers. Soft-shadow shaders now explicitly return acceptance on the disabled flag before texture fallback, index/UV fetches, or sampling. Overflow still uses scalar alpha for enabled testing.
- Shadow metadata copies the packed cutoff and flags. Shared tracing snapshots retain instance-level opacity selection; the standalone collection/population path now also selects ForceOpaque/ForceNonOpaque from the packed flag. Transparency remains a separate collection option and is excluded by default. Geometry construction and ownership are unchanged.
- Audited construction and copies: the base constructor uses member defaults; draw clones copy source attributes before cloning texture settings. JSON material serialization writes authored cutoff and only replaces it when present on input; newly constructed materials with omitted cutoff inherit zero. BG2 material loading uses this serializer. OBJ loading explicitly sets zero. Material modifiers only replace cutoff when supplied. No editable material merging or scene material library was introduced.
- Added `tracing.opaqueInstances`, `tracing.alphaTestInstances`, and `shadowTable.opaqueInstances`; existing shadow alpha-test/texture lookup counters remain available. These are instrumentation, not measured evidence of speedup.

Static checks performed: `git diff --check`; source contract assertions for threshold centralization, finite validation before classification, effective packing, shared defaults, glTF override, and GLSL/Metal early returns. Source audit covered raster/query/RT helpers, shared and standalone preparation, transparent admission, clones, and supported material loaders. No native correctness or timing result is claimed.

## Focused native verification cases (pending)

Use `examples/draw/10_deferred_renderer` and its textured cutout canopy on Metal and Vulkan. Run the shadow v3/indexed-UV entry matrix from the summary first and save reference captures. Backend, resolution, scene/table counts, active effects, and capture conditions must accompany results.

| Case | Expected result |
|---|---|
| Authored cutoff `0`, `0.001`, and `std::nextafter(AlphaTestCutoffThreshold, 0.0f)` | Authored value preserved; packed cutoff zero; flag clear; admitted RT instance opaque; even zero-alpha texels survive alpha test |
| Cutoff equal to the threshold and `std::nextafter(AlphaTestCutoffThreshold, 1.0f)` | Authored cutoff packed unchanged; flag set; admitted RT instance nonopaque; alpha holes filtered |
| Negative, greater than one, NaN, and either infinity | `effectiveAlphaTestCutoff` and packing reject the value |
| Temporarily change the named threshold | CPU packing, raster, shadow metadata, shared tracing and standalone opacity all follow it without shader numeric changes |
| Each valid cutoff with raster transparency disabled/enabled, including solid transparency | Composition follows transparency; default tracing excludes transparent objects; explicit admission follows alpha-test classification independently |
| Alpha texture, scalar alpha, UV0/UV1 and scaling, capacity overflow | Enabled tests preserve previous filtering and fallback; disabled tests omit shadow alpha texture admission and filtering |
| Repeated edits crossing the threshold, with shadows/AO/GI/reflections and standalone shadows | Opaque/alpha-test counters change consistently; geometry build counts do not increase solely from cutoff changes; no stale resources |
| glTF OPAQUE/BLEND/MASK with missing/default/custom cutoff | Zero for OPAQUE/BLEND, exactly `0.5` for MASK; blending flags unchanged |
| Default material, omitted JSON cutoff, explicit BG2/JSON cutoff, clone and export/import into another scene | Defaults zero; explicit authored values survive round trips; edits remain independent, including after source-scene destruction |

Still pending: native library and shader checks on both backends; the full entry matrix (nonzero geometry bases, submeshes, barycentrics, UV selection/scaling, holes, overflow, empty scenes, standalone shadows, slot reuse, aborted recording); visual comparisons; settings/resize/scene replacement/cleanup; independent object round trips; counter captures and exclusive CPU/GPU timing evidence. Do not begin phase 02 acceptance from an unvalidated baseline.
