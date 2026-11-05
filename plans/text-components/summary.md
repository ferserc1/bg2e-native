# Text components implementation plan

Status: planning complete; implementation has not started. Read [progress.md](progress.md) before doing any work.

## Agreed architecture

- Use the current `bg2e::draw` rendering infrastructure. Do not migrate rendering APIs or introduce dependencies from `gpu` into scene/text services.
- `base::Text` is CPU data: UTF-8 content, scene-wide font identifier, font size, paragraph geometry/layout and bitmap resolution. It contains no color or other material appearance. Match `base::Light`'s `deserialize(shared_ptr<JsonNode>)` / `serialize()` contract.
- `base::TextFont` owns loaded font bytes and private stb state. `scene::TextFontComponent` declares a collection of uniquely identified font files, preferably on the root, but legally on any scene node. A scene-owned registry resolves identifiers without ancestor shadowing. Collections allow multiple fonts on one node under the current component storage rules.
- Load all font declarations before realizing any text, regardless of node order. Missing identifiers and duplicate declarations have explicit diagnostics. Fonts and material images are copied as external files when saving a self-contained scene.
- `scene::TextComponent : scene::DrawableComponent` owns exactly one procedural quad and one conventional material. Its albedo uses `base::ProceduralTextureGenerator`: RGB is white, including transparent pixels; alpha is glyph coverage. Color, unlit, alpha test and all appearance belong exclusively to the material.
- Textures expose `sourceEditable()` (default true) and `ignoreSerialization()` (default false), with setters consistent with existing base APIs. The generated albedo sets them to false/true. Only replacing or removing its source is blocked by editor mutation paths. UV set, UV scale and all sampling controls remain available and survive reload. No protection against arbitrary mutable C++ API use is required; no per-frame albedo-repair loop.
- `DrawableComponent::requiresBg2Export() const` is virtual, read-only and defaults to true; TextComponent overrides it to false. This differs from ignoring serialization: text data and embedded material must still be saved. Never bake text into a `.bg2` or export its generated albedo during normal scene saving.
- Use only independently vendored `stb_truetype` under its MIT option. No dependency on ImGui's private stb copy and no Fontstash in this scope. Fontstash's zlib license is acceptable for a future evaluation but is not a reason to add it now.
- Picking selects the entire text rectangle, including transparent areas, and works from the back when double sided. Rendered lighting and shadows must respect the actual glyph alpha mask.

## Ordered milestones

| Step | Deliverable | Details |
| --- | --- | --- |
| 1 | Generic double-sided materials, independent of text | [step-01-double-sided-materials.md](step-01-double-sided-materials.md) |
| 2 | Texture source edit/export metadata | [step-02-texture-metadata.md](step-02-texture-metadata.md) |
| 3 | stb, licensed font asset and scene font resources | [step-03-font-resources.md](step-03-font-resources.md) |
| 4 | Text data and CPU procedural paragraph rasterization | [step-04-text-rasterization.md](step-04-text-rasterization.md) |
| 5 | Procedural TextComponent and resource lifecycle | [step-05-text-component.md](step-05-text-component.md) |
| 6 | Editable scene persistence and resource packaging | [step-06-scene-persistence.md](step-06-scene-persistence.md) |
| 7 | Composer authoring and integrated interaction | [step-07-composer-integration.md](step-07-composer-integration.md) |
| 8 | Final API documentation, runnable tutorial and beginner guide | [step-08-documentation-and-tutorial.md](step-08-documentation-and-tutorial.md) |

Implement sequentially, one milestone per request to continue. The final milestone includes the tutorial source code; it is not documentation-only.

## Execution contract

1. Before implementation, read this summary, progress, the current step, applicable AGENTS.md instructions and relevant source. Read `skills.md` if available; it was absent during planning. Preserve unrelated/user changes and reconcile partially implemented work before editing.
2. Do not implement anything as part of writing this plan. Do not add automated tests, unit tests, integration tests, test harnesses or verification executables in any milestone. The final tutorial is a learning example, not a test application.
3. Do not configure CMake, compile the library or any executable, launch applications, or run automated runtime checks. The user builds and manually checks the work. Source/diff review is allowed; never equate it with a successful build.
4. Every milestone ends with a Spanish report: what changed, what the user should check in bg2e Composer, what cannot yet be checked there, known limits, and exactly what the next milestone adds. Then stop for user feedback.
5. For a milestone with no Composer-observable behavior, the user's successful compilation of both the library and Composer is sufficient acceptance; do not invent extra sample programs or checks. Mark it pending until the user reports acceptance or asks to continue without reporting a failure. User continuation accepts the preceding milestone unless stated otherwise. Build fixes remain part of that milestone.
6. Update progress before every stop/interruption. Distinguish implemented from accepted. Record changed files, decisions, unfinished work, feedback, and the exact next file/action. Fix reported problems before advancing. Do not skip an awaiting-acceptance milestone automatically.
7. The plan explicitly authorizes narrowly scoped CMake edits needed to integrate stb_truetype, font/license asset copying and the requested tutorial target, overriding the repository's default CMake prohibition only for those purposes. Do not regenerate build trees. Reuse source discovery where sufficient.
8. Follow full project GPL headers/style for repository-owned source; preserve third-party licenses. Keep stb implementation symbols private to avoid ImGui collisions. Never expose ImGui headers/types or stb implementation types in public engine headers.
9. All generated files, source comments and documentation are English. User-facing work reports remain Spanish. No commits or unrelated refactors are requested.

## Font asset decision

Use **Didact Gothic Regular**, a legible sans-serif static TrueType font licensed under **SIL Open Font License 1.1**, a permissive font license allowing redistribution with its copyright/license notice. A file already exists at `assets/DidactGothic-Regular.ttf`; do not overwrite it blindly or create unnecessary duplicates.

Step 3 must establish a canonical verified asset: inspect the existing file's provenance; reuse it if it matches the upstream original, otherwise add the upstream original at a distinct documented path and preserve the existing asset. Include the upstream OFL notice, source URL, pinned revision and font checksum. The tutorial uses this same asset. License/source references located during planning:

- Font: <https://github.com/google/fonts/blob/main/ofl/didactgothic/DidactGothic-Regular.ttf>
- License: <https://github.com/google/fonts/blob/main/ofl/didactgothic/OFL.txt>
- Original notice: Copyright (c) 2009, 2010, 2011 Daniel Johnson.
- Rasterizer: <https://github.com/nothings/stb/blob/master/stb_truetype.h>

The dependency's MIT license does not replace the font's OFL license. stb explicitly does not guarantee safety for untrusted font files; document that limitation and support trusted project assets initially.

## Completion boundary

All eight milestones must be implemented and user accepted. The final tutorial must create a window, a floor, a live-editable `Hello World` text and a directional light casting a glyph-shaped shadow onto the floor. It must teach font registration/loading and TextComponent construction by code, using only engine UI wrappers. Full international shaping/bidi, font discovery, Fontstash, SDF/MSDF, rich text, asynchronous rendering and a multiple-drawable-per-node redesign are out of scope.
