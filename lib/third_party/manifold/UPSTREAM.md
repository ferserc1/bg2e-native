# Manifold

This directory contains the unmodified C++ core of Manifold 3.5.5 from
[commit 32271a1a999ce721ffaf5ab10e9746ceeb696547](https://github.com/elalish/manifold/commit/32271a1a999ce721ffaf5ab10e9746ceeb696547).
The SHA-256 of the downloaded commit archive was
`742d68ebb4fa88fb8ba3dea8de2094829730b2d246c7f4f4ad7eac3314ac6c5f`.

The vendored files are the top-level C++ sources and internal headers from
`src`, the headers under `include/manifold` used by the C++ core, and the
upstream license and authors list. CrossSection, bindings, tests, examples,
and upstream build scripts are omitted. Original source notices are preserved.
The [Apache-2.0 license](LICENSE) and [third-party notices](THIRD_PARTY_NOTICES.txt)
are included in the engine distribution.

The build defines a PIC, serial-only static target named `bg2e_manifold`.
Only the engine links it. The installed SDK exposes no Manifold headers,
library, or CMake package. Static archive objects are linked as needed by
engine code.

`MeshGL` uses float properties and 32-bit indices. `MeshGL64` uses double
properties and 64-bit indices, with `Manifold(const MeshGL64&)` and
`GetMeshGL64()` as the corresponding import and export APIs. Triangle
provenance uses `runIndex`, `runOriginalID`, and `faceID`. Run IDs are
32-bit even in `MeshGL64`. `Manifold::ReserveIDs(uint32_t)` reserves
consecutive original IDs and returns the first one. Keep those IDs separate
from the editable recipe IDs.
