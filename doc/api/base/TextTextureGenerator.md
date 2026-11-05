# TextTextureGenerator

**Header:** `<bg2e/base/TextTextureGenerator.hpp>` · **Namespace:** `bg2e::base`

Final [ProceduralTextureGenerator](Texture.md#proceduraltexturegenerator) owning
an independent Text value/cache plus a shared immutable font. The caller supplies
the font and its registry generation; this class resolves no identifiers/files.

```cpp
TextTextureGenerator(const Text& text, std::shared_ptr<const TextFont> font,
    uint64_t fontVersion = 0);
TextTextureGenerator(const TextTextureGenerator& source);
TextTextureGenerator& operator=(const TextTextureGenerator&) = delete;

const Text& text() const;
const std::shared_ptr<const TextFont>& font() const;
uint64_t fontVersion() const;
size_t retainedBytes() const noexcept;
bool configure(const Text& text, std::shared_ptr<const TextFont> font,
    uint64_t fontVersion = 0) noexcept;
std::shared_ptr<TextTextureGenerator> clone() const noexcept;
uint8_t* generate() noexcept override;
std::string imageIdentifier() noexcept override;
base::Color::Type colorType() override;

std::shared_ptr<Texture> createTextTexture(
    std::shared_ptr<TextTextureGenerator> generator) noexcept;
```

`configure()` compares complete paragraph values, immutable font identity and
fontVersion. Effective changes advance the inherited procedural revision; no-ops
return false. Failure can also return false with a warning. Material color, UVs
and sampling do not belong to generator state. `colorType()` is TypeLinear.

`generate()` caches CPU layout/pixels until a source revision changes, but returns
a **fresh `new uint8_t[]`** buffer on each call for Image's delete[] ownership.
Independent generators/clones have different process-local image identities;
these are not persistent hashes. Copy/clone creates independent editable data,
cache and identity, sharing only immutable font bytes. A failed clone returns null.

Null/unavailable fonts yield cached transparent output, not an unrelated fallback
font. Raster/allocation failure uses a cached transparent 1x1 fallback; if even
four bytes cannot be allocated, generate returns null. Consume actual dimensions
**after generation**, not only the requested Text resolution. Inherited manual
dimension tampering is repaired to authored dimensions with a warning. Repeated
unchanged failures do not cause raster/diagnostic retries every frame.

`createTextTexture()` sets linear color/filtering, clamp-to-edge U/V/W and no
mipmaps initially, returning null on allocation failure. It does **not** establish
component ownership permissions; TextComponent sets sourceEditable=false and
ignoreSerialization=true. Regeneration does not reset later sampling choices.
`retainedBytes()` conservatively accounts CPU paragraph/cache/font storage for
editor resource retention; it is not a GPU allocation query.

This mutable generator/cache is not a concurrent worker API. Configure, clone and
generate only at coordinated owner/resource boundaries. Constructors and argument
copies can allocate; contain those failures in the application. Prefer the
[component's lifecycle](../scene/TextComponent.md#resource-lifecycle) unless
implementing a custom procedural owner.
