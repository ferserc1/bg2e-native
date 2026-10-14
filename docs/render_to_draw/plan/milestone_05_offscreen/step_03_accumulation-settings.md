# M05-03 — Headless configuration import and exact frame accumulation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M05-02 and M03 histories/settings codec.

## Scope and deliverable

- `lib/include/bg2e/draw/RenderSettingsPreferences.hpp`
- `lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp`
- `lib/include/bg2e/draw/pipeline/FrameAccumulator.hpp`
- `lib/include/bg2e/app/DrawOffscreenApplicationDelegate.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Define CLI/config precedence: defaults, persisted defaults if explicitly enabled, imported graphics document, then explicit CLI resolution/frame overrides. Use the M01 codec with active headless renderer/upscaler targets.
2. Define --frames N as exactly N rendered-scene samples for one frozen view by default. Reset all relevant histories at job start; presentation counts and unrendered updates do not count.
3. Accumulate the scene in a declared linear/HDR domain before final encoding using public pipeline accumulation support. If existing TemporalAccumulator only provides reprojected/EMA histories, add the smallest public frame-average mode/object rather than mislabel warm-up as accumulation.
4. Specify behavior with FSR and effect histories, jitter, animated scenes/fixed timestep and N=1. Preserve separate output extent and lower internal resolution; no-RT/no-effect accumulation is still meaningful and valid.

## Verification and exit conditions

- N=1 matches the single-frame conversion; a controlled varying sample sequence verifies the configured accumulated result and N stopping condition.
- An exported UI configuration imports without UI; malformed dimensions/counts/config types fail before starting the job and unsupported options yield declared diagnostics.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M05-04/M05-05 demonstrate output semantics. M08 imports the same graphics configuration but has independent UV-sample accumulation rules.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
