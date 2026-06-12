# Step 05 — Extend `PipelineLayout` backends with resource bindings

## Title
Build descriptor set layouts from `resourceBindings` (Vulkan) and store a binding
table (Metal).

## Objective
Make `PipelineLayout` aware of `PipelineLayoutDescription::resourceBindings` so
that pipelines created with it expose the descriptor set layouts (Vulkan) and so
that resource sets can resolve `(set,binding)` to Metal encoder indices.

## Context
`vk::PipelineLayout` currently passes only push-constant ranges to
`vkCreatePipelineLayout` (no `pSetLayouts`). `metal::PipelineLayout` already
stores the whole `PipelineLayoutDescription` and exposes
`pushConstantBufferIndex`. Existing examples pass empty `resourceBindings`, so the
behavior for them is unchanged.

## Expected previous state
- Steps 01–04 complete. `resourceBindings` exists on
  `PipelineLayoutDescription`; `ResourceSet` interface declared.

## Files to review / modify
- Review: `lib/src/bg2e/gpu/vk/PipelineLayout.cpp`,
  `lib/include/bg2e/gpu/vk/PipelineLayout.hpp`,
  `lib/src/bg2e/gpu/metal/PipelineLayout.cpp`,
  `lib/include/bg2e/gpu/metal/PipelineLayout.hpp`,
  `lib/src/bg2e/gpu/vk/GraphicsPipeline.cpp` and
  `vk/ComputePipeline.cpp` (they consume `layout->handle()`, unchanged).
- Modify: the four `PipelineLayout` files above.

## Proposed design
### Vulkan
Group `description.resourceBindings` by `set`. For each set, build a
`VkDescriptorSetLayout` with one `VkDescriptorSetLayoutBinding` per
`ResourceBinding`:

- `descriptorType`: `ResourceType -> VkDescriptorType`
  (`SampledImage -> VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE`,
   `StorageImage -> VK_DESCRIPTOR_TYPE_STORAGE_IMAGE`,
   `Sampler -> VK_DESCRIPTOR_TYPE_SAMPLER`,
   `UniformBuffer -> VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`,
   `StorageBuffer -> VK_DESCRIPTOR_TYPE_STORAGE_BUFFER`).
- `stageFlags`: reuse the existing `shaderStageToVkFlags`.
- `descriptorCount`: `binding.count`.

Store the created `std::vector<VkDescriptorSetLayout>` (ordered by set index, with
empty layouts for any gap so set indices stay contiguous — for this iteration
sets are `{0}` only). Pass them to `VkPipelineLayoutCreateInfo::pSetLayouts` /
`setLayoutCount` alongside the existing push-constant ranges. Destroy all set
layouts in `cleanup()`. Expose a getter so `ResourceSet` can allocate matching
descriptor sets:

```cpp
VkDescriptorSetLayout descriptorSetLayout(uint32_t set) const;
const gpu::PipelineLayoutDescription& description() const;
```

### Metal
`metal::PipelineLayout` already keeps `_description`. Add a helper that resolves a
`(set, binding, ResourceType)` to a Metal argument index and reports the stage,
e.g.:

```cpp
struct MetalBinding {
    uint32_t    index;   // Metal texture/sampler/buffer index
    ResourceType type;
    ShaderStage stage;
};
const std::vector<ResourceBinding>& resourceBindings() const;
uint32_t metalIndex(const ResourceBinding& b) const; // see note below
```

For this iteration the simplest stable mapping is **`index = binding`** within
each Metal resource namespace (textures, samplers, buffers are independent index
spaces in Metal), because the example uses small, distinct binding numbers. Keep
push-constant buffer index reservation (`PushConstantBufferIndex = 0`) in mind:
the gradient compute and textured-triangle bindings in the example are textures
and samplers, which do **not** collide with the buffer index space, so no
conflict arises. Document this assumption.

## Required changes
1. `vk::PipelineLayout`: build, store, pass, and destroy descriptor set layouts;
   add the type-mapping helper and the `descriptorSetLayout(set)` /
   `description()` getters.
2. `metal::PipelineLayout`: add binding-table accessors / index resolution
   helpers (description already stored).

## Compilation criteria
- Project compiles. Existing examples create layouts with empty
  `resourceBindings`, so `setLayoutCount = 0` and behavior is byte-for-byte
  equivalent to today.

## Validation criteria
- `examples/gpu/05_simple_triangle` still renders the triangle exactly as before.
- A layout created with one `StorageImage`/Compute binding produces a single
  non-null `VkDescriptorSetLayout` (verified indirectly in Step 09).

## Risks / things to check
- Set-index contiguity: Vulkan requires `pSetLayouts[i]` to describe set `i`. If
  the example only uses set 0, this is trivial; keep the grouping logic robust to
  a single set.
- Lifetime: descriptor set layouts must outlive any pipeline and any descriptor
  set created from them; destroy them only in `PipelineLayout::cleanup()`.
- Metal index-namespace assumption (`index = binding`) must be documented so a
  future `UniformBuffer` binding does not silently collide with the
  push-constant buffer index.

## What NOT to do in this step
- Do not allocate `VkDescriptorPool` / `VkDescriptorSet` here (Step 06).
- Do not change `GraphicsPipeline` / `ComputePipeline` creation — they already
  consume `layout->handle()` unchanged.
