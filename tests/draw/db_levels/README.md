# Manual DB representation and draw loading probe

Include `probe.hpp` in a manually linked executable. No CMake target is added.

- `runSceneRepresentationProbe(existingDisposableDirectory)` needs no Engine, SDL initialization, device or referenced assets. It writes small documents only inside the supplied directory. It checks legacy-name decoding, unresolved mesh/environment references, component priority, node flags, reference cloning, path rebasing and unknown document/component retention in raw IO. Typed graph loading must reject unknown component types explicitly.
- `runDrawSceneLoadProbe(engine, scenePath)` requires an initialized draw Engine with producers stopped and no active frame. Supply a scene with BG2/OBJ references. It checks draw-only realization and repeated same-context loading without replacement. Run on available Vulkan and Metal backends. Release all graph/resource owners before Engine teardown.

Also check the following manually:

1. Compile the library and existing production/draw examples, including public overloads and external custom component registrations.
2. Load a textured two-submesh BG2 via `loadDrawableBg2(path, engine)` and `loadScene(path, engine)`; compare material descriptors, textures, names/groups/visibility and progress callbacks. OBJ is geometry only; MTL import remains future work. Try OBJ with supplied normals/UVs and missing normals/UVs, and reject malformed indices.
3. Read a graph through `loadScene(path)` while mesh/HDR files are absent; serialize references into another directory and verify that neither resource was opened, copied nor created. Raw document IO must preserve unknown JSON fields/components. Typed graph IO supports registered schema parsers and intentionally rejects unsupported components.
4. Initialize the previously unresolved graph with `rootNode()->loadResources(engine)` after making its assets available. Verify errors for absent/unsupported resources, production-mode components and a second Engine; failure must not silently yield a partial success from the high-level load helper.
5. Register a custom component with its default creator, graphics-free `deserialize(data, basePath)`, `serializeReferences(basePath)` and optional `loadResources(engine, progress)`. Verify that the new path never calls its production Creator. A production-only custom component must report its missing schema contract.
6. Check existing production scene load/save. Production DrawableComponent accepts both legacy `name` and explicit `path` references. New draw scenes use `saveSceneReferences` for reference documents; complete draw resource export is deliberately unavailable until M06/M07, including GPU-only images and edited material/geometry contents.

These probes have not been compiled or executed by the agent. Full resource export, glTF/MTL import and the deferred environment pipeline are separate planned steps.
