# Text components progress

## Resume here

- Planning: complete.
- Implementation: steps 1–6 implemented and accepted; step 7 implemented awaiting user acceptance; step 8 not started.
- Current milestone: **7 — Composer authoring and integrated interaction**, implemented with user-reported quad/gizmo fixes; awaiting confirmation of those fixes.
- Next milestone: **8 — API documentation and runnable tutorial**, not started.
- Next action: await user confirmation that single-sided text is readable from its +Z front and gizmo translate/rotate/scale plus undo/redo work for Text and font-bearing nodes. Fix further reports within step 7 before advancing. Step 8 is not authorized by this bug report.
- Blockers: the user reported mirrored single-sided text and gizmo admission refusal; targeted source fixes are implemented but not yet user-confirmed. Other step-7 manual checks passed according to the user. Preceding uncommitted work is preserved.
- Builds/application checks: no agent execution. The user ran the step-7 Composer checks and reports success except for the two issues recorded in the follow-up below.
- Canonical font: `assets/fonts/didact-gothic/DidactGothic-Regular.ttf`, OFL 1.1; verified against pinned upstream in step 3. The differing pre-existing root-level asset is preserved.
- Mandatory user correction: no recoverable text/font/layout/source-edit exceptions. Warn and repair parameters, return transparent/unavailable text or safe rejected status, contain lower-level exceptions, preserve the running scene. The [recovery policy](recovery-policy.md) supersedes historical exception/rejection decisions below.

## Milestone ledger

| Step | Description | Implementation | User acceptance |
| --- | --- | --- | --- |
| 1 | Generic double-sided materials | Implemented | Accepted by user |
| 2 | Texture source edit/export metadata | Implemented | Accepted by user |
| 3 | Font resources, stb and licensed asset | Implemented | Accepted by user |
| 4 | Text data and procedural rasterization | Implemented | Accepted by continuation |
| 5 | TextComponent | Implemented | Accepted after user compilation |
| 6 | Scene persistence and packaging | Implemented | Accepted by continuation |
| 7 | Composer integration | Implemented | Pending |
| 8 | API docs and interactive tutorial | Not started | Pending |

## Mandatory handoff record

Append a record after each implementation milestone and update the resume block/ledger. Preserve earlier feedback and decisions.

### Step N — Title

- Status: in progress / implemented awaiting user acceptance / accepted / blocked.
- Changed files:
- Delivered behavior and API decisions:
- Source review performed (not a build):
- Composer manual checks to communicate to the user:
- If not observable in Composer: explicitly state that user compilation of library and Composer is sufficient for this milestone.
- User-reported result and acceptance evidence:
- Known limits, blockers and unfinished work:
- Next milestone and its purpose:
- Exact next action / file to resume:

Do not mark implementation or acceptance complete based on this template. No build/test execution is authorized by this plan.

### Step 1 — Generic double-sided materials

- Status: accepted.
- Changed files:
  - `lib/include/bg2e/base/MaterialAttributes.hpp`
  - `lib/src/bg2e/base/MaterialAttributes.cpp`
  - `lib/src/bg2e/utils/MaterialSerializer.cpp`
  - `lib/src/bg2e/utils/MaterialModifier.cpp`
  - `lib/src/bg2e/db/scene_gltf.cpp`
  - `lib/src/bg2e/ui/DrawMaterialEditor.cpp`
  - `apps/bg2e_composer/src/command/material/MaterialInteraction.cpp`
  - `lib/include/bg2e/draw/ShaderData.hpp`
  - `lib/src/bg2e/draw/ShaderData.cpp`
  - `lib/include/bg2e/draw/pipeline/GBuffer.hpp`
  - `lib/src/bg2e/draw/pipeline/GBuffer.cpp`
  - `lib/include/bg2e/draw/pipeline/TemporalSurfacePass.hpp`
  - `lib/src/bg2e/draw/pipeline/TemporalSurfacePass.cpp`
  - `lib/src/bg2e/manipulation/DrawPicking.cpp`
  - `lib/include/bg2e/utils/ViewpointAnalyzer.hpp`
  - `lib/src/bg2e/utils/ViewpointAnalyzer.cpp`
  - `lib/src/bg2e/draw/CollectRayTracingInstancesVisitor.cpp`
  - `lib/include/bg2e/draw/pipeline/SoftShadowPass.hpp`
  - `lib/src/bg2e/draw/pipeline/SoftShadowPass.cpp`
  - `lib/src/bg2e/draw/pipeline/TracingPersistentResources.hpp`
  - `lib/src/bg2e/draw/pipeline/TracingSceneResources.cpp`
  - `shaders/src/draw/raster/gbuffer.frag.glsl`
  - `shaders/src/draw/raster/gbuffer.frag.metal`
  - `shaders/src/draw/temporal_surface/surface_v2.frag.glsl`
  - `shaders/src/draw/temporal_surface/surface_v2.frag.metal`
  - `shaders/src/draw/lib/tracing.glsl`
  - `shaders/src/draw/lib/tracing.metal`
  - `shaders/src/draw/lib/rtgi_any.glsl`
  - `shaders/src/draw/lib/rtgi.metal`
  - `shaders/src/draw/lib/rt_reflections.glsl`
  - `shaders/src/draw/lib/rt_reflections.metal`
  - `shaders/src/draw/query/soft_shadows.comp.glsl`
  - `shaders/src/draw/query/soft_shadows.comp.metal`
  - `plans/text-components/summary.md`
  - `plans/text-components/progress.md`
- Delivered behavior and API decisions:
  - `MaterialAttributes::doubleSided()` / `setDoubleSided(bool)`, default false; assignment uses authored revisions. JSON uses `doubleSided`; glTF imports its equivalent flag. The existing full attribute-copy clone/snapshot/replacement paths inherit the value; Composer's separate interactive value capture/write/equality now includes it for undo/redo.
  - Composer's material editor exposes **Double sided** beside Transparent, Solid and Unlit. The existing patch/command owner applies it to live materials and definitions.
  - Packed material flag 128 participates in classification revisions. GBuffer and temporal receiver replay each own shared Back/None cull variants for writable/read-only depth. Selection uses a shared variant per pick invocation, without changing tool/gizmo pipelines or alpha-independent rectangle selection. Viewpoint analysis also classifies each submesh by sidedness.
  - Tangent-frame policy: retain the authored front-face TBN, apply normal mapping, then negate the whole resulting normal exactly once on a double-sided backface. Temporal normals use the same policy before previous-frame transport. Reflection shading uses geometric winding, not the perturbed normal, to decide reversal.
  - Tracing candidates reject backfaces for single-sided materials before alpha testing; transformed triangle edges preserve winding under mirrored transforms. Single-sided instances are nonopaque to traversal so query/any-hit rejection can run. Double-sided non-cutout instances retain the opaque shortcut. Soft-shadow metadata now includes the model matrix (112-byte CPU/GLSL/Metal stride); its persistent identity includes transforms to avoid stale shadow tables.
  - Scope compatibility note: raster/picking retain Back culling by default. Traced occlusion/reflections and viewpoint analysis previously accepted both sides unconditionally; these paths now follow the material. Existing scenes relying on invisible backfaces as ray blockers can therefore change even with the default false. Enable Double sided on such thin surfaces. This is an explicit consistency correction, not an API migration.
  - No changes to `gpu`, fullscreen/overlay semantics, CMake, dependencies, assets, text classes or tutorial sources.
- Source review performed (not a build): inspected material packing/cache revisions, attribute cloning/snapshots and Composer commands, BG2 JSON persistence and glTF import, raster/temporal/picking/view analysis, shadow table persistence, GLSL query/any-hit and Metal tracing consumers. `git diff --check` passed. No configure/build/application/test/runtime checks were performed or added. `skills.md` was not present.
- Composer manual checks to communicate to the user:
  1. Build both library and Composer (including changed shaders).
  2. Select an existing plane/thin mesh, open its material editor and toggle Double sided. Inspect both sides with lighting, with and without a normal map; also inspect a mirrored/nonuniformly scaled instance.
  3. Select from the back when enabled; verify backface selection remains unavailable when disabled. Alpha cutouts must not turn picking into per-pixel alpha selection.
  4. Save/reopen, copy/paste or replace materials, clone geometry and exercise undo/redo; confirm the value and live appearance survive.
  5. Where supported/enabled, inspect soft shadows, AO/GI and reflections from both sides, with an alpha-cutout material and mixed single/double-sided geometry. Check temporal accumulation after a live toggle for stale classification artifacts.
- What cannot yet be checked: no font registration, text rasterization/component, locked generated albedo or text authoring exists. Vulkan and Metal changes have source-level parity only; unsupported ray-tracing devices retain existing feature unavailability.
- Known limits and unfinished work: user compilation/runtime validation pending. UV-space bake receiver capture intentionally retains authored normals and no culling: UV winding is not a viewing side, and the existing lightmap has one value per texel, not separate front/back lighting. Existing tracing texture-capacity neutral/scalar-alpha fallbacks remain; no expanded capacity or transparency redesign. Generic picking remains alpha-independent. Diagnostic `rt_probe` shaders use their own synthetic probe material schema, not scene materials, and are unchanged.
- User-reported result and acceptance evidence: the user explicitly said the implementation is correct and requested the next step. They also stated awareness of the pre-existing two-sided ray-tracing behavior and that it is acceptable. No failure or request to revert the accepted implementation was reported; step 2 does not change ray tracing.
- Next milestone and its purpose: step 2 adds independent texture source-edit/export metadata, protects only source replacement/removal in editor-owned mutation paths (including material paste), and retains serializable UV/sampling controls while omitting ignored sources/assets.
- Exact next action / file to resume: await user feedback; fix build/behavior reports in the affected files above. On acceptance/continuation, reread this progress file and `step-02-texture-metadata.md`, then start with `lib/include/bg2e/base/Texture.hpp`.

