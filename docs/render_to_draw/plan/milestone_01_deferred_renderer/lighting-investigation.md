# Black geometry investigation

Status: HDR float16 overflow identified and corrected in source; runtime validation pending with the project lead. The reported Metal image shows a visible environment background and black geometry. Disabling cast shadows reportedly leaves the image unchanged. The lead clarified that the image remains black with Transparent models disabled and Final selected; the earlier opposite observation was a misunderstanding concerning albedo debug. All per-light casting and global shadow generation are disabled without fixing the image. Therefore opaque lighting, reconstruction and final processing remain candidates. No compilation or runtime verification was performed by the agent.

## Source findings

- Cast-shadow controls affect direct-light visibility. Both lighting variants still evaluate direct BRDFs, environment irradiance, prefiltered radiance and the BRDF LUT. Invalid indirect values can therefore poison the sum even with white shadow visibility.
- The shared working G-buffer is overwritten by each transparent item. Showing the last layer is intentional example behavior, not a defect. Its debug views do not independently establish opaque input correctness. The lead's Final comparison with Transparent models disabled is stronger evidence: the opaque-only final path renders without the reported black-image failure.
- Deferred lighting allowed the normalized normal/view dot product to exceed one through rounding before `pow(1 - nv, 5)`. The BRDF LUT generator similarly left its view/half dot product unbounded above. Negative bases are outside the supported domain of these floating-power expressions. Clamp both cosines to their mathematical domain in GLSL and Metal. This is a concrete numerical defect, but has not been demonstrated to explain the entire screenshot. Rebuilding environment shaders and regenerating the environment is necessary to exercise the LUT correction.
- Static inspection of frame/light/environment resource indices and layouts did not establish a binding mismatch. This is not runtime evidence that their contents are correct.

## Updated investigation scope

The clarified opaque-only observation supersedes the earlier transparency hypothesis. Last-layer G-buffer debugging remains intentional behavior. The sky being visible does not by itself validate geometry HDR: sky and lit geometry supply different values to the shared final processing. No complete root cause is established.

Final / Debug view now includes Raw HDR (no color processing). It samples reconstructed color like Final, bypasses exposure/tone mapping/gamma/brightness/contrast, and preserves the existing native output encoding convention. This is a clamped display of HDR on the normal output, not a radiometric readback. If geometry is visible here but black in Final, inspect color settings/processing. If both are black, use Lighting / Unlit albedo, Direct light only, IBL only and Nonfinite values to narrow the upstream failure. Comparing with resolution scale 1 also tests whether reduced-resolution reconstruction is required to reproduce it.

## Added diagnostics

`RendererDeferred::setInspectOpaqueLayer(true)` skips transparent G-buffer and composition passes. The final G-buffer debug sources then refer to the opaque layer; final HDR is opaque-only. This does not modify scene materials and resets upscaler history when changed.

`DeferredLayer::setDebugView(LightingDebugView)` selects final lighting, unlit albedo, direct contribution, indirect contribution, emission, or nonfinite detection. Live settings expose these under Renderer / Diagnostics and Lighting / Diagnostics. Nonfinite mode shows magenta for invalid reconstructed positions, normals or lighting and gray for valid covered pixels. Empty pixels retain the background. Metal detection inspects exponent bits to avoid relying on floating finite predicates under fast math. Diagnostic contributions bypass transparent blending; use opaque inspection to isolate the initial pass.

The logical pass image contracts keep their identities and inputs/outputs. The built-in lighting pipeline layout and paired shaders now additionally require a 16-byte fragment push constant (`uvec4`/`uint4`, x = mode), Metal buffer0. Callers reusing `deferredBindingLayout()` with the engine lighting shader must supply this value; zero restores final lighting. Independent injected implementations retain their own native shader ABI and logical image contracts.

## Lead runtime procedure

Rebuild the library and paired raster/environment shaders together, restart the example to regenerate IBL, and keep Final / Debug view at Final so lighting diagnostics reach the output. Leave bilinear reconstruction selected.

