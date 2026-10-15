# M02-01 validation

`main.cpp` is a manual CPU-only contract fixture. The project lead compiles/links
it against the engine with the ordinary public include/SDK dependencies. No CMake
registration or new runnable example is added by this step. It does not initialize
a GPU/window: it checks draw selection/dedup/callbacks, same-count geometry
replacement, detached-component and deleted-node lifetime, checked submesh access,
schema round-trip, fresh clone IDs, typed gizmo access and overlay mode.
The agent has not compiled or run it.

With available Vulkan and Metal, check the library and existing deferred example
still compile. At a stopped scene/GPU boundary, attach a draw GizmoComponent to
camera/light/environment nodes, call synchronizeResources and inspect drawDrawable
and transformDrawDrawable after making the transform facet visible. Expect matching
asset submesh names/groups, axis colors/unlit materials, no tracing meshes and fresh
per-instance draw resources for clones. Release tools/resources before Engine cleanup;
cleanupDrawResources is available at the same stopped boundary. Remove a node before
a queued gizmo update executes: weak callbacks must skip it without dereferencing it.

Check production model editor/composer and deferred/forward manipulation: submesh
selection, groups, additive/removal callbacks, camera input capture, type gizmos,
axis transforms/highlight restore and cleanup retain their behavior. Both execution
modes reject cross-mode geometry. Draw CPU selection can be used before GPU init;
draw GPU picking explicitly throws until M02-02, and native overlay draw rejects
draw mode until M02-03 adds its GPU recording contract. Those expected rejections
must not clear existing selection. Optional tools are application-owned and no
renderer requires them; headless work does not need a manager.