### Step 2 — Texture source editing and export metadata

- Status: accepted.
- Changed files:
  - `lib/include/bg2e/base/Texture.hpp`
  - `lib/include/bg2e/ui/DrawMaterialEditor.hpp`
  - `lib/include/bg2e/ui/DrawTextureWidgets.hpp`
  - `lib/include/bg2e/utils/MaterialModifier.hpp`
  - `lib/include/bg2e/utils/MaterialSerializer.hpp`
  - `lib/src/bg2e/draw/Texture.cpp`
  - `lib/src/bg2e/ui/DrawMaterialEditor.cpp`
  - `lib/src/bg2e/ui/DrawTextureWidgets.cpp`
  - `lib/src/bg2e/utils/MaterialModifier.cpp`
  - `lib/src/bg2e/utils/MaterialSerializer.cpp`
  - `lib/src/bg2e/utils/TextureCache.cpp`
  - `lib/src/bg2e/db/draw_resource_io.cpp`
  - `apps/bg2e_composer/src/command/material/MaterialCommands.hpp`
  - `apps/bg2e_composer/src/command/material/MaterialCommands.cpp`
  - `plans/text-components/summary.md`
  - `plans/text-components/progress.md`
- Delivered behavior and API decisions:
  - `base::Texture::sourceEditable()` / `setSourceEditable(bool)` defaults true; `ignoreSerialization()` / `setIgnoreSerialization(bool)` defaults false. These independent CPU policy flags do not advance the pixel/sampler revision, so setting them does not trigger uploads. Default copy and explicit assignment preserve both. They are deliberately not scene-JSON ownership state; procedural owners reestablish them.
  - Existing settings/material/drawable clone, snapshot, retirement and exchange paths retain policy through texture copies. Filesystem cache identities distinguish policies rather than inheriting another source's ownership flags. Editor policy reads authored attributes/definitions, not shared or older realized settings.
  - `MaterialModifier::changesTextureSource(slot)` provides read-only patch introspection; low-level mutable C++ APIs remain unrestricted. UI helpers validate patch/clear intents against authored source locks. Picker/history/drag-drop/clear controls are not offered if any selected receiver's corresponding source is locked. Preview, UVs, channels, inversions, color and sampling controls remain usable.
  - Both mutation-owner intents and standalone queued editor edits validate all source edits before publication. Standalone queued edits resolve/preflight all valid receivers before the first mutation. Composer checks the actual live/definition receivers before private preparation and retains its existing all-target atomic publication. Rejected locked-source edits identify the slot and do not partially apply to other selected targets.
  - Full material paste and prepared whole-material replacement preserve each receiver's locked source, ownership flags and retained native pixels while allowing material values/UVs and incoming sampling to change. History snapshots/exchanges preserve this state. Composer also captures/rechecks policy independently of material revisions, so delayed commands cannot miss a policy-only change.
  - Sampling edits clone authored settings to preserve policy and apply only sampler/color fields; opaque callbacks cannot smuggle a source/metadata replacement through the sampling path. Added typed Address W and min/max LOD edits/controls alongside existing filtering/addressing/mipmap/color/bias controls.
  - Procedural snapshots now retain authored color interpretation instead of overwriting a sampling edit with the generator's native encoding. Procedural constructors/source assignment establish native encoding initially, preserving the previous default realized interpretation; subsequent color edits survive cloning/reload.
  - Existing material JSON schema is reused: `<textureKey>Settings`, UV and scale fields remain even when a source is ignored; the source key and resource-list entry are omitted. Reference-only and self-contained model/scene resource exports skip ignored textures. The self-contained snapshot skips before procedural cloning, image copying or GPU readback/writing.
  - Settings-only records restore sampling onto a preexisting source supplied in the deserialization result (documented in `MaterialSerializer.hpp`); no invalid empty-path texture is created. UV/channel controls parse independently of source presence. Future procedural components must seed their regenerated texture before deserializing material sampling. Metalness/roughness inversion values are now also written for round-trip consistency.
  - No text/font classes, assets, dependency integration, CMake changes, tests, harnesses or sample executables were added. No step-1 ray-tracing changes were made.
- Source review performed (not a build): traced texture revisions, copying/snapshots, cache ownership, sampling reload paths, standalone UI queues, Composer patch/paste/prepared commands and replay, serializer/resource collection and both export modes. `git diff --check` passed. No configure/build/application/automated runtime checks were run. `skills.md` remains absent.
- Composer manual checks to communicate to the user: existing texture replacement/clear, UV and sampling editing should remain available for ordinary materials; no special program or fixture is required. **Successful user compilation of both the library and Composer is sufficient acceptance for this milestone.**
- What cannot yet be checked in ordinary Composer authoring: no generated text albedo or component-owned locked source exists yet. The integrated locked-source/export behavior becomes observable after TextComponent and its authoring integration (steps 5–7); no synthetic demo was added.
- Known limits and unfinished work: user compilation pending; source-lock behavior is enforced only by supported editor mutations, not arbitrary mutable C++ calls. Sampling-only JSON needs an owner-supplied source to restore texture settings; a generic material without such an owner intentionally cannot reconstruct omitted pixels. Existing bake-mode/UV validation and resource-export boundaries are unchanged. No per-frame source repair is introduced.
- User-reported result and acceptance evidence: after the pre-existing Clang warning follow-up, the user explicitly said "el paso 2 es correcto" and requested the next milestone. No outstanding step-2 failure was reported.
- Next milestone and its purpose: step 3 independently vendors pinned MIT-option stb_truetype, verifies/reuses or safely adds the canonical Didact Gothic Regular font with its full OFL notice/provenance/checksum, and adds CPU font objects plus scene-wide declaration/loading/registry lifecycle. Composer font authoring controls remain a later milestone.
- Exact next action / file to resume: await user build/feedback and fix step-2 failures first. On acceptance/continuation, reread this progress file and `step-03-font-resources.md`; begin by inspecting `assets/DidactGothic-Regular.ttf` provenance and current scene initialization/resource preparation code. Do not overwrite the existing font or configure/build anything.

### Follow-up — Pre-existing Clang typeid warning

- User feedback: reported a pre-existing `-Wpotentially-evaluated-expression` warning in `MaterialInteraction.cpp:227`, not a step-2 regression. No explicit step-2 acceptance or request to advance was supplied.
- Changed files: `apps/bg2e_composer/src/command/material/MaterialInteraction.cpp`, this progress file.
- Fix: obtain a raw pointer from the retained shared pointer before the condition and use `typeid(*instance)`, matching the existing material-command pattern. The null short-circuit and exact dynamic-type check remain unchanged; no warning suppression or behavior change.
- Verification: source/diff review only; no build or runtime checks performed. No new Composer behavior to check; the user can confirm the warning disappears on recompilation.
- Status/next action: step 2 remains implemented awaiting acceptance. Await user feedback/continuation; do not start step 3 automatically.

### Step 3 — Font resources, dependency and licensed asset

- Status: accepted. The user reported successful compilation and execution and requested step 4.
- Changed files (step 3 only; prior uncommitted step-2/warning changes were preserved):
  - `lib/third_party/stb_truetype/stb_truetype.h`
  - `lib/third_party/stb_truetype/README.md`
  - `lib/cmake/deps.cmake`
  - `assets/fonts/didact-gothic/DidactGothic-Regular.ttf`
  - `assets/fonts/didact-gothic/OFL.txt`
  - `assets/fonts/didact-gothic/README.md`
  - `assets/licenses/stb_truetype-MIT.txt`
  - `lib/include/bg2e/base/TextFont.hpp`
  - `lib/src/bg2e/base/TextFont.cpp`
  - `lib/include/bg2e/base/all.hpp`
  - `lib/include/bg2e/scene/TextFontComponent.hpp`
  - `lib/src/bg2e/scene/TextFontComponent.cpp`
  - `lib/src/bg2e/scene/SceneTextFonts.cpp`
  - `lib/include/bg2e/scene/Scene.hpp`
  - `lib/src/bg2e/scene/Scene.cpp`
  - `lib/include/bg2e/scene/Node.hpp`
  - `lib/src/bg2e/scene/Node.cpp`
  - `lib/include/bg2e/scene/all.hpp`
  - `apps/bg2e_composer/src/AppDelegate.hpp`
  - `apps/bg2e_composer/src/DocumentIo.cpp`
  - `apps/bg2e_composer/src/StageScene.cpp`
  - `plans/text-components/summary.md`
  - `plans/text-components/progress.md`
