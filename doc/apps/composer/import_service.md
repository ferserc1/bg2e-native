# HTTP Scene Import Service

`bg2e_composer` exposes a localhost-only HTTP service that lets external tools
(DCC applications, scripts, Postman during development) push glTF scenes into
the running Composer session without user interaction. All of the service code
lives at application level (`apps/bg2e_composer/src`); the engine itself is
not the owner of HTTP queues or import publication policy.

## Part 1 — Using the service

### Startup and default port

The service is configured through the `Window > Import Settings` menu entry
and persists its state across sessions. On application startup:

- If the service was enabled the last time Composer ran (the default), the
  server starts automatically on the configured port.
- The **default port is `8643`**.
- The server binds to `127.0.0.1` only; it is never reachable from other
  machines. There is no TLS.
- If the port is already occupied, the service does not start and an error is
  shown in the status bar (`Import service failed: ...`).

### Settings window

`Window > Import Settings` opens the **Import Settings** window, which shows:

- **Port** — a text field with the TCP port. It is editable only while the
  service is stopped (the field is disabled while running). Valid range:
  `1024–49151`.
- **Service enabled** — a checkbox that starts/stops the server. Enabling it
  validates the port (an error is reported for invalid input or a busy
  port), persists the new settings, and starts listening. Disabling stops the
  server immediately; any in-flight request fails.
- A status line at the bottom shows either `Listening on 127.0.0.1:<port>` or
  `Service stopped`.

Both settings (port and enabled flag) are persisted to
the `draw.bg2e_composer.import` preference scope and restored on the next launch.

### Endpoints

Base URL: `http://127.0.0.1:<port>` (default `http://127.0.0.1:8643`).

#### `GET /status`

Health check. Returns the running state, bound port, and whether an import is active:

```json
{ "running": true, "port": 8643, "busy": false }
```

#### `POST /import`

Imports a glTF file from the local filesystem into the current scene.

- Header: `Content-Type: application/json` (mandatory).
- Body fields:

| Field | Type | Required | Description |
|---|---|---|---|
| `filePath` | string | yes | Existing `.gltf` or `.glb` path; use an absolute path. Relative paths resolve against Composer's working directory. |
| `fileName` | string | no | Informational name; defaults to the file name portion of `filePath`. |
| `units` | string | no (default `"m"`) | Source file units: `m`, `cm`, `mm`, `in`, `ft`. Drives a uniform scale on the imported wrapper node (meters per source unit: 1, 0.01, 0.001, 0.0254, 0.3048). Case-insensitive. |
| `coordinateSystem` | string | no (default `"y_up"`) | Source axis convention: `y_up` (glTF native, no conversion) or `z_up` (applies a −90° rotation on X to convert to Y-up). Case-insensitive. |

Example:

```sh
curl -X POST http://127.0.0.1:8643/import \
  -H "Content-Type: application/json" \
  -d '{"fileName":"DamagedHelmet.glb","filePath":"/abs/path/DamagedHelmet.glb","units":"cm","coordinateSystem":"z_up"}'
```

The request is **synchronous**: the HTTP response is not sent until the scene
has been updated, the modal loader has closed, and Composer has resumed
interaction (or the import fails). The service imposes no import timeout; an
HTTP client or intermediary may impose its own.

Responses:

| Status | Meaning |
|---|---|
| `200` | Import succeeded: `{"status":"ok","message":"Imported <fileName>"}` |
| `400` | Malformed JSON, missing `filePath`, unknown `units`/`coordinateSystem`, or unsupported extension (only `.gltf`/`.glb`) |
| `404` | `filePath` does not exist on disk |
| `415` | `Content-Type` is not `application/json` |
| `500` | The import failed (message contains the reason) |
| `503` | The service is busy with another import; retry after it completes |

### Import semantics

- The imported glTF subtree hangs from a **wrapper node** named from fileName (default: the source filename). The wrapper carries the unit scale and the optional Z-up→Y-up rotation
  in its transform, so the source asset is never modified.
- On a first import, the destination is captured as the primary selected node
  or the editable root. HTTP reimport retains the previous wrapper's parent,
  independently of later selection. Selection/clipboard clearing occurs at
  stopped publication after candidate preparation, not in an HTTP handler.
- The wrapper node (and every light/camera/environment/drawable node inside
  the imported subtree) is **selectable**: it receives a
  `SelectableComponent` so it can be picked in the viewport, plus a
  `GizmoComponent` for the transform/type gizmo — exactly like any node
  created through the Composer UI.
- The service keeps a table mapping `file path → imported node instance`
  (tracked by instance identity, never by name, so user renames are
  irrelevant). Re-importing the same path **replaces** the previous node.
- If the user deletes an imported node, closes the scene, or opens another
  scene, the table entry is cleaned up automatically and the path can be
  imported again from scratch.

## Part 2 — Implementation

The HTTP service admits one validated import at a time. Its mutex-protected
slot remains occupied until the result is published; another import receives
`503` immediately. Multiple HTTP handler threads keep `/status` responsive
while the accepted handler waits. `stop()` fails and wakes the active request
before joining those handlers.

`AppDelegate::prepareFrame()` dispatches imports on the main thread. The app's
SceneImporter owns one joinable CPU reader and up to 16 waiting local jobs.
Shared CPU parsers prepare data; graph factories, image decoding, native resource
realization and publication run at the stopped main-thread boundary. HTTP owns
one admitted slot, distinct from the local queue. No handler touches graph/GPU state.

Publication validates request/document/renderer and destination identities.
Canonical HTTP reimports replace the old wrapper only after a complete candidate
is prepared; ordinary failed publication restores graph order, camera/environment,
dirty state and renderer binding. Parser cancellation is cooperative, and an
active synchronous realization cannot be interrupted. External images must remain
available through realization. Reply follows publication/rebinding and loader
cleanup; request IDs prevent late completion after stop/restart.

`GET /status` stays responsive while the accepted import waits. There is no
application-level import timeout; clients may impose their own. Port conflicts
between two Composer instances are reported rather than selecting another port.
Service stop fails/wakes its request before joining handlers. Document replacement
and cancelled jobs reject late results. Shutdown stops/joins HTTP and CPU readers
before renderer/scene/Engine teardown.

See the maintained [import lifecycle](draw_import_lifecycle.md),
[application guide](../draw_composer.md),
[ImportServer](../../../apps/bg2e_composer/src/ImportServer.cpp),
[ImportActions](../../../apps/bg2e_composer/src/ImportActions.cpp) and
[project-lead checks](../../../tests/draw/composer_import/README.md).
These are source-reviewed contracts, not newly executed HTTP/backend tests.
