# Mandatory review after every milestone

[Plan index](index.md) · [Architecture decisions](architecture-decisions.md)

This is an executable review procedure for the months-long implementation. Every milestone's final step adds specific comparison questions. Completing implementation without this review leaves the milestone incomplete.

## 1. Establish the actual baseline

Record the starting and finishing revisions, working-tree changes, relevant platform/backend/SDK facts and exact executable targets. Review all changes since the previous accepted milestone, including changes made outside this plan. Read the current summaries and steps for **all pending milestones**, not just the next one.

Do not replace source inspection with a prior agent's completion claim. Check declarations, implementation, shader libraries and actual application callers. Treat the older assessment as a source snapshot, not an immutable specification.

## 2. Compare planned and actual contracts

For each implemented public object, record actual namespace/header, lifecycle, ownership, accessors, reflected settings, initialization overloads and downstream callers. Compare particularly:

- Runtime renderer/delegate ownership and catalog rebinding.
- Dual legacy/draw scene/DB/manipulation/cache paths.
- Drawable deep GPU clone and safe rare-edit behavior.
- Public component injection, shader selection and format/input compatibility.
- GPU capabilities/primitives and real Vulkan/Metal differences.
- Shader source/output paths, bindings, format packing and native interop location.
- Mesh/texture lifetime versus per-frame shader/history lifetime.
- Settings serialization IDs, versioning, export/import and headless precedence.
- Product requirements and explicit exclusions.

Classify deviations as **equivalent refinement**, **intentional scope/contract change**, **missing requirement**, or **verification gap**. Record why and which pending steps they affect. An implementation convenience is not enough to contradict a user decision; required changes should be presented concretely for user review.

## 3. Verify the working product honestly

Record static checks, explicitly authorized builds, runtime/backend/device scenarios and result artifacts. Include expected and observed behavior, failure logs and reference image/statistics where useful. A missing backend machine remains `not verified`. When builds are not authorized, record planned runtime checks as pending and leave product acceptance awaiting that evidence.

Separate regression checks for production through M08 from new-feature checks. From M09 onward, verify absence of the deleted API and continued functioning of the new canonical products. Do not preserve obsolete application compatibility after the user-requested deletion.

## 4. Update the pending plan, not only a report

Produce an impact table mapping every changed contract to pending milestone/step IDs. Inspect the actual affected files and update their prerequisites, affected paths, actions, examples and exit checks. Remove requirements already satisfied; split tasks that grew; add missing tasks rather than leaving vague notes. Keep completed milestone records as history; do not rewrite them to pretend the original plan matched implementation.

Update the root index/architecture decisions if names or boundaries change. Distinguish user-approved architecture changes from implementation details. A new unresolved decision becomes a clearly labeled checkpoint in the relevant pending step, with options and a concrete decision owner; it must not disappear in prose.

## 5. Persist evidence

At milestone completion create `review.md` in that milestone directory with:

1. Revisions/date and product status.
2. Planned-versus-actual contract table.
3. Completed, missing and deliberately excluded requirements.
4. Verification table by backend/scenario, including unavailable tests.
5. Architecture/API decisions and rationale.
6. Pending-plan impact table and list of edited step files.
7. Open issues and conditions to enter the next milestone.

Do not pre-populate a successful review during planning. Milestone summaries include proposed gate checks, not executed results. If necessary, attach a small settings export, screenshot, trace or output-file metadata as review evidence after implementation.

## 6. Transition gate

Enter the next milestone only after the review accurately reflects code and unresolved gates are visible. User acceptance/verification policy controls acceptance; elapsed time does not. A harmless filename/API refinement may be incorporated directly. A change to the user's stated scope, clone semantics, shader philosophy or retirement policy requires an explicit decision rather than silently adapting the plan away from it.

M10 still performs this review: there are no pending implementation milestones, so compare documentation with final source/products, mark the plan's implemented end state and list any follow-up work outside scope.
