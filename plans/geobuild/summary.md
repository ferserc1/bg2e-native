# Geometry builder implementation plan

## Product and artifact

Build an industrial scene-authoring tool for map layouts, interiors and architectural volumes directly in the Composer viewport. Designers work against imported reference meshes with the existing scene camera. The C++ SDK retains an editable ordered recipe and evaluates mesh booleans with Manifold. The output is an ordinary owner-local drawable with conventional positions, normals, tangents, UVs, submeshes and material slots. A standalone model can travel with its ordinary texture dependencies. Scene loading in TypeScript uses the baked drawable and the existing unknown-component policy.

## Execution protocol

1. On every implementation or validation request, first read [progress.md](progress.md), this protocol, the applicable repository instructions and the single linked substep. The cursor in progress is authoritative. Folder listings, existing code and elapsed time never authorize advancing it. Follow numeric order across directory boundaries.
2. **READY** means the named substep alone may be implemented when the user requests implementation. Record **IN_PROGRESS** before editing. On resumption inspect the working tree and the recorded evidence for that same substep. Keep unrelated user changes. A correction during review stays within the current substep.
3. At the end of that substep update progress in the same working session to **AWAITING_USER_VALIDATION**. Keep Current unchanged. Write the successor path and one-sentence preview in Next after validation. Record the actual short summary, changed areas, checks run with outcomes, checks still unverified, and exact user validation instructions. A pointer to the successor in this state is informational only. Tell the user in their chat language what changed, what was verified and which application actions to try with expected results. If the substep has no dedicated application interface, user validation consists of launching Composer and confirming that existing workflows continue to work normally. Report implementation checks separately. Stop there.
4. Explicit user acceptance promotes the successor to Current and sets **READY**. Append the accepted substep and validation outcome to the log, then record the following successor. Tell the user briefly what the newly ready substep will deliver. Validation alone advances the cursor; wait for a request to implement it. If the user explicitly combines acceptance and a request to implement the next substep, implement that one substep only. A failed review, requested correction or ambiguous response leaves Current unchanged. Rework, report the changes and request validation again.
5. If a real permission or technical dependency prevents completion, record **BLOCKED**, its specific cause, what is already complete and how to resume. Keep Current unchanged. Return to **IN_PROGRESS** once resolved. The repository requires explicit requests for compilation and CMake edits. This Markdown plan does not grant either permission. Identify any required API contract change outside the agreed scope and seek authorization before making it.
6. After the last substep, still wait in **AWAITING_USER_VALIDATION**. On acceptance record **COMPLETE**, Current and Next as none, and summarize the delivered result and any tests explicitly waived by the user. Never silently convert an unavailable SDK, platform, build or application test into a pass.

Update only the cursor for the current unit of work. Retain the append-only validation log and existing notes about unresolved checks through state changes. Each substep ends at its own review boundary, including pure SDK work and the final substep of each major step.

## Implementation boundaries

- **UI framework.** Workspace collection, 32-bit action masks, effective shortcut dispatch and toolbar presentation belong to bg2e::ui. Keep ImGui private. Composer owns workspace composition, gesture policy, session state and document/history transitions.
- **Geometry.** bg2e::geobuild owns CPU recipes, Manifold adaptation, ray construction/intersections, region identity and dimension operations. Keep public CPU geometry usable with no scene, renderer, GUI or GPU initialization. Keep resource realization at the existing draw safe boundary. Keep native GPU diagnostics intact.
- **Scene integration.** On this checkout Composer uses bg2e::draw::Drawable through scene::DrawableComponent. Use these actual owner, cache, instance and export paths. The component holds the recipe; transient editing helpers never enter serialized scene content. General picking remains in bg2e::manipulation.
- **First release.** Ordered list, cube/cylinder/sphere, union/subtraction/intersection, one affine transform per primitive, selectable logical regions and group-based submeshes. Nested operations, profile extrusion, triangle painting, editable UV controls and permanent material tracking across topology changes are deferred.
- **World and local spaces.** Boolean geometry and standalone mesh positions are local to the owner. UV generation uses one world frame across the scene. Moving a finished piece leaves its UVs attached. Regenerating geometry uses the then-current world frame. Regrouping alone preserves UVs.
- **Authorization and verification.** Follow AGENTS.md for project conventions, license headers, GLM and builds. The root checkout contains architecture.md; consult the applicable architecture references and current code before changing internals. Use existing verification surfaces in tests/app/command_memento, tests/manipulation, tests/draw and tests/ui where relevant. Add narrowly targeted regression coverage where existing tests miss the behavior. Application checks are instructions to the user unless execution is explicitly requested and possible.
- **Defaults specified by this plan.** Linear snapping starts disabled with a relative step of 0.05 scene units. UVs use one repeat per scene unit. Cylinder and sphere tessellation defaults appear in step 4. On geometry changes, unmatched regions return to default, and output material slots follow index-based replacement. Treat these as explicit implementation choices for review. Use existing platform conventions for screen coordinates and keyboard modifiers.

