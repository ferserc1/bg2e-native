# Step 4 — Text data and procedural paragraph rasterization

Dependency: accepted step 3.

## Data contract

1. Implement `base::Text` with UTF-8 content, font identifier, size, line spacing, additional character spacing, alignment, margins, wrapping/overflow policy and bitmap resolution. No color, unlit or other appearance fields.
2. Match `base::Light`'s JSON method signatures and tolerant optional-field reading, while validating dimensions, finite positive sizes and allocation limits. Do not read files, query a scene or create GPU resources in deserialization.
3. Define units explicitly before coding: a logical paragraph box determines wrapping/layout; raster resolution scales that box without changing line breaks; physical quad dimensions are independently managed by the component, preserving aspect by default. Serialize the necessary geometry, and record defaults and overflow behavior in progress for the editor/tutorial.
4. Add revisions or equivalent dirty tracking; unchanged setters must not regenerate pixels. Distinguish paragraph/font changes from material-only edits.

## Rasterization

1. Implement a CPU layout/rasterization service using Text and an explicitly resolved TextFont. Decode UTF-8, map glyphs, apply advances/kerning, line breaks, word wrapping, margins and alignment. Define replacement-glyph/invalid-UTF-8 behavior and handle empty content safely.
2. Initial support is simple left-to-right text, including Spanish precomposed characters. Do not promise full Unicode shaping, combining mark positioning, bidi, emoji or arbitrary OpenType features.
3. Rasterize individual glyph coverage into temporary masks, compose overlaps correctly and clip all writes. Avoid baseline/descender clipping, unsigned offset errors and oversized-buffer arithmetic overflow.
4. Adapt this service through a `base::ProceduralTextureGenerator` implementation, provisionally `TextTextureGenerator`. Expose no stb types publicly. Use RGBA8 with white RGB everywhere and coverage in alpha. Use an allocator compatible with the existing Image/generator ownership contract; inspect that contract first.
5. Generator identifiers/revisions incorporate all pixel-producing state and font content/version so unrelated paragraphs do not alias. Reuse existing texture revision mechanisms. Mutable generators must not unintentionally be shared by cloned components.
6. Start with sensible linear filtering/clamp defaults and an explicit mipmap policy; preserve later user sampling edits. No SDF/MSDF, atlas GPU, per-glyph scene meshes or automatic resolution changes.

## User handoff: Composer

Explain the data/layout defaults and supported scripts. No component or Composer authoring exists yet, so successful user compilation of both library and Composer is sufficient acceptance. Do not add rasterization tests, bitmap fixtures, CLI tools or another verification executable.

Update progress and stop.

## Next milestone

Step 5 connects the generator to a one-quad/one-material TextComponent, with controlled GPU realization, cloning and full-rectangle picking.
