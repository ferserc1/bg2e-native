# UI icons progress

- Planning: complete.
- Implemented steps: steps 1–7 (step 7 awaiting user validation).
- User-accepted implementation steps: steps 1–6 (user requested each next step).
- Current step: step 7 complete; stopped for final user validation.
- Changes: all planned implementation and public documentation delivered; details below.
- Tests/builds: not performed; excluded by the execution contract.
- Next action: final user validation. Address any reported issues before claiming acceptance. No implementation step remains; record final acceptance/completion when the user accepts the delivered documentation/feature. Do not start new feature work or run builds/tests automatically.

## Step 1 — Shared image infrastructure

Status: implemented; accepted by the user instruction to implement the next step.

Changed files:

- Added `lib/src/bg2e/ui/detail/ImageRegistry.hpp/.cpp`, replacing `detail/PreviewRegistry.hpp` and the registry implementation formerly inside TexturePreview.cpp.
- Added `lib/src/bg2e/ui/detail/ImageDraw.hpp/.cpp`.
- Adapted `lib/include/bg2e/ui/TexturePreview.hpp`, `lib/src/bg2e/ui/TexturePreview.cpp` and `lib/src/bg2e/ui/UserInterface.cpp`.
- Updated this progress record and summary status.

Decisions and ownership:

- ImageRegistry accepts shared GPU image/sampler owners; it does not load textures or cache paths. It validates sampleability, format, layout, resource validity and matching UI device. UserInterface::texturePreview retains the source texture/engine association check and constructs the opaque preview.
- ImageEntry retains exact image/sampler snapshots. Registry entries retain their existing deferred retirement, lazy backend registration, generation tracking and context checks. Strong consumer ownership keeps an entry live; the future IconManager can retain entries without changing preview retirement policy.
- Retirement still runs before UI composition in newFrame. A preview released during composition stays in registry ownership through submission, and is considered for deferred retirement at the following frame. Shutdown waits for GPU completion, invalidates all surviving entries and runs before backend destruction.
- ImageDraw centralizes validity, image drawing and image-button submission using float dimensions without automatic scaling/aspect changes. TexturePreview preserves its public integer dimensions, conversions, invalid-handle no-op/false behavior and public method signatures. Additional icon-specific drawing options remain for step 3 rather than introducing unused option types now.
- The private preview constructor now accepts ImageEntry and is accessible to UserInterface. Public headers still expose no ImGui/native identifiers. The existing `ui.retiredTexturePreviews` profile counter is preserved.

Source inspection:

- Reviewed preview consumers DrawTextureWidgets and BakeLightmapPreview and searched other call sites; their APIs need no changes.
- Reviewed device ownership, cleanup-manager submission contract, initialization, newFrame and shutdown ordering.
- Inspected source diff, checked for remaining PreviewEntry/PreviewRegistry references in UI and ran `git diff --check` successfully.
- No CMake, GPU/backend, widget or general texture-cache changes. Repository `skills.md` was not found.

Validation limits: no builds, tests or runtime execution performed. Step 1 was accepted by continuation; this is not a claim that builds or runtime checks were performed by the agent. No unfinished step 1 implementation items.

## Step 2 — Icon resources and permanent path cache

Status: implemented; accepted by the user instruction to implement the next step.

Changed files:

- Added public `lib/include/bg2e/ui/Icon.hpp` and `IconManager.hpp` and implementations `lib/src/bg2e/ui/Icon.cpp` and `IconManager.cpp`.
- Added private `lib/src/bg2e/ui/detail/IconData.hpp` and `IconAccess.hpp`.
- Updated `UserInterface.hpp/.cpp`, `ui/all.hpp`, summary and this progress record. Step 1 changes remain in the worktree; do not overwrite or reimplement them.

Public contract:

- `ui.icons().load(const std::filesystem::path&)` returns `std::shared_ptr<Icon>`; an empty path returns nullptr, meaning no icon. Icon is an immutable/noncopyable resource shared through its handle.
- `Icon::source()` retains the normalized source identity; `isFallback()` identifies asset failures even after session cleanup; `valid()` means a drawable image is still owned by the live UI session (including the question-mark texture). A textureless fallback has `isFallback() == true` and `valid() == false`.
- Access is UI-thread-only with the owning ImGui context active. Session misuse raises logic_error. The accessor is available after manager creation and before delegate init, even though the overall UI ready flag is still false.

