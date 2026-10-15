# Mandatory review after every milestone

[Plan index](index.md) · [Architecture decisions](architecture-decisions.md) · [Cross-chat execution](execution-protocol.md) · [Progress](progress.md)

This is an executable review procedure for the months-long implementation. Every milestone's technical review adds specific comparison questions; M01–M09 then require a separate final cumulative documentation step. Completing implementation without this review leaves the milestone incomplete.

## 1. Establish the actual baseline

Record the starting and finishing revisions, working-tree changes, relevant platform/backend/SDK facts and exact executable targets. Review all changes since the previous accepted milestone, including changes made outside this plan. Read the current summaries and steps for **all pending milestones**, not just the next one.

Do not replace source inspection with a prior agent's completion claim. Check declarations, implementation, shader libraries and actual application callers. Treat the older assessment as a source snapshot, not an immutable specification.

For every touched source/header/shader and every milestone's final static review, check all repository style requirements: full GPL header on repository-owned files (preserving third-party licenses); braces for all control flow; C/C++ code-block braces on new lines except class/struct/namespace; unindented namespace contents; angled includes except necessary private includes; blank separation between consecutive `.cpp` function definitions; and purpose-based grouping of class/struct methods with blank lines between groups. Record violations and exact paths; do not treat a whitespace mention elsewhere in a step as a substitute for these explicit checks.

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

Compilation, runtime/backend checks and build-producing scripts are delegated to the project lead after each step. Record the agent's static inspection separately from lead-reported results/artifacts. Run builds/tests only if explicitly instructed. Include expected and reported behavior, failure logs and reference images/statistics when supplied; do not require those artifacts before accepting a continuation instruction.

Under the [execution protocol](execution-protocol.md), the lead's instruction to continue means they compiled and checked the preceding delivered step without finding failures. Record `accepted by project lead continuation instruction`, with backend/scenario details `not specified` unless provided. Do not ask for reconfirmation or independently repeat the checks. Silence alone does not accept a step. Known failures are corrected first and handed back for validation.

Separate regression checks for production through M08 from new-feature checks. From M09 onward, verify absence of the deleted API and continued functioning of the new canonical products. Do not preserve obsolete application compatibility after the user-requested deletion.

## 4. Update the pending plan, not only a report

Produce an impact table mapping every changed contract to pending milestone/step IDs. Inspect the actual affected files and update their prerequisites, affected paths, actions, examples and exit checks. Remove requirements already satisfied; split tasks that grew; add missing tasks rather than leaving vague notes. Keep completed milestone records as history; do not rewrite them to pretend the original plan matched implementation.

Update the root index/architecture decisions if names or boundaries change. Distinguish user-approved architecture changes from implementation details. A new unresolved decision becomes a clearly labeled checkpoint in the relevant pending step, with options and a concrete decision owner; it must not disappear in prose.

Recompute and write progress.md's **Next step to implement**, **Next action** and **Condition before starting** after editing pending order/steps. On delivery of an M01–M09 technical review, link that milestone's final documentation step while leaving the review awaiting lead acceptance. After documentation delivery, link the following milestone's first required step with the normal acceptance condition. The project lead should never need to choose that file manually.

## 5. Persist evidence

At milestone completion create `review.md` in that milestone directory with:

1. Revisions/date and product status.
2. Planned-versus-actual contract table.
3. Completed, missing and deliberately excluded requirements.
4. Verification table distinguishing agent static checks, project lead reports/continuation acceptance and unspecified/unavailable backend details.
5. Architecture/API decisions and rationale.
6. Pending-plan impact table and list of edited step files.
7. Open issues and conditions to enter the next milestone.

Do not pre-populate a successful review during planning. Milestone summaries include proposed gate checks, not executed results. If necessary, attach a small settings export, screenshot, trace or output-file metadata as review evidence after implementation.

## 6. Transition gate

Enter the next milestone only after the technical review accurately reflects code, the final cumulative documentation step is accepted, and unresolved gates are visible. User acceptance/verification policy controls acceptance; elapsed time does not. A harmless filename/API refinement may be incorporated directly. A change to the user's stated scope, clone semantics, shader philosophy or retirement policy requires an explicit decision rather than silently adapting the plan away from it.

The review itself is a tracked step: deliver it, update progress and leave it awaiting project lead validation. For M01–M09, continuation after review delivery accepts the technical review and selects its same-milestone documentation step. Continuation after documentation delivery accepts milestone closure and selects the following milestone without a second approval request. Apply the lead's acceptance convention to unspecified test evidence; do not erase explicitly known incomplete requirements or failures.

M10 still performs this review: there are no pending implementation milestones, so compare documentation with final source/products, mark the plan's implemented end state and list any follow-up work outside scope.

## 7. Documentation closure

Follow [documentation-policy.md](documentation-policy.md). Documentation.md coverage evidence and maintained doc/ updates close M01–M09 after the technical review. Append that handoff to review.md and preserve the earlier review history. M10 audits all accumulated pages and closes the plan; no future implementation may be documented as available.
