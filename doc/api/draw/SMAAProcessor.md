# SMAAProcessor and FinalProcessor

Headers: `bg2e/draw/pipeline/SMAAProcessor.hpp`,
`bg2e/draw/pipeline/FinalProcessor.hpp`. Namespace: `bg2e::draw::pipeline`.
These are optional draw API components; production render remains separate.

Inject a shared SMAAProcessor into RendererConfiguration::finalProcessor before
initializing RendererDeferred. It starts disabled; setEnabled(true) or use the
shared settings target `finalProcessor`. Both existing advanced examples already
inject one. Null keeps the ordinary final pass. FSR is a separate optional HDR
reconstruction component; SMAA also operates with default resampling and no RT.

The renderer initializes the component with Engine, output format and acquired
slot count, records after final color and before overlays/UI, then cleans it up at
a stopped boundary. Output format changes recreate the renderer lifecycle. Size
changes need no component rebuild: each call allocates scratch for its actual
input extent. Cleanup drains the device. The caller keeps Engine/Device alive.

For direct use initialize with Device, BackendType and slot count, then call
process(SMAAInput{command, slot, serial, color}). The command must be outside
rendering/compute scopes. Caller owns submission, input lifetime and completion;
reuse only a retired slot and process once per slot/serial. A disabled/unavailable
result borrows source with no owner. Active output is a fresh sampled/storage
RGBA16 float image; retain its owner through use. Later slot reuse cannot mutate
retained outputs. This interface needs no Engine scene graph or private targets.

Controls are enabled, threshold [0.001,1] (default 0.05), searchSteps [1,32]
(default 16), plus read-only available/status. Virtual controls and matching
settingsType/settingsInstance support replacement components and live metadata.
Compatible custom FinalProcessor implementations own their metadata and matching
instance address, initialize/cleanup at stopped boundaries, retain resources
through completion, do not submit, and leave scopes closed and final color usable.

The shipped profile ports the existing orthogonal SMAA 1x kernels: luma edges with
local contrast adaptation, official area/search lookup weights and neighborhood
blend. It does not implement diagonal/corner detection or temporal supersampling.
Alpha is opaque one. Input is filterable normalized/float single-sample sampled
color 2D. Renderer integration operates on final display color, not tracing HDR.
For sRGB targets sampling decodes and final attachment encoding restores transfer;
UNORM display values retain their existing final-pass interpretation.

Native resources under `shaders/src/draw/smaa` are paired edge/weights/blend
compute and copy fragment kernels. Compute bindings are set0: UBO0 (Metal buffer1),
sampled images1..3 (Metal textures0..2), linear clamp sampler4 (Metal sampler0),
output image5 (Metal texture3); metrics/parameters UBO is 32 bytes. Dispatch is
16x16 with odd-extent bounds checks. Storage edges RG8, weights RGBA8, output
RGBA16 require filterable sampled/storage/transfer-source support. Area lookup is
160x560 RG8, search lookup 64x16 R8, with sampled/transfer-destination support and
linear filtering. Official bytes remain in `lib/third_party/iryku_smaa` under their
original license. Missing formats/native kernels report unavailable and preserve
input. No RT, indexed material arrays or FidelityFX capability gate is used.

[Integration contracts](../../../docs/render_to_draw/plan/milestone_03_ray_tracing/integration.md)
· [Manual validation](../../../tests/draw/advanced_integration/README.md).
The implementation is accepted by project lead continuation; exact backend/scenario coverage is unspecified. No documentation-agent compilation/runtime execution is claimed.
