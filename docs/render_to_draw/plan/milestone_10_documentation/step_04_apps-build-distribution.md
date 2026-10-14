# M10-04 — Application, CLI, build and final breaking-change docs

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M10-01–M10-03 and final targets from M09.

## Scope and deliverable

- `doc/apps/`
- `doc/api/app/`
- `doc/workspace_api.md`
- `doc/safe_update_scene.md`
- `doc/window_api.md`
- `doc/input_delegate.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [doc/apps/](../../../../doc/apps), [doc/api/app/](../../../../doc/api/app), [doc/workspace_api.md](../../../../doc/workspace_api.md), [doc/safe_update_scene.md](../../../../doc/safe_update_scene.md), [doc/window_api.md](../../../../doc/window_api.md), [doc/input_delegate.md](../../../../doc/input_delegate.md).

## Implementation work

1. Update all supported application guides to final apps/draw targets/source locations and actual UI/material/import/viewpoint/bake workflows.
2. Document offscreen and lightmap-generator CLI configuration, output paths/counts/formats/preflight/errors, settings import and overwrite policy.
3. Update build/platform generator/resource/SDK/package/standalone guidance to final distribution, keeping required GPU Vulkan/Metal deployment distinct from deleted render wrappers.
4. State the deliberate removal of previous application APIs and draw promotion. Do not promise compatibility shims, old MSAA/forward behavior or future ResourceManager/Metal/DLSS features.

## Verification and exit conditions

- A user can identify the correct final executable and necessary deployment/resources for each app/CLI from documentation.
- Settings/file/output examples and safe editing/async lifecycle descriptions match observed product behavior and source.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M10-05 audits all remaining pages, including broader conceptual documents whose rendering references are easy to miss.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
