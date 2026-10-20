# Composer environment selection and synchronization correction

The lead requires fresh scene selection, editable HDR/procedural colors, default
sky after active-component removal, and ignored additions while another component
is active. This corrects M07-07; it neither accepts earlier pending native checks
nor authorizes M07-08. Existing uncommitted tracing/shadow work is preserved.

## Confirmed source findings

- Composer created a hidden Environment on its auxiliary stage root, outside the
  authored tree. Existing adoption removed it when finding an authored component,
  but editor and renderer each independently resolved the first Environment.
- Inspector edits removed/replaced the component and directly synchronized the
  generator with the edited clone, even if that component was not the selected
  source. Generation errors rolled the edit back, making colors appear to revert.
- Renderer generation intentionally retains the last good GPU snapshot on failure.
  The supplied primitives_scene refers to `black.jpg`, not a floating-point HDR;
  that is an 8-bit JPEG, not floating-point HDR, so it is incompatible with the
  environment HDR loader and can leave procedural sky visible. Other reported HDR
  sources were not inspected/reproduced; their validity is not inferred.
- Earlier chat explanations that CubemapRenderer::cleanup clears its settings or
  `_pathSource` prevents color rebuild were incorrect. Cleanup preserves settings;
  synchronize includes `_rebuildRequested`, and renderer prepare runs even while
  scene presentation is paused. Those are not the diagnosed causes or changes.

## Delivered contracts

The first Metal `metalArgumentRecordArray` experiment used `constant Record*`
function arguments for every buffer. That MSL form is reserved for Tier-2
pointer-indexed resources and is not valid for ordinary one-record argument
buffers. GPU now distinguishes `metalArgumentRecordsIndexable`: draw tracing uses
true for image/sampler records8/9, while query shadows and primitive arrays use
ordinary argument buffers. The ResourceSet exposes each group in its native form;
rebuild paired Metal shaders after this contract correction.

Scene environment selection requires an attached Scene root. Calling
setMainEnvironment before root attachment (including renderer startup) leaves
selection implicit; it cannot freeze a default null before document load. Once
attached, explicit null means default sky. Automatic first lookup captures once
and ignores disabled branches; updateEnvironment never promotes an ignored sibling
after selected-source removal.
Existing Model Edit does not opt into explicit selection: its legacy lookup
re-resolves after updateEnvironment, so remove/re-add keeps its previous behavior.

`Scene::setMainEnvironment(EnvironmentComponent*)` opts into explicit selection;
nullptr explicitly means renderer defaults, not a traversal fallback. Weak node
and component identities plus enabled membership validation prevent use of stale,
removed, detached or replaced sources. Scene root changes/release clear selection.
Scene destruction clears the retained root's back-pointer as well, so draw
consumers cannot dereference a destroyed owner when callers retain its graph.
Unmodified Scene clients retain first-enabled traversal behavior on their first
lookup, then capture that component. `updateEnvironment()` never silently promotes
another component after explicit selection was removed.

Composer no longer creates a hidden stage Environment. Its StageScene records
eligible authored node/component identities, resets selection on successful
document adoption, and retains a valid selection over subsequent additions. If
the active component disappears, previously ignored components stay ignored and
the renderer's own procedural sky is used. Only a fresh/newly enabled component
can fill that empty selection. Multiple components on initial Open choose the
first enabled depth-first entry. This is runtime selection, not a new on-disk
active-ID format; reopening a document starts a new deterministic choice.

RendererDeferred, RenderQueueVisitor snapshots and live settings all honor the
same explicit selection, including nullptr. CPU queue extraction never clones
an ignored Environment into an active snapshot. Failed import publication restores
the prior selected component and eligibility snapshot before renderer rebinding.

Built-in Environment edits copy validated authored settings in place, retaining
component/widget identity. Custom component replacement transfers explicit active
selection when appropriate. The inspector no longer directly generates resources
for arbitrary edited environments; the stopped renderer synchronization publishes
only the selected source. HDR assignment, clearing, stored RGB and default fallback
use the existing generator's change detection. HDR failures retain the requested
component and last good GPU resources, and are exposed in status/inspector rather
than silently reverting values. `environmentImage` JSON aliases the existing
`equirectangularTexture`; missing/empty references clear previous image state.
The reflected resource picker exposes Browse and Clear, accepts intentional empty
selection, and filters to `.hdr`/`.exr`. Cancelling Browse preserves the selected
path; Clear or explicit empty selection returns to procedural sky. Its Clear button
is opt-in metadata, so unrelated resource editors retain their prior layout.

## Validation and scope

Fresh document adoption requests a stopped generator source-cache reset and
rebuild, even if the HDR pathname matches the preceding document, so Open rereads
the file contents. Ordinary in-place edits still use the generator's normal cache.
The supplied `primitives_scene.json` names `black.jpg`; it is an 8-bit JPEG, not a
linear floating-point HDR accepted as an environment source. The renderer reports
the failed request and keeps last-good resources. Validate with a real HDR and
check `environmentError`; selection correctness alone does not prove HDR decode or
sky generation succeeded.
SceneInputs queries the attached Scene's explicit selection instead of retaining
an old Environment pointer. Selection resolution skips disabled branches, and
opening an attached root resets its prior choice before selecting the new document.

Extended the CPU Composer schema probe with selection precedence, explicit null,
active removal, disabled/detached/foreign owners, new selection, root release,
legacy automatic behavior and image alias/clearing. Native/GUI scenarios are in
[the Composer IO matrix](../../../../tests/draw/composer_io/README.md).

Static signatures, ownership, selection/change/publication paths, source style/GPL,
local links and diff whitespace only. No configure/build, shader compilation,
C++ fixture, application, GPU or package execution. Lead must test real valid HDR
files and RGB sky changes on available Metal/Vulkan, ignored components, removal,
save/reopen, failed sources/import rollback and ordinary Model Edit coexistence.
No shaders, CMake, GPU APIs, resource-table capacities or texture-count policy are
changed by this correction. RendererConfiguration.environment remains an explicit
application-owned fixed-resource override outside Composer's default setup.
