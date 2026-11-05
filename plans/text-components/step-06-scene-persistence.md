# Step 6 — Scene persistence and external resource packaging

Dependency: accepted step 5.

Mandatory: [recovery-policy.md](recovery-policy.md). One malformed/unavailable text or font must not abort loading the scene; retain authored data and warn. Saving failures return explicit status with rollback, not an escaping exception or false success.

## Implementation

Step-5 handoff: saves containing authored TextComponent are temporarily refused with warning/status rather than losing data or baking text. Replace those guards in `db/scene.cpp` and `db/draw_resource_io.cpp` once embedded serialization/resource copying is complete. Keep `saveSceneReferences()`'s boolean success status and all failure/rollback checks. TextComponent currently reads basic `text`, `size`, `preserveAspect`, `pivot` representation but deliberately has no successful serializer yet.

1. Implement TextComponent representation-only `deserialize`, `serializeReferences` and `serialize`, plus resource realization. Store one Text object and one material object directly in component JSON using the existing material serialization contract, not a JSON string nested inside JSON or a new incompatible material dialect.
2. Ignore incoming albedo source references before they can trigger file access, decoding or resource export. Keep the albedo color and independent UV/sampling settings. After deserialization, recreate the procedural source, reapply those settings and restore source/export flags from component logic, not serialized permissions.
3. In `lib/src/bg2e/db/draw_resource_io.cpp` and all equivalent graph-save paths, inspect `requiresBg2Export()` before snapshot creation, model texture collection, `.bg2` allocation or resourcePath mutation. False skips model export only; component serialization still runs normally. Audit subclass lookups so TextComponent is not accidentally treated as a resource-backed model elsewhere.
4. Serialize generated albedo neither as a filesystem reference nor as exported pixels. MaterialSerializer must keep other material textures and their settings. Preserve source-less albedo sampling metadata using the schema established in step 2.
5. Collect/copy font files from TextFontComponent declarations and external images from embedded materials when producing self-contained scenes. Reuse existing collision handling, path normalization, deduplication, transactional publication and rollback patterns; rewrite relative references without leaving live source paths changed after failure.
6. Handle Save, Save As, copy-to-another-directory and reference-only saving distinctly according to current document contracts. Preserve original file extensions and accompanying font license/provenance notices when distributing the bundled tutorial font. Include necessary notices for other user fonts when supplied; do not invent redistribution rights.
7. On scene open, deserialize all declarations, build the scene registry/load fonts, then realize texts and other resources as required. Do not depend on font nodes preceding text nodes. Keep SceneLoadProgress/resource errors coherent.
8. Ensure copies, duplication, snapshots and resource cleanup retain editable text/material data, with no accidental conversion into a model-backed Drawable. `ignoreSerialization()` still excludes an entire component and remains separate from requiresBg2Export.
9. Guard nullable serialization/resource results. Use defaults for recoverable representation errors, keep unavailable text transparent and report resource-copy/save failures via console warnings and normal operation status. Preserve editable source references; do not silently discard authored data or report incomplete self-contained publication as successful.

## User handoff: Composer

Explain the new document representation and absence of generated `.bg2`/albedo assets. Until Composer authoring arrives in step 7, user compilation of both library and Composer is sufficient acceptance; do not ask the user to hand-author scene JSON. Ordinary existing model saves should retain their previous behavior.

Update progress with concrete schema decisions/resource paths and stop.

## Next milestone

Step 7 adds Composer font/text authoring, live layout editing and material interactions, making the full save/reopen and selection workflow observable.