- Dependency/license/provenance:
  - Unmodified independent stb_truetype v1.26, pinned `nothings/stb` revision `6e9f34d5429cf16790ec43c9bac3f1ee4ad1f760`; header SHA-256 `ecd30b05e0dd4fea3a13c26810dd9e1992dc379049482c393d5a19e6b5090aab`. Source URL and license choice are recorded in the adjacent README. The complete dual-license notice remains in the header; the project explicitly selects MIT. The MIT notice is also in the shipped asset tree.
  - `TextFont.cpp` is the only repository-owned TU defining `STB_TRUETYPE_IMPLEMENTATION` for this dependency; `STBTT_STATIC` keeps entry points private and independent of ImGui. A narrowly scoped vendor-include diagnostic push handles unused static entry points. No stb types occur in public engine headers and no Fontstash is added.
  - Canonical font pinned to `google/fonts` revision `c741144416d5635d5de832dff8ba09292c112036`, source `ofl/didactgothic/DidactGothic-Regular.ttf`; SHA-256 `b98f9e091b6337a86f4e0f69c94c31905e1e287782853f472e176b4e28d3f81f`. Full OFL 1.1 notice and pinned URLs/checksum are adjacent to the canonical font.
  - Existing `assets/DidactGothic-Regular.ttf` came from repository commit `2c384266`; SHA-256 `b41981e61095049122f77e334758fa635d11195814e1130efe4b7d810445d741`. Byte comparison with the pinned upstream original failed. Its precise upstream revision was not established, so it remains untouched and a distinct, documented canonical asset was added as authorized.
  - Existing recursive asset bundling, Distribution install and standalone-project asset copying cover `assets/fonts/` and `assets/licenses/`. Only a private stb include directory was added to CMake; source glob discovery handles new implementations. No build tree was regenerated.
- Delivered behavior and API decisions:
  - `base::TextFont::load(path, faceIndex)` returns an immutable CPU resource owning font bytes and opaque stb state. Exposes unscaled metrics, scalar-to-glyph lookup, glyph bounds/advance, kerning, pixel scale and alpha glyph bitmap rasterization. Glyph 0 is the missing-codepoint result; no automatic font fallback or shaping is introduced.
  - Basic container/table bounds, file I/O, Unicode scalar/glyph indices, pixel sizes and subpixel offsets are checked. Limits: 64 MiB per file, at most 32 collection faces, 4096 pixels per glyph bitmap dimension. **These checks do not make stb safe for untrusted fonts; trusted project fonts only.** The same limitation is documented publicly and with the dependency/font asset.
  - `scene::TextFontComponent` is a declaration collection, factory type `TextFonts`, with `{identifier, path, faceIndex}` entries and immutable read access plus validated set/add/remove methods. Identifiers use 1–128 ASCII letters/digits or `_`, `-`, `.`, `:`; paths are bounded to 16 KiB and reject NUL. Each collection and the entire scene allow at most 128 declarations. Relative input paths resolve at the component resource-reference boundary; serialization emits lexical relative paths without reading font files. Deserialization does not load files.
  - Detached clones have independent declaration state and preserve component priority/serialization policy; they own no registry/font parser state. Public base/scene module includes expose the new APIs.
  - `Scene::prepareTextFonts()` gathers every declaration before loading any font, including disabled nodes and later siblings. Identifiers belong to one scene-wide namespace without ancestor shadowing. Duplicates identify both owner nodes; unresolved identifiers identify the missing name. Total registered source bytes are capped at 128 MiB.
  - Preparation stages all resources privately and publishes complete availability outcomes; unchanged path/face declarations reuse immutable fonts. Under the later mandatory recovery correction, failed resources cache unavailable/null outcomes, not exceptions; unaffected fonts remain available. Declaration/graph edits invalidate; explicit reload rereads files. No global font registry or per-frame file I/O/retry loop exists.
  - `textFont()`, `textFontsReady()` and `textFontVersion()` are the consumer contract. Dirty/failed registries do not resolve stale fonts. Previously retained shared resources remain valid immutable snapshots; future text consumers must track both scene identity and generation, particularly across scene transfer/undo.
  - Node attachment/removal/clear and atomic child placement invalidate affected scene registries; font component membership/edit hooks do likewise. Scene root replacement/release clears cached resources. `clearChildren()` now detaches retained children's parent pointers as needed to prevent stale scene ownership after removal.
  - `Node::loadResources()` performs scene-wide font preparation before any component realization. Detached nodes containing font declarations must be adopted by a Scene before resource loading. Graphics-free Scene deserialization remains graphics/file-loading free.
  - Composer's synchronous candidate loader temporarily supplies scene ownership. Its incremental loader retains a private resource Scene across component loads and prepares fonts before the first one; adoption/compensation prepares the active scene registry. These are resource-ordering changes only, not font authoring controls.
- Source review performed (not a build): inspected declaration parsing/factory/module discovery, node/root/component membership paths, registry staging/failure/reload semantics, synchronous and incremental Composer loading, private stb integration, and existing asset distribution rules. Downloaded pinned upstream files, compared font bytes and recorded SHA-256 hashes; verified the original root-level font hash is unchanged. `git diff --check` passed. No CMake configure, build, application launch, test executable or automated runtime check was performed or added. `skills.md` remains absent.
- Composer handoff: **successful user compilation of both the library and Composer is sufficient acceptance for this milestone**. No dedicated scene, sample or manual runtime check is required. Font authoring controls arrive in step 7, so ordinary Composer use cannot yet create/edit these declarations or render text.
- Known limits/unfinished work: user build pending; trusted static project fonts only; no discovery/fallback chains/live watching/shaping/bidi. Glyph rasterization is a low-level font operation, not paragraph layout or a procedural text bitmap yet. Font-file consolidation when saving self-contained scenes remains step 6. Node-only resource loading of detached font-bearing subtrees requires an explicit temporary/owning Scene. The current Composer candidate/adoption phases may load a font in two different scene registries; this is bounded resource-boundary work, not a per-frame retry.
- User-reported result and acceptance evidence: "compilación y ejecución correcta, implementa el siguiente paso". This accepts step 3; no outstanding failure was reported. The agent did not perform those checks.
- Next milestone and its purpose: step 4 adds `base::Text` CPU paragraph data with Light-style JSON serialization and the procedural UTF-8 paragraph rasterizer (white RGB, glyph coverage alpha). No drawable TextComponent or Composer text controls are part of that next milestone.
- Exact next action/file to resume: await user compilation/feedback and fix step-3 issues before advancing. On acceptance/continuation, reread this record and `step-04-text-rasterization.md`, then start with `lib/include/bg2e/base/TextFont.hpp`, `lib/include/bg2e/base/Texture.hpp` and `lib/include/bg2e/base/Light.hpp`. Do not compile or launch applications.

### Step 4 — Text data and procedural paragraph rasterization

- Status: accepted by continuation after the recovery correction. The user requested the next milestone without reporting a failure; no agent build/runtime check is implied.
- Changed files (step 4 only; existing uncommitted work from earlier milestones is preserved):
  - `lib/include/bg2e/base/Text.hpp`
  - `lib/src/bg2e/base/Text.cpp`
  - `lib/src/bg2e/base/TextUtf8.hpp` (private decoder shared by normalization/layout)
  - `lib/include/bg2e/base/TextRasterizer.hpp`
  - `lib/src/bg2e/base/TextRasterizer.cpp`
  - `lib/include/bg2e/base/TextTextureGenerator.hpp`
  - `lib/src/bg2e/base/TextTextureGenerator.cpp`
  - `lib/include/bg2e/base/TextFont.hpp`
  - `lib/src/bg2e/base/TextFont.cpp`
  - `lib/include/bg2e/base/Texture.hpp`
  - `lib/include/bg2e/base/all.hpp`
  - `plans/text-components/summary.md`
  - `plans/text-components/progress.md`
- Data/API contract and defaults:
  - `base::Text` is CPU paragraph state only; no color, unlit, material, GPU or scene state. Exposes validated individual setters, atomic `setParameters(Parameters)`, value equality and authored revision tracking. Copy assignment compares values rather than importing another object's revision. No-op setters/deserialization preserve the revision.
  - Matches Light's `deserialize(shared_ptr<JsonNode>)` and `serialize()` signatures. Missing/wrongly typed optional fields leave existing data unchanged. Under the later mandatory recovery correction, invalid numeric ranges/enums recover to defaults/safe limits with warnings; no text exception escapes. Parsing never loads a file, queries a scene or creates GPU resources.
  - JSON keys: `content`, `fontIdentifier`, `fontSize`, `lineSpacing`, `characterSpacing`, `paragraphSize` (`width`, `height`), `bitmapResolution` (`width`, `height`), `margins` (`left`, `top`, `right`, `bottom`), `alignment`, `verticalAlignment`, `wrapMode`, `overflowPolicy`. Geometry/layout and sampling resolution are separate serialized values.
  - Defaults: empty content; explicit font identifier `default` (must be registered, never an implicit fallback); 512×256 logical paragraph; 1024×512 RGBA bitmap; logical font height 32; line spacing 1.2; additional character spacing 0; margins 8 on all sides; left/top alignment; word wrapping; clip overflow.
  - Logical units are independent of world units. `fontSize` is ascent-to-descent height in logical units. Baseline advance is `(ascent - descent + max(lineGap, 0)) * logicalScale * lineSpacing`. Character spacing adds logical advance between adjacent glyphs. Physical quad width/height and pivot remain component-owned step-5 work; default aspect must follow logical paragraph dimensions, not bitmap dimensions.
  - Resolution affects only horizontal/vertical pixel densities, not layout measurements or line breaks. Nonuniform bitmap aspect is supported by independent glyph raster scales. Coupled box/margin/resolution edits can use atomic Parameters replacement rather than invalid intermediate setter states.
  - Limits: 64 KiB normalized UTF-8; logical box dimensions finite, positive and at most 1,000,000; font size [1,4096]; line spacing [0.1,10]; finite character spacing [-4096,4096]; nonnegative margins leaving a positive interior; bitmap axes [1,8192], at most 16 Mi pixels (64 MiB RGBA). Font sampling density must stay within the 4096-pixel font scale limit; actual glyph bitmap dimension limits also remain enforced.
