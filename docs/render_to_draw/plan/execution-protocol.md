# Execution across chats and project lead validation

[Plan index](index.md) · [Progress record](progress.md) · [Architecture decisions](architecture-decisions.md) · [Milestone review](review-protocol.md)

These general execution instructions preserve the project lead's workflow across independent chats. They apply to every implementation, correction and review step. They override earlier plan wording that could imply the agent must run builds or obtain test artifacts before continuing.

## 1. Session context and responsibility

The human user is the **project lead** and owns compilation, runtime testing and acceptance. Different steps will be implemented in different chats. Do not depend on another chat's hidden history or a previous agent's memory; persist relevant state, decisions and handoff details in this plan directory.

An instruction to implement the next step authorizes one step, including the integration and corrections needed to make it reviewable. It does not authorize executing the rest of the milestone automatically. Finish that step, update the progress record, provide the project lead's checks and stop for their validation. Do not start another step merely because its prerequisites are now available.

The project lead has established this acceptance convention:

> When the project lead says to continue with the next step, assume they have compiled and performed their checks on the preceding delivered step and found no failures.

Treat that instruction as acceptance, including when it arrives in a new chat. Do not ask them to confirm again, provide logs, enumerate tested devices or repeat tests before advancing. Record the acceptance basis as `project lead continuation instruction`; do not claim the agent executed those checks or invent backend/test details. A failure report instead starts correction work on the reported step.

A request explicitly naming the immediate next step after a delivered step also expresses continuation. Do not require the exact words “continue with the next step.” This does not turn an unrelated later-step request into acceptance of every missing intermediate step.

The plan is a months-long reimplementation, not a request to finish it in one chat. Earlier architectural recommendations remain historical where the project lead's conclusions supersede them. Routine names/implementation choices may be refined and recorded; preserve the fixed user decisions in [architecture-decisions.md](architecture-decisions.md). Missing architecture decisions explicitly deferred to M03 are not resolved by generic permission to continue.

## 2. Start every implementation chat

1. Read applicable `AGENTS.md` instructions and the current user request.
2. Read [index.md](index.md), this protocol, [progress.md](progress.md), [architecture-decisions.md](architecture-decisions.md) and [review-protocol.md](review-protocol.md). Consult [requirements-map.md](requirements-map.md) and [source-anchors.md](source-anchors.md) for the selected work.
3. Read the current milestone summary, the selected step and the most recent actual milestone `review.md`, if one exists. Read handoff notes and any open failure report in the progress record.
4. Inspect the current working tree, relevant declarations/implementations/shaders and previous-step integration. The documentation's original commit is a reference snapshot, not a checkout to reset to. Preserve unrelated and uncommitted user work.
5. Apply any project lead acceptance or failure report to the record **before** choosing work. A new chat is not a reason to restart M01 or redo an accepted step.
6. Open the step file linked in progress.md's **Next step to implement** field and its milestone summary. The project lead need not name it. If the pointer conflicts with a failure, explicit direction, prerequisite or revised order, reconcile the record using the rules below and write the corrected link yourself.

No implementation step has been recorded when this protocol is first added. Existing GPU/draw examples and a committed implementation plan do not establish completion of M01-01 or later steps. Start from the initialized record unless actual user instructions or recorded implementation evidence establish a different position.

## 3. Determine the next step

Start from the explicit **Next step to implement** link in progress.md. Use the ordered step links in milestone summaries, in M01–M10 order, to validate and maintain it. Do not infer order only from source filenames, a commit title or a previous final response. Step IDs identify work; summaries may be revised after milestone reviews. Never ask the project lead to locate/name the next step when these files already determine it.

Apply these rules in order:

