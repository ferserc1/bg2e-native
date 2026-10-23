# Step 1 — Implement the private compute stage and paired shaders

## Outcome and files

Provide a complete private implementation that can be called by Step 2 but is not yet connected to the rendering graph. This step includes `RendererDeferred.hpp`, `RendererDeferred.cpp`, and both new `gi_upsampling/joint_bilateral.comp` shader files. All referenced types and functions must have definitions; shader entry points and their CPU ABI must agree. Do not initialize the pass or allocate unused per-slot output images in the active renderer yet.

Declare a private forward-declared state type and a `shared_ptr` owner in the header. Define the state in `.cpp`, with pipeline/layout/shader/sampler owners and per-slot invocation retention. Provide private initialization, acquired-slot retirement, recording, and teardown helpers as needed, keeping implementation details in the state. A recording helper receives the selected GI signal, normal, depth, inverse jittered view-projection, world-to-view, output, frame-slot, and serial/layer identity. Return the selected image or an explicit failure/bypass outcome. Avoid adding public settings or an independent public pass class.

## Fixed GPU interface

Use one compute layout, set 0. `gpu::ShaderBinding` is `{Vulkan binding, Metal native index}`:

| Resource | Binding | Metal declaration |
| --- | --- | --- |
| Uniform block | `{0,1}` | `constant ... & [[buffer(1)]]` |
| Low-resolution GI | `{1,0}` | `texture2d<float> [[texture(0)]]` |
| Encoded full-resolution normals | `{2,1}` | `texture2d<float> [[texture(1)]]` |
| Full-resolution depth | `{3,2}` | `depth2d<float> [[texture(2)]]` |
| Nearest clamp sampler | `{4,0}` | `sampler [[sampler(0)]]` if declared |
| Full-resolution GI output | `{5,3}` | `texture2d<float, access::write> [[texture(3)]]` |

GLSL uses separate texture/sampler declarations with integer `texelFetch`; Metal uses integer `.read`. No bilinear filtering of radiance or geometry before guide rejection. Declare an `rgba16f` GLSL storage image. Use a 16x16 workgroup and guard destination bounds; Metal entry point is `compMain`, GLSL entry point is `main`. Load `draw/gi_upsampling/joint_bilateral` with `ShaderLib::compute`.

Use an `alignas(16)` uniform structure with two `mat4` values (inverse jittered view-projection and world-to-view), `uvec4 extent = {F.width,F.height,L.width,L.height}`, and two `vec4` parameter groups. Make layout explicit with `sizeof`/`offsetof` assertions: 176 bytes total, extent at 128, parameter groups at 144 and 160. Parameter groups contain absolute depth tolerance, relative depth tolerance, minimum normal cosine, spatial sigma; then depth sigma as a fraction of tolerance, normal sigma, weight epsilon, reserved. Mirror field order in MSL and GLSL.

## Geometry association

For destination pixel `p`, read its own guide at integer `p`. For low-resolution candidate `q`, read GI at `q` and fetch its associated guide at:

`h(q) = min(floor((q + 0.5) / L * F), F - 1)`

Use the same floating-point expression/order as the current RTGI shaders; do not infer a different half-resolution downsample, average guides, or fetch normal/depth at a fractional UV. Reconstruct each point using its guide pixel center:

`uv = (h + 0.5) / F`

`H = inverseJitteredViewProjection * vec4(2*uv.x - 1, 1 - 2*uv.y, depth, 1)`

`P = H.xyz / H.w; z = (worldToView * vec4(P,1)).z`

Use the same operation with `h = p` for the destination. This matches the existing top-left UV and zero-to-one clip-depth conventions on both backends; no GL-style depth remap or additional Metal Y flip. Do not use the scene upscaler's near/far parameters to linearize depth, since explicit and orthographic camera matrices are supported.

A guide is eligible only if encoded RGB is finite and has squared length > 1e-8, depth is finite and in [0,1), abs(H.w) > 1e-8, reconstructed position/view depth are finite, and the decoded signed normal has nonzero finite length. Decode `normalize(encoded*2-1)` only after checking its length. Alpha of normal/emission is emission, not coverage. A GI sample requires finite RGBA and alpha > 0.5; a valid black RGB sample remains eligible.

## Joint bilateral kernel

For each covered destination, compute source coordinate `x = (p + 0.5) / F * L - 0.5` and visit the 4x4 candidates `floor(x) + (i,j)` with each offset in [-1,2]. Skip out-of-bounds candidates rather than clamping their GI coordinates and counting duplicates. Use all actual dimensions, including 1-pixel axes and rounded quality extents.

For each valid candidate, define:

- `delta = (vec2(h(q)) - vec2(p)) * vec2(L) / vec2(F)` (distance in source-texel units).
- `dz = abs(zq - zp)` and `c = clamp(dot(Nq,Np), -1, 1)`.
- `t = max(absDepthTolerance, relativeDepthTolerance * max(abs(zp),abs(zq)))`.
- Reject if `dz > t` or `c < minimumNormalCosine`.
- `w = exp(-0.5 * (dot(delta,delta)/spatialSigma^2 + (dz/(depthSigmaFraction*t))^2 + ((1-c)/normalSigma)^2))`.

Use private defaults: absolute tolerance 0.01 world units, relative tolerance 0.01, minimum cosine 0.9, spatial sigma 1.0 source texel, depth sigma fraction 0.5, normal sigma 0.1, weight epsilon 1e-8. Treat these as documented initial quality choices, not universal scene-scale guarantees. Do not couple them to denoiser enablement/settings or ray-origin bias.

Accumulate nonnegative linear RGB and normalize by the finite weight sum. With adequate support, write finite RGB clamped to RGBA16F's maximum 65504 and alpha 1. If the sum is too small, choose the spatially nearest **already geometrically eligible** candidate from this same neighborhood (stable iteration order breaks ties); otherwise write `(0,0,0,0)`. Never relax the depth/normal gate to find a bright fallback. Uncovered destinations always write zero. Every dispatched destination is written, so previous-frame and previous-layer contents cannot survive in holes.

## Recording and ownership contract

Use a fresh uniform buffer/resource-set lease per invocation, following existing `TemporaryResourcePool` semantics, or equivalently retained immutable per-invocation objects. Keep every lease and referenced signal/output owner alive until the slot's GPU work is retired. Use separate invocation identities for opaque layer 0 and `TransparentTemporalLayer`; do not update one descriptor set or uniform buffer twice before submission.

Transition sampled inputs to ShaderReadOnly, output to General, beginCompute, bind, dispatch ceil(F/16), endCompute, then transition output to ShaderReadOnly. All transitions occur outside active rendering/compute scopes. Existing transitions must cover prior opaque fragment reads before a transparent overwrite and compute writes before lighting reads. No CPU waits inside record and no raw Vulkan/Metal commands.

## Handoff

Update `plans/depth-aware-upsampling/progress.md`: complete Step 1, set `last_completed_step: 1`, list implemented helpers and shader paths, and describe any deviation from this exact ABI/kernel. Rendering remains disconnected until Step 2. Leave verification status `not_run_user_owned`.