- Layout/rasterization decisions:
  - `TextRasterizer::layout(text, resolvedFont)` returns logical glyph origins/baselines, lines, content bounds and overflow state. `rasterize()` returns the same layout and an RGBA8 bitmap. The immutable font is supplied explicitly; neither API resolves identifiers, accesses files or creates GPU resources.
  - Scalar UTF-8 LTR layout supports Latin/Spanish precomposed characters when present in the chosen font, native glyph advances and kerning. Invalid UTF-8 consumes one byte per replacement and is normalized to U+FFFD when authored, keeping serialized content valid UTF-8. Missing font glyphs use glyph 0 (.notdef), with no font fallback.
  - CR, LF and CRLF are explicit line breaks; tabs expand to four spaces, not tab stops. Other C0/C1 control scalars render as U+FFFD. Word wrapping uses ASCII spaces/tabs and falls back to glyph breaks for oversized words; character and no-wrap modes are also available. Trailing breakable spaces are trimmed from visual lines. Explicit leading spaces are otherwise retained; automatic breaks discard the preceding line's trailing separator. Empty content produces zero lines and a transparent bitmap; explicit trailing line breaks retain their final empty line.
  - Horizontal left/center/right and vertical top/center/bottom align against the margin interior. Measurements account for advances, negative bearings and actual glyph ink bounds as well as font ascent/descent, so normal baselines/descenders are not accidentally cropped. Low line-spacing/negative character-spacing overlap is supported.
  - Overflow `clip` clips ink with a warning; `hide` returns transparent output with a warning. Legacy JSON `error` and C++ `OverflowPolicy::Error` are aliases for hide, never exceptions. Logical layout/world size never auto-expands. Safety-only resolution reduction is permitted for invalid/excessive requests, not per-frame adaptive resizing. Full bitmap pixels inside margins are eligible, with integer-edge roundoff tolerance.
  - RGB remains 255 even in transparent texels; masks compose source-over alpha and signed intersections are clipped before indexing. Layout is bounded to 65,536 expanded glyphs/line breaks and raster work to 64 Mi glyph-mask pixels. Under the recovery correction, excessive layout is truncated with warnings and raster failure/work exhaustion returns transparent/unavailable output, never an escaping exception.
  - `TextFont::rasterizeGlyphScaled()` supplies independent X/Y pixels-per-font-unit scales and subpixel coverage, including subpixel-height glyphs. The existing isotropic API delegates to it and retains its input contract. stb implementation types remain private.
- Generator/resource decisions:
  - `TextTextureGenerator` owns an independent Text value, immutable resolved font and caller-supplied font version. `configure()` compares complete Text values plus font identity/version, coalesces no-ops and increments the existing procedural revision only for changes. Material-only changes are outside its state.
  - Repeated `generate()` calls reuse cached CPU layout/pixels but return a fresh `new uint8_t[]` buffer for Image's existing `delete[]` ownership contract. Failed rasterization is not published into the cache. Inherited dimension tampering is diagnosed; resolution is authored through Text/configure.
  - Image identifiers include an independent process-local generator namespace, current pixel revision and font version. All configured pixel-producing state contributes through revision changes; immutable font replacement/reload also changes it. Independent paragraphs and cloned generators cannot alias even when their local revisions match. These are deliberately runtime identifiers, not cross-process content hashes.
  - Copy construction/`clone()` create independent Text/cache/generator identity; only immutable fonts are shared. Future TextComponent cloning must use this independent generator path rather than copying a mutable shared pointer. Existing draw texture snapshots already freeze pixels independently.
  - `createTextTexture(generator)` initializes linear color interpretation, linear min/mag filtering, clamp-to-edge on all axes and mipmaps disabled. Later configure calls never reset texture sampling. The future component, not this CPU helper, establishes sourceEditable=false / ignoreSerialization=true and owns the one material/quad.
  - `Texture::generateImage()` now uses local RAII for the Image and new[] result so expected paragraph/overflow/allocation failures cannot leak intermediate CPU objects. Its external ownership/signature is unchanged.
- Source review performed (not a build): inspected Light/JsonNode contracts, font metrics/bitmap API, Image's delete[] ownership, UTF-8 normalization, logical-only wrapping/alignment, signed clipping and alpha composition, bounded allocation/work, generator revision/no-op/clone identity and procedural snapshot compatibility. `git diff --check` passed. No CMake configure, compilation, executable/application launch, automated runtime check, test, fixture or harness was performed or added. `skills.md` remains absent. No CMake, scene, Composer, rendering backend or shader files were changed for this milestone.
- Composer handoff: **successful user compilation of the library and Composer is sufficient acceptance**. No special scene, sample program or runtime observations are required. The data/rasterizer has no authoring UI or drawable component yet, so text cannot yet be created or inspected in ordinary Composer use.
- Known limits/unfinished work: user compilation pending; trusted fonts only; no shaping/bidi, combining positioning, grapheme-aware wrapping, emoji/color fonts, Unicode line-break rules, fallback chains, SDF/MSDF, GPU atlas or asynchronous rasterization. Safety limits recover with warned truncation/resolution repair/transparent output as recorded below. No physical quad, text material, coordinated GPU upload, embedded persistence or text editor exists yet.
- User-reported result and acceptance evidence: pending for step 4. The latest successful build/execution report applies to step 3 only.
- Next milestone and its purpose: step 5 adds `TextComponent : DrawableComponent`, exactly one procedural quad/submesh/material, logical-aspect world size/pivot, resolved-font/generator lifecycle, generated-albedo ownership flags, default double-sided alpha-tested material, coordinated GPU realization, detached independent clones and shared glyph-alpha shadow/visibility/full-rectangle picking integration. No Composer text authoring UI or completed embedded persistence yet.
- Exact next action/file to resume: await user build/feedback and fix step-4 issues first. On acceptance/continuation, reread this record and `step-05-text-component.md`, then inspect `lib/include/bg2e/scene/DrawableComponent.hpp`, its implementation, current draw loading/mutation boundaries and `TextTextureGenerator.hpp` before adding TextComponent. Preserve all preceding uncommitted changes; do not compile or launch applications.

### Cross-cutting correction — Recoverable errors never interrupt text/scene use

- Status: implemented, awaiting user review/build feedback. This is a correction to implemented steps 2–4, not acceptance of step 4 or permission to start step 5. Earlier acceptance remains historical; this revised behavior has not been user-verified.
- User requirement: invalid/recoverable parameters must use defaults or safe recovery. If text cannot render, emit a console warning and continue; never generate/propagate a recoverable text exception capable of interrupting the program. The entire remaining plan must obey this criterion.
- Changed implementation files:
  - `lib/src/bg2e/base/TextDiagnostics.hpp` (new private nonthrowing console-warning helper)
  - `lib/include/bg2e/base/Text.hpp`, `lib/src/bg2e/base/Text.cpp`
  - `lib/include/bg2e/base/TextFont.hpp`, `lib/src/bg2e/base/TextFont.cpp`
  - `lib/include/bg2e/base/TextRasterizer.hpp`, `lib/src/bg2e/base/TextRasterizer.cpp`
  - `lib/include/bg2e/base/TextTextureGenerator.hpp`, `lib/src/bg2e/base/TextTextureGenerator.cpp`
  - `lib/include/bg2e/base/Texture.hpp`
  - `lib/include/bg2e/scene/TextFontComponent.hpp`, `lib/src/bg2e/scene/TextFontComponent.cpp`
  - `lib/include/bg2e/scene/Scene.hpp`, `lib/src/bg2e/scene/SceneTextFonts.cpp`
  - `lib/include/bg2e/ui/DrawMaterialEditor.hpp`, `lib/src/bg2e/ui/DrawMaterialEditor.cpp`
  - `lib/include/bg2e/utils/MaterialModifier.hpp`, `lib/src/bg2e/utils/MaterialModifier.cpp`
  - `apps/bg2e_composer/src/command/material/MaterialCommands.cpp`