1. **Current explicit user direction:** a named step, correction, pause or changed requirement takes precedence over the recorded next-step field. Record the direction and its impact, including the updated step link/action/condition. Explicitly requested later work still needs its real prerequisites; explain a missing prerequisite rather than silently claiming it exists.
2. **Failure report:** set the affected/current delivered step to `needs_fix`, record the issue and fix it. Do not advance to a new implementation step while the reported failure is unresolved. Fixes may touch earlier files; record that dependency impact without inventing a new plan step or automatically undoing previous acceptances.
3. **Continuation instruction with a pending delivered step:** accept that step under the project lead's convention, resolve its corrected failure entry when applicable, update the last-accepted field and clear its validation condition. Implement the step already linked in **Next step to implement** after reconciling prerequisites. Explicit acceptance such as “this step works” also accepts it; it does not itself authorize implementing another step unless the user asks to proceed.
4. **Partial/in-progress step:** resume it, inspect partial changes and finish its remaining work. Do not jump to the following step or overwrite another chat's changes. An active record from another chat calls for reconciling actual current work, not starting a competing implementation.
5. **Awaiting validation without acceptance:** retain `awaiting_project_lead`. Do not infer success from silence, elapsed time, a commit or the absence of a failure report. Only a direct acceptance/continuation instruction changes that state.
6. **No active/pending correction:** select the first step not accepted in the current ordered summaries, after all preceding steps/prerequisites are accepted or an explicit plan revision accounts for them. If no implementation has begun, this is [M01-01](milestone_01_deferred_renderer/step_01_public-contracts.md).

Recompute and write the next-step link/action/condition from the summaries and ledger whenever the plan changes. The pointer is a required handoff field and must be current; summaries/prerequisites and lead acceptance still control whether it can execute. If code and record disagree, inspect and reconcile the relevant change; do not reset code or discard the record. If a required human fact cannot be recovered, ask only for that fact, not for a step filename already discoverable in the plan.

The technical review and final cumulative documentation step of M01–M09 are separate ordinary required steps. Do not skip either because the example/application runs. The review updates pending milestones and hands off to documentation in the same milestone. Continuation after the review accepts that review and selects documentation, not the next milestone. Continuation after the documentation delivery accepts milestone closure and selects the following milestone. M10 audits the accumulated documentation and retains its final plan review. See [documentation policy](documentation-policy.md).

## 4. Maintain progress.md

[progress.md](progress.md) is the shared execution record. Update it in the repository at the beginning and end of each step and after each failure/acceptance instruction. Do not create separate competing per-chat trackers. No commit is required for the record to be valid; do not commit, reset or stage unrelated work unless instructed.

Keep the current-position fields, touched-step ledger, open issues, decisions/handoff notes and chronological history consistent. An absent ledger row means `not_started`; step documents describe planned work and are not the status database.

The **Next step to implement** field must contain the actual step ID/title and a relative Markdown link. Maintain **Next action** and **Condition before starting** alongside it:

| Handoff event | Next-step link | Next action / condition |
|---|---|---|
| No step started | First required step. | `implement` when requested. |
| Work interrupted/partial | The active incomplete step. | `resume`; inspect partial changes. |
| Step delivered | Its successor in the current summary order, including reviews. | `wait_for_project_lead`; identify the delivered step awaiting acceptance. |
| Lead asks to continue | The recorded successor, after accepting the previous delivery. | `implement`; no repeated confirmation. |
| Lead accepts without requesting further work | The recorded successor. | `implement` when requested; clear the acceptance condition but remain idle. |
| Lead reports failure | The affected step. | `fix`; do not advance while unresolved. |
| Correction delivered | The affected step's successor after ledger reconciliation. | `wait_for_project_lead`; correction awaiting acceptance. |
| Plan reordered/revised | Reconciled next required step. | Record the real prerequisite/validation condition. |
| Final step delivered/accepted | No successor. | Wait for final acceptance, then `complete`. |

At the end of every implementation/correction/review step, updating this pointer is part of delivery. Do it before the final response. Do not leave the just-delivered step as the next implementation when the next chat should advance after acceptance. Do not mark its successor active before the project lead asks to continue.

| State | Meaning | Next action |
|---|---|---|
| `not_started` | No implementation recorded. | Select when preceding steps/prerequisites permit it. |
| `in_progress` | Implementation is partial/active. | Finish this step and preserve partial work. |
| `awaiting_project_lead` | Implementation or correction delivered; agent static checks complete; lead validation pending. | Stop until acceptance, continuation or a failure report. |
| `needs_fix` | Lead reported a failure or the delivered step has a known incomplete requirement. | Correct this step, then return to `awaiting_project_lead`. |
| `accepted` | Lead explicitly accepted it or requested continuation after delivery. | Eligible to select the following step when authorized. |

For every touched step, record its ID/path, state, implemented scope, affected files/actual API decisions, agent static checks, lead validation/acceptance basis and relevant issues. Record the starting revision plus uncommitted changes or the actual implementation revision if available. Never invent a “completed commit” for work not committed.

`Last implemented step` and `Last accepted step` are different fields. A delivered step updates the first and waits for the lead; it does not update the second. Corrections stay associated with the same step. Keep earlier acceptance history if a later regression reopens work. Record enough context to explain an out-of-order explicit request rather than making the headline fields appear to be a contiguous completion range.