## Dependency-ordered index

### 1. [Workspace support in bg2e::ui](step_1/)

Supply reusable workspace selection, action filtering, scoped shortcuts and independent toolbar layouts. Preserve single-toolbar applications.

- [1.1 Define workspace masks and active workspace](step_1/substep_1.md)
- [1.2 Project actions into menus and toolbars](step_1/substep_2.md)
- [1.3 Dispatch workspace shortcuts exactly once](step_1/substep_3.md)
- [1.4 Isolate toolbar customization by workspace](step_1/substep_4.md)

### 2. [Composer workspace integration](step_2/)

Move the current scene UI into a workspace, identify common actions and implement transitions on the existing document and viewport.

- [2.1 Extract the existing scene workspace](step_2/substep_1.md)
- [2.2 Classify common and scene-only actions](step_2/substep_2.md)
- [2.3 Coordinate transitions and clear history safely](step_2/substep_3.md)

### 3. [Grid increments for scene gizmos](step_3/)

Introduce the shared displacement rule and deliver it through the general scene translation gizmo before using it for volume editing.

- [3.1 Define shared snapping calculations and preferences](step_3/substep_1.md)
- [3.2 Integrate increments with the scene translation gizmo](step_3/substep_2.md)

### 4. [Volume definitions and CPU boolean evaluation](step_4/)

Represent ordered operands, construct closed triangulations, and evaluate them through a private, pinned Manifold integration.

- [4.1 Define the editable CPU recipe](step_4/substep_1.md)
- [4.2 Integrate a pinned private Manifold dependency](step_4/substep_2.md)
- [4.3 Construct closed primitives with source labels](step_4/substep_3.md)
- [4.4 Evaluate the ordered boolean sequence](step_4/substep_4.md)

### 5. [Logical faces, groups, UVs and Drawable generation](step_5/)

Extract selectable regions from boolean topology, assign independent regions to material groups, bake UVs and construct standard draw geometry.

- [5.1 Extract independently selectable logical faces](step_5/substep_1.md)
- [5.2 Define group assignment and revision persistence](step_5/substep_2.md)
- [5.3 Bake scene-aligned UVs into render vertices](step_5/substep_3.md)
- [5.4 Assemble the ordinary Drawable](step_5/substep_4.md)

### 6. [GeometryBuilderComponent and persistence](step_6/)

Keep a versioned C++ recipe alongside an independently usable Drawable owner. Cover replacement, scene persistence, standalone export and cross-SDK consumption.

- [6.1 Attach and register the C++ component](step_6/substep_1.md)
- [6.2 Serialize the versioned recipe and surface assignments](step_6/substep_2.md)
- [6.3 Publish the recipe and owner as one change](step_6/substep_3.md)
- [6.4 Verify scene save, standalone export and SDK interoperability](step_6/substep_4.md)

### 7. [Reusable geobuild editing queries and operations](step_7/)

Implement screen-to-result picking and dimension edits in the C++ SDK so another editor can use them without Composer.

- [7.1 Build viewport rays in geobuild](step_7/substep_1.md)
- [7.2 Pick all intersected original operands](step_7/substep_2.md)
- [7.3 Pick logical regions on the evaluated mesh](step_7/substep_3.md)
- [7.4 Resize primitives using opposite-face anchoring](step_7/substep_4.md)

### 8. [Volume workspace in Composer](step_8/)

Integrate transient editing, preview and undo, geometric and material tools, publication and saving on the current scene. Validate the industrial asset workflow.

- [8.1 Enter and leave a volume editing session](step_8/substep_1.md)
- [8.2 Edit and preview an ordered list of operands](step_8/substep_2.md)
- [8.3 Wire viewport selection and full-gesture ownership](step_8/substep_3.md)
- [8.4 Connect transform gizmos, face handles and grid](step_8/substep_4.md)
- [8.5 Edit surface groups with pastel previews](step_8/substep_5.md)
- [8.6 Apply, cancel and save the active draft](step_8/substep_6.md)
- [8.7 Validate the complete industrial asset workflow](step_8/substep_7.md)

## Dependency notes

Steps 1 and 2 establish the UI and document boundary. Step 3 reuses that UI and supplies a general manipulation feature. Step 4 is technically independent of the workspaces and remains fourth in the agreed execution order. Step 5 consumes the evaluated source topology. Step 6 consumes the drawable and recipe. Step 7 consumes step 3 for quantized dimensions and steps 4 and 5 for picking. Step 8 combines the validated layers. Each implementation pass follows the cursor even where technical dependencies permit parallel development.

## Reference scenario

A full-height cut creates two independently paintable fronts. A window leaves a single connected front around the opening. Several disconnected faces may belong to one submesh. A subtractive operand remains pickable. Adjacent pieces share texture phase in the scene and each retains its UVs on independent export. These cases recur at the earliest boundary where each property can be observed.