- Plan changes: added `recovery-policy.md`, linked it from `summary.md` and all eight milestone files, revised contradictory requirements, and added future component/persistence/UI/tutorial recovery obligations and a manual Composer fault checklist. This record supersedes historical exception-based recovery decisions, even where earlier delivered-code notes describe the former behavior.
- Implemented recovery:
  - Text repairs invalid enums/identifiers, sizes, spacing, margins and bitmap axes using defaults/clamps; invalid nonpositive dimensions/font heights/spacing use defaults. Logical dimensions are now bounded to [1,1,000,000]. Margins that consume an axis become zero on that axis. Excessive bitmap pixel/scale requests reduce resolution while keeping logical layout/world size unchanged. Oversized/invalid UTF-8 normalizes and truncates at scalar boundaries; warnings explain repairs. Allocation failure in edits preserves prior state.
  - `TextFont::load()` is nonthrowing and returns null with a warning for missing/unreadable/unsupported/invalid files or allocation failures. Invalid face indices use face 0 where possible (the resource reports the actual selected face). Invalid Unicode/glyph indices use replacement/.notdef; unsafe scales/offsets use defaults/limits. `GlyphBitmap::valid` distinguishes a failed glyph raster from an empty whitespace glyph.
  - Font declarations skip malformed entries, normalize face indices, cap collections and warn for duplicate/invalid entries. Detached resources warn/defer rather than throwing. Serialization/clone failures return unavailable status with warnings. Registry preparation remains transactional but publishes all availability outcomes: valid fonts work, failed ones remain null, cross-node duplicate identifiers are disabled, excess resources are omitted. Resolution returns null and deduplicates warnings by generation (bounded diagnostic keys); no stale handles or rethrown cached exceptions. Failed file reads remain cached until source change/explicit reload; ambiguity removal and resource-budget relief can recover on registry change.
  - `TextLayout::renderable` signals transparent/unavailable results. Layout/raster entry points contain unexpected exceptions. Expanded layout limits truncate with warnings; overflow clips or hides with warnings. JSON `hide` is canonical; legacy `error` / `OverflowPolicy::Error` remains compatible as hide-and-warn. Raster failure/work-budget exhaustion clears coverage rather than publishing a partial paragraph. Allocation failure may return an empty raster result, which callers must check.
  - The generator accepts a null font and caches transparent pixels, so a missing font cannot abort a scene or repeatedly rasterize/log every frame. Invalid independent dimensions restore authored dimensions. Raster/allocation failure uses a warned transparent 1×1 fallback, cached until revision changes; the last-resort return is nullptr if even four bytes cannot be allocated. `Texture::generateImage()` reads actual dimensions after generation and handles unavailable pixels without dereference. No stale glyph pixels are published as a successful fallback.
  - Source-lock and invalid edit-slot validation now returns false plus console warnings, not exceptions. Standalone UI queues still preflight all targets before mutation; Composer discards complete private preparations on rejection, retaining all selected live sources. Other generic engine/material/backend invariants were not globally refactored; the future TextComponent must contain their failures at its text boundary per the revised plan.
- Source review: searched text/font implementation and scene font preparation for explicit throw/rethrow paths; none remain. Reviewed nullable consumers, fallback dimensions/ownership, enum compatibility, no-op/cached unavailable behavior and all source-validator callers. `git diff --check` passed. No build, configure, application launch, runtime checks, automated tests, fixtures or harnesses were performed/added.
- Composer handoff: ordinary materials remain editable; locked-source rejection is a nonfatal warning/no-op. Full text-failure observations are still unavailable without TextComponent/authoring; successful user library/Composer compilation is sufficient for this correction now. The step-7 checklist includes missing fonts, duplicates, invalid parameters and overflow recovery without warning floods or interrupted rendering.
- Known limits: nullable/empty results must be checked by callers (including future component, cloning and save paths). Native parser safety for malicious fonts is not guaranteed; trusted project fonts only. Catastrophic OS/native faults are not made recoverable by C++ warning handling. No new authoring UI/component, save-resource packaging, backend recovery refactor or new milestone was implemented.
- Exact next action: wait for user review/build feedback on this correction. Fix reported recovery/build problems before advancing; only after acceptance/continuation read `recovery-policy.md` and `step-05-text-component.md` and begin TextComponent with explicit null/status checks and contained backend operations.

### Step 5 — TextComponent and procedural drawable lifecycle

- Status: accepted. After implementation, the user explicitly reported successful compilation and requested a commit. No runtime observation is implied by that compilation report.
- Changed files (step 5 only; preceding uncommitted changes preserved):
  - `lib/include/bg2e/scene/DrawableComponent.hpp`
  - `lib/include/bg2e/scene/TextComponent.hpp`
  - `lib/src/bg2e/scene/TextComponent.cpp`
  - `lib/include/bg2e/scene/all.hpp`
  - `lib/include/bg2e/draw/DefaultRenderLoopDelegate.hpp`
  - `lib/src/bg2e/draw/DefaultRenderLoopDelegate.cpp`
  - `lib/include/bg2e/draw/RenderQueueVisitor.hpp`
  - `lib/src/bg2e/draw/RenderQueueVisitor.cpp`
  - `lib/src/bg2e/draw/RenderQueue.cpp`
  - `lib/include/bg2e/draw/ShaderData.hpp`
  - `lib/src/bg2e/manipulation/DrawPicking.cpp`
  - `lib/include/bg2e/draw/pipeline/SoftShadowPass.hpp`
  - `lib/src/bg2e/draw/pipeline/SoftShadowPass.cpp`
  - `lib/src/bg2e/draw/pipeline/TracingSceneResources.cpp`
  - `shaders/src/draw/query/soft_shadows.comp.glsl`
  - `shaders/src/draw/query/soft_shadows.comp.metal`
  - `shaders/src/draw/lib/tracing.glsl`
  - `shaders/src/draw/lib/tracing.metal`
  - `lib/include/bg2e/db/scene.hpp`
  - `lib/src/bg2e/db/scene.cpp`
  - `lib/src/bg2e/db/draw_resource_io.cpp`
  - `plans/text-components/summary.md`
  - `plans/text-components/progress.md`
  - `plans/text-components/step-06-scene-persistence.md`
- Component/API and geometry decisions:
  - `DrawableComponent::requiresBg2Export() const` is virtual, read-only, default true. `TextComponent` overrides false and retains ordinary authored scene geometry status (not an overlay). Factory type is `Text`, exported through `scene/all.hpp`.
  - Owns base::Text, an independent text generator, one conventional material and one quad/submesh: four vertices, six indices. XY plane, +Y up, front normal -Z, +X tangent, matching +V downward bitangent; both UV sets cover top-left (0,0) to bottom-right (1,1). Winding and normals agree; existing double-sided shaders orient the back side.
  - Default world height is 1, width follows logical paragraph aspect (initially 2), pivot is normalized (0.5,0.5) from bottom-left. `setSize()` selects manual world dimensions; `setHeight()` and `setPreserveAspect()` support aspect-preserving sizing; `setPivot()` validates/clamps. Bitmap resolution never sets world size. Positive world dimensions are repaired/clamped to [0.001,1,000,000]; oversized derived aspect scales both dimensions together to preserve aspect.
  - Mutable `text()` exposes CPU data only; `setText()` is a value convenience. `material()` exposes conventional material attributes, not Text appearance fields. `setMaterial()` stages an owned-source-preserving CPU replacement. Direct model-backed source/drawable conveniences are hidden from TextComponent's direct interface, not claimed safe against base casts/arbitrary mutable API use.
  - New text defaults: white conventional albedo, doubleSided=true, alphaCutoff=0.1, ordinary opaque/solid/lit defaults. Generated albedo uses TextTextureGenerator, sourceEditable=false and ignoreSerialization=true. Initial sampling remains linear/clamp, no mipmaps; regeneration retains user UV/color/filter/address/mipmap/LOD choices and all other material values/textures.
- Lifecycle/resource publication:
  - `prepareResources(engine)` / static `prepareResources(root, engine)` are explicit stopped mutation boundaries. DefaultRenderLoopDelegate calls the subtree path after queued edits/reconfiguration and before frame acquisition/renderer preparation, then invalidates histories/requests scene work if a text version changed. Direct/custom render-loop users must call it at the same coordinated boundary.
  - Scene-wide font preparation precedes text resolution. Text tracks CPU revision, geometry values, scene/font generation and immutable font identity, engine and explicit edit/retry version. This handles declaration changes, removal, root transfers and coincident generation values across different Scene instances. No files, rasterization or GPU updates run in update/draw traversal.
  - Coalesces edits into the latest version per preparation. Unchanged calls do not rasterize/upload/drain/rebuild geometry. Pixel-only changes replace albedo without rebuilding vertex/index buffers or BLAS; size/pivot changes use prepared geometry/BLAS and preserve CPU mesh/drawable/material identities. Ordinary CPU typing/animated labels keep the last complete resource version until publication, not a perpetually blank dirty frame.
  - Pending material/source/geometry states are prepared privately and validated before exchange. ResourceUpdateScope and a final resource-update drain protect discarded submitted resources. Loaded material state is exchanged into its stable live identity; geometry contents are exchanged without changing CPU mesh identity, retaining selection IDs for pixel-only edits.
  - Incoming whole-material albedo is sampling metadata only; it is never evaluated/read as a replacement source. Full replacement preserves the owned mask and incoming appearance/UV/sampling, or retains prior sampling when no incoming albedo exists. No per-frame detect-and-repair source loop is added.
  - Empty/whitespace, failed, clipped-away or missing-font masks produce no raster/traced geometry contribution, even if a material disables cutoff; prepared quads remain available for full-rectangle picking. Font invalidation suppresses stale glyphs/shadows until the new resource boundary. All TextComponent resource/clone/parse failures warn and contain lower-level exceptions; failed attempts are cached until source/material change or `retryResources()`.
  - `update()` only requests future scene work when dirty. `releaseResources()` explicitly drains the old engine and keeps independent CPU data before context replacement/teardown; changing engines without release warns/omits instead of destroying another context's in-flight owners. Renderer-first synchronized cleanup and normal Drawable ownership conventions still apply; applications stop background producers for the whole mutation scope.
  - Detached clones retain independent Text, generator/cache, CPU geometry/material settings and component policy. Loaded clones duplicate native geometry/material images/BLAS via existing clone paths. GPU clone failure retains an independent unrealized CPU copy for later preparation; an allocation failure can return null with a warning. Clones re-resolve the owning scene's fonts on preparation rather than draw stale cached glyphs.
