# TextFontComponent

**Header:** `<bg2e/scene/TextFontComponent.hpp>` · **Namespace:** `bg2e::scene`

Final Component, factory/JSON type **`TextFonts`**. Declares fonts; it owns no
parser state. [Scene](Scene.md#text-font-registry) owns loaded immutable resources.
Place a collection on the root for clarity, but any node is legal. All identifiers
belong to one scene-wide namespace: no ancestor shadowing or traversal-order rule.

```cpp
struct Declaration {
    std::string identifier;
    std::filesystem::path path;
    uint32_t faceIndex = 0;
    std::vector<std::filesystem::path> notices{};
    bool operator==(const Declaration&) const = default;
};
static constexpr size_t MaxDeclarations = 128;
TextFontComponent() = default;
const std::vector<Declaration>& declarations() const;
void setDeclarations(std::vector<Declaration> declarations) noexcept;
void addFont(std::string identifier, std::filesystem::path path, uint32_t faceIndex = 0);
void removeFont(const std::string& identifier);
static bool validateDeclaration(const Declaration& declaration) noexcept;
std::shared_ptr<Component> clone() const noexcept override;
void deserialize(std::shared_ptr<json::JsonNode> data,
    const std::filesystem::path& basePath) noexcept override;
std::shared_ptr<json::JsonNode> serializeReferences(const std::filesystem::path& basePath) noexcept override;
std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) noexcept override;
void loadResources(draw::Engine& engine, SceneLoadProgress* progress = nullptr) noexcept override;
```

Identifiers use 1–128 ASCII letters/digits or `_`, `-`, `.`, `:`. Font paths are
nonempty, bounded to 16 KiB and reject NUL. API paths normalize to absolute lexical
paths; JSON relative paths resolve against basePath, not an ancestor node.
Invalid declarations warn/skip, extra collection entries beyond 128 are omitted,
bad face indices recover to 0, and later same-collection duplicates are skipped.
Allocation failures retain the previous collection. Composer's buffered adapter
keeps the preceding valid declaration while incomplete rows are being typed.

`notices` can supply up to 32 license/provenance file paths per declaration. They
are not fonts and are not parsed for rendering. Supplying a notice does not grant
redistribution rights. Declaration edits/membership changes invalidate the owning
scene registry; clones keep independent declarations/component policy, no registry.

## Representation and packaging

JSON contains common type/priority and a `fonts` array of
`{identifier,path,faceIndex,notices}`. Paths/notices serialize relative to the
output directory. Parsing/serialization is graphics-free and opens no font file;
failed serialization/clone returns null with a warning. Entire-component
ignoreSerialization is independent of font availability.

`loadResources()` asks the owning Scene to prepare **all** declarations before
text realization. Detached collections warn/defer; adopt the subtree into a Scene
first. A self-contained [scene save](../db/DrawResourceIO.md#editable-text-and-font-assets)
copies font bytes/notices with original extensions and rewritten relative paths.
When no notices are explicit, common adjacent notices are discovered, including
the canonical font's OFL.txt/README.md. Missing required files fail complete saving
with status/rollback; unavailable fonts do not abort scene loading/rendering.

See [trusted fonts and licensing](../base/TextFont.md#security-and-licensing),
[TextComponent](TextComponent.md), and the [beginner guide](../../tutorials/text/README.md).
