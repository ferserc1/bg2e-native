# M09-04 — Native confinement and final render retirement

[Progress](../progress.md) · [Step](step_04_native-render-deletion.md) ·
[Source delta](native-changes.tsv) · [GPU bridge reference](../../../../doc/api/gpu/Gui_bridges.md)

## Delivery and acceptance basis

Implemented over inherited uncommitted M09-01/02/03 work at HEAD
`1889bdb12ade85d820eaab4a7365760fa9c34f50`. The project lead's continuation
accepts M09-03 under the execution convention; compilation/checks are assumed
successful with backend/scenario details unspecified. This delivery awaits the
lead's validation. No configuration, C++ compilation, shader compilation,
application, native backend, package or runtime test was executed by the agent.
No CMake file, shader, SDK asset or third-party file was changed in this step.

The source ledger records 23 removed, nine added and nine modified C/C++/header
files. All remaining render files are retired; no forwarding namespace or archived
runtime implementation remains. The relocated data counts once as removal and
once as addition. Historical M09-01 inventory files remain unchanged; use all three
subsequent delta ledgers when comparing that snapshot to current source.

## Actual contracts and ownership

| Planned seam | Actual implementation and responsibility | Assessment |
|---|---|---|
| Native UI composition | Public `gpu::GuiRenderer` in `gpu/GuiRenderer.hpp`, factories in `gpu::vk` and macOS `gpu::metal`. Initialization borrows Instance/PhysicalDevice/Device/WindowSurface, never a draw Engine or UI object. prepareFrame/draw accept GPU command/frame objects. | Equivalent refinement: one portable bridge interface rather than two native public interfaces. |
| UI lifetime | Existing private ImGuiBackend interface remains UI-owned. New private `ui/detail/GpuImGuiBackend.hpp` only adapts draw Engine accessors to the GPU bridge and forwards frame/texture operations. UserInterface retains context, platform backend, prepared-pair validation and preview registry. | Required private adapter keeps existing UI lifecycle isolated from public native-free GPU API. It is coordination, not a renderer/pass implementation. |
| Vulkan ownership | `gpu/vk/GuiRenderer.cpp` owns descriptor pool, current surface configuration, actual swapchain image/minimum counts, configuration-pointer lifetime, renderer recreation and registered preview IDs/generation. Existing LOAD/STORE, waitIdle-before-recreate, texture invalidation and native recording retained. | Preserved; source relocation with primitive pointers. |
| Metal ownership | `gpu/metal/GuiRenderer.cpp` retains autorelease scopes, acquired-texture metadata, surface/format/sample generation checks and color-only pass validation. GPU owns and ends the encoder exactly once. Metal bridge borrows primitive device/surface pointers. | Preserved; no depth/stencil or submission added. |
| Initialization/cleanup | GuiRenderer requires a current context without a renderer backend and rejects repeated initialization before shutdown. Owner explicitly shuts down after stopping GPU users, including partial initialization, before context/device destruction. UI's existing failure cleanup does this. | Equivalent defensive validation; no implicit synchronization in destructor. |
| Drawable pixels | `gpu::drawableSize(SDL_Window*, BackendType)` in WindowGraphics.hpp/.cpp; DrawGraphicsExecution coordinates zero-size skips and resize using returned pixels. MainLoop retains logical window/input coordination and backend window flags; stale SDL Vulkan headers removed. | Equivalent refinement; unavailable backend/null window rejected. |
| Standalone chooser | `gpu::StandaloneGuiWindow`, PImpl/RAII, owns OpenGL context/window, SDL platform/render backend calls, native frame/presentation and balanced SDL subsystem references. app owns temporary ImGui context, widgets/preferences and device selection. Current context explicitly selected and previous ImGui/native GL contexts restored. | Equivalent refinement; checks creation failures and unwinds partial initialization. Correct macOS platform macro enables required core 3.2 GLSL150. Uses drawable-pixel viewport for high-DPI display. No new engine rendering backend. |
| CPU blue noise | Exact-byte move to `lib/src/bg2e/utils/blue_noise_data.h`; same-folder private include and generator usage path. Generator now emits the existing GPL prefix, with control-flow formatting only otherwise. | Data and generation algorithm preserved. SHA-256 `d5dbbb33825512404e4b7966ef8d6e515bcaf997e63ecf4004ee3e0861046204`. No regeneration executed. |