Implementation decisions:

- One native-path-keyed unordered_map strongly retains successes and failed assets until UI teardown. Paths are absolute and lexically normalized without lowercasing, canonicalization, symlink lookup or lossy string cache keys. Relative paths resolve against the current working directory on every load call. Hits do not decode/read/upload an image again; relative resolution still consults the working directory.
- If absolute resolution fails, the supplied lexically normalized path serves as the cached failure identity. Once resolution succeeds later, the absolute identity is distinct. Empty paths are deliberately not cached as failures.
- Reuse db::imageFormatFromPath/loadImage for PNG extension validation and RGBA8 decoding. Existing db::loadImage internally uses narrow path strings; the cache preserves native identities, but this pre-existing decoder I/O limitation for some Windows Unicode paths is unchanged and can result in fallback. No other namespace was changed to address it.
- Decoder/filesystem runtime errors become a fallback, logged once after cache insertion. Normal upload/registration/device exceptions and allocation exceptions are outside that catch and propagate; they are not permanent missing-file records.
- Upload reuses a temporary draw::Texture and ImageRegistry::create with linear filtering, clamp-to-edge and no mipmaps. A private nonempty source label prevents draw::Texture from retaining the CPU copy required by its empty-path reload contract. That temporary texture is never reloaded; Icon owns the actual native path. Decoded pixels and temporary settings are released after upload.
- The current upload path uses RGBA8_UNORM and preserves raw PNG channels. Both vendored ImGui backends multiply sample values by vertex colors and blend with source alpha, so icon metadata uses TypeLinear without decoding sRGB/premultiplying bytes or changing shaders/targets. This matches existing UI numeric color treatment; color appearance still awaits user runtime validation.
- The question-mark image is generated directly as 16x16 white straight-alpha RGBA pixels (not an atlas). Its upload is attempted only on the first failed asset. Runtime/logic failures of that best-effort upload are logged once and retain a textureless fallback; allocation exhaustion is not swallowed. Normal successful-path GPU errors still propagate.
- IconData holds the source, failure flag, shared ImageEntry and a weak session token. Cache/resources belong to UserInterface::Impl. Teardown removes the context lookup and invalidates the session token before registry/backend cleanup. Retained caller handles cannot enter a later session, even if a context address is reused.
- IconAccess is a private bridge for step 3: current() looks up the manager by active ImGui context; belongsToCurrentSession() rejects stale/cross-context handles; image() returns the common entry only for this session. The lookup owns no engines, icons or managers.

Source inspection and boundaries:

- Reviewed UI delegate initialization and failure cleanup ordering, common registry ownership, image decoding, draw::Texture CPU retention/upload and both vendored ImGui blending/sampling paths.
- Inspected new files and diffs and ran git diff --check. No compilation, tests, examples or runtime execution performed.
- No widget drawing API, preload facility, cache eviction, file watching, atlas, general texture-cache changes, GPU/backend changes or CMake edits added.

Next implementation action after acceptance: step 3 adds path-first image/button wrappers, size/tint handling, shared measurement and a text question mark when a live-session fallback has no drawable texture. Use IconAccess to distinguish that state from stale/cross-session handles. Do not interpret Icon::valid() alone as successful file loading: check isFallback() where needed.

## Step 3 — Image and button wrappers

Status: implemented; accepted by the user instruction to implement the next step.

Changed files:

- Added `lib/include/bg2e/ui/Image.hpp`, `IconDrawOptions.hpp` and `lib/src/bg2e/ui/Image.cpp`.
- Added private `lib/src/bg2e/ui/detail/IconWidgets.hpp/.cpp`.
- Updated `Button.hpp/.cpp`, `ui/all.hpp` and private `ImageDraw.hpp/.cpp`.
- Updated summary and this handoff. Earlier implementation steps remain uncommitted in the same worktree; preserve them.

Final API signatures (both overload families):

- `Image::icon(pathOrSharedIcon, float size = 20.0f, const IconDrawOptions& options = {})`.
- `Button::iconButton(const std::string& id, pathOrSharedIcon, float size = 20.0f, const IconDrawOptions& options = {})`.
- `Button::iconTextButton(const std::string& id, const std::string& label, pathOrSharedIcon, float size = 20.0f, const IconDrawOptions& options = {})`.
- Here pathOrSharedIcon is either `const std::filesystem::path&` or `const std::shared_ptr<Icon>&`; no manager/UI argument is needed.
- IconDrawOptions contains `base::Color tint = White()`, optional `std::string tooltip`, `bool sameLine = false` and `bool disabled = false`.

