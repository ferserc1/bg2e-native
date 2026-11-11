# Component issue regression probes

`probe.cpp` is a standalone C++20 source using public SDK APIs. It requires the
bg2e library, but no application loop or graphics device initialization. Like
other repository probes, it is intentionally not wired into CMake. It has not
been compiled or executed as part of this change.

The probe covers independent dependency severities, directional Text/Drawable
conflicts (excluding self and non-scene editor overlays), immediate-parent Chain requirements, distinct missing/empty font
errors, initialized suggested camera projections, and stale fix rejection/no-op.
Sun repair checks both preexisting and newly added transformations, exact
membership/order restoration, retained identities on redo, and restoration of
an existing Spot light without deleting or replacing it.

Manual Composer checks after an explicitly requested build:

1. Select each affected component and check its issue messages, wrapped help and
   action descriptions. Fold it and verify the issue counts remain visible.
2. Repair an Environment with/without a Transform; undo and redo. Existing
   transformations and light identities must survive unchanged.
3. Convert a Spot to Sun using the Environment fix. Undo must restore Spot and
   remove only a transformation created by that fix.
4. Add an Orbit controller without Camera/Transform. Verify two independent
   issues, one undo entry per clicked solution, and no implicit controller when
   adding a Camera.
5. Add Text to a node with Drawable. Only Text owns the warning. Existing
   Composer restrictions on manually adding a second Drawable remain intact.
6. Remove document font declarations; repair from Text. Verify the fix modifies
   the document root or existing font node, never Composer's runtime root.
   Undo/redo must restore declarations and component membership. Save/reopen
   should preserve the font and notices.
7. Configure a missing font file or duplicate identifier and verify an error
   without silent replacement. Configure an HDR image on an Environment without
   Sun and verify its warning disappears.
8. Use ComponentInspector without an issue-fix handler in an application with a
   MainLoop: repairs should execute at the safe boundary without requiring a
   CommandHistory. Failures should appear as inspector feedback.
