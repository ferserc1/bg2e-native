# M02-04 delivered draw material patch contracts

Date: 2026-10-16. Baseline `7b217bcb6d9a3473ea522dd258d8d87b87150ddb`, with
preserved uncommitted M02-02/M02-03 work. M02-03 accepted by project lead
continuation; additional backend coverage is unspecified.

MaterialModifier now has draw::MaterialBase reference/shared-pointer and
draw::Drawable index/exact-name/applyAll/exact-group/regex-search overloads.
Return values, first-name match, group matching, no-op counts, invalid/null target
handling, parser snapshot and callback/failure policy follow production overloads.
Missing/null/wrong-type/unknown properties remain ignored; texture null/empty
values do not remove textures. The shared patch decoder additionally accepts
finite numeric alphaCutoff on base, production and draw targets, using the native
setter's existing range behavior. No other parser/property policy changes.

Drawable::materialDefinition(index), with mutable and const overloads, exposes
stored CPU SubmeshAttributes material only. material() retains its existing live
alias after loading. Drawable modifier application resolves the live target first,
then applies only specified properties to the CPU definition and separately to
live MaterialBase attributes. Each patched texture gets a fresh base descriptor
for each target. It does not call setMaterialAttributes/updateMaterials or copy a
whole material. Unrelated CPU/live values can differ and remain untouched.
Direct MaterialBase overloads affect live state only. No target ownership is
retained after an application call; the modifier itself remains a reusable snapshot.

The existing MaterialBase::updateTextures path handles realized resource changes,
cache intent, callbacks, completion and final waitIdle before replacing GPU owners.
No MaterialBase implementation change is needed. It also flushes any pre-existing
pending slots, including on a valid no-op patch, matching existing update/callback
semantics. Clean realized slots keep their GPU owners. Callbacks happen per loaded
non-null descriptor before commit; exceptions propagate and are nontransactional.
CPU/live edits and earlier submesh edits are not rolled back, and GPU realization
can still reflect old owners until retry. Callers must stop scene/frame producers;
this utility does not make concurrent scene mutation safe.

Renderer invalidation remains the application's responsibility: queue at a safe
pre-acquisition boundary using DefaultRenderLoopDelegate::enqueueRendererUpdate
and its renderer generation, validate weak target membership/resource identities,
request a scene frame and reset relevant histories, including after partial
failure. RendererDeferred::resetUpscaler is the current history reset hook.
Normal frame extraction rebuilds material uniforms, texture descriptors and
query-alpha tables; there is no stale permanent material table to patch manually.
No renderer pointer, ResourceManager, cache catalog or new native API is introduced.
M03 must extend this application edit notification to its future effect histories.

The seam preserves current Drawable lifecycle semantics: first realization uses
CPU definitions; reload/updateMaterials retain existing live materials. cleanup
snapshots live state into definitions, and clone snapshots source live values into
its independent definitions/live resources. This step does not reinterpret those
lifecycle policies or promise to preserve a CPU/live divergence through cleanup
or cloning. In-place partial application preserves that divergence immediately.

[CPU fixture and live backend/production checklist](../../../../tests/utils/material_modifier_draw/README.md)
cover independent targets, single texture/alpha/UV edits, matching, callbacks and
nontransactional failures, clones, query/no-RT/FSR and redraw/reset integration.
The fixture is supplied for the lead and was not compiled or executed. No new
runnable target or CMake changes. Static checks inspect signatures/callers,
setter mapping, independent targets/texture descriptors, resource replacement,
GPL/style/whitespace and document links. No build/shader/runtime check was run.
M02-04 awaits lead validation; M02-05 widget adaptation is the successor.
