# M02-02 delivered picking contract

Source baseline: `7b217bcb6d9a3473ea522dd258d8d87b87150ddb` (2026-10-16).
M02-01 accepted by lead continuation, with macOS Metal and Linux Vulkan validation
reported through that state. This step has no agent build/runtime results.

SelectionManager's draw overload now implements synchronous geometric picking
through public GPU primitives in DrawPicking.cpp. RenderQueueVisitor supplies
canonical visible geometry/world transforms and retained queue-local 24-bit IDs;
zero is background. A submission-local map connects those IDs to typed weak
SelectionItems, global Selectable identifiers and exact component/resource owners.
The same snapshot owns mesh buffers until immediate submission and full tightly
packed RGBA8 readback complete. Results never resolve against another frame's
queue or query-instance ordering. SelectionItem validation rejects stale resources
or identities. Tool entries retain weak gizmo membership and select their owner;
node-only objects without gizmos remain tree-only.

Paired draw/picking/picking vertex/fragment shaders use an 80-byte matrix/uvec4
vertex push block (Metal buffer1), flat 24-bit ID and linear RGBA8 encoding without
blending. D32 depth picks nearest geometry. Type gizmos render without depth over
geometry; transform handles render last with fresh depth and preserved ID color,
using a separate rendering scope and fresh depth image so Metal never clears a
materialized encoder. Explicit color transfer/source transitions provide the
Vulkan dependency before loading preserved IDs in that scope.
Only visible transform handles participate. Native staging/readback remains in
GPU; no new GPU methods, native implementation or depth readback are needed.
Texture alpha is deliberately not tested, matching production geometric picking.

The caller supplies unjittered view/projection and a top-left input viewport.
setPickPixelScale(x,y) defines drawable pixels per input unit; default1 supports
pixel coordinates. Subtract viewport origin, multiply scale and floor to locate
the pixel; image extent is ceil(viewport size times scale). Both backends map
positive clip Y to top image rows (Vulkan negative viewport, Metal native viewport),
so no readback row inversion is applied. Invalid/nonfinite configuration throws;
outside/empty/disabled roots return false with an empty result. Renderer internal
scale and FSR jitter do not enter picking.

Picks require a stopped scene/frame producer boundary, outside command recording.
The device is drained before extraction; immediateSubmit completes before readback
and before local resources are released. Resources/pipelines are intentionally
submission-local for this infrequent click path; no device cache or renderer
attachment lifecycle is introduced. Draw init remains CPU readiness only. A local
ToolVisitor is necessary solely to collect optional gizmo owners, which the shared
extractor intentionally excludes; it does not duplicate scene geometry extraction.

Shared click/additive/drag/capture semantics remain; transform handle lookup now
uses the correct typed drawable. Draw empty-pick clearing honors its option and
transform picks preserve selection. Production entry points and native code remain.
No CMake files changed: existing recursive shader-library/source discovery covers
the new paths. No new runnable target is required until M02-07.

Static inspection covers public signatures/callers, mesh retention/weak identity,
shader ABI and native vertex push slot, completion/readback, rendering load/store,
viewport orientation and recursive shader discovery. GPL/style/delimiter and
whitespace checks cover all touched source/shaders. No compilation, shader
compilation or runtime execution was performed. See the
[lead checklist](../../../../tests/manipulation/picking/README.md).
M02-03 follows after acceptance; overlays remain unimplemented.
