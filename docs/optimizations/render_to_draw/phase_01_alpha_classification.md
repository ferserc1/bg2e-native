# Phase 01 Alpha classification

## Outcome

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