- Extraction/picking/shadow audit:
  - RenderQueueVisitor recognizes TextComponent through DrawableComponent polymorphism and contains text-specific extraction failures. No resource repair/upload occurs there. Lower-level queue publication rolls back its first append if routing allocation fails, preventing a phantom ray-only text item after a contained failure.
  - Picking extraction explicitly retains prepared text rectangles even when the bitmap has no coverage. Existing picking shaders still draw all six indices and do not sample alpha. Material double-sided flags select the ordinary shared cull variants; selection/gizmo code already resolves DrawableComponent subclasses and stable mesh/material identities. No multiple-drawable-per-node redesign was introduced.
  - Audited GBuffer/temporal alpha discard, soft-shadow GLSL/Metal alpha sampling, nonopaque tracing instances, query/any-hit GI/reflection acceptance and existing directional/Sun light dispatch. The ordinary default cutoff enables the shared glyph-alpha path; no tutorial-only shadow implementation is required. User changes that intentionally disable alpha testing retain ordinary material semantics.
  - Text queue items annotate the existing reserved `MaterialData.emissionAlpha.z` as an essential alpha source (not a new authored appearance field). Soft-shadow metadata uses reserved options.w likewise. If a tracing/shadow descriptor capacity omits that mask, the shaders omit that text visibility contribution instead of accepting a rectangular blocker; existing capacity diagnostics now explain this. Ordinary non-text fallback semantics are unchanged. Raster/temporal pipelines and draw/gpu shader layout sizes are unchanged.
- Persistence boundary/safety:
  - Embedded material/persistence/resource consolidation remains step 6. TextComponent can read basic CPU `text`, `size`, `preserveAspect`, `pivot` representation, but its serializer deliberately returns unavailable with a warning until the complete persistence contract exists.
  - To prevent silent data loss or accidental baking during this intermediate milestone, engine scene/model save paths inspect the read-only export query before text snapshot/BG2 generation and refuse authored-text saves with warning/status. Reference saves preflight before writing; `saveSceneReferences()` now returns bool/noexcept, containing serialization/callback/write failures (engine complete export checks its status). Text-save refusal leaves existing files/live paths unchanged; complete exports retain staged rollback. Successful ordinary model saves are unchanged; this does not redesign the existing raw reference writer into a transactional filesystem API. This is a temporary guard, not step-6 persistence implementation.
- Source review performed (not a build): inspected geometry winding/UV/tangent conventions, component/factory/clone ownership, font-first and delegate mutation ordering, no-op checks, stable selection identities, picking subclass paths, alpha-test packing/queue/temporal/shadow/query/any-hit consumers for both shader backends, capacity fallback and save rollback guards. `git diff --check` passed. No configure/build, shader compilation, application launch, automated runtime check, test/fixture/harness or temporary example was performed/added. No CMake, gpu implementation or UI authoring files were changed in this milestone.
- Composer handoff: **user compilation of the library and Composer (including changed shaders) is sufficient acceptance**. Font/text authoring controls do not exist yet. Integrated visual lighting, glyph shadows, backface/empty-rectangle picking, regeneration, duplication and recovery observations become available in step 7; none have been agent-verified.
- Known limits: build/backend/runtime validation pending; ray shadows/reflections require existing backend/device support. Current generic Composer component-history/type whitelists and text controls are not integrated until step 7. Authored-text saving is intentionally unavailable until step 6. Catastrophic clone/allocation results remain nullable and must be checked; arbitrary base-pointer/mutable-API corruption is not guarded. Direct renderers/custom loops must call the explicit stopped preparation API. External scene/resource owners must release before engine teardown.
- User-reported acceptance evidence: "la compilación es correcta. Haz un commit con los cambios". Compilation is sufficient acceptance for step 5 under the execution contract. The agent did not compile or run applications.
- Exact next action/file to resume: await user compilation/feedback and fix step-5 build/recovery problems first. On acceptance/continuation, read recovery-policy.md and step-06-scene-persistence.md, then implement TextComponent's embedded material serialization/deserialization and resource collection. Replace temporary save refusals in `lib/src/bg2e/db/scene.cpp` and `draw_resource_io.cpp` with correct no-BG2/no-albedo-pixel export while preserving boolean statuses, rollback and all authored material/UV/sampling data.

### Commit handoff

- User authorized committing the pending text work after reporting correct step-5 compilation. Double-sided materials are already recorded in commit `a8001d3b`; the new commit contains pending steps 2–5, the warning/recovery correction, dependency/font assets and implementation progress.
- No build/runtime/test checks or push are authorized/performed by this commit request. Source review and `git diff --check` only.
- Steps 1–5 are now accepted. Stop after committing and await a separate instruction before implementing step 6. Start the next implementation with recovery-policy.md, step-06-scene-persistence.md, TextComponent serializers and the temporary DB save guards.

### Step 6 — Editable persistence and resource packaging

- Status: accepted by continuation. The user requested step 7 without reporting a step-6 failure; no agent build/runtime verification is implied.
- Changed files:
  - `lib/include/bg2e/scene/TextComponent.hpp`
  - `lib/src/bg2e/scene/TextComponent.cpp`
  - `lib/include/bg2e/scene/TextFontComponent.hpp`
  - `lib/src/bg2e/scene/TextFontComponent.cpp`
  - `lib/src/bg2e/scene/Node.cpp`
  - `lib/include/bg2e/db/scene.hpp`
  - `lib/src/bg2e/db/scene.cpp`
  - `lib/src/bg2e/db/draw_resource_io.cpp`
  - `apps/bg2e_composer/src/StageScene.cpp`
  - `plans/text-components/summary.md`
  - `plans/text-components/progress.md`
- Schema and resource decisions:
  - Factory type `Text` embeds `text` (the existing base::Text object) and `material` (the existing MaterialSerializer PBR object), plus common priority, authored world `size`, `preserveAspect` and normalized `pivot`. No nested JSON strings or model source path. Stored size is the authored pair, not the aspect-derived display size, preserving subsequent aspect/manual editing behavior.
  - Deserialization is representation-only. Incoming `material.albedoTexture` is removed before conventional material parsing, even when malicious or nonexistent. A fresh CPU procedural source seeds sampling restoration; its sourceEditable=false/ignoreSerialization=true permissions come from component logic. Albedo color, UV set/scale and all conventional sampling fields remain material-owned. Invalid material sampler enums and indirect mode use previous/default values with warnings; unexpected parser/allocation failures are contained and preserve the paragraph and recovered/default material.
  - Serializers always omit the albedo source irrespective of mutable source flags, retaining its settings/UV fields. Other material filesystem sources use lexical paths relative to the output directory. Nonignored non-albedo procedural images need complete saving; reference-only serialization returns unavailable instead of emitting an empty/stale reference.
  - `TextFontComponent::Declaration::notices` is an optional bounded collection (32 paths) of supplied license/provenance files. JSON stores a `notices` array alongside identifier/path/faceIndex; paths resolve relative to the document and survive cloning, reference saves and reopening. It carries no rendering state or legal permission claim.
  - Complete saves inspect requiresBg2Export before model snapshots/texture collection/path changes. Text uses only embedded serialization: no generated text BG2, quad snapshot, albedo pixel generation/readback or albedo sidecar. Other embedded filesystem images are copied without decoding or changing extensions; procedural non-albedo images are encoded to PNG from CPU pixels. Temporary material substitutions use noexcept move/swap rollback, restoring authored material fields, source paths, flags and revisions on both success and failure.
  - Fonts and supplied notices are copied byte-for-byte. When no notices are explicit, adjacent OFL.txt, LICENSE/LICENSE.txt/LICENSE.md, COPYING, NOTICE/NOTICE.txt and README.md are recognized. This includes the canonical Didact Gothic font's full OFL and pinned provenance README. Explicit notice arrays retain those associations after packaged filenames change and across repeated saves. Missing explicit files fail the complete save; no discovered notices warns the caller to supply necessary notices/verify rights, without inventing a license.
  - Embedded images/fonts/notices deduplicate by normalized canonical source path, not basename. Sidecars use the existing unique generation/sequence naming with original file extensions, beside the document; distinct source files named alike cannot overwrite each other. Existing legacy model-image basename sharing remains unchanged.
