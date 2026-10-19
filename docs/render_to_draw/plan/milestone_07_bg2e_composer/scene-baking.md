# M07-05 — Integrated Composer scene batches

M07-04 is accepted by project lead continuation. Starting HEAD remains
`f7c2498cb4c29e27921c91809043dcf5fe4d963c`; inherited M07-03/04 work is preserved.
This delivery changes only Composer orchestration and documentation, not the
production application, GPU/bake APIs, shaders or CMake.

## Implemented contract

- `AppDelegate::startLightmapBatch` collects unique drawable nodes recursively
  beneath the current selected nodes or the document root. Runtime helpers are
  excluded by `Node::drawable()`'s scene-geometry contract. The preview target
  remains available as a separate single-target action.
- The stopped request freezes target identities, drawable owners, settings,
  document and renderer generations. All targets receive UV2 structural preflight
  with `UvAtlasOverlapPolicy::Warn`; overlap warnings do not exclude triangles.
  UV regeneration and per-target UV1/UV2 previews retain the existing safe path.
- One `IntegratedBakerContext` borrows the Engine and document/environment
  snapshot. One live baker records into ordinary scene frames. `retireBakeFrame`
  waits for GPU completion before committing progress or retaining output.
  The previous baker is cleaned and released before creating the next one.
- Every target's `LightmapResult` and typed `LightmapPixels` are captured before
  any material changes. The whole batch must finish before automatic application
  is queued. Cancellation, import/edit invalidation and renderer-binding changes
  discard pending batch adoption. The queued apply additionally checks its weak
  context identity, document generation, target membership and drawable identity.
- Apply first prepares all unique temporary sources and loaded draw textures.
  RGB8 uses the baker's linear/exposed byte pixels in PNG; RGB32F uses a native
  texture archive without HDR clipping. Float values outside the finite half-float
  display range reject application rather than silently overflowing the G-buffer.
  Preparation failure removes only this operation's unused temporary directories.
- Only after preparation are all live/stored submesh materials updated with
  linear UV2, unit scale and explicit baked AO/GI mode. Completed per-target
  previews retain the applied image independently of the released context; select
  a target in SceneTree to inspect its corresponding result. Cleanup drops these
  owners before Engine teardown.
- `notifyEdited` retires the batch, marks the scene dirty and rebuilds extraction
  and settings. Existing complete draw scene save copies the temporary sources
  into published resources and marks clean only after successful DB publication.
  Existing raw/reference saves are not substituted for this operation.

## Limits and failure policy

The scheduler is app-local and serial, not `StandaloneBakeBatch`. It retains all
GPU/CPU results until adoption, with memory proportional to target count. It uses
the existing synchronous completed-frame drain. Unsupported required AO/GI
capabilities fail through the integrated context while ordinary editing survives.

Temporary sources backing applied materials remain on disk, as in the accepted
model editor; they must survive until complete scene save. A material setter or
tool-rebind failure after adoption starts may leave partial material edits: the
document is marked dirty, histories invalidated and referenced sources retained.
This is not a crash-atomic multi-target filesystem/material transaction. M08 owns
standalone protected-input/batch-output policy, not this editor scheduler.

## Verification

Agent checks are source/API/ownership/style and whitespace inspection only. No
configuration, compilation, shader compilation, app, GPU, package or probe run.
Project-lead checks: [scene bake matrix](../../../../tests/draw/composer_baking/README.md).
Backend/numerical/package coverage is unspecified. M07-06 remains the next product
validation step; this implementation does not close M07.
