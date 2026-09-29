# Step 07 — Verification plan (Postman / curl)

## Setup

Build and launch `bg2e_composer`. The service auto-starts on `127.0.0.1:8643`
(default preferences). Pick a test model from `assets/` (e.g. an absolute path
to a `.glb` file).

Sample request body used below:

```json
{
  "fileName": "DamagedHelmet.glb",
  "filePath": "/absolute/path/to/assets/DamagedHelmet.glb",
  "units": "m",
  "coordinateSystem": "y_up"
}
```

## Test matrix

| # | Stage | Action | Expected result |
|---|-------|--------|-----------------|
| 1 | Server up | `GET /status` | `200 {"running":true,"port":8643,"queued":0}` |
| 2 | Validation | `POST /import` with malformed JSON | `400` + error message |
| 3 | Validation | Valid JSON, missing `filePath` | `400` + error message |
| 4 | Validation | `Content-Type: text/plain` | `415` |
| 5 | Validation | Nonexistent `filePath` | `404` + path in message |
| 6 | Validation | `"units":"furlong"` / unknown `coordinateSystem` | `400` |
| 7 | Validation | `.obj`/`.bg2` path | `400` (only .gltf/.glb for now) |
| 8 | First import | Valid POST | `200`; node appears in Composer scene tree as child of the editable root (nothing selected) |
| 9 | Selection rule | Select a node in Composer, POST a different file | New node is a child of the selected node |
| 10 | Units | POST with `"units":"cm"` | Wrapper node has uniform scale 0.01 (visible in NodeEditor transform) |
| 11 | Coordinates | POST with `"coordinateSystem":"z_up"` | Wrapper node has −90° X rotation |
| 12 | Replacement | Repeat the exact same POST | `200`; single instance — the previous one was replaced, no duplicates in the tree |
| 13 | Rename safety | Rename the imported node in the scene tree, repeat POST | Still replaced (table keyed by instance, not name) |
| 14 | Deletion sweep | Delete the imported node in Composer, repeat POST | `200`; fresh import (table entry was swept, no leak, re-import works) |
| 15 | Scene swap sweep | `File > Open Scene` (open any scene), then POST | Old entries swept; import lands in the new scene |
| 16 | Port validation UI | Settings window: enter `80`, `50000`, `abc` → enable | Error dialog (invalid range/format) |
| 17 | Port busy UI | `python3 -m http.server 8643 --bind 127.0.0.1`, then enable service | `MessageBox` alert: port in use, try another port |
| 18 | Disabled field | While the service is running | Port input is greyed out (validates step 01) |
| 19 | Toggle off/on | Disable service → `curl` → enable again | Connection refused while off; `200` after re-enable |
| 20 | Persistence | Change port to 9000, quit, relaunch | `preferences_import.json` holds 9000; service listens on 9000 |
| 21 | Shutdown safety | POST a large file and quit mid-import | App exits cleanly (server stopped and joined in `cleanup()`) |
| 22 | Concurrent load | Start `File > Open Scene` on a big scene, fire POSTs during the load | Requests queue up and complete after the load; no crash |

## curl equivalents (for CI-style checks)

```sh
# 1
curl -s http://127.0.0.1:8643/status

# 5
curl -s -X POST http://127.0.0.1:8643/import \
  -H 'Content-Type: application/json' \
  -d '{"fileName":"x.glb","filePath":"/nope.glb","units":"m","coordinateSystem":"y_up"}'

# 8
curl -s -X POST http://127.0.0.1:8643/import \
  -H 'Content-Type: application/json' \
  -d '{"fileName":"DamagedHelmet.glb","filePath":"/abs/path/DamagedHelmet.glb","units":"m","coordinateSystem":"y_up"}'
```

## Regression checks

- Interactive imports (`File > Import GLTF Scene`, drag & drop) behave exactly
  as before.
- `Scene > Remove Selection` confirmation flow unchanged.
- Engine library builds with the extended `Value::text` signature; existing
  callers unaffected (default argument).