Keep handoff notes concrete: actual public names/ownership, unresolved compile-risk assumptions, modified pending steps, required test fixtures and reported errors. If a decision changes future work, update the actual pending step files and architectural decisions as required by the milestone review protocol. A note alone is not a plan update.

## 5. Checks delegated to the project lead

By default the implementing agent **does not compile, configure a build, compile shaders, run executables/tests, launch the example, or run packaging/update scripts that build code**. Those checks belong to the project lead after every step. A request to continue, implement a step or fix a compiler error is not a request for the agent to compile. If the lead explicitly instructs the agent to run a particular check, follow that instruction and record who ran it and its actual result.

The agent still performs relevant **nonexecuting static checks**: review the diff, verify includes/signatures/call sites/ownership/dependencies, inspect shader bindings/packing, check documentation links and use checks such as `git diff --check`. Do not label those as successful compilation or runtime verification. No need to build an additional automated validation system just to hand work back to the lead.

Deliver each step in an intended compilable/integrated state, including dependent call-site changes within its scope. Avoid deliberately breaking current targets until a later step. Early steps may only introduce contracts/primitives; tell the lead which existing targets or header/primitive checks apply now, rather than requiring the final milestone example before it exists. A missing future feature remains scheduled; a compiler/linker error in current targets is a failure to repair.

At step completion, give the lead a concise checklist drawn from that step:

- Step ID and implemented scope/changed behavior.
- Relevant target(s), or an explicit statement that this is a contract/documentation step without a new executable.
- Static checks the agent actually performed.
- Compilation and runtime checks delegated to the lead, with concrete expected behavior and relevant backend/fallback cases.
- Known limitations or deferred checks that genuinely apply to this intermediate step.
- The next step ID/file already recorded in progress.md, still conditional on the lead's acceptance/continuation.

Do not ask “may I compile?” or require a separate confirmation that the lead tested. End with the work ready for their validation. For a documentation/review step, the lead reviews the report/updated plan and may compile as desired; continuation uses the same acceptance convention.

## 6. Failure and correction loop

When the lead reports a compilation/runtime/behavior failure, save the essential report and affected step in `progress.md`, reproduce the reasoning through source inspection and fix the cause. Do not merely change the tracker or mark the report resolved before implementing the correction. Do not ask for a build command/approval unless actual missing information prevents useful work.

Complete all affected source/shader/call-site changes needed by that correction, retain the rest of the step, update downstream plan assumptions if the API changed, run applicable static checks and return the step to `awaiting_project_lead`. Explain what was corrected and what the lead should recompile/retest. The lead will report remaining failures; their instruction to continue accepts the corrected step.

If a later step exposes a regression in earlier code, repair it as part of the reported step and preserve the history of prior acceptance. Explicit user instructions can reopen or change acceptance; do not silently erase history, skip required behavior, or invent a permanent compatibility workaround that contradicts the architecture.

## 7. Validation evidence and milestone reports

Lead validation is sufficient to advance under this workflow. Record `accepted by project lead continuation; compilation/checks assumed successful under the agreed convention` when no detailed results are supplied. Platform/backend/test coverage then remains `not specified`; do not fabricate traces, image comparisons or an all-platform matrix.

Milestone `review.md` still compares actual code with every pending milestone. It distinguishes agent static inspection, lead-reported results, acceptance under the continuation convention and any explicit unresolved limitations. Do not hold up the next step solely because the lead provided no logs or device list. A known missing requirement or explicitly unresolved failure must be recorded and resolved or explicitly addressed by the lead; acceptance does not mean features absent from code were implemented.

This protocol complements, rather than replaces, the milestone-specific downstream comparisons. The review step must update pending instructions for the actual APIs that future chats will see.

## 8. Starting prompt for another chat

The project lead only needs to reference index.md and progress.md and ask to implement the next step. The agent reads the explicit next-step link in progress.md and follows the linked protocol, summary and step; the lead need not attach the step file manually. A self-contained prompt is:

> Implement the next step indicated in docs/render_to_draw/plan/progress.md, following docs/render_to_draw/plan/index.md. Treat my continuation as acceptance of the previously delivered step. Update the next-step link when you finish, and leave compilation and runtime checks to me as project lead.

This is a suggested prompt, not a message sent to another chat and not authorization to create or contact chats automatically.
