# Step 3 — Font resources, dependency and licensed asset

Dependency: accepted step 2.

## Dependency and asset

1. Vendor an independently pinned `stb_truetype.h`, retaining its MIT license option and upstream notices. Compile its implementation exactly once with private symbols; do not use ImGui's copy or expose stb structs in public headers. Do not add Fontstash.
2. CMake edits are explicitly permitted for this dependency and font/license asset distribution. Prefer existing discovery/copy mechanisms when adequate; no build/configuration commands may be run.
3. Use Didact Gothic Regular, a static legible sans-serif font under OFL 1.1. Inspect existing `assets/DidactGothic-Regular.ttf` provenance before changing anything. Compare with a pinned upstream original; reuse the existing asset if appropriate. Otherwise add the verified original under a new canonical assets path without overwriting user/unknown content.
4. Add the complete upstream OFL notice adjacent to the canonical font (or a clearly associated license path), plus a short provenance record with source URL, pinned revision and file checksum. Preserve the font's license instead of applying project GPL text to it. Ensure package asset copies include the notice. Do not duplicate the font unnecessarily.

## Font objects and scene integration

1. Implement `base::TextFont` as a CPU resource owning font bytes and private face state, index, metrics and rasterizer operations. Keep GPU/scene dependencies out of base. Stable bytes must outlive all stb references. No process-global mutable font registry.
2. Implement `scene::TextFontComponent` with a collection of declarations `{identifier, path, faceIndex}`. One component can declare several fonts on a root node. Any node may declare fonts; all identifiers belong to one scene namespace.
3. Separate representation-only deserialization from file loading. Serialize portable references; resolve relative paths at the component/scene resource boundary.
4. Add a scene-owned registry and explicit font-first resource preparation phase before text realization, independent of traversal order. Duplicate identifiers fail clearly. Rebuild/invalidate the registry on declaration edits, removal and scene replacement without stale handles.
5. Define missing-font behavior and reload notification/versioning so future texts regenerate when their font changes. Fail without publishing partially updated resources; avoid repeated per-frame file reads or retries.
6. Implement registration/factory discovery, public module includes and detached independent cloning. Immutable font bytes may be internally reused only if that does not share editable declaration state; respect the repository's component clone contract.
7. Validate identifiers, face indices, file I/O and sensible resource limits. State explicitly that basic checks do not make stb safe for malicious fonts: trusted project fonts only initially.

Do not implement automatic font discovery, fallback chains, live file watching or shaping/bidi libraries.

## User handoff: Composer

Report the concrete asset/license paths and new APIs. Font authoring controls arrive in step 7; no dedicated Composer behavior is required yet. User compilation of library and Composer is sufficient acceptance. The agent does not compile or create a temporary sample.

Update progress, recording the pinned dependency/font sources and canonical font path, and stop.

## Next milestone

Step 4 adds serializable text data and CPU paragraph rasterization through ProceduralTextureGenerator.
