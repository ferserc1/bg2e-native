# Step 7 — Composer authoring and integrated interaction

Dependency: accepted step 6.

Mandatory: [recovery-policy.md](recovery-policy.md). Recoverable authoring failures use defaults or rejected/no-op status and console warnings; never stop the application or lose editable text.

## Implementation

1. Inspect current Composer commands, reflection/property UI, document mutation boundaries and resource ownership. Add text creation and font declaration editing using those mechanisms rather than a parallel editor state system.
2. Expose the TextFontComponent collection: identifier, source-file picker and face index. Allow declarations on any node and recommend root placement. Show duplicate/missing/load errors with actionable diagnostics. A single root component can hold multiple fonts.
3. Add TextComponent creation with a sensible default paragraph, selection and material. Expose content, font identifier, size, paragraph dimensions, resolution, margins, wrapping, alignment, line spacing and character spacing according to step 4's units. List available registered fonts; do not require knowledge of node traversal order.
4. Do not create text appearance fields: color, unlit, alpha test, double sided and all other appearance are edited in the ordinary material editor. Provide access to the existing one-submesh material workflow.
5. Show the generated albedo preview but disable replace/clear only. Keep UV and sampling controls enabled, including scale. Preserve these controls after paragraph edits, source-font reload, material copy/paste, save/reopen and undo/redo. Apply the source-lock check at command execution, including mixed selections and deferred intents.
6. Route edits through existing history/transactions and coordinated GPU updates. Coalesce typing changes appropriately, mark the document dirty, support duplication/deletion/reparenting, and invalidate dependent text when font declarations change. Undo restores declarations and dependent resource state coherently; avoid shared mutable generators between duplicates.
7. Integrate normal rectangular selection and gizmos. Preserve the planned multiple-drawable limitation without introducing a new mitigation architecture.

## User handoff: Composer

Give exact menu/control locations and a concise manual checklist the user can perform:

- Declare the bundled font at the root; create a text node referring to its identifier and edit the string live, including an empty string and Spanish precomposed characters.
- Change paragraph geometry and bitmap resolution; explain the expected distinction between layout, pixel quality and scene size.
- Edit material color, unlit, alpha cutoff and double sided. Replace/clear albedo must be unavailable, while UV scale and sampling remain editable and survive regeneration.
- Select empty areas of the text rectangle and select from behind with double sided enabled; move/rotate it with ordinary gizmos.
- Use a floor and a directional light to observe glyph-shaped shadows rather than an opaque rectangle; note which existing renderer settings enable shadows.
- Duplicate text, edit only the duplicate, and use undo/redo for text/font/material changes.
- Save and Save As, reopen from the destination, and inspect packaged resources: text remains editable, fonts/other material images are present, and text produces neither a `.bg2` nor a generated albedo PNG. Existing model exports still work.
- Check recoverable faults through normal controls: invalid size/spacing/margins/resolution, missing/unreadable fonts, duplicate identifiers and overflowing content. Expect defaults/clipping or transparent/omitted text plus console warnings, continued interaction/rendering, no exception interruption or per-frame warning/retry flood, and recovery after correction/explicit reload.

These are user manual observations, not automated tests. Do not execute builds or launch Composer. Record user feedback, fix blockers within this milestone, and stop for acceptance.

## Next milestone

Step 8 is the final documentation milestone: update doc/api and add the minimal interactive Hello World tutorial with its full beginner walkthrough.