1. Enable Renderer / Inspect opaque layer. Existing Final G-buffer debug views now inspect opaque inputs. Restore Final view afterward.
2. Select Lighting / Unlit albedo. Colored geometry establishes that opaque albedo reaches the downstream HDR reconstruction/final path.
3. Compare Direct light only and IBL only, with shadows disabled. Record which contribution is black or invalid. Do not infer correctness merely because both are dark: actual light/environment values still matter.
4. Select Nonfinite values. Magenta establishes invalid data before final color processing; gray establishes only finite values, not physically correct lighting.
5. Compare normal lighting before/after disabling opaque inspection. A failure appearing only with transparency narrows investigation to those passes.

Report these observations before attributing the full failure to the numerical correction. No backend equivalence or image correctness is claimed from source checks.

## Lead contribution comparison

The lead reports correct Direct light only output, black IBL only, and black Emission only. This establishes that a useful direct contribution traverses reconstruction and final color processing with these settings. It weakens a global final-processing explanation. The images retain sky background; some unlit material faces retain albedo in IBL mode because unlit shading bypasses the ordinary IBL computation.

The shader preserves direct lighting, computes `indirect = ambient * baked`, and adds `indirect + emission`. A finite zero IBL/emission cannot erase direct light. A nonfinite or sufficiently negative indirect value could. No final multiplicative combination was found. Emission is `albedo.rgb * normalEmission.a`, where the G-buffer stores sampled material emission intensity in normal/emission alpha. Zero material emission legitimately gives black; this alone does not indicate a missing texture.

Added raw sampled irradiance, prefiltered reflection, BRDF LUT, AO and baked-multiplier views in both paired lighting variants. These expose inputs before their IBL products/sum without substituting fallback values or hiding bad data. Nonfinite mode remains available to discriminate zero from invalid output. The sampled texture native indices are irradiance9, prefiltered10 and LUT11; the environment binding is explicitly initialized at texture base8 and the fullscreen input map reserves masks12/13. Static source inspection did not find an index collision. Environment generation loops and push sizes also match statically, which does not prove generated texel correctness. No root cause or runtime fix is yet claimed.

Next runtime evidence: existing Nonfinite values view with the same failing settings; then raw IBL samples if needed. Use opaque inspection to avoid interpreting the intentional last transparent-layer coverage as an opaque result. Rebuild library and shaders together for the new sample views. No compilation or runtime execution by the agent.

## HDR overflow cause and correction

The lead confirmed magenta Nonfinite diagnostics, valid AO/baked/LUT views, and black sampled irradiance/prefiltered radiance. Transparency investigation is explicitly deferred by the lead. Direct analytical BRDF lighting does not sample the split-sum IBL LUT, so valid direct lighting alone did not establish LUT validity; the new LUT image now provides that separate evidence.

Read-only decoding of the example asset `assets/autumn_field_4k.hdr` (Radiance RGBE RLE, 4096x2048) found a maximum linear RGB component of 163840 and 31 pixels with at least one component above 65504. `EnvironmentGenerator::build(path)` previously always allocated RGBA16 float. The shared float-to-half uploader converts overflowing finite values to infinity. Thus invalid radiance was introduced before any IBL generation or deferred lighting. Sky visibility elsewhere does not exclude this: the bright source pixels occupy a tiny area, while convolution spreads their contribution. An infinite indirect contribution can also produce NaN through zero factors and contaminates the additive lighting result.

The loader now scans RGB magnitude and rejects nonfinite file data. It chooses RGBA32 float when the conservative irradiance bound `max(abs(RGB)) * PI` exceeds 65000 (headroom below half maximum); lower-range inputs retain RGBA16. A float32 input also promotes realized sky conversion, irradiance and specular generation to RGBA32, preventing an intermediate conversion back to half. Requested tool formats are restored on success and exception, and BRDF generation is unchanged. Explicit float32 image inputs receive the same intermediate promotion. Existing format-capability checks and nearest-filter fallback remain in use. No clamping or nonfinite suppression conceals the defective radiance.

Validation performed: asset data inspection, uploader/format/convolution source trace, promotion/restoration/paired diagnostics source checks, and whitespace checks. No compilation or GPU execution. Lead validation: rebuild the library, restart/rebuild the environment, then check irradiance/prefilter samples, nonfinite diagnostics, and Final with transparencies disabled. Expected outcome is finite IBL and restored combined lighting. Transparency is not part of this correction.
