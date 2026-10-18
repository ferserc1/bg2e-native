# Bake CPU contract fixture

Include probe.hpp temporarily in an existing engine test/example harness and call
`bg2e::bake::tests::runContractsProbe()`. No Device or shader resource is required
by this fixture, though it links the engine's new bake sources. No CMake target is
added. The project lead compiles and runs it; the agent has not executed it.

Checks actual settings metadata writes and strict SettingsCodec serialization,
invalid scalar/enum/NaN, mode-distance/bias and dilation-resolution validation,
valid RGB8 and above-one/black float pixels, zero/mismatched/wrong-variant/negative/
nonfinite payloads, checked huge dimensions and missing context/target owners.
These validate CPU contracts, not integrated bake behavior. Check exported methods
link on Windows as well as Vulkan/Metal host builds. Concrete GPU execution,
exact accumulation, UV chart isolation and readback are subsequent steps.