Behavior and shared infrastructure:

- Path wrappers use private resolveIcon() and IconAccess::current(), then delegate to the same shared-resource renderers. No session means no loading or drawing. Cache hits reuse the manager resource; relative paths still require working-directory resolution as documented in step 2.
- Positive finite `size` defines a logical square multiplied by UserInterface::getScale() exactly once. Source dimensions are fitted proportionally and centered within that square; existing style padding/spacing and fonts are already scaled, so they are not scaled again. Invalid size raises invalid_argument when an icon box is used.
- Image::icon uses one noninteractive Dummy item and overlays its fitted image. Empty/null input is a no-op.
- Buttons use exactly one regular ImGui::Button with a blank visible label inside an explicit ID scope. Icon/text content is overlaid using the draw list and public ImGui APIs. Standard button activation, keyboard/gamepad navigation, focus, hover, background and disabled behavior are retained. Nonempty IDs are required; visible labels are literal text, independent of identity, including any ## sequences.
- Empty/null icons omit the icon slot; iconTextButton still renders the text under the explicit ID. An icon-only call with no icon produces an empty ordinary button, with standard minimum line height/padding. Nonempty failed source paths keep their icon slot and use the cached fallback.
- Existing Button::button/checkBox/radioButton behavior is untouched. No menu/toolbar/settings implementation was added.
- IconButtonMetrics/measureIconButton() is the single internal measurement policy for content width/height, icon square, literal text (including multiline height), gap and frame padding. Both button rendering and later toolbar alignment can use it without loading images during measurement. Metrics use scaled float UI coordinates.
- Shared ImageDraw now centralizes identifier lookup and supports drawImageAt() for geometry without creating another UI item. TexturePreview continues using its original unscaled integer-size API; its image/image-button calls reuse the same identifier helper.
- Texture tint uses GetColorU32 exactly once so style alpha and enclosing disabled alpha apply coherently. Button text uses the current text color. Tooltips remain available for disabled items. Clipping prevents overlaid content escaping its item rectangle, and ID/disabled/clip stacks use scoped cleanup if resource registration throws.
- Live-session failures without a fallback texture draw a fitted text question mark inside the same square. Stale/cross-session resources are rejected (image no-op, button false without layout or drawing), rather than being reattached or presented as missing assets. No active manager also means image no-op/button false. Calls within a valid session must occur inside a normal UI window/frame drawing scope.

Source inspection and validation limits:

- Reviewed overload forwarding, public header includes, source lifetime, alpha handling, literal-label measurement and ImGui's existing Button implementation for navigation/background/size behavior.
- Reviewed exception-scope cleanup and invalid-session/empty/fallback paths. Confirmed no imgui_internal.h dependency or ImGui types/functions in new public headers.
- Inspected source/diff and ran git diff --check. No compilation, unit/integration tests, applications or runtime verification were performed.
- Existing Windows narrow-path decoder limitation from step 2 remains unchanged. Runtime visual/interaction behavior awaits user validation.

Next action after acceptance: implement step 4 menu icons while preserving menu/check/shortcut behavior. Reuse IconAccess, resolveIcon, ImageDraw and the same source cache. paintIcon is currently local to IconWidgets.cpp; extract its fitted rendering policy into a private reusable helper if menus need it, rather than duplicating aspect/fallback/tint logic. Do not implement menu rows as separate image/button items.

## Step 4 — Menu icons

Status: implemented; accepted by the user instruction to implement the next step.

Changed files:

- Updated public `lib/include/bg2e/ui/Menu.hpp` and `lib/src/bg2e/ui/Menu.cpp`.
- Updated private IconWidgets.hpp/.cpp and ImageDraw.hpp/.cpp to share fitted painting with an optional parent draw-list target.
- Updated `lib/src/bg2e/ui/Window.cpp` to use Menu::beginMenuBar/endMenuBar instead of native calls, so menu-bar header positioning is available to the wrappers.
- Updated summary and this progress record. Preserve the earlier uncommitted implementation steps in the worktree.

API:

