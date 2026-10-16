# M02-05 draw widgets — lead validation

No new executable/target is added until M02-07. Rebuild the library and existing
draw/production callers. The agent has not compiled or run these checks.

In a draw UserInterfaceDelegate, keep a DrawMaterialEditor member, initialize it
with the draw Engine and shared DefaultRenderLoopDelegate, and call
setSelectionManager(drawSelectionManager). Alternatively setEditMaterial(material)
for explicit live-only edits. Call editor.draw(*ui) from drawUI. The UI session
must be initialized; call editor.cleanup during tool teardown before Engine
cleanup. Widgets have weak delegate/selection/material ownership, and pending
commands cancel on cleanup/destruction/rebinding. init and cleanup run on the main
thread at lifecycle boundaries. Never draw one preview into another UI context.

1. On Metal and Vulkan, pick a loaded submesh. Check first-material display,
   scalar/color/alpha/UV/channel/invert/scale controls and all six texture slots.
   With multiple selected submeshes, edited fields apply to all valid targets;
   untouched fields retain each target's own values. Direct-material mode changes
   live attributes only; selection mode patches CPU definitions and live fields.
2. Programmatically call queuePatch(MaterialModifier) and queueClearTexture(slot).
   Pending controls disable until the next safe delegate update; success updates
   the next scene frame and resets deferred FSR history. Custom renderer/effect
   histories use onChanged(Renderer&), called at the safe boundary. No material
   setter/upload/readback is invoked during UI drawing.
3. Select an image using the file/history picker (including paths with spaces,
   non-ASCII text and quotes). Actual texture loads happen in the queued edit.
   The picker history uses names rather than loading thumbnail files during UI
   drawing. Check all six previews after replacement and Clear. Clear is explicit
   native attribute intent; MaterialModifier null/empty-path policy is unchanged.
4. Reload a Texture in place or update its sampler at a stopped boundary. The
   next draw detects new image/sampler identities. Old GPU snapshots/registrations
   survive previous UI submissions and are retired at a later frame after drain.
   No stale descriptor, dangling texture or per-frame registration growth.
   Metal uses ImGui's fixed linear-clamp preview sampler; Vulkan uses the supplied
   texture sampler. Scene sampling still uses the actual material sampler.
5. Resize repeatedly, including a Vulkan surface format/image-count reinit.
   Active preview handles re-register with the new backend generation; obsolete
   descriptors must be removed before renderer shutdown. Check normal stable
   frames do not wait for preview retirement; waits occur only on retirement or
   lifecycle transitions. Images must already be sampleable ShaderReadOnly 2D
   non-integer color textures. Unsupported previews show a diagnostic.
6. Queue edits then remove/reparent/replace a node/component/mesh, replace the
   scene/renderer, change editor binding or destroy the editor before the queue
   executes. Stale generations and invalid weak identities must not edit another
   target. Original queued selection is used if it remains a valid scene member.
   All-invalid targets report an error; valid remaining targets may still apply.
7. Force missing texture files, load failures and throwing change notifications.
   Error text appears without dangling callbacks. Existing nontransactional
   attribute edits remain; upscaler invalidation and redraw still occur after
   partial failure. Reapply after correcting the input. Fully paused scenes keep
   their retained image until resumed, as in the existing delegate contract.
8. Destroy widgets with previews used earlier in the same UI frame. Retire them
   only at the next frame after GPU completion; UI cleanup invalidates retained
   preview handles and releases GPU snapshots before renderer/context shutdown.
   Reinitialize the UI and use the widgets again; no old session ID is reused.
9. Check no-RT, shadows-off, normal/reduced/available FSR and query alpha cutouts
   after material changes. Keep GraphicsSettingsWindow live/dynamic. Ordinary
   production MaterialEditor/TextureWidgets/ComponentInspector still operate.

The widget decision table and deferred scene/property/UV/bake scope are in
[widgets.md](../../../docs/render_to_draw/plan/milestone_02_manipulation/widgets.md).
