# M07-03 scene editing handoff

M07-02 is accepted by project lead continuation. Implementation starts at
`f7c2498cb4c29e27921c91809043dcf5fe4d963c` with a clean checkout. No CMake,
build, shader, runtime, packaging, commit, reset or staging operation is performed.
The repository's referenced skills.md is absent; operative architecture decisions,
architecture.md, current headers and the latest M06 review are the source baseline.

## UI and selection

Composer uses the shared SceneTree with optional onSelect/onReparent intent
callbacks. Production immediate behavior remains the default. Drag payloads are
checked against the current tree before their node address is dereferenced.
Composer callbacks queue weak node/parent references through AppDelegate and check
renderer/document generation and current membership at the stopped boundary.

SelectionManager::addNodeToSelection explicitly selects a hierarchy node without
choosing submesh zero. The tree uses this path; the selected mesh's separate
submesh rows use typed draw selection and the existing DrawMaterialEditor. Ctrl
extends/toggles hierarchy selection; the existing checkbox extends submesh picking.
Structural/property actions use the primary selected node. Existing material and
geometry actions retain their selection-wide behavior. Tree reparent changes local
placement semantics: it preserves the local transform and therefore may change the
world pose. Cycles, detached/cross-document targets and document-root moves fail.
Camera/light limit failures restore the previous parent.

All document nodes receive draw gizmo tooling. The current selected node controls
the transform handles; camera/light/environment icons retain their own types.
DrawableComponent::isSceneGeometry defaults to true; GizmoComponent overrides it
to false. Node::drawable resolves only scene geometry, including custom subclasses,
so helper icons cannot enter normal rendering, bounds, model export or bake target
selection as authored meshes. Explicit getComponent<GizmoComponent> remains the
tool access path. Composer geometry and UV target lookup use Node::drawable.

## Inspector and graph publication

Add group/camera/light/environment creates a child of the primary selection (or
root). Add transform/controller attaches a missing component to that selected node.
Cameras may remain static; no automatic controller is invented. The inspector edits
lightweight cloned transform/camera/light/environment/orbit components using the
shared ReflectionWidget. GPU Drawables are never cloned while drawing the UI.
Component replacement checks the original weak component's current owner and exact
concrete type; priority/serialization flags survive. Transform removal is disabled
because editor tooling requires a transform. Other supported components can be
removed. Existing typed material, environment and camera projection controls remain.

StageScene::validateEditingGraph requires an enabled camera and at most eight
enabled lights across enabled branches, and prepares/validates camera projections.
Enable/reparent/component/new-node actions validate before retaining publication,
rolling back the relevant change on failure. Removing the active camera branch via
the node delete action remains explicitly rejected; disable/remove its component
is allowed when another enabled camera survives. The first enabled depth-first
camera is active. The stage fill is suppressed when eight authored lights are
present; zero authored lights are permitted and the fixture can be switched off.
The old five-light creation limit is removed.

Queued edits drain the device, retire bake sources, rebuild tools/selection when
required, mark dirty, invalidate histories and reattach extraction/catalog state.
Environment-image inspector changes generate a candidate snapshot before retaining
the new component; generation failure restores the old component. Partial edit
failures still notify dirty/history state conservatively. Async import and future
multi-target bake consumers must use these exact mutation/notification boundaries.

## Camera and settings

Viewport input is delivered only to the active camera's nearest authored ancestor
orbit controller, respecting its enabled state; inactive controllers receive no
mouse events. Switching cameras clones old/new controller chains to clear gesture
state while preserving authored settings. Manipulation invalidates baked sources.

Framing and explicit scored view analysis reuse the existing stopped GPU actions.
World candidates are converted into the orbit parent's coordinates before the
shared utils helper applies them, retaining projection settings. Rigid/uniformly
scaled parents are supported; singular/reflected/sheared/nonuniform parents are
rejected without changing the pose. Static cameras remain usable for rendering and
IO; framing requires an authored orbit controller. Camera-child offset adjustment
retains the shared helper's existing axial-offset semantics. GraphicsSettingsWindow
continues to own live renderer/upscaler import/export and settings rebinding.

## Static evidence and project lead checks

Agent checks: declarations/call sites, weak ownership/document membership,
rollback paths, camera/light policies, GPU helper separation, include boundaries,
GPL headers, source formatting, Markdown links and git diff whitespace. No
compilation or runtime evidence is claimed. Backend/package coverage is unspecified.

After compiling draw_bg2e_composer, check on available Vulkan/Metal backends:

1. Tree selection of groups, meshes, cameras and lights; Ctrl multi-selection;
   separate submesh/material selection; viewport picks and transform handles agree.
2. Duplicate a subtree, edit transforms/materials/projection and verify the original
   remains independent. Reparent locally, reject cycles/root moves, and save/reopen.
3. Add/remove/edit supported components. Disable/remove the last enabled camera:
   expect rejection and unchanged usable camera. Switch to another/static camera:
   only its controller responds; static rendering needs no controller.
4. Create/import/enable/reparent eight lights, reject the ninth enabled light, and
   verify rollback under disabled branches. Remove all authored lights and turn off
   the stage fill. Check camera/light icons stay outside scene exports/bounds/bakes.
5. Frame/analyze geometry with translated/rotated/uniform camera parents; unsupported
   parent transforms produce a status message and preserve the prior pose.
6. Edit environment HDR (including an invalid path), material textures and graphics
   import/export; verify dirty/history updates, fallback/SDK-unavailable settings,
   cancellation of active bake sources and safe document replacement/close.
7. Regression-check legacy SceneTree immediate selection/reparenting and production
   mesh rendering/custom Drawable subclasses after helper/geometry separation.

M07-04 owns background/service import; M07-05 owns multi-target bake scheduling.
This delivery does not establish either feature or application-wide runtime parity.
