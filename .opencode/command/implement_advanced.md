---
description: Implements the next step of an advanced implementation plan.
agent: build
---

Implement exactly the next pending step of the advanced plan located at `$ARGUMENTS`. The argument must be the path to the plan's root directory.

## Plan orientation and step selection

1. Confirm that `$ARGUMENTS` identifies an existing directory. Read its `README.md` and follow all references to other relevant plan documents; do not rely on the README alone.
2. Enumerate phase directories directly under the plan root. Identify phases by names beginning with a step number, such as `01_refactoring`; sort them numerically, not alphabetically. Read each phase's summary document (`overview.md`, `summary.md`, `readme.md`, or equivalent) as needed to understand the plan and its dependencies.
3. Read `last_step.md` at the plan root, if it exists. If it does not exist, consider the plan not yet started and begin with the first phase and its first step. The file must clearly identify the last completed phase and step (for example, `01_refactoring / step_02_name.md`) so work can resume unambiguously. If the existing format does not make the next step certain, inspect the plan files and ask the user before implementing.
4. In the active phase, identify `step_XX_[description].md` and `prestep_XX_[description].md` files. Sort them by XX. Implement exactly one `step_XX` per invocation; `prestep_XX` files provide context and requirements for that step and are not independent implementation steps. Do not advance to another phase until all its steps are complete. Once a phase is complete, the next invocation starts with the first step of the next phase.
5. Read the selected `step_XX` file in full and its `prestep_XX`, if present. Also read the next step's `prestep` if one exists, because the current step may need to update it. Consider the general instructions, references, phase summaries, and related documentation before determining scope. If a `prestep` conflicts with its corresponding `step`, the `prestep` takes precedence.

## Implementation

- If there are ambiguous decisions, unresolved dependencies, or project information that could materially change the implementation, ask the user before deciding. Use and incorporate their judgment as the project lead. Do not ask about details made clear by the plan and code.
- Inspect the code and repository instructions relevant to the step. Follow the project's architecture, conventions, and constraints. Implement only the selected step and changes necessary to satisfy it; do not start later steps.
- If you discover that a plan instruction is outdated, incomplete, or conflicts with the actual project state, explain the finding and consult the user when the decision is not obvious. Do not silently change scope.
- `prestep` files are optional to modify. Update the next step's `prestep` only when the current step has produced information necessary to implement that next step correctly (for example, an agreed decision, a real dependency, or a scope change). Preserve useful existing content, add a specific note, and explain what changed and why in the response. Do not edit a `prestep` just to record progress; progress belongs in `last_step.md`.
- Run appropriate checks required by the plan and repository instructions. Do not run builds when repository instructions prohibit them or unless the user expressly requests them.

## Completion and progress tracking

- Mark the step complete only after implementing its requirements and running applicable checks. If it is blocked or incomplete, do not advance the tracking file; report the blocker and the missing decision or information.
- After completing the step, create or update `last_step.md` at the plan root. Record the just-completed phase and step filename, along with enough information to identify the next step. If the whole phase is complete, state that and name the next phase; otherwise, name the next step in the current phase. Keep the file brief and readable.
- In the final response, briefly summarize the implementation, checks performed, and progress (completed phase/step and next step). Explicitly state whether you modified the next `prestep`; if so, summarize what information you added and why. State that no change was needed if you left it untouched. Highlight if the phase is complete.
