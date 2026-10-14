# M01-10 — Basic deferred renderer and injectable public layers

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01-02/M01-04/M01-05/M01-08/M01-09.

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

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M01-11 adds query lighting; M03 adds advanced effects to these contracts and M04 adds actual baked multipliers without rebuilding the architecture.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
