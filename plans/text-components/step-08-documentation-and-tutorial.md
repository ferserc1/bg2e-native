# Step 8 — Final API documentation and interactive tutorial

Dependency: accepted step 7. This is the last implementation milestone and includes the requested example source, not only Markdown.

## API documentation

1. Update `doc/api` for every new public class and API, including Text, TextFont, TextFontComponent, TextComponent, the procedural generator if public, and any public registry/lookup helpers actually delivered.
2. Update existing material/texture/drawable/scene/editor references for doubleSided, sourceEditable, ignoreSerialization, requiresBg2Export, font-first realization, embedded materials and external resource copying. Add index/navigation links following repository conventions.
3. Document exact signatures/defaults, units, validation/error behavior, supported scripts, trusted-font restriction, ownership/threading, coordinated regeneration and clone behavior. Explain API-mutable access is not guarded and why UV/sampling edits remain allowed.
4. Explain font file licensing separately from stb/project licensing. Link to the bundled font's exact OFL/provenance files. Do not document hypothetical APIs or deferred Fontstash/shaping features as implemented.

## Minimal runnable example

Create `examples/tutorials/text/` following the current tutorial target/source layout, with an English README and the smallest practical complete application. Scope CMake edits to registering this target and making required assets/licenses available; do not configure or compile it.

The example must:

1. Create the application/window, draw engine/delegate, renderer and scene through current public APIs.
2. Set a simple camera with a fixed useful view. Avoid optional navigation/gizmos/editor systems unless genuinely required for window operation.
3. Add a procedural floor and a text quad positioned above it so a shadow is plainly visible.
4. Declare the bundled Didact Gothic font under a clear scene-wide identifier using TextFontComponent, preferably on the root. Show loading/resource realization in the correct order. Use the canonical verified asset from step 3, not a system-installed font or runtime network download.
5. Create TextComponent by code with the initial string exactly `Hello World`, readable dimensions/resolution and a conventional illuminated material with alpha test and double sided enabled. Appearance stays in the material, not Text.
6. Add one directional light and the minimal renderer settings required to cast a glyph-shaped shadow on the floor. No HDR environment, unrelated models, textures or extra lights should be needed. A procedural quad's shadow must not be a solid rectangle.
7. Add one small UI window containing an editable text field initialized to `Hello World`. Use only bg2e::ui wrappers; add/reuse a narrowly scoped wrapper if the existing public API cannot express the field. Update the text through the supported change API, with resource publication at its correct boundary, not arbitrary GPU work in the UI draw callback.
8. Keep sources easy to follow: only necessary includes, initialization, scene construction, UI/update handling and cleanup. Do not import Composer command/history infrastructure or add a test harness, diagnostic console, feature gallery or complex abstraction framework.
9. Ensure font and OFL/provenance assets can be located using the repository's normal runtime asset conventions and are included in the tutorial's distributable assets.

## Beginner walkthrough

Create `doc/tutorials/text/README.md`, linked from appropriate documentation navigation. Explain the example step by step without assuming prior engine knowledge. Use actual final APIs and match the source exactly.

Cover:

- Required tools/platform prerequisites, obtaining the repository/submodules, Vulkan SDK configuration and where the tutorial files live.
- How to create the tutorial directory, CMake target, source file and required asset paths from scratch. Include user-run configuration/build/run instructions appropriate to supported platforms/generators, but do not run them as the agent.
- Each necessary include, application/delegate role, engine lifecycle, window creation, render loop and cleanup.
- Nodes, components, transforms, camera placement, procedural floor, text-plane orientation/size and the relationship between logical paragraph size, world size and bitmap resolution.
- Why a font file needs a license notice; how to declare its identifier/path, load it before texts and resolve missing-font errors.
- Constructing Text, TextComponent and material by code; white RGB/coverage alpha, alpha test, double sided, color and unlit. Explain why the example remains lit for demonstrating its appearance and shadow.
- Creating/configuring the directional light and shadow settings, including the floor/text/light placement that makes the glyph shadow visible.
- Constructing the UI text field, handling edits, invalidation and safe resource updates during execution.
- Where to place/run assets, expected screen result and concise troubleshooting for missing fonts, invisible/back-facing text, clipped layout, absent shadows and wrong working directory.
- Complete final minimal source (or clearly linked canonical complete source plus fully explained sequential code sections), CMake contents and all required files so a beginner can recreate the application without missing setup steps.

Avoid generic engine surveys, unexplained helper functions, hidden initialization and unrelated effects. Do not include unit-test sections or automated verification instructions.

## User handoff

Report API documentation links, example target/path, beginner guide and canonical font/license paths. In Composer, give a short final reminder of text editing, material controls, rectangular selection and save/reopen behavior. For the new tutorial, give user-run build/run directions and describe the intended result: window, floor, Hello World, directional glyph shadow and live text field. Clearly distinguish these example observations from Composer checks.

Do not compile or launch anything. Update progress to implemented/awaiting final acceptance, explicitly state there is no next implementation milestone, and wait for user feedback. Only mark the plan complete after user acceptance.