- MenuItem has an optional public `std::filesystem::path iconPath`; all existing constructors retain their meanings.
- `Menu::beginIconMenu(id, label, iconPath, enabled = true, reserveIconSpace = true)` pairs with the existing endMenu().
- `Menu::iconMenuItem(id, label, iconPath, shortcut = "", selected = false, enabled = true, reserveIconSpace = true)` returns native activation.
- These are explicitly named methods, avoiding ambiguous label/path overloads. IDs must be nonempty, stable and independent of icon paths. Labels retain native ImGui ## display conventions.
- For low-level manual menu composition, use iconMenuItem/beginIconMenu also on siblings without images, passing an empty filesystem path, to reserve the same gutter. Legacy menuItem/beginMenu APIs remain unchanged and do not automatically reserve a gutter.

Rendering and identity:

- Native ImGui public APIs do not expose a texture-valued menu icon column. Each new wrapper reserves a measured blank prefix in its native label and appends an explicit ### identity suffix, then overlays the icon on the same native row. There is no extra selectable/button and no dependency on imgui_internal.h or vendored edits.
- Prefix width includes one current text-line-height icon square and ItemInnerSpacing.x. The complete prefix is measured to handle fractional space advances correctly. ImGui therefore includes the gutter in its native label width while independently maintaining shortcut and mark/arrow columns.
- Icon sizing uses already-scaled line height directly (no second UI scale application). Shared paintIcon handles aspect fit, cached question-mark image/text fallback, tint and style alpha. Selected/checkable rows retain their native marks alongside the left icon; submenu arrows and shortcut text remain native.
- Menu::draw and each submenu scan siblings for icon paths and reserve a common gutter only when needed, excluding separators. Groups without icons retain text layout. Top-level menu-bar siblings follow the same policy.
- MenuItem scopes native IDs by its owned instance and a fixed entry suffix. Dynamic labels and icon changes do not alter identity. Instance identity is stable while the menu tree is structurally stable; moving/rebuilding the tree can change it. Low-level APIs provide programmer-owned IDs when persistence across structural rebuilds is required.
- The existing shortcut registration code and Menu::_shortcutInitialized flow are unchanged. Drawing or cache hits never register shortcuts; existing callbacks, enabled functions and checked functions are preserved.
- Menu-bar scopes track only non-owning context/draw-list references, paired by beginMenuBar/endMenuBar. This distinguishes horizontal headers from vertical popup rows without inspecting ImGui internals. Window now routes its menu bars through these wrappers too. No assets/resources are owned by this scope tracking.
- BeginMenu switches the current window to the child popup on success. Capture the parent draw list and header position before that call; paint on the captured list using the restored parent last-item bounds afterward. Resolve/register images before entering the popup so backend registration failures cannot leave a newly opened popup scope unmatched.
- Native calls have already restored their internal disabled alpha before overlay painting. Explicit disabled rows apply the matching tint alpha; enclosing style/disabled alpha is still handled by the common painter. Parent-row clipping is scoped on the captured target.
- Shared paintIcon now rejects stale/cross-session resources itself. With no ImGui context the new low-level methods return false; a bare ImGui context without the UI manager can still draw native labels/gutters but cannot load icons. Normal usage is within an initialized UserInterface drawing frame.

Source inspection and limits:

- Reviewed current native BeginMenu/MenuItem/MenuBar implementations for columns, spacing, popup last-item restoration and native interaction; no native sources were changed.
- Inspected shared painting changes, public API compatibility, sibling gutter calculation and Window menu-bar integration. Ran git diff --check.
- No compilation, tests, application launches or runtime verification performed. Geometry, navigation and popup behavior await user validation.
- No application menu migration, new artwork, toolbar/settings implementation, CMake/GPU/backend changes or shortcut-system changes included.

Next action after acceptance: step 5 adds toolbar icon paths/tooltips, content modes, size configuration and stable button IDs, reusing the existing shared draw/measure helpers. Keep menu gutter policy independent of toolbar content measurement.

## Step 5 — Toolbar icons and presentation

Status: implemented; accepted by the user instruction to implement the next step.

Changed files:

- Added public `lib/include/bg2e/ui/ToolbarConfiguration.hpp` and private `lib/src/bg2e/ui/detail/ToolbarConfiguration.hpp`.
- Updated Toolbar.hpp/.cpp, UserInterface.hpp/.cpp and ui/all.hpp.
- Extended private IconWidgets.hpp/.cpp with shared content measurement/drawing for noninteractive entries.
- Updated summary and this record. Preserve all earlier implementation changes in the worktree.

API and presentation:

