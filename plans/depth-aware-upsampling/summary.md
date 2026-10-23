# Depth-aware GI upsampling — implementation plan

## Objective and scope

Add a custom joint bilateral reconstruction of reduced-resolution diffuse GI to the architecture owned by `lib/include/bg2e/draw/RendererDeferred.hpp`, with equivalent Vulkan and Metal implementations. Keep two implementation steps, each leaving declarations, definitions, and shader references complete as a buildable unit. No compilation, tests, benchmarks, diagnostic tooling, or validation tasks are part of this plan; the user will perform those activities.

This revision replaces the earlier outline. No engine implementation has been performed.

Allowed implementation files:

- `lib/include/bg2e/draw/RendererDeferred.hpp`
- `lib/src/bg2e/draw/RendererDeferred.cpp`
- New `shaders/src/draw/gi_upsampling/joint_bilateral.comp.glsl`
- New `shaders/src/draw/gi_upsampling/joint_bilateral.comp.metal`
- Progress notes in this plan directory.

These shader files are the executable part of the renderer-owned stage. Keep its C++ architecture private to `RendererDeferred`; do not create a new public pipeline API. Do not change `bg2e::render`, `bg2e::gpu`, GI tracing, temporal/denoise implementations, G-buffer producers, lighting shaders/contracts, application code, dependencies, or CMake. Preserve the full GPL header and repository source style in every implementation file.

## Findings from the existing code

| Location | Observed behavior | Consequence |
| --- | --- | --- |
| `RendererDeferred.cpp::buildTargets` | G-buffer and HDR composition use `internalExtent`; `reconstructed` uses `outputExtent`. | GI reconstruction must target **internalExtent**, where its receiver guides and lighting consumers exist. |
| `RTGlobalIllumination.cpp::outputExtent` | Low/Medium/High/Ultra scale each internal axis by 1/3, 1/2, 2/3, 1, with rounding and a minimum of 1. | Use actual source/destination dimensions, including odd and very small extents. No fixed factor-of-two assumption. |
| `shaders/src/draw/lib/rtgi.glsl` and `.metal` | GI texel `q` samples full-resolution receiver `floor((q + 0.5) * F / L)`, clamped to `F - 1`. | Fetch each candidate's guide at that exact receiver, rather than filtering depth/normals at arbitrary UVs. |
| `SurfaceInputs` and RTGI shaders | Camera-profile normals are world normals encoded in RGB, with zero RGB indicating no coverage. GI stores incoming diffuse radiance/pi in RGB and validity in alpha. | Decode normal as `normalize(encoded * 2 - 1)` and preserve binary validity; zero radiance can be valid. |
| `RendererDeferred.cpp::recordLinear` | GI flows through optional temporal accumulation and denoise before deferred composition. | Reconstruct their returned signal, including their bypass paths, without changing history resolution or keys. |
| `DenoiseFilter.cpp` and `shaders/src/draw/lib/denoise.glsl` | The built-in denoiser retains signal dimensions and associates each signal texel with the same receiver mapping. | Post-denoise joint reconstruction has a defined low-resolution guide association for built-in effects. |
| `DeferredLayer.cpp::deferredContract` | GI input is `ExtentDomain::Scaled`, including a same-size texture. | An internal-resolution image can be bound without changing the contract or its shader variants. |
| `FullscreenPass.cpp` | Lighting uses nearest sampling. | Direct scaled GI sampling explains visible block boundaries; an explicitly reconstructed texture removes that specific sampling artifact. |
| Deferred GI fragment shaders | Only `gi.a > 0.5` replaces diffuse IBL. | `(0,0,0,0)` means use existing environment lighting, not black indirect lighting. |
| Transparent receiver/composite loop | One full-domain transparent effects invocation is shared across subsequent individual transparent draws; depth writes are disabled and opaque depth is preserved. | Record reconstruction once for the shared transparent receiver, before individual draws overwrite its normals. |
| `cmake/utils.cmake` paired shader helper and `gpu::ShaderLib` | Stage files are discovered recursively; compute paths resolve to `.comp.spv` / `.comp.metallib`, with `main` / `compMain`. | New paired shader files need no build-system edit. |

