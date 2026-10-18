# Lightmap output manual fixtures

No executable target is added. These C++ fixtures are source-only and have not
been compiled/executed by the agent. Run from an existing initialized GPU harness
using native compiled bake shaders; unavailable required formats/profiles must
throw, rather than accepting a lower-precision successful fallback.

- runBakePixelConversionProbe checks known HDR/black/row-preserving float output,
  GI byte exposure/clamp/round, AO exposure exclusion, and invalid float rejection
  before byte conversion. It needs no GPU execution.
- runBakeOutputGpuProbe(Device&,BackendType,shaderRoot) needs no RT. Two known raw
  R16 AO samples on a 9x9 atlas yield exact sums .5/1.5 and means .25/.75 on two
  adjacent charts with identical world position/normal guides. It verifies fixed
  interiors, deterministic chart tie, Euclidean radius, neutral padding, zero
  dilation, retained history and actual packed sum readback.
- runBakeGiCompositionProbe(Device&,BackendType,shaderRoot,validEnvironment) needs
  no RT, but borrows valid sky/prefilter/BRDF owners. A constant reference cube
  irradiance=(2PI,4PI,0), raw incoming=(4,2,8), and count one must produce
  multiplier=(2,.5,0), preserving HDR and the zero-reference channel. Byte export
  at -1 EV must be (255,64,0), from the identical final GPU texture.
- The updated integrated fixture in ../integrated/probe.hpp now expects the last
  completed validated sample to publish Complete/result/readPixels. Its callback
  uses Device::immediateSubmit's actual CommandBuffer* signature.

Additional backend/application checks:

1. On available Metal/Vulkan, run the above known-value cases and the integrated
   probe for AO/GI with real acquired-frame fences alongside presentation/picking.
2. Feed covered GI alpha=0, NaN/Inf/negative RGB, or any raw channel >=65504.
   Verify final validation rejects all without a successful neutral/clipped map.
   Exact 65504 is conservatively rejected because the public half-float GI pass
   saturates there. Valid black with alpha=1 must increment counts and publish zero.
3. Vary a covered texel's chart/coverage between updates, omit a covered update,
   corrupt counts, or use an empty atlas. Expect explicit failure/no result.
4. Exercise divisor zero, tiny positive divisor causing float32 overflow, negative
   or invalid environment reference, and large finite HDR multipliers. Float output
   must never clamp; RGB8 alone clamps after GI exposure.
5. Compare GPU sampled values and RGB32F/RGB8 CPU pixels at interiors, atlas edges,
   v=0 top row, padding ties and adjacent charts; ensure no world-guide bleed.
6. Record then discard/abandon a sample, including a final composition. Retrying
   uses the previous completed history, not a discarded contribution. Cancel while
   pending; confirm no commit, count advance or stale result. Retain old output
   images externally while resetting resolution or reloading source geometry.
7. Inject compatible output pass subclasses/custom shader prefixes; verify strict
   source/target/geometry metadata and full-float format validation. Incompatible
   interfaces/missing shaders/unsupported storage formats fail at initialization.
8. Measure final synchronous validation/readback separately from sample GPU work.
   Large dilation radii use bounded horizontal/vertical scans and are deliberately
   not capped silently; inspect the cost at requested resolution/radius.

Use [the output contract](../../../doc/api/bake/LightmapOutput.md). All owners live
through GPU completion, and Engine/Device outlive retained images and contexts.
