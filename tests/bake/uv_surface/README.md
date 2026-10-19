# UV surface and safe reload manual validation

Include probe.hpp in an existing temporary draw harness. No new target is added.
The project lead compiles engine and native shaders before running:

- `bg2e::bake::tests::runUvSurfaceCpuProbe()`: no GPU; shared-edge chart connectivity,
  duplicate seam/material partition, deindex ranges, invalid indices, nonfinite UVs,
  nonfatal positive-area overlap and invalid normals. Overlapping triangles must
  remain in the packed mesh; only structural/coordinate/geometry errors reject it.
- `bg2e::bake::tests::runUvSurfaceGpuProbe(engine, shaderRoot)`: stopped initialized
  Engine, deployed bake/uv_surface libraries. Required unavailable formats/kernels
  throw (not a passing or skipped numerical test). 33x17 asymmetric UV rectangle,
  v=0 top orientation, signed inverse-transpose normals, translation/rotation/
  nonuniform mirrored scale, typed uint clears, two targets per slot, external
  retained output after slot reuse, CPU/GPU/material clone independence and fresh
  BLAS identity where available. UV recording itself requires no RT hardware.

No fixture was compiled/executed by the agent. Keep Device and owners alive;
cleanup is a stopped boundary. Restore the temporary harness afterwards.

Additional Vulkan/Metal validation:

1. Inspect native diagnostics and all six paired stage files. Four MRTs total
   32 bytes/sample, with R8 coverage from a separate compute pass, no depth/MSAA.
   Test missing four-MRT/format support; required unavailable profile fails explicitly.
2. Check chart identity on separate nearby/coincident-world UV islands, material
   boundaries, winding, nonzero submesh firstIndex, tiny/zero-area faces, degenerate
   geometry, UV 0/1 borders, 1x1/odd extents and coverage holes. Chart IDs are local
   and conservative, not permanent material IDs. Compare raw integer readback.
3. Reject stale/unloaded/replaced geometry, foreign Device, wrong slot/serial,
   duplicate beginFrame, invocation overflow, oversized extent, singular/nonfinite
   transforms and active command scope. Do not reuse an unretired GPU slot.
4. For regenerateDraw retain its SafeUpdateToken. Exercise expired/moved node,
   replaced scene/renderer/component/mesh, cancellation, generation failure and
   throwing invalidation/completion callbacks. Invalidate active bake/source state
   through beforeChange; refresh weak selection/preview/catalog consumers on success.
5. Check target-only publication when other nodes share the original Drawable/CPU
   mesh; the original and independent clones remain unchanged. Verify submesh names,
   transforms/material channels/textures and GPU-authored texture levels survive.
   Generation/reload failure leaves the published source intact; reported failures
   may still have invalidated active jobs conservatively.
6. Run asynchronous acquired slots with retained outputs/geometry through completion;
   caller must discard aborted recordings and reprepare after geometry edits. No
   hidden submission or per-record waitIdle. Use the existing AO/GI explicit surface
   fixtures with surfaceInputs() while retaining result owners; chart-safe denoise
   and exact accumulation remain later work, not part of this fixture.

Record hardware/backend, native diagnostics and actual outcomes rather than
inferring parity from source inspection or continuation acceptance.
