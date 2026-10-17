# M03-08 implementation handoff

[Step](step_08_denoise.md) · [Public API](../../../../doc/api/draw/DenoiseFilter.md) · [Manual validation](../../../../tests/draw/denoise/README.md)

## Public surface, signal and ownership

DenoiseFilter, DenoiseConfiguration/Settings/Input/Result and DenoiseSignal/Key are implemented under draw::pipeline. They consume the existing SurfaceInputs camera or explicit-world profile rather than a GBuffer class. Input signal is sampled R16 visibility or RGBA16 linear diffuse/reflected radiance at an independent nonzero extent. Reflection material/threshold specifies receiver eligibility. Camera worldToView is finite, invertible and affine; explicit surfaces need no camera. Input images are borrowed through completion; result owns a new same-format/same-size sampled/storage/readback output or borrows raw input on no-op/fallback.

Filtering is stateless across frames. Each retired acquired slot retains per-invocation output, uniforms and descriptors, keyed separately by signal/view/layer. Duplicate active key/serial calls and budget overflow are diagnosed. maxInvocations defaults to 256 per slot. Fresh outputs prevent an earlier effect/layer or a retained caller result from being overwritten; cleanup drains before dropping slot owners. Device must outlive external output owners. This is separate scratch from temporal history and bake accumulators. No per-frame waitIdle, frame rings of source assets or native Vulkan descriptors are exposed.

Shader root/prefix/interface and slot budget are configurable; lifecycle/beginFrame/record/capability methods and control getters/setters are virtual for compatible injected components. settingsType() describes actual virtual accessors, not copied settings fields; settingsInstance() supplies the matching base address. A custom metadata override must also supply the corresponding instance address. SettingsCatalog can enumerate these directly and retain the shared component. Built-in UI/preferences target binding is M03-09 work.

## Algorithm and validity

The normalized bilateral kernel has radius 0..8, hard depth/world-distance threshold, normal cosine threshold, spatial Gaussian sigma in signal pixels, distance sigma in world units and normal sigma on 1-cosine. Sigmas are constrained to [1e-6,1e6] to keep their denominators representable. Camera geometry distance is absolute linear view Z difference, reconstructed from jittered depth and transformed by worldToView. Explicit geometry distance is Euclidean world separation, useful for rejecting unrelated adjacent UV chart points. M04 must still define chart isolation/coverage policy when geometrically nearby chart samples should remain separate.

Nearest pixel-center mapping relates signal coordinates to surfaces. Out-of-bounds neighbors are skipped rather than duplicated at borders. Weights are normalized; the eligible center contributes, with a safe source fallback if custom profiles yield no weight. Constant finite nonnegative HDR values remain constant, including at odd extents and borders; output is not LDR-clamped. Normal and position inputs must be finite/covered, and normal magnitude valid after normalization. Camera linear depth overflow is rejected.

Visibility averages scalar [0,1], uncovered=one. Diffuse GI rejects invalid centers/neighbors through alpha validity, keeps black valid, and emits alpha one for filtered valid centers. Reflection eligible misses participate in the spatial confidence mean; RGB is weighted by confidence and returned unpremultiplied. Zero accumulated confidence returns zero RGB. Roughness-rejected centers and invalid coverage stay zero-confidence, even if neighboring hits are valid. Nonfinite signal data is rejected; finite HDR is bounded only to representable half range. Black RGB never substitutes for explicit validity/confidence.

## Native shaders and ABI

Four profiles (camera/world x scalar/HDR) have explicit GLSL and Metal compute wrappers. Each is selected independently from actual R16/RGBA16 sampled/storage/readback support and shader creation. Missing profiles leave diagnostic status and return the original signal rather than allocating a wrong-format output. No RT/descriptor-array capability is required and no GPU primitives or CMake changes are added.

Set0 map: UBO binding0/native buffer1; normal1/texture0; depth-position2/texture1; coverage3/texture2; signal4/texture3; material5/texture4; nearest sampler6/native0; output7/texture5. Camera Metal uses depth2d, explicit Metal uses texture2d. Scalar storage is r16f, HDR rgba16f. DenoiseUniforms is 192 bytes: inverseVP0, worldToView64, thresholds128 (distance threshold, normal cosine, depth sigma, normal sigma), parameters144 (spatial sigma,maxRoughness), source/signal extents160, kind/radius176. Dispatch is ceil(signal/16), with bounds checks. Sources transition to ShaderReadOnly, fresh output to General and then ShaderReadOnly. No sampled/storage aliases are required.

## Renderer ordering and settings

RendererConfiguration appends shared denoise injection, null selecting enabled radius-two defaults. RendererDeferred exposes denoise(), begins the acquired filter slot once, and resolves each active effect/layer as tracing -> temporal -> denoise -> lighting. Temporal receives only the unfiltered effect and is not overwritten by filtered output. A temporal pass-through still supports denoise; a denoise pass-through preserves accumulated data. Source geometry remains the current layer's surface, including transparent layers. Reflection quality remains display-relative; filter output simply follows its signal extent.

Renderer signature uses virtual denoise getter values, so edits and custom filter controls coordinate the existing history/SDK reset path before jitter preparation. Filter outputs need no persistent resize/reset because each invocation allocates from its current signal dimensions. Existing material/texture mutation hooks and surface lifetimes remain unchanged. Production render and its private DenoiseFilter remain untouched.

## Validation and downstream work

Manual fixture checks actual virtual accessor metadata read/write, injected record dispatch, scalar noise reduction, constant HDR energy, odd 5x3 extents/borders, coverage, explicit distance and normal discontinuity rejection, confidence-weighted unpremultiplied reflection RGB, invalid GI holes, roughness rejection, camera-depth profiles, a 3x1 HDR signal over 5x3 surfaces, retained earlier outputs and radius-zero/disabled borrowing. The fixture is not executed by the agent.

Lead must compile engine/native shaders and run Vulkan/Metal diagnostics, unsupported/partial profile fallback, extreme sigma/radius settings, asynchronous slot retention, custom resource/shader replacement and existing deferred/manipulation examples. The source checks validate contracts, maps, includes/preprocessor/delimiters/licenses and git diff --check only. No build, shader compiler, fixture, renderer or packaging execution.

M03-09 binds real metadata controls/preferences and verifies existing examples; M03-10 reviews effect ordering/performance. M04 decides whether its UV surface/chart policy permits this filter; exact bake accumulation stays separate and must not be silently replaced by bilateral output. No UI migration, new example, image pool, chart identifier or bake orchestration is delivered here.
