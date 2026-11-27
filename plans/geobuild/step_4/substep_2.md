# 4.2 Integrate a pinned private Manifold dependency

## Entry condition

[4.1 Define the editable CPU recipe](../step_4/substep_1.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

lib/cmake/deps.cmake, lib/CMakeLists.txt, project packaging and lib/third_party conventions. Upstream: https://github.com/elalish/manifold .

## Implementation contract

First inspect available dependency wiring. Manifold was absent at planning time. If build-system edits are required, obtain explicit user authorization for the specific CMake changes under AGENTS.md before editing them. Pin a released upstream revision by immutable commit, record version and Apache-2.0 notices. Prefer a repository-pinned dependency and private static linkage into bg2e with PIC where required. Disable unused bindings, examples, tests and optional parallel dependencies. Keep ordinary builds and installed C++ headers independent of network fetching, Manifold include paths for consumers and additional required SDK variables.

## Acceptance evidence

Record exact version/commit, license location, private link target, available native precision, provenance and ID-allocation API for that revision. Verify standalone/package implications on supported platforms; label untested platforms. A dependency download or compilation requires the applicable permissions.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [4.3 Construct closed primitives with source labels](../step_4/substep_3.md)