- ToolbarButton appends `std::filesystem::path iconPath` and `std::string tooltip` after the existing id/label/action/disabled fields; existing positional/designated initializers remain compatible.
- ToolbarPresentation provides Text, IconsWithTooltips and IconsAndText. ToolbarConfiguration defaults to Text with a 16.0 logical-unit icon square (matching the current UI font/body layout); ordinary standalone icon/button defaults remain 20.0.
- Toolbar::configuration() returns effective settings. setConfiguration(), setPresentation() and setIconSize() create a complete per-toolbar override. inheritsSessionDefaults() reports inheritance; useSessionDefaults() removes the override. Setting only presentation or size snapshots the other current value too; overrides are intentionally whole configurations, not independent per-field inheritance.
- Added UserInterface::getToolbarConfiguration()/setToolbarConfiguration() as static, active-session APIs, consistent with scale access. Actual storage belongs to UserInterface::Impl and a non-owning context lookup resolves it. Setter validation rejects unknown enum values and nonfinite/nonpositive sizes. Getter returns Text/16 without an active session; setter requires a live session, including delegate initialization.
- The session configuration foundation was implemented in this step to make inheritance concrete and usable programmatically. Step 6 adds UISettingsWindow controls and persistence; neither exists yet. Configuration lookup is installed before delegate initialization and removed before resource/backend cleanup, including initialization failures. Each new session starts with fresh defaults until step 6 loads preferences.
- Buttons without icon paths always retain text. Nonempty failed icon paths use IconManager's existing cached substitution in either icon mode. Text mode never resolves/loads PNGs. Explicit tooltips apply in all modes; IconsWithTooltips otherwise uses the label's visible text. Actionless entries use the same content modes without becoming buttons.
- Added updateButtonIconPath() and updateButtonTooltip(). Existing label/enable/disable access works on either side. Path changes request a new cached resource on a future icon draw; older cache entries are not removed. The rvalue label update now actually moves its input.

Layout and identity:

- All actionable entries, including text-only presentation, use drawIconButton with explicit button ID under a toolbar-instance ID scope. Mode/path/label changes preserve widget identity. Text labels preserve the old button ## hidden suffix convention before shared literal text drawing; actionless labels retain their original literal Text behavior.
- Added measureIconContent()/drawIconContent() and a common content painter. Button measurement adds frame padding to exactly the same content metrics; actionless entries use content metrics without interactive padding. Disabled alpha, clipping and tooltips reuse shared infrastructure.
- Both left/right groups are snapshotted before drawing and measured in float coordinates each frame. Tallest content determines row height and all entries are vertically centered. Right alignment uses the content edge and exact group widths/spacing. If the available width is insufficient, entries clip instead of overlapping left entries or wrapping. The row submits a final Dummy to reserve its full layout height.
- Actions run after row submission using copied callbacks, so adding/updating buttons during a callback cannot invalidate the current rendering iteration. Their visual changes appear next draw.
- Explicit duplicate IDs now throw invalid_argument on addButton across both sides. findButton now searches containers independently (fixing the previous cross-container iterator comparison) and has a const overload. Mutable findButton pointers must not be used to change IDs; draw also rejects negative/duplicate IDs introduced through direct access. ID exhaustion raises overflow_error instead of signed overflow.

Source inspection and limits:

- Inspected existing app ToolbarButton initializers, Toolbar/Window/menu integration, Workspace's fixed-height toolbar placement and shared renderer sizing. Existing menu hookup behavior is unchanged in this step.
- Reviewed session init/cleanup, configuration validation and inheritance; source declarations/definitions and draw/measure consistency were inspected. Ran git diff --check.
- No compilation, tests, launches or runtime checks performed. User validation remains required for alignment, tooltips, clipping and live configuration changes.
- No application artwork/migration, CMake, GPU/backend, general cache or preference/UISettingsWindow changes. Workspace still owns window height; unusually large icons or multiline labels require enough height in the embedding window. Toolbar does not silently resize the workspace or shrink the requested icon size.

Next action after acceptance: step 6 should use the existing active-session configuration APIs/storage, add the Toolbar controls to UISettingsWindow, load validated `ui` preferences before delegate initialization, and persist changes. Keep per-toolbar overrides effective and avoid introducing another defaults store.

## Step 6 — Runtime toolbar settings

Status: implemented; accepted by the user instruction to implement the next step.

Changed files:

