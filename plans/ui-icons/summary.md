# UI icons implementation plan

Status: steps 1–7 implemented; steps 1–6 accepted by continuation; step 7 and final completion await user validation. No implementation step remains. Follow-up correction: default toolbar presentation is now IconsAndText so assigning a PNG path displays it; explicit saved Text settings still take precedence. See [progress.md](progress.md).

## Agreed scope

- Provide PNG icons with transparency throughout `bg2e::ui`, including standalone images, buttons, menus and toolbars.
- The normal high-level input is `const std::filesystem::path&`. Widgets resolve resources internally; application authors do not need to learn resource-loading APIs.
- Also expose `ui.icons().load(path)` returning an opaque shared `Icon` resource for explicit reuse. This is ordinary demand loading, not a preload subsystem.
- Identify icons by normalized filesystem source path, independently of widget identity. Specify size, tint and presentation at drawing time, never in the resource cache key.
- Retain successful icons and failed-path records for the entire UI session (normally the application lifetime). Do not implement eviction, cache clearing, automatic reload, file watching, asynchronous loading or preloading.
- Share texture registration, GPU lifetime handling and image drawing with `TexturePreview`. Do not merge icon caching with preview ownership or general texture caches.
- Support toolbar Text, Icons with tooltips, and Icons and text modes, with runtime changes through the existing UI Settings window. Toolbar configuration owns toolbar icon size; ordinary buttons accept size when drawn.
- Use a visible question-mark substitute for missing/invalid images and log once per failed normalized path. Missing assets must not terminate the application.
- Image atlases are excluded.

## Architecture and baseline

`UserInterface` currently uses `draw::Engine`, private `ImGuiBackend` adapters and Vulkan/Metal implementations. Reuse these actual integration points; do not migrate rendering APIs or modify `bg2e::gpu`.

`TexturePreview` and `detail::PreviewRegistry` already retain exact image/sampler snapshots, register lazily, track backend generations, check ImGui context identity, defer orphan removal until GPU completion and invalidate resources on shutdown. Extract this into a private, neutrally named image registry and shared drawing helpers before adding icons. Keep the existing public preview contract and signatures.

`db::loadImage` decodes PNG to RGBA. `draw::Texture` uploads resources. `draw::ImageLoadContext` demonstrates normalized source keys and image sharing, but retains decoded CPU pixels and includes texture interpretation in image keys. Do not introduce it or `utils::TextureCache` as a second permanent cache beneath the icon cache without a concrete need.

Use one strong icon cache owned by `UserInterface::Impl`. Each icon retains a shared registered-image resource; previews retain the same resource mechanism with their existing transient ownership policy. Registry bookkeeping is not a path cache. Free icon ownership before registry/backend shutdown; outstanding public handles become invalid safely after session cleanup.

For static widget APIs, add a private current-session lookup bound to the active ImGui context, installed during initialization and removed during shutdown. Resolve it at draw time, never store a process-global icon cache or engine pointer in widgets. Existing text-only widgets must continue working unchanged. Missing session is a programming/lifecycle error, distinct from a missing asset; avoid stale access and define a safe fallback for path-based drawing outside an initialized session.

Use linear filtering, clamp-to-edge, straight alpha and no mipmaps initially. Inspect existing backend color/blending behavior to select the correct texture color interpretation; do not guess from material texture defaults or change global UI shaders. Preserve aspect ratio inside a requested logical-size box. Apply `UserInterface` scale once; shared preview methods retain their existing size semantics.

## Implementation steps

| Step | Deliverable | Dependency |
| --- | --- | --- |
| [1](step-01-shared-image-infrastructure.md) | Extract shared image resources and drawing from previews | None |
| [2](step-02-icon-manager.md) | Session-owned IconManager, Icon, path cache and fallback | Step 1 |
| [3](step-03-image-and-button-wrappers.md) | Path/resource image and button wrappers | Step 2 |
| [4](step-04-menu-icons.md) | Path-based icons in menu wrappers and MenuItem | Step 3 |
| [5](step-05-toolbar-icons.md) | Toolbar presentation, sizing and stable identity | Step 3; follows step 4 for sequential delivery |
| [6](step-06-runtime-settings.md) | Runtime toolbar defaults and persistent UI settings | Step 5 |
| [7](step-07-documentation.md) | Public API and usage documentation | Steps 1–6 |

## Execution and resumption rules

1. Implement one numbered step per instruction to continue, then stop for the user's validation. Do not implement any step as part of this planning task.
2. Before each step, read this summary, its step file, `progress.md`, applicable `AGENTS.md` and current source/worktree. Read `skills.md` if present; it was not found during planning. Reconcile partial work before editing.
3. Update `progress.md` before every stop/interruption: implemented and accepted steps separately, current work, changed files, decisions, unfinished items and exact next action. Update affected plan files if implementation requires a justified adjustment.
4. At the end of every step, state in Spanish what was just implemented and which step comes next, including its purpose. Say explicitly that the completed step awaits the user's validation. User instruction to continue accepts the preceding step unless their message says otherwise; fix reported issues before advancing.
5. Do not create or run unit/integration tests. Do not configure, compile, launch applications or run automated runtime verification. The user performs all testing and validates each step. Source/diff inspection is permitted; never report unperformed checks as successful.
6. Do not modify CMake files, vendored dependencies, GPU backends or unrelated rendering systems. Preserve existing public APIs unless an adjustment is explicitly recorded and justified. Follow repository C++ style and full GPL headers.
7. Generated files and documentation use English; chat and completion reports use Spanish. No commits or unrelated application migrations are part of this plan.

## Completion boundary

The feature is implemented after step 7, and accepted only after user validation. No dedicated sample application, application icon artwork, build changes, preload facility or atlas work is required.