GuiRenderer's registerTexture/removeTexture/textureGeneration retain the existing
backend preview helpers and Vulkan generation invalidation. No reflected settings,
shader bindings, public pass contract, resource representation or graphics ABI was
changed. The GPU layer acquires no dependency on draw, UI, scene or another
higher-level engine namespace. No public engine header includes ImGui.

## Static verification

| Check | Observed result |
|---|---|
| `python3 .../audit_native_sources.py` | 704 library/supported app/example/test source/header files: no retired render/scene-vk references, missing bg2e includes, native graphics calls/headers outside GPU, forbidden GPU includes or public ImGui includes. 41 source-delta hashes and GPL prefixes verified. Textual check, not preprocessing. |
| Native method comparison against step-entry snapshot | 16 Vulkan/Metal methods identical modulo whitespace and Engine-to-primitive pointer substitution: composition, resource registration/generation, shutdown, surface configuration/pool/recreation, Metal preparation/release. Initialization guards and Vulkan prepare-state validation reviewed separately. |
| Protected engine snapshot | 308 pre-existing GPU/draw/bake/base files byte-identical, excluding the additive gpu/all.hpp bridge exports. Includes current native tables/cache/residency, timing/completion/generation/BLAS, pools/profiling and FSR implementation. |
| Data and dependencies | Exact whole-file blue-noise hash and 128x128/RG8/four-layer metadata retained. Vendored SMAA tables/licenses, Vulkan SDK/MoltenVK/deployment files and shaders untouched; draw SMAA includes retained. |
| Source style | 18 modified/new source/header GPL prefixes and lexical delimiters reviewed; braces and private/public include boundaries inspected. Generator control flow braced, no algorithm change. |
| Updated inventory, temporary output | 1,364 tracked plus nonignored untracked source/config files, 3,225 resolved lexical include edges; every missing M08 baseline path has an explicit retirement ledger row. Old inventory snapshots not overwritten. |
| Remaining global references | 65 files with broad lexical legacy matches are outside supported engine/products or contain textual historical/template evidence; obsolete examples/shaders/templates/distribution cleanup belongs to M09-05. No claim of complete M09 retirement across those pending surfaces. |
| Whitespace and documentation | `git diff --check`, changed-page local links and exact relocated architecture excerpts checked. |

Reproduce the focused source check with:

```sh
python3 docs/render_to_draw/plan/milestone_09_cleanup/audit_native_sources.py
python3 docs/render_to_draw/plan/milestone_09_cleanup/inventory_sources.py --output-dir /tmp/bg2e-m09-04-inventory
```

The inventory generator now includes nonignored untracked files, understands all
three removal ledgers, recognizes relocated data as retained and scans native
Metal/SDL/OpenGL seams. Its optional output directory preserves original evidence.
Lexical evidence includes comments/strings and backend binding/cast seams; it is
not a semantic call graph. These commands do not execute engine/native code.

## Lead validation and pending-plan impact

| Owner | Required follow-up |
|---|---|
| Project lead, M09-04 acceptance | Compile both supported backend configurations. Exercise Vulkan/Metal overlay, preview removal/rebind and resize/minimize/restore/configuration changes; check Vulkan pool counts/generation and Metal encoder ownership. Verify shutdown after partial initialization and after stopping users. Exercise chooser zero/one/multiple devices, accept/cancel/close, high DPI and failure cleanup, then launch the selected engine. |
| M09-05 | Retire obsolete examples/shaders/build/template/test references using the latest ledger. Keep new globbed GPU sources, native vendor backend linkage, SDK/Metal deployment, relocated data and SMAA resources. Do not restore render headers to repair an obsolete target. No CMake changes authorized or performed in this delivery. |
| M09-06 | Run final installed/headless/no-RT/optional SDK and product matrices on final contracts, plus native GUI lifecycle checks. Static source confinement is not native/performance proof. |
| M09-08 / M10-02 | Keep canonical API docs in sync with GuiRenderer, StandaloneGuiWindow, drawableSize and primitive ownership. Old ui native implementation paths are thin adapters now. |

The affected pending steps, architecture decisions and maintained GPU/UI/MainLoop
references have been updated. M09-05 is identified as next; further implementation
waits for the lead's acceptance or correction instruction.
