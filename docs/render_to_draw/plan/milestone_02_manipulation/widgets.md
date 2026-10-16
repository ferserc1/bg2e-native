# M02-05 delivered widget decisions

Date: 2026-10-16. Baseline `7b217bcb6d9a3473ea522dd258d8d87b87150ddb` plus preserved
uncommitted M02-02–04 work. M02-04 accepted by project lead continuation; extra
backend/scenario coverage unspecified. No agent compilation/runtime results.

## Widget inventory and decisions

| Widget/workflow | Delivered choice | Later owner |
|---|---|---|
| Production MaterialEditor | Preserve existing signatures/behavior. Use separate DrawMaterialEditor: production synchronous setters, native preview fields and callbacks are incompatible with draw queued lifetimes. | M06/M07 integrate the draw widget, M09 removes production. |
| Material scalars/colors/alpha/UV/channels/inversion/scales | DrawMaterialEditor reads fresh primary live getters, collects only changed JSON properties, queues one partial MaterialModifier patch for valid selected submeshes or an explicit live material. Multi-target values show the first material, with explicit UI explanation. | M02-07 presets, M06/M07 inspectors. |
| TextureWidgets | Preserve production widget. Separate DrawTextureWidgets draws a session preview, selects file/history paths or returns explicit Clear intent. It never loads files/mutates materials during drawing. | M02-07 and M06/M07. |
| Preview registration | Native-free TexturePreview/UserInterface::texturePreview; private UI registry and GPU backend native registration helpers. | Reuse in later image widgets; M09 retires old native previews. |
| History thumbnails | Existing filename history works in the draw widget. No on-demand thumbnail texture loading in UI draw. General async/prefetched thumbnail browsers are not required by M02 and remain app work. | M06/M07 browser workflows. |
| SelectionManager/SelectableList | Typed weak M02 selection and existing graphics-independent SelectableList wrappers are reused. Material editor resolves selected draw submeshes each frame, with no retained selection callback. | M02-07 input/list UI. |
| SubmeshSelector/DrawableEditor | Existing production resource mutation/mesh interfaces are not assumed draw-compatible. Compose typed selectedItems/DrawMaterialEditor and generic list controls for M02. | M06/M07 full mesh/metadata tools. |
| ComponentInspector/ReflectionWidget/NodeEditor/SceneTree | Existing component inspector directly invokes reflected setters/removal against raw nodes. Do not attach it to live draw scene editing in M02. Generic reflection value editing and primitive widgets remain reusable. Safe node/component/property/tree queues belong to app integration. | M06-03/M07-03. |
| GraphicsSettingsWindow | Reuse the M01 dynamic catalog/facade unchanged; no fixed RT/effect sections added. | M03 and apps add actual live descriptors. |
| UvMapPreview/bake widgets | Not ported into material widgets. | M04 bake preview contracts; M06/M07 application integration. |
| Light/camera/environment/gizmo property tools | Primitive controls and reflection value editing are reusable; full scene-property workflows are deferred. | M06/M07. |

New public headers are ui/DrawMaterialEditor.hpp, DrawTextureWidgets.hpp and
TexturePreview.hpp, exported by ui/all.hpp. They expose no ImGui/native Vulkan
types. Existing production UI headers/implementations are preserved; UserInterface
retains its earlier production overloads alongside the new preview method.

## Safe material editing

DrawMaterialEditor binds a borrowed draw Engine and weak DefaultRenderLoopDelegate.
It selects either a weak draw SelectionManager or one weak MaterialBase. draw()
reads fresh values and queues changed fields; queuePatch and queueClearTexture
also support programmatic use. Named/group/regex presets use the M02-04 utility
from application-owned safe commands. Pending controls disable while queued.

Commands carry renderer generation, weak editor state, binding revision and a
copy of typed weak SelectionItems (or weak explicit material). At execution,
selected nodes must still belong to the delegate's current scene and retain valid
component/resource/selection identities and engine. Rebinding/cleanup/destruction
cancels old commands. Invalid targets are skipped; entirely invalid targets report
an error. It intentionally edits the original valid queued selection even if the
user subsequently selects another object without rebinding the editor.

Selection edits patch independent CPU/live material targets through MaterialModifier;
explicit material mode is live-only. Clear uses an explicit texture setter on the
same targets, then existing updateTextures; it does not change the utility's
null/empty-path policy. Combined control changes and Clear can share one command.
Texture/cache/callback errors remain nontransactional; the widget catches errors
for display. The queued delegate requests redraw, and touched targets trigger
RendererDeferred::resetUpscaler plus optional onChanged(Renderer&) at the safe
boundary, including partial failure. Future effect histories use that callback.
All editor/UI access is main-thread only. Fully paused scenes retain their image.

## Preview resources and UI session lifetime

Texture exposes shared imageResource/samplerResource snapshots. A preview retains
those exact GPU wrappers rather than the mutable Texture object. DrawTextureWidgets
uses weak identities to detect in-place reload/sampler changes. Valid previews
require same-engine, initialized, Sampled ShaderReadOnly 2D non-integer color
images; drawing never uploads/transitions source images. Depth/cubemap/integer
preview conversion is outside this step. Shader/color interpretation is the UI
backend's ordinary sampled texture behavior; no HDR tonemapping is inserted.

The private PreviewRegistry belongs to UserInterface's draw session. It owns
registration/resource entries while handles are alive and through the current
UI draw data. At a following newFrame, orphan entries cause a GPU drain before
unregistration/release. Stable frames perform no preview retirement waits.
Cleanup drains, unregisters and invalidates handles/releases snapshots before
backend/context shutdown, even if handles outlive the session. A preview refuses
a different active UI context. Tools must release their other resources before
Engine teardown as usual.

Vulkan's existing private UI backend delegates AddTexture/RemoveTexture to new
gpu::vk helpers. It tracks preview IDs, removes them before backend restart, and
increments a texture generation; active handles register again on next use.
Metal delegates to a gpu::metal helper exposing the retained native texture token;
ImGui Metal uses its standard linear-clamp sampler, independently of the retained
material sampler. Native handling stays in GPU, which gains no dependency on
higher engine namespaces. ImGui remains a third-party implementation dependency
only; no ImGui types appear in these public contracts. No CMake change is needed
for the existing source discovery/include/link configuration.

## Verification and handoff

[Lead integration/backend/production checklist](../../../../tests/ui/draw_material/README.md)
checks controls, matching/lifetimes/queue generations, clear/failure/history reset,
preview reload/retirement/UI reinit, no-RT/FSR and production regressions. M02-07
supplies the final example; this step adds no executable. Agent checks are static:
source/include/caller interfaces, GPU dependencies, setter mapping/queued weak
ownership, preview session/descriptor generations and resource retention,
GPL/style/whitespace and local links. No build, shader compilation, executable or
runtime check was run. M02-06 viewpoint tools follows lead acceptance.