- Publication, loading and subclass audit:
  - Removed temporary authored-text scene-save refusals. `saveSceneReferences()` keeps its bool/noexcept contract and now shares the transactional Publication implementation in draw_resource_io.cpp: serialize relative to the final target directory, write privately and atomically replace the document last, copying no resources. Failed serialization/callback/write/publication preserves any existing document.
  - Complete Save/Save As/copy-to-another-directory continue using the existing stopped Engine boundary, private staging, unique sidecar publication and main-document-last replacement. Failed file copies/publication roll back new sidecars; live model references and embedded materials are restored. Composer still commits savePath/history only after saveScene succeeds.
  - Node serialization propagates unavailable Text/TextFonts representations and failed children to the save owner, rather than silently dropping editable authored data. ignoreSerialization continues to omit the entire component independently of requiresBg2Export.
  - Composer model export previously wrapped every Drawable subclass in a plain DrawableComponent, losing the export policy. It now refuses procedural text before wrapping or publishing any selected batch model; direct storeDrawableBg2 already refuses it. No Composer authoring controls or command/history whitelist redesign were added.
  - Reviewed existing font-first loading: graphics-free graph parsing completes before Scene registry preparation and resource realization; synchronous Node loading and Composer's private incremental resource Scene both prepare declarations before the first text, including later siblings. Existing node-based synchronous progress and component-based incremental progress remain unchanged; text/font failures stay contained at their resource boundaries. Existing TextComponent clone/release paths retain independent editable data; no model conversion was added.
- Source review performed (not a build): reviewed conventional material JSON/settings parsing, source stripping, size/aspect round-trip, nullable serializers, node failure propagation, reference and complete save staging/rollback, font notices across reopening/resaving, path identities/extensions, Composer Save/Save As/model export and font-first synchronous/incremental loading. `git diff --check` passed. No CMake edits/configuration, compilation, shader compilation, application launch, automated runtime checks, tests, fixtures, harnesses or verification executables were performed/added. `skills.md` remains absent.
- Composer handoff: **successful user compilation of both the library and Composer is sufficient acceptance for this milestone**. Ordinary existing model saves should retain their behavior. No hand-authored JSON, special scene or sample is required. Text authoring, integrated save/reopen/material/selection observations are not yet available in ordinary Composer use; they arrive in step 7.
- Known limits: no agent build/runtime validation. Trusted fonts only. Complete saving correctly fails if an authored font, supplied notice or required material resource cannot be copied; reference-only saving can retain unavailable filesystem references without opening them. User font redistribution rights remain the user's responsibility; nonstandard notice filenames must be supplied explicitly. Existing legacy model-image filename sharing and unique old sidecar retention are unchanged. Generic Composer text component commands/inspectors remain step 7; arbitrary mutable API corruption is outside the ownership protection contract.
- User-reported result/acceptance: the user requested the next milestone, accepting step 6 by continuation. The latest explicit successful compilation report applies to step 5; no additional user build report was supplied.
- Next milestone: step 7 adds Composer font collection/text creation and inspectors, undoable live paragraph/geometry editing, conventional material/source-lock interactions, and integrated selection/duplication/save/reopen/recovery workflows.
- Exact next action/file to resume: await user library/Composer compilation and feedback; fix step-6 failures in the files above before advancing. On acceptance/continuation, read `plans/text-components/recovery-policy.md` and `plans/text-components/step-07-composer-integration.md`, then inspect Composer component creation menus, inspectors and command/history registration. Do not start step 7 automatically.

### Step 7 — Composer authoring and integrated interaction

- Status: implemented, awaiting user compilation and manual Composer acceptance. Step 6 was accepted by the user's continuation, not by an agent build. All preceding uncommitted step-6 changes are preserved.
- Changed files (step 7 only):
  - `apps/bg2e_composer/src/TextEditor.cpp` (new)
  - `apps/bg2e_composer/src/command/component/TextCommands.hpp` (new)
  - `apps/bg2e_composer/src/command/component/TextCommands.cpp` (new)
  - `apps/bg2e_composer/src/AppDelegate.hpp`
  - `apps/bg2e_composer/src/EditorUi.hpp`
  - `apps/bg2e_composer/src/EditorUi.cpp`
  - `apps/bg2e_composer/src/EditorTools.cpp`
  - `apps/bg2e_composer/src/SceneEditor.cpp`
  - `apps/bg2e_composer/src/SceneCommands.cpp`
  - `apps/bg2e_composer/src/SceneStructure.cpp`
  - `apps/bg2e_composer/src/EditorActions.cpp`
  - `apps/bg2e_composer/src/EditorBaking.cpp`
  - `apps/bg2e_composer/src/command/component/ComponentCommands.cpp`
  - `apps/bg2e_composer/src/command/scene/StructureCommands.hpp`
  - `apps/bg2e_composer/src/command/scene/StructureCommands.cpp`
  - `apps/bg2e_composer/src/command/SceneRetention.cpp`
  - `lib/src/bg2e/scene/reflection/TextComponentReflection.cpp` (new)
  - `lib/include/bg2e/ui/Value.hpp`, `lib/src/bg2e/ui/Value.cpp`
  - `lib/include/bg2e/base/TextTextureGenerator.hpp`, `lib/src/bg2e/base/TextTextureGenerator.cpp`
  - `lib/include/bg2e/scene/TextComponent.hpp`, `lib/src/bg2e/scene/TextComponent.cpp`
  - `plans/text-components/summary.md`, `plans/text-components/progress.md`
- Delivered authoring:
  - **Create Node > Text** inserts an independent Hello World TextComponent with the existing default conventional material/quad and selects its node, in one structural/history command. EditNodes has an optional insertion-selection flag using its existing before/after selection snapshots (default false for existing callers). Parenting follows the existing selected-node-or-document-root contract. It refers to explicit identifier `default`; creation does not silently install or shadow fonts.
  - Built-in reflection type registration exposes **Text** and **Text Fonts** in **Scene > selected node > Add Component** with ordinary remove/history behavior. Specialized transactional controls appear below the standard Components section: **Text paragraph**, **Text rectangle**, **Font declarations**. No generic mutable source setter or parallel persistent editor data model was added.
  - Paragraph controls: multiline UTF-8 content, editable font identifier plus a scene-wide declared-font list with availability/duplicate labels, logical font size, paragraph width/height, line and character spacing, bitmap resolution, four margins, word/character/no wrapping, horizontal/vertical alignment and clip/hide overflow. Geometry controls expose authored world height, optional manual width, logical-aspect preservation and normalized pivot. There are no text color/unlit/material appearance fields.
  - Font collections work on any node; the UI recommends root placement. Each row exposes identifier, typed path, **Choose font file...**, face index, availability/recovery guidance, **Remove font**, and optional license/provenance notice add/remove controls. **Add bundled Didact Gothic font** inserts the canonical asset using `default` when free, otherwise the first available fontN identifier. Multiple rows may be changed to other trusted files. Explicit notices survive source-file changes; the UI reminds the user to verify rights/appropriate notices.
  - **Reload fonts / retry text** queues scene-wide explicit reload at the stopped renderer boundary, without manufacturing an authored history entry. It advances font generation so unavailable texts can retry; no filesystem loading occurs in UI drawing.
- Transactions, ownership and recovery:
  - New EditText is a CPU-only CommandTransaction using Composer's existing valueEditIntent admission, FIFO reservation, EditSession lifecycle, dirty tracking, commit/cancel and undo/redo. It captures paragraph/world geometry or declaration collections only, never a material, generator, mesh, GPU image or registry. Typing/dragging coalesces into one gesture; previews invalidate lightmap sources/render histories through existing effects. Normal TextComponent preparation realizes the latest CPU state after queued changes, not in the UI/property callback.
  - Authoring repairs delegate to existing base::Text/TextComponent/font declaration APIs. Incomplete/invalid identifiers or paths, collection overflow and same-collection duplicates keep the preceding valid declarations as an accepted no-op draft, with warnings, rather than deleting a row while its identifier/path is being typed. Cross-node duplicate identifiers are allowed as declarations but disabled by the scene registry with diagnostics. Missing files preserve the authored paths and omit dependent text. Exceptions at editor/transaction boundaries are contained; failed updates recover the preceding preview and failed replay recovers its preceding state. Catastrophic failed recovery reports history Faulted rather than falsely claiming success.
  - The text EditSession participates in per-frame missing-control cancellation, document generation binding, global commit/cancel settlement for Save/history/lifecycle, and cleanup. UI IDs have scoped cleanup even if a picker/allocation fails. No per-frame albedo repair, file retry or font reload was introduced.
  - TextComponent::authoredSize() is a read-only getter used to snapshot dormant manual width exactly while aspect mode is enabled. TextComponent::retainedCpuBytes() and TextTextureGenerator::retainedBytes() provide read-only CPU cache/font ownership accounting. SceneRetention now admits exact built-in Text/TextFonts types and accounts actual generator caches/font bytes/declaration strings in addition to existing drawable/native/BLAS accounting; it does not reserve a maximum-size font for every label. History value snapshots account CPU proposals/recovery separately.
  - Ordinary node duplicate/delete/reparent, component membership, selectable rectangles, gizmos and existing TextComponent independent clones are reused. A second Drawable-derived component is rejected in the admission and execution paths; users create a child node instead. This enforces the agreed one-drawable limitation, not a storage redesign.
  - Text's generated one-submesh material remains in the existing **Model Properties** editor. **Select text material** selects submesh 0; ordinary color/unlit/alpha cutoff/double-sided controls, albedo preview, source locks, UVs, sampling, material copy/paste/patch and their execution-time all-target policy checks remain the established step-2 implementation. Paragraph/font transactions never restore a stale material snapshot over newer sampling/appearance edits.
  - Generic model geometry conversions/centering/floor placement, UV2 atlas regeneration and model-only submesh metadata editing are refused for Text with warning/status, including selected batches before publication. Text exposes its own world rectangle and node transform/name/Enabled instead. These model-only controls are hidden for the text submesh; the material workflow remains available. No unsupported submesh name/group edits are offered that would be lost by embedded persistence. Standard model operations remain unchanged.
