# Step 2 — Texture source editing and export metadata

Dependency: accepted step 1.

## Implementation

1. Add independent `sourceEditable` (default true) and `ignoreSerialization` (default false) metadata to `base::Texture`, following existing accessor/setter conventions. Decide explicitly whether these metadata affect authored revisions without causing unnecessary pixel uploads.
2. Preserve metadata through copy/assignment, texture-settings cloning, MaterialBase snapshots, drawable cloning and resource retirement/exchange paths. Do not depend on metadata being saved into scene JSON to restore text invariants.
3. Update `DrawMaterialEditor` and Composer's mutation owner to consult source editability before offering or applying replacement/removal. Cover queued intents, drag/drop, clear operations, material patches and mixed selections. Reject the complete source edit if any target is locked, with a useful error; do not partially apply it.
4. Keep UV set, UV scale, filtering, addressing, color interpretation, mipmap and LOD controls available. A source lock is not a general property-editing lock. Color, unlit and alpha cutoff remain ordinary material values.
5. Preserve a locked source when applying/pasting an entire material through editor-supported paths. Do not add API-wide protection, frame-by-frame repair or a provider/override resource system.
6. Extend MaterialSerializer and resource collection/export paths to omit an ignored texture's source and pixel asset. Keep independently editable UV/sampling settings serializable so a procedural owner can reapply them to a regenerated source. Inspect the current serialized schema before designing this distinction.
7. Make omission consistent for both reference-only and self-contained export; never realize or write a texture merely to discover it should have been ignored.

## Boundaries

No synthetic locked-texture demo or test fixture. Ordinary textures remain fully editable and exportable. TextComponent will establish the generated texture flags later. No arbitrary mutable-API assignment validation is required.

## User handoff: Composer

Explain that existing texture editing should remain unchanged, but no text-generated locked source is available yet. This milestone's new restriction is not independently observable in the ordinary Composer workflow; successful user compilation of both library and Composer is sufficient acceptance. Do not require a special program. Later step 7 exposes the complete locked-albedo behavior in Composer.

Update progress and stop for acceptance.

## Next milestone

Step 3 integrates stb, establishes a properly licensed sans-serif font asset, and implements scene-wide font declaration/loading resources.
