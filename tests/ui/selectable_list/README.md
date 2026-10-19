# SelectableList table-scope regression

Manual lead checks; not compiled or run by the agent.

The draw Composer selection crash reached ImGui::TableNextRow with no current
table. Its submesh inspector called SelectableList::item without beginList/endList.
Separately, the shared wrapper discarded BeginTable's false result for skipped
content. The corrected API returns bool; every repository caller now conditionally
submits rows and closes only a successfully opened table. ImGui is unchanged.

After rebuilding the engine and affected callers:

1. In draw_bg2e_composer select a mesh in the tree and viewport, select each
   submesh, switch to a group/camera/light, deselect and repeat. Check multi-selection
   and material editing, not just absence of the crash.
2. Resize/scroll the Scene Properties panel until the list is clipped, then restore
   it. Collapse/reopen the panel and switch selections. No row/end-table call may
   execute after beginList returns false; use a debugger breakpoint if necessary.
3. Repeat list selection and clipping in draw_model_edit, production model_edit,
   shared SubmeshSelector and the GPU selection dialog. Check one/multiple columns
   and itemButton with the wrapper recipe where applicable.
4. On available Vulkan/Metal hosts, enable ImGui assertions and verify no table
   scope/stack errors during selection, resize, panel hiding or shutdown. This is
   a CPU UI contract correction, not a backend-specific graphics workaround.

External callers must also adopt `if (SelectableList::beginList(...))` around all
items and endList; rebuild callers against the changed return-type declaration.
