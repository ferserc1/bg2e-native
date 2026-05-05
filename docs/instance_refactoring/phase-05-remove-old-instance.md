# Phase 5: Remove Old `render::vulkan::Instance`

## Objective

Delete the now-unused `bg2e::render::vulkan::Instance` class files and remove it from the
aggregate header. After this phase, the Instance refactoring is complete.

**After this phase:** Only `bg2e::gpu::Instance` (abstract) and `bg2e::gpu::vk::Instance`
(concrete) exist. The old `render::vulkan::Instance` is gone.

---

## Files to Modify/Delete

| File | Action |
|------|--------|
| `lib/include/bg2e/render/vulkan/Instance.hpp` | **Delete** |
| `lib/src/bg2e/render/vulkan/Instance.cpp` | **Delete** |
| `lib/include/bg2e/render/vulkan/all.hpp` | Modify — remove `Instance.hpp` include |

---

## 5.1 — Delete Old Files

Remove the following files:
```
lib/include/bg2e/render/vulkan/Instance.hpp
lib/src/bg2e/render/vulkan/Instance.cpp
```

---

## 5.2 — Update Aggregate Header

**File:** `lib/include/bg2e/render/vulkan/all.hpp`

Remove the line that includes `Instance.hpp`:

```cpp
// REMOVE this line:
// #include <bg2e/render/vulkan/Instance.hpp>
```

The remaining includes in `all.hpp` should be verified — none of them should depend on
`Instance.hpp` since Phase 2 removed all such dependencies.

---

## 5.3 — Final Verification

1. **Compile check:** Build the entire project. No file should reference
   `render::vulkan::Instance` anymore.
2. **grep verification:**
   ```bash
   grep -r "render::vulkan::Instance" lib/
   ```
   This should return zero results.

3. **Runtime check:** Run the engine with:
   - A windowed application (model editor or any example)
   - An offscreen application (e.g., example 16)
   - Verify validation layers still work in debug builds

4. **Full regression:** Run through the examples to verify nothing is broken:
   - `01_setup` — basic window
   - Any example with scene rendering
   - `16_offscreen_scene_render_cli` — offscreen mode

---

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| Delete rather than deprecate | Since this is an internal refactoring (not a public API change), there's no need for a deprecation period. All callers have been migrated in Phases 2-4. |
| Keep `render/vulkan/all.hpp` | Other `render::vulkan` classes still exist and are still used. Only the Instance include is removed. |
