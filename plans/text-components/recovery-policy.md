# Mandatory text recovery policy

User requirement: recoverable text/font/resource/authoring errors must **never
generate or escape as exceptions capable of interrupting the application**.
This supersedes earlier exception-based decisions in historical progress records
and applies to all implemented and remaining milestones. It does not authorize an
unrelated, engine-wide exception refactor.

## Recovery decisions

| Condition | Recovery |
| --- | --- |
| Invalid sizes, spacing, margins, dimensions or enums | Console warning and defaults/safe limits, with a positive paragraph interior. |
| Oversized/invalid UTF-8 | Normalize U+FFFD, truncate at a scalar boundary within the content budget, warn. |
| Excessive bitmap allocation/density | Warn and reduce resolution to safe bounds without changing logical layout/world size; validation repair, not adaptive per-frame resolution. |
| Missing/unreadable/unsupported font | Warn, return null/unavailable, make dependent text transparent/omitted; no unrelated fallback font. |
| Invalid face index | Warn and use face 0 where possible, otherwise mark unavailable. |
| Bad declaration | Warn and skip that declaration, not the scene. |
| Duplicate within a collection | Warn and ignore later entries. |
| Duplicate across scene declarations | Warn and disable the ambiguous identifier, preserving other fonts; no ancestor shadowing. |
| Registry capacity/budget exceeded | Warn and omit excess resources; publish a complete set of availability outcomes. |
| Layout overflow | Clip-with-warning by default, or hide-with-warning. Legacy `error` / `OverflowPolicy::Error` means hide, never throw. |
| Missing glyph | Warn and use U+FFFD/glyph 0 as appropriate, without font fallback chains. |
| Raster failure or work-budget exhaustion | Warn and produce transparent/unavailable text, not an unintended partial bitmap. |
| Bitmap allocation failure | Try a transparent 1×1 image; if even that is unavailable, return null/empty status and safely omit the resource. |
| Locked source or malformed edit slot | Warn, return false/rejected/no-op status, keep every selected source unchanged; no exception/partial mutation. |
| Serialization/save-resource failure | Warn and return explicit unavailable/failure status. Preserve editable data and rollback; never falsely report an incomplete save as successful. |

## Implementation rules

- No `throw`/`rethrow` for normal text validation or recovery control flow.
- Contain unexpected allocation, filesystem and backend exceptions at text
  resource/mutation boundaries, converting them to warnings and safe statuses.
  Adding `noexcept` alone is insufficient: it must not turn an exception into termination.
- Check nullable fonts, clones, textures, images and representations before use.
  `TextLayout::renderable == false` and empty rasters are valid failure outcomes,
  never reasons to dereference/upload unavailable storage.
- `textFontsReady()` means preparation completed, not that every font loaded.
  `textFont()` may return null. Keep authored paragraphs/materials editable and
  never draw stale glyphs/shadows after their font becomes unavailable.
- Consume actual generator dimensions after generation: fallback may be 1×1.
- Deduplicate diagnostics by source revision/registry generation. Cache unavailable
  outcomes and retry on authored change/explicit reload, not through per-frame
  file I/O, warning floods or regeneration loops.
- One bad text must not interrupt scene loading, frame rendering or ordinary
  editor use. Preserve unaffected geometry/resources.
- Trusted project fonts remain required: structural checks and C++ exception
  containment do not sandbox stb against malicious native input/faults.

No agent builds, application launches, automated tests, fixtures or harnesses are
authorized. The user compiles; later Composer checks cover recovery observations.
