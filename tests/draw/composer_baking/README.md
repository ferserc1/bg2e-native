# Composer integrated batch checks

Manual project-lead matrix for `draw_bg2e_composer` (M07-05). Not executed by the
agent. Compile the existing application/library and native paired shaders using
the supported platform generator. No new target or CMake configuration is added.

1. Build a scene with at least three independently transformed drawable nodes,
   distinct submeshes/materials, a group and a camera. Generate UV2 for each target
   through the existing preview-target action. Include a genuine overlapping atlas:
   its warning must remain nonfatal; malformed coordinates must fail preflight.
2. Select two nodes (then a parent and its descendant). Bake selected subtrees
   with AO and GI. Progress must identify each target; duplicates must bake once.
   No receiver material should change before the final target finishes.
3. Bake the whole scene, including an unselected target. Editor floor/gizmo/fill
   helpers must not become receivers. Compare single-target and batch output for
   the same frozen source/settings. Check chart padding and RGB UV2 orientation.
4. Select each completed node in SceneTree. Its UV and completed result preview
   must correspond to that node, not the last baked node. Closing/reopening the
   bake window must preserve completed results; new document must hide old ones.
5. Repeat with RGB8 and RGB32F, distinct resolutions and GI exposures. Verify
   linear PNG versus HDR archive sources, finite display limits, baked AO/GI
   metadata, all submesh slots and preservation of unrelated texture settings.
6. Cancel during the first/last target and restart. Edit/delete/clone/reparent a
   source, regenerate UV2, import/reimport, replace the document, resize/rebind the
   renderer or quit with work in flight. Retire GPU work safely; never apply a
   canceled batch or allow an old queued apply to affect the restarted job.
7. Save/Save as the completed scene, restart and load it. Verify every baked map,
   mode, UV and material identity. Exercise canceled save, unwritable destination,
   existing protected inputs and preparation failures. Failed saves must not mark
   clean; failed bake preparation must not change receiver materials. If an
   adoption setter fails, the error must report possible partial edits and keep
   the scene dirty and usable.
8. Check Vulkan and Metal where available, missing required RT/native profiles,
   raster/no-RT editing fallback, shared graphics controls, production Composer
   and draw_model_edit regressions. Report actual coverage without inferring it
   from this checklist or static review.