The referenced `skills.md` is absent from the repository file inventory. This plan uses the actual draw implementation and `AGENTS.md` instead.

## Resolution contract and limits

Use `F = internalExtent` and `L = actual selected GI signal extent`. The graph becomes:

`G-buffer(F) -> RTGI(L) -> temporal(L) -> denoise(L) -> joint bilateral GI(F) -> deferred HDR(F) -> existing scene upscaler(outputExtent) -> final conversion`

When internal and output extents match, GI reconstruction reaches final framebuffer resolution. If the renderer independently scales the entire scene, GI reconstruction stops at the G-buffer/lighting resolution and the existing scene upscaler remains responsible for final output. Reconstructing GI directly at `outputExtent` would lack matching full-resolution geometry guides and would require a broader pipeline redesign.

Depth and normals cannot reconstruct surfaces with no valid GI sample in the search neighborhood, distinguish all coplanar disconnected geometry, or undo contamination already introduced by upstream temporal/denoise processing. Reject incompatible support and retain IBL through alpha zero rather than importing an unrelated bright sample. The pass reduces reconstruction-induced leaking; it cannot guarantee its elimination throughout the existing pipeline.

Transparent GI currently combines transparent normals with **opaque** depth, even when transparent temporal metadata is enabled. Preserve that existing input contract, explicitly acknowledge that it is not true transparent receiver geometry, and do not substitute temporal metadata depth only in the new pass: it would no longer describe the surfaces used by the GI producer. Correct transparent receiver depth requires a separate, wider change.

## Design decisions

- Implement a private renderer-owned compute stage using existing `gpu::Device`, `ShaderLib`, pipeline layouts, resource sets, and command APIs. Use compute on both backends; Vulkan RTGI's ray-tracing dispatch does not imply ray tracing for this pass.
- Keep a private opaque state type declared in `RendererDeferred.hpp` and defined in `.cpp`, held by `shared_ptr` to avoid introducing incomplete-type destructor requirements into this header. Keep output image ownership in each `Targets` slot.
- Use one RGBA16F Sampled | Storage destination per frame slot when `L != F`. Opaque lighting consumes it before the transparent invocation overwrites it. Separate per-invocation bindings/uniforms remain alive through GPU completion.
- Bypass reconstruction for equal extents, inactive/unavailable GI, and unsupported custom producer/receiver contracts. Restrict automatic activation to exact built-in GBuffer/RTGI implementations with the documented camera-profile mapping. Also bypass if injected temporal/denoise effects return a signal with a different extent from the producer. Do not add a public compatibility API in this task.
- Preserve binary alpha: accepted reconstruction writes alpha 1; uncovered/unsupported receivers write alpha 0. Never weight RGB by a fabricated confidence or apply albedo, material occlusion, exposure, or gamma here.
- Fix the kernel, ABI, lifetime, and fallback rules in [Step 1](step-1-resources.md), then wire lifecycle and graph ordering in [Step 2](step-2-integration.md).

## Implementation steps

1. [Implement the private compute stage and paired shaders](step-1-resources.md). Provide all state definitions and helper definitions together, but leave the stage disconnected from rendering. This is the first complete compilation unit boundary.
2. [Integrate resources, lifecycle, and GI composition](step-2-integration.md). Activate the stage only for compatible reduced-resolution GI and connect its output in both opaque and shared transparent effect paths. This completes the feature.

Do not claim either step was compiled unless the user separately authorizes compilation. “Buildable unit” describes the required completeness of each change, not a command to execute.

## Resume protocol

Read this summary, `progress.md`, and the first incomplete step before implementation. `progress.md` lives **in this directory**, alongside the step documents (not in their parent). Record the current step before editing. At a session boundary, record partial work and the next concrete action. After finishing all source work in a step, mark it complete, update `last_completed_step`, and describe decisions/deviations and files changed before starting the next step. Never mark partial work complete or treat an unchecked implementation step as done because its plan exists. Keep verification status explicitly `not_run_user_owned`.