- Updated `lib/src/bg2e/ui/UISettingsWindow.cpp` and `lib/include/bg2e/ui/UISettingsWindow.hpp`.
- Updated `lib/src/bg2e/ui/UserInterface.cpp`, `lib/include/bg2e/ui/UserInterface.hpp` and `ToolbarConfiguration.hpp`.
- Updated summary and this record. All earlier uncommitted implementation steps remain in the same worktree.

Runtime UI and configuration:

- Added a Toolbar section immediately after Interface Scale. It uses the existing Value::comboBox wrapper for Text / Icons with tooltips / Icons and text, and Numeric::sliderFloat for Icon Size.
- The slider operates in logical UI units from 12 to 32, with a tooltip clarifying scale semantics. Temporary text input is explicitly clamped to that range; nonfinite input falls back to 16. The selector/slider state is read from the active session every draw; no extra mutable defaults are stored in UISettingsWindow and no Toolbar pointers are retained there.
- Expanded the initial window to 400x450, or 400x550 when either existing performance section is enabled. Existing ordinary Window scrolling/resizing remains available, including for expanded gizmo controls.
- Changes call the step 5 UserInterface::setToolbarConfiguration API. Inheriting toolbars observe new mode/size on their next draw; whole per-toolbar overrides are untouched, and useSessionDefaults() still restores inheritance. No widget identities, source/cache keys or texture registrations are rebuilt for a presentation-only change.

Persistence:

- Context: `ui`; keys: `toolbarPresentation` and `toolbarIconSize`.
- Persisted mode values are explicitly fixed in ToolbarPresentation: 0 = Text, 1 = IconsWithTooltips, 2 = IconsAndText.
- Defaults remain Text and 16 logical units. UserInterface initialization loads the configuration from PreferencesStore before context/delegate initialization and registers that session's owned configuration before delegate init.
- Mode reads use the existing strict integer preference reader (rejecting fractional, nonnumeric and out-of-range integers), followed by explicit enum membership validation. Size reads require a finite positive number. Missing or malformed values fall back independently to the corresponding defaults, preserving any valid other field.
- The UI slider bounds are an editing convenience, not a new API restriction: finite positive programmatic sizes outside 12–32 remain valid, persist and reload unchanged. Editing Icon Size through the slider clamps the new value; changing presentation alone does not clamp an existing programmatic size.
- setToolbarConfiguration validates, updates session state immediately, then sets/saves both preference values only when mode or size actually changes. Programmatic changes have the same persistence as settings-window edits. This follows the existing explicit save-on-change convention used by scene frame-rate controls; it avoids writes on ordinary draw frames. Per-toolbar overrides are never persisted as global defaults.
- Preferences use the existing save/error behavior. Invalid stored values are used only as fallback inputs; initialization does not rewrite the file simply to normalize them. Session lookup/teardown and fresh Impl construction from step 5 remain unchanged; no additional process-global configuration store was added.

Source inspection and validation limits:

- Reviewed PreferencesStore ownership, typed preference fallback, integer validation and explicit save conventions; reused those APIs without changing the preferences namespace.
- Inspected startup order, setter change detection, controls, bounds handling and whole-configuration inheritance. Ran git diff --check.
- No compilation, unit/integration tests, applications or runtime verification performed. User validation remains required for live controls and preference restoration.
- No application changes, artwork, CMake/GPU/backend changes, cache eviction/reload or workspace sizing changes. The embedding window still needs sufficient height for unusually large toolbar icons, as recorded in step 5.

Next action after acceptance: step 7 documents final public signatures and real behavior, including source path policy/Windows loader limitation, fallback and handle lifetime, widget identity, logical sizes, menu sibling gutter policy, toolbar whole overrides, stable preference keys and UI versus API size ranges. Do not add tests or a preload facility as part of documentation.

## Step 7 — Public documentation and final handoff

Status: implemented, awaiting final user validation. No implementation step remains.

Documentation changes:

- Added `doc/api/ui/Icons.md`: path-first usage, explicit resource loading, Icon/IconManager signatures, source identity, strong cache/failure retention, thread/context requirements, fallback states, PNG alpha/color/sampling policy and shared preview infrastructure.
- Added `doc/api/ui/Image.md`: both Image::icon overloads, IconDrawOptions, logical size/aspect fit, tooltip/disabled behavior and empty/stale/session handling.
- Expanded Button.md and Menu.md with additive signatures and examples for shared PNG paths, separate widget IDs, icon-plus-text buttons, MenuItem::iconPath, submenu headers and mixed-row gutter reservation.
- Updated Toolbar_and_StatusBar.md with actual ToolbarButton fields, the configuration API/defaults, whole overrides/inheritance, live path/tooltip updates, exact float measurement, clipping/height ownership and ID/lookup rules. Retained the existing StatusBar reference and described the unchanged toolbar menu-hook behavior accurately.
- Updated Settings_Windows.md and UserInterface.md for session-owned icon/default access, init timing, runtime controls, UI size, preference keys/mode encoding, independent fallback validation and UI versus programmatic size ranges.
- Updated index.md and reference.md so the new APIs and guides are discoverable. Updated plan summary and this record.

Documented boundaries:

- Explicit load uses the same permanent cache as path widgets, with no separate preload subsystem, atlas, eviction, watcher or automatic retry.
- Empty source differs from missing source. Image validity differs from successful source loading; a live textureless fallback still draws text through widgets. Normal backend/allocation/session errors remain distinct from recoverable asset failures.
- Paths remain native cache identities; the existing narrow-string decoder limitation on some Windows Unicode paths is disclosed.
- TexturePreview keeps its original public signatures, integer-size semantics, transient snapshots and GPU-completion retirement. Shared image registration/drawing does not merge preview/icon/general texture caches.
- Menu labels retain native ## conventions; iconTextButton labels are literal; toolbar actionable labels retain prior hidden suffix behavior. Identity remains independent of the path. Tree-instance stability and manual explicit menu-ID use across structural rebuilds are distinguished.
- Toolbar settings are Text/16 by default, with ui preference keys toolbarPresentation (0/1/2) and toolbarIconSize. UI slider edits 12–32 logical units; the API and valid stored sizes remain any finite positive size. Local overrides are complete configurations and are not global preferences.

Source/document inspection:

- Compared public headers and current resource/renderer/menu/toolbar/settings implementation against the delivered references and examples.
- Inspected changed file scope: this step edits documentation/plans only. Earlier source changes remain in the worktree; no CMake, vendored or GPU/backend changes are part of the feature.
- Reviewed new public UI headers for ImGui/native identifier leakage; none is introduced. Existing GPL/style source requirements remain unchanged.
- Inspected local file targets from links in the ten touched/new UI guides; no missing relative file targets were found. Ran git diff --check successfully.
- No unit/integration tests, verification apps, test instructions, compilation, application launches or runtime verification were added or run. This is source/document review only, not a claim of executable validation.

Final handoff: steps 1–6 were accepted by continuation; step 7 and overall completion remain pending explicit user validation. If feedback identifies an implementation/documentation mismatch, fix it and update this record before marking final acceptance. Otherwise record completion after the user accepts. Do not infer further implementation scope from the existence of possible future extensions.

## Post-implementation correction — Toolbar icons hidden by the initial mode

Status: corrected in source, awaiting user validation. This decision supersedes
historical references to the Text presentation default in steps 5–7 above.

The user supplied Composer button PNG paths in EditorUi::init, but the toolbar
still displayed only labels. Source inspection confirmed that those paths are
passed to ToolbarButton and that Toolbar::draw deliberately skips resolving them
when the effective mode is Text. ToolbarConfiguration had selected Text by default.

Changed ToolbarConfiguration's initial presentation to IconsAndText (size remains
16). This makes a supplied path visible without requiring an additional toolbar
configuration call; existing pathless buttons remain textual. UserInterface's
missing/invalid preference fallback and the no-active-session getter use the same
configuration default automatically. Stable persisted values (0/1/2) are unchanged.
An explicit saved Text preference is still respected; in that case the user must
choose Icons and text or Icons with tooltips in the existing UI Settings window.
No application-specific override was added, so session preference inheritance
continues to work.

Updated public comments and API documentation for the new default and clarified
why an explicit Text mode hides PNGs. Preserved the user's Composer lambda changes
and staged PNG assets. No CMake, image loader or renderer changes were needed.
Source/diff inspection only; no builds or tests performed. Runtime display remains
pending user validation.

## Handoff record template

For each implemented step record: status (in progress / awaiting validation / accepted), changed files, implementation decisions, source inspection performed, known limits, unfinished work and exact next action. Preserve earlier user feedback rather than replacing it with an unsupported acceptance claim.
