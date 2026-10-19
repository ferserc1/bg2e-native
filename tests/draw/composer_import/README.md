# Composer asynchronous import/service checks

Target: draw_bg2e_composer. These are project-lead checks; the agent has not
compiled or executed them. Use available Vulkan/Metal backends, raster/no-RT
hardware and SDK-unavailable fallbacks. Keep legacy bg2e_composer and draw model
editor regressions distinct from the new app's evidence.

## HTTP probe

Start the app, open Window > Import Settings and choose a free localhost port.
The default is 8643. Use an existing supported static glTF/GLB without cameras,
lights, skinning or animation. Example commands (replace the local model path):

```sh
curl -i http://127.0.0.1:8643/status
curl -i -H 'Content-Type: application/json' --data '{"filePath":"/absolute/model.glb","fileName":"Probe","units":"cm","coordinateSystem":"z_up"}' http://127.0.0.1:8643/import
```

Expect running/port/busy on status. Import waits for publication and restored
bindings, then returns 200/status=ok; one converted wrapper appears at the captured
parent and the document becomes dirty. Repeat the same path: it replaces that
wrapper, retaining its current parent rather than adding another copy. A manual
local import of the same path remains independent.

While a deliberately large request is outstanding, use another terminal for
/status (busy=true) and another /import (503). Check missing JSON/type/body,
unknown units/coordinates, unsupported extension and missing file return the
expected 400/415/404 errors. Valid requests with unsupported glTF resources return
500 and retain the previous import. Cancel or stop the service during parsing:
expect failed completion, no late scene publication and a responsive restored UI.
Restart on another port. Start the legacy app on the occupied port: expect a clear
bind failure, not a shared listener or a second app silently taking the request.

## Queue/lifetime matrix

1. Queue multiple local imports through the loading panel, mixing glTF/GLB,
   BG2/VWGLB, OBJ/MTL and scene prefabs. All valid jobs should publish independently
   in queue order even though each publication rebinds the renderer/settings.
   A failed job must not discard unrelated queued jobs.
2. Check local unit/axis choices, nested parent transforms, OBJ names/materials/
   MTL-relative textures, BG2 chain joints and prefab camera/projection metadata.
   Save/reopen the resulting complete scene, including HDR/native texture resources.
3. Cancel while a large parser is busy. Edit/replace the scene after cancellation,
   wait for the old reader to finish and verify it cannot publish. A third-party
   parser is cooperatively canceled at its boundary; another reader may wait.
4. Exercise New/Open while a job is outstanding (through the app's existing
   shortcut/drop/API paths). Successful replacement invalidates both pending HTTP
   admission and worker jobs; canceled/failed replacement preserves the document.
5. Exercise removal/reparent of an import destination or tracked reimport node
   while its worker is outstanding via the public app edit API/debug harness.
   Membership/parent checks reject stale results. Replace the renderer: reject the
   job; resize/rebind the same renderer: reacquire bindings and allow it to finish.
6. Fill eight enabled lights via prefabs. Replace an imported subtree at that
   limit: exclude the previous subtree during preflight. A ninth enabled light or
   removal of the final camera fails with the previous graph and sibling order
   retained. Check disabled ancestors, zero lights and stage-fill suppression.
7. Missing external textures/MTLs, malformed source, unsupported glTF and invalid
   environment resources must preserve prior scene content. Inspect history,
   selection/material/UV previews and settings targets after success/failure.
   If a binding failure is injected, verify rollback reattaches the old graph.
8. Start import during a bake: source work retires before publication. Reopen
   graphics/debug panels after import; RT/SDK unavailable states remain coherent.
9. Close/quit with active parsing, queued local jobs and an admitted HTTP request.
   Discard/save/cancel close behavior remains intact. Confirm handlers wake,
   readers join before app teardown, no stale callbacks/GPU uploads occur and
   native validation reports no use-after-free.
10. Regression-check existing production glTF/OBJ/BG2 path overloads and draw_model_edit
    resource import/export after the shared CPU-parser splits. Keep source arrays
    independent when realizing one ObjMesh/Bg2Mesh more than once.

GPU realization/image decoding is synchronous while stopped; the loading UI does
not promise intermediate presented frames or cancellation inside that call.
Record backend, fixture and actual results separately from continuation acceptance.
