# TextComponent

**Header:** `<bg2e/scene/TextComponent.hpp>` · **Namespace:** `bg2e::scene`

Final `DrawableComponent`, factory/JSON type **`Text`**. Owns exactly one procedural
quad/submesh, one conventional material and an independent cached text generator.
It uses the current draw renderer, not a separate font-rendering backend.

## Authored data and geometry

```cpp
TextComponent();
bool requiresBg2Export() const override; // false
base::Text& text();
const base::Text& text() const;
void setText(const base::Text& value) noexcept;
glm::vec2 size() const noexcept;
glm::vec2 authoredSize() const noexcept;
void setSize(const glm::vec2& value) noexcept;
void setHeight(float value) noexcept;
bool preserveAspect() const noexcept;
void setPreserveAspect(bool value) noexcept;
glm::vec2 pivot() const noexcept;
void setPivot(const glm::vec2& value) noexcept;
base::MaterialAttributes& material();
const base::MaterialAttributes& material() const;
void setMaterial(const base::MaterialAttributes& value) noexcept;
```

Default Text parameters are documented [here](../base/Text.md). World height is 1,
manual pair `(2,1)`, preserveAspect=true and pivot `(0.5,0.5)` measured from the
bottom-left. `size()` computes width from **logical paragraph aspect**, never
bitmap resolution. `authoredSize()` preserves the dormant manual width, useful
for exact editor snapshots. `setSize()` selects manual sizing; setPreserveAspect
can restore automatic width. Height changes do not implicitly change that mode.

World dimensions recover to defaults/safe `[0.001,1000000]` limits with warnings;
oversized derived aspect scales both dimensions down together. Nonfinite pivots
recover to center, finite pivots clamp to `[0,1]`.

The quad is local XY with readable front **+Z**, +X tangent and +Y up. Both UV
sets map top-left `(0,0)` to bottom-right `(1,1)`. Vertices are top-left,
top-right, bottom-right, bottom-left; indices `{0,2,1,0,3,2}` agree with the +Z
normal. The back is naturally mirrored when double sided, like a thin printed
sheet. Picking uses the **whole rectangle**, including transparent/empty regions,
and honors sidedness, not glyph alpha.

## Conventional material ownership

Initial appearance is white, doubleSided=true, alphaCutoff=0.1, lit, opaque/solid,
with the ordinary material roughness/metalness defaults. Color, unlit, alpha
test, other textures and all appearance belong exclusively to the material.
Albedo pixels are white RGB with glyph coverage alpha. The component establishes
sourceEditable=false and ignoreSerialization=true; these are ownership logic,
not serialized permissions. Initial sampling is linear/clamp/no mipmaps.

`setMaterial()` stages a CPU replacement. Incoming albedo is **sampling metadata
only**, never a replacement file/generator to evaluate. It preserves the owned
mask while applying conventional values, UV/scale and incoming sampling. Font or
paragraph regeneration retains those authored settings. There is no per-frame
albedo detect-and-repair loop. Arbitrary mutable C++/base-cast access is not
protected; supported editor commands enforce replacement/removal restrictions.

## Resource lifecycle

```cpp
bool resourcesDirty() const noexcept;
size_t retainedCpuBytes() const noexcept;
bool hasQuadResources() const noexcept;
bool canRender() const noexcept;
void retryResources() noexcept;
void releaseResources() noexcept;
bool prepareResources(draw::Engine& engine) noexcept;
static bool prepareResources(Node* root, draw::Engine& engine) noexcept;
void reportRenderFailure() noexcept;
void update(float delta) override;
void loadResources(draw::Engine& engine, SceneLoadProgress* progress = nullptr) noexcept override;
std::shared_ptr<Component> clone() const noexcept override;
```

Prepare only with frame/background producers stopped, before acquisition or
recording. This is a resource boundary, not an ordinary draw/update operation.
The default draw delegate calls subtree preparation after queued changes, then
invalidates histories/requests scene work when text versions change. Custom
loops must arrange the same boundary; never upload in a UI callback.

Preparation resolves the owning Scene's registry first, coalesces latest authored
changes, and tracks Text revision, geometry, engine, Scene identity and font
generation/identity. Unchanged calls do no raster/upload/drain/rebuild work.
Pixel-only edits retain live CPU mesh/drawable/material identities; geometry edits
publish prepared geometry/BLAS. `prepareResources()` false means unchanged, not
"every resource succeeded"; inspect readiness separately. Failed attempts warn
and are cached until authored change or `retryResources()`. `update()` only
requests future work; traversal does not rasterize or repair GPU resources.

Ordinary edits retain the last complete realization until publication. Font
invalidation/unavailability suppresses stale glyphs/shadows. Empty, whitespace,
missing-font, failed or clipped-away masks contribute no raster/traced text,
even if cutoff is disabled; a prepared quad remains pickable.
`hasQuadResources()` reports prepared geometry availability; `canRender()` also
requires coverage/current font availability. Native/allocation/parser failures
are contained at text boundaries and omit only that text. Render extraction can
call `reportRenderFailure()` to suppress an invalid state until a source/material
change or explicit retry. Renderer/device capability remains separate from these
safe statuses.

`releaseResources()` drains the old context and retains independent CPU data;
call before rebinding/destroying an Engine. A cross-engine rebind without release
warns/omits rather than destroying another context's active owners. Release
renderer snapshots first, then text/drawable native owners, while Engine is alive.
Clones retain independent CPU data/generator/material settings and, where loaded,
independent native resources. Failed GPU cloning keeps an unrealized CPU copy;
catastrophic clone allocation can return null. Retained immutable fonts remain
snapshots and are re-resolved on scene adoption. `retainedCpuBytes()` is conservative
CPU cache/font accounting for editor history, not GPU memory usage.

## Persistence

```cpp
void deserialize(std::shared_ptr<json::JsonNode> data,
    const std::filesystem::path& basePath) noexcept override;
std::shared_ptr<json::JsonNode> serializeReferences(const std::filesystem::path& basePath) noexcept override;
std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) noexcept override;
```

Stores common priority plus `text` and `material` **objects**, authored `size`
(two-element array), `preserveAspect` and `pivot` (two-element array). The material
uses the existing MaterialSerializer PBR schema, not a nested JSON string.
Incoming albedo source references are removed before conventional parsing can
resolve/read them. Generated sources are recreated and sampling-only settings
restored on that source; no color/appearance lives in Text JSON.

`requiresBg2Export()` is always false; text data/material still serialize.
Whole-component ignoreSerialization remains a separate policy. Complete scene
saves copy declared fonts/notices/other material images, but generate **no text
BG2 and no albedo PNG**. Reference-only serialization requires filesystem sources
for other nonignored images; procedural ones need complete saving. Null serializer
results must be checked; graph save propagation refuses a lossy/incomplete save.
See [complete IO](../db/DrawResourceIO.md#editable-text-and-font-assets).

Use [Composer](../../apps/draw_composer.md#text-and-font-authoring) for history-aware
authoring, or the [minimal tutorial](../../tutorials/text/README.md). One drawable
per node remains the editor contract; create child text nodes rather than adding
another drawable to a model node.