- Source review performed (not a build): inspected component reflection discovery, creation/membership and structural selection commands, value queue/history/settlement contracts, multiline UI capacity, full CPU snapshot replay, invalid font drafts, generation-based reload, retention budgets and nullable clones, generic geometry/UV/metadata paths, ordinary material source-lock ownership and synchronous/incremental scene loading/persistence inherited from step 6. `git diff --check` passed. New sources use existing discovery; no CMake edits/configuration, compilation, shader compilation, application launch, automated runtime checks, tests, fixtures, harnesses or verification executables were performed/added. `skills.md` remains absent.
- Composer manual checklist (user-run, not agent verification):
  1. Compile the library and Composer. Start a scene. Open **Window > Scene**, select the document root, choose **Text Fonts** beside **Add Component**, add it, scroll below Components to **Font declarations**, and click **Add bundled Didact Gothic font**. Confirm identifier `default` and the canonical fonts/didact-gothic path; additional rows/file pickers support other trusted fonts.
  2. Select the root and use **Create Node > Text**. The new node is selected. In **Scene > Text paragraph**, type Spanish precomposed characters such as `Hola, acción y pingüino`, multiple lines and an empty string. Restore Hello World; use **Declared fonts** or **Font identifier**. Rectangle picking must still work over transparent/empty regions, and from behind with Double sided enabled. Move/rotate using ordinary transform/gizmo tools.
  3. Change logical paragraph dimensions, margins, font size, spacing, wrapping/alignment and overflow. Change bitmap dimensions separately: they alter pixel quality, not logical line breaks or world size. Change World height and Preserve logical paragraph aspect; manual World width appears when aspect preservation is off. Check undo/redo, including pivot and aspect-mode changes.
  4. Click **Select text material**, then open **Window > Model Properties**. Edit ordinary albedo color, Unlit, alpha cutoff and Double sided. Generated albedo preview must be visible, replace/clear unavailable, while UV set/scale and all sampler controls remain usable. Change paragraph text, reload fonts, paste another material and undo/redo; owned albedo and authored UV/sampling must survive. Mixed text/model source-changing edits must reject atomically rather than modify only the model.
  5. For shadows use an authored **Create Node > Create Plane** floor and a **Create Node > Light** with **Light Source > Properties > Type = Directional** and **Cast Shadows** enabled. Put text above the floor and rotate its XY rectangle to face the light (for example approximately X=-90 degrees, Y translation=1 for downward light, using the corrected +Z front). In **Window > Graphics Settings**, enable Shadows/enabled and inspect capability/status availability. Keep alpha cutoff > 0. Expect glyph-shaped, not rectangular, shadows where the existing backend/device supports query shadows.
  6. Use **Edit > Duplicate**, modify only the copy and exercise text/font/material undo/redo. Delete/undo, reparent/undo and add/remove Text Fonts on a non-root group. Declarations are scene-wide, not ancestor-scoped; root placement is merely recommended. Copying a declaration-bearing group may intentionally create duplicate identifiers until renamed.
  7. **File > Save**, then **Save as...** to another directory and reopen from that destination. Text remains editable; fonts, required notices and other material images are copied. Text itself generates neither a BG2 sidecar nor an albedo PNG; a floor or other ordinary model still legitimately generates its BG2. Ordinary selected model exports remain available; Text is refused as a BG2 model.
  8. Through normal controls try zero/invalid logical dimensions, spacing, margins, excessive/zero resolution, overflowing content, missing/unreadable font paths and duplicate identifiers across nodes. Expect warned defaults/limits, clipping/hiding or omitted text, continued interaction/rendering, no per-frame warning/retry flood, and recovery after correcting data or **Reload fonts / retry text**. Same-collection invalid drafts leave the last valid declaration intact instead of removing its row.
- Known limits: compilation/backend/runtime behavior remains unverified by the agent. Trusted fonts and simple LTR layout only; no shaping/bidi/font discovery. Normal history memory limits can reject resource-heavy duplication/membership, and no new budget override is introduced. Font file dialogs inherit the repository's macOS generator/native-dialog constraints. The final public API reference, runnable tutorial and beginner walkthrough are not yet delivered; no tutorial/example source or target was added in this milestone.
- User-reported result/acceptance: pending for step 7. No user build/runtime result for this milestone has been supplied.
- Next milestone: step 8 updates doc/api and adds the full runnable interactive Hello World tutorial (window, floor, live editable text, font registration and directional glyph shadows), plus the beginner walkthrough using only engine UI wrappers. It is not documentation-only.
- Exact next action/file to resume: await user compilation/manual feedback and fix step-7 failures in the files above before advancing. On acceptance/continuation, read `plans/text-components/recovery-policy.md` and `plans/text-components/step-08-documentation-and-tutorial.md`; inspect `doc/api` and the current draw example/tutorial layout and source discovery before implementing the final milestone. Do not compile or launch applications.

### Step 7 follow-up — Readable front face and gizmo admission

- Status: targeted fixes implemented, awaiting user confirmation; step 7 is not yet accepted and step 8 has not started.
- User feedback: with Double sided disabled, the remaining visible text was mirrored; gizmo transforms were rejected with `A component requires a dedicated transform editing adapter`, although the component transform inspector worked. The user reports that the rest of the manual checks passed.
- Changed files for this follow-up:
  - `lib/src/bg2e/scene/TextComponent.cpp`
  - `lib/include/bg2e/scene/TextComponent.hpp`
  - `apps/bg2e_composer/src/TransformInteractions.cpp`
  - `plans/text-components/progress.md`
- Front-face correction: the old -Z winding/normal put the readable +X/right, +Y/up image on the culled side under the renderer's ordinary front-view convention. The quad now faces **+Z**, using indices `{0,2,1,0,3,2}` and normal `(0,0,1)`, keeping +X tangent and both top-left-origin UV sets unchanged. This fixes sidedness at geometry creation rather than mirroring the bitmap/UVs or special-casing culling/shaders. Both triangle cross products agree with the new normal; raster, full-rectangle picking and traced front/back rejection consume the same mesh. The back of a double-sided text remains naturally mirrored, like a printed thin sheet. This supersedes the step-5 historical -Z orientation/+V-frame description above; no per-vertex tangent-handedness or renderer convention change is introduced.
- Gizmo correction: the dedicated transform admission list still recognized only exact DrawableComponent, not its new exact TextComponent type or TextFontComponent on a group/root. Both built-ins are now allowed, since neither owns/writes the node transform. Existing matrix preview/commit/cancel/undo/redo is reused; unknown/custom writers and fixed-scale controller restrictions remain unchanged. The working reflected component-transform path is unchanged.
- Existing saved text stays procedural and regenerates with the corrected quad on scene open; no schema/asset rewrite or model conversion is needed. Positive-scale transforms were the reported case; ordinary mirrored transforms retain the existing material/rendering behavior.
- Source review only: inspected quad UV/normal/winding, conventional front-face pipeline defaults, shared raster/traced consumers and the gizmo admission/transaction flow. `git diff --check` passed. No compilation, application launch, configure, runtime checks or automated tests were executed/added.
- User checks: rebuild/reopen and disable Double sided; from the visible front, text must read normally, with the opposite side culled. Reenable Double sided and confirm rectangle picking from both sides and lighting/shadows. Drag translate/rotate/scale gizmos, then undo/redo, on a text node and on a group/root carrying Text Fonts. Existing component-inspector transforms must continue working. The floor-shadow checklist now uses X=-90 degrees to face the corrected +Z front upward.
- Known limits: both reported fixes need user build/runtime confirmation; previously documented rendering/device and trusted-font limitations remain. No new Composer features or tutorial files were added.
- Exact next action: await feedback on these two fixes. Further failures remain part of step 7. Only after acceptance/continuation read `plans/text-components/step-08-documentation-and-tutorial.md` and begin the final API documentation/runnable tutorial milestone.

### Commit handoff — Persistence and Composer authoring

- The user authorized a commit of the pending changes: step-6 editable persistence/resource packaging, step-7 Composer authoring/history integration, and the reported front-face/gizmo fixes.
- No additional build/runtime confirmation was supplied with the commit request. Step 7 remains implemented awaiting confirmation of those two fixes; the user's other manual observations remain recorded above. Committing does not authorize step 8.
- Verification for this commit is source/diff review and `git diff --check` only. No builds, application launches, automated checks or push are requested/performed.
- Next action: stop after committing and await user feedback or a separate continuation request. Fix outstanding step-7 reports before advancing to the final documentation/tutorial milestone.
