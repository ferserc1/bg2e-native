# HTTP Scene Import Service

`bg2e_composer` exposes a localhost-only HTTP service that lets external tools
(DCC applications, scripts, Postman during development) push glTF scenes into
the running Composer session without user interaction. All of the service code
lives at application level (`apps/bg2e_composer/src`); the engine itself is
untouched except for one small UI helper.

## Part 1 — Using the service

### Startup and default port

The service is configured through the `File > Import Settings...` menu entry
and persists its state across sessions. On application startup:

- If the service was enabled the last time Composer ran (the default), the
  server starts automatically on the configured port.
- The **default port is `8643`**.
- The server binds to `127.0.0.1` only; it is never reachable from other
  machines. There is no TLS.
- If the port is already occupied, the service does not start and an error is
  shown in the status bar (`Import service failed: ...`).

### Settings window

`File > Import Settings...` opens the **Import Settings** window, which shows:

- **Port** — a text field with the TCP port. It is editable only while the
  service is stopped (the field is disabled while running). Valid range:
  `1024–49151`.
- **Service enabled** — a checkbox that starts/stops the server. Enabling it
  validates the port (an error dialog appears for invalid input or a busy
  port), persists the new settings, and starts listening. Disabling stops the
  server immediately; any in-flight request fails.
- A status line at the bottom shows either `Listening on 127.0.0.1:<port>` or
  `Service stopped`.

Both settings (port and enabled flag) are persisted to
`preferences_import.json` and restored on the next launch.

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
| `filePath` | string | yes | Absolute path to a `.gltf` or `.glb` file. The file must exist. |
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

- The imported glTF subtree hangs from a **wrapper node** named after the file
  stem. The wrapper carries the unit scale and the optional Z-up→Y-up rotation
  in its transform, so the source asset is never modified.
- Every import (HTTP or `File > Import GLTF Scene`) **clears the current
  selection first**, so the new content never hangs from a node that is about
  to be removed. On a first import the wrapper hangs from the primary selected
  node (or the editable root if nothing is selected); on a **reimport** it
  hangs from the **same parent as the previous imported node**, regardless of
  the current selection.
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

`AppDelegate::update()` takes the pending request on the main thread and starts
`asyncLoadGuarded()`. The worker loads the glTF, reports image and mesh progress
to the modal loader, and queues insertion and any replacement removal through
`MainLoop::safeUpdateScene()`. Selection changes and the import table remain on
the main thread. `MainLoop` applies the queued scene changes, closes the loader,
and resumes the scene in order. Its completion callback then updates the table
and calls `ImportServer::fulfil()`, releasing the HTTP waiter. Failed imports
leave any previous imported node intact.

`GET /status` reports `busy` while the slot is occupied. There is no queue of
waiting imports and no application-level timeout. The service's request ID
prevents a late completion from answering a request after stop and restart.
