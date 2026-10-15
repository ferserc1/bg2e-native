# M01-10 — Basic deferred renderer and injectable public layers

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01-02/M01-04/M01-05/M01-08/M01-09.

## Scope and deliverable

- `lib/src/bg2e/render/gbuffer/GBufferManager.cpp`
- `lib/src/bg2e/render/deferred/DeferredLayer.cpp`
- `lib/include/bg2e/draw/RendererDeferred.hpp`
- `lib/include/bg2e/draw/pipeline/GBuffer.hpp`
- `lib/include/bg2e/draw/pipeline/DeferredLayer.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/render/gbuffer/GBufferManager.cpp](../../../../lib/src/bg2e/render/gbuffer/GBufferManager.cpp), [lib/src/bg2e/render/deferred/DeferredLayer.cpp](../../../../lib/src/bg2e/render/deferred/DeferredLayer.cpp).

## Implementation work

1. Implement public GBuffer/pass/layer configuration with the current six color channels plus depth; use neutral white baked multipliers without importing bake functionality.
2. Record skybox, opaque and transparent/solid-transparent deferred layers with explicit inputs/outputs, copied/preserved opaque depth, correct alpha test, PBR/light/emission/sheen/Fresnel/refraction and color correction.
3. RendererDeferred owns the ordered pipeline and output/internal extents. Default components are replaceable through configuration; bake will later reuse compatible public building blocks.
4. Expose only implemented debug views/settings. Do not create AO/GI/reflection/history/denoise objects as inactive placeholders. No MSAA or forward workaround for transparency.
5. Expose reusable generic render-target/fullscreen/compute pass helpers where required by the pipeline, covering the old ColorAttachments/Canvas/GPUProcess/GPUTextureGenerator responsibilities. They remain public graphics building blocks, with explicit shader bindings and no private renderer-only implementation family.

## Verification and exit conditions

- Reference-like opaque/transparent scene renders with matching material channels and unlit/refraction behavior on both backends.
- An application-provided compatible subclass/shader pass can replace a default component; incompatible extent/format/input selection is rejected clearly.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M01-11 adds query lighting; M03 adds advanced effects to these contracts and M04 adds actual baked multipliers without rebuilding the architecture.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-04 shader deployment handoff

Use the paired source/output paths and actual ShaderLib entry points documented in [shader-libraries.md](shader-libraries.md). The registered recursive draw shader tree already handles shared includes, native compilation and app/install/standalone deployment. Add concrete pipeline stages there; do not flatten outputs or treat the `probe` libraries as the final renderer implementation. Keep Vulkan `main`, Metal stage-specific entry points and explicit backend resource indices.

## M01-06 resource handoff

M01-06 supplies the six material texture channels, neutral defaults, live MaterialAttributes, UV/channel/invert/scales and color-space metadata. SDR allocations are UNORM; implement explicit intended color decoding in native shaders, honoring per-texture colorType and avoiding double conversion of linear inputs. Use the standard two-UV/tangent draw mesh and M01-09 bindings. See [drawable-resources.md](drawable-resources.md).

## M01-08 environment integration

Use public EnvironmentGenerator/SkyboxRenderer and validated shared EnvironmentResources from [environment-tools.md](environment-tools.md). Generate/default/synchronize HDR environments during attachScene or an explicit stopped-producer safe-update boundary, never renderer update/record inside an active frame. Rebind consumers on snapshot replacement; retain resources until submitted work completes. Irradiance includes PI (apply albedo/PI), specular mip LOD = perceptual roughness * (levels-1), BRDF RG = A/B at (NdotV, roughness), linear specular = prefilter * (F0*A+B). Sky remains linear HDR until final processing; depth-aware skybox preserves foreground and never writes depth. Renderer environment/blur switching must use these actual services, with default sky and failed-source retention.

## M01-09 binding and extraction ABI

Consume [bindings-visitors.md](bindings-visitors.md): RenderQueueVisitor produces canonical one-based object IDs, retained drawMesh/drawMaterial/six textures, MaterialData and world matrices; both transparent classes stable-sort back to front without renumbering. Prepare Frame/Object/Light/EnvironmentDataBinding once per acquired presentation slot before recording and retain resources through completion. Use ShaderData/native bindings headers and the actual four-set layout, including separate vertex/fragment UBO aliases. Keep fullscreen G-buffer inputs compatible with the real set/index map rather than assuming production descriptor layouts. MaterialData preserves all six UV/scale/channel/color fields; alpha test uses cutoff and linear alpha, manual RGB decode excludes native sRGB images already decoded in hardware. AO presence is not a baked-lightmap flag. Extracted environment intent is a snapshot; synchronize it at a safe pre-acquire boundary, then bind the validated M01-08 resources. New DrawVisitor records only indexed draws inside caller-owned rendering scope/pipeline.

## M01-10 implementation handoff

Actual APIs, ordering, formats, native bindings, lifetime and lead checks: [raster-layers.md](raster-layers.md). RendererDeferred, GBuffer, DeferredLayer, FinalLayer, RenderTarget and FullscreenPass now implement this step. Transparent items use copied opaque depth and separate sorted deferred composites; ordinary bilinear reconstruction and single final color processing are implemented. Native D16/D32 exact copies extend the earlier color-copy primitive. No CMake changes or agent compilation/runtime execution.
