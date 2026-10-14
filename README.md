<p align="center">
  <img src="assets/35mmisle-logo.svg" alt="35mmIsle — a weathered film SLR rediscovered on a sunlit beach, with a volcanic island reflected in its lens" width="1000">
</p>

# 35mmIsle

**35mmIsle is the native port of Pokémon Snap (1999) to modern systems, based on [libultraship (LUS)](https://github.com/Kenix3/libultraship).** The initial implementation targets are Linux x64 and Windows 11 x64.

The port builds on the authoritative [Pokémon Snap decompilation](https://github.com/ethteck/pokemonsnap), with integration patterns informed by other mature LUS-based ports. ROMs and extracted game assets are not distributed with this repository.

## Development documentation

Start with [PLAN](docs/planning/PLAN.md) for architecture and design decisions. The following documents cover source baselines, porting research, implementation contracts and validation.

| Document | Purpose |
|---|---|
| [PLAN](docs/planning/PLAN.md) | Proposed architecture, faithful core loop, decision register and stop/go questions |
| [ASSESSMENT](docs/planning/ASSESSMENT.md) | Independent planner/reviewer adjudication, H1–H8 amendments, tradeoffs and falsifying experiments |
| [BASELINES](docs/planning/BASELINES.md) | Seven immutable source pins, authority and preserved local-reference state |
| [DECOMP](docs/planning/DECOMP.md) | Authoritative Snap source, scheduler, assets, graphics/photo, save and audio boundaries |
| [LUS / Ghostship](docs/planning/LUS_GHOSTSHIP.md) | Current host APIs, template mismatch, resource/archive and rendering/readback contracts |
| [2Ship / MM](docs/planning/2SHIP_MM.md) | Paired native-port/decomp comparison: transferable patterns and rejected assumptions |
| [Shipwright / OOT](docs/planning/SHIPWRIGHT_OOT.md) | Independent paired comparison of lifecycle, resources, ABI, audio, saves and packaging |
| [ROADMAP](docs/planning/ROADMAP.md) | Dependency-gated work units, waves, ownership and integration contracts |
| [VALIDATION](docs/planning/VALIDATION.md) | Future original-ROM fixtures, behavioral acceptance and platform/release gates |
| [LICENSING](docs/planning/LICENSING.md) | Original-work MIT scope, third-party restrictions and import/publication gates |
| [INVENTORY](docs/planning/INVENTORY.md) | Frozen portability screening, direct-review evidence and coverage limits |
| [VERIFICATION](docs/planning/VERIFICATION.md) | Recorded development-documentation checks and their evidence limits |
| [BEADS](docs/planning/BEADS.md) | Planning/implementation issue mapping; Beads remains the live status authority |

The [upstream `ethteck/pokemonsnap`](https://github.com/ethteck/pokemonsnap/tree/df8815e75f3d79618466e01e0827c6165b93d11c) pin is game-source authority, not the modified local fork. The initial ROM baseline is the exact US version recorded in BASELINES.

## Project scope

The port's scope covers all seven courses, original interactions and unlocks, camera/focus/shutter, photo selection and Oak evaluation, report/album/gallery, durable save and cold reload, original audio, ending and credits. Faithful photography requires both per-object live focus-color history and reconstructed-photo depth/scoring semantics. ROADMAP and VALIDATION define implementation dependencies and fidelity requirements.

## Building the original runtime and tools

Requires CMake 3.24+, a C11/C++20 compiler, native thread support and a 64-bit target. Linux presets use Ninja; Windows presets use Visual Studio 2022 x64. The default build is independent of LUS and all game/reference checkouts:

```sh
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release
build/linux-release/35mmisle-rom check /path/to/pokemonsnap.z64
```

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
.\build\windows-release\Release\35mmisle-rom.exe check "C:\private\pokemonsnap.z64"
```

Debug presets are `linux-debug` and `windows-debug`. CTest runs independently authored synthetic fixtures for native memory primitives, ROM identity, stackful processes, owned message queues and VPK0 decoding; checks remain active in Release. Public jobs never receive a ROM or extracted game content.

Current implemented surfaces are **not a playable Pokémon Snap executable**:

- `35mmisle-rom` / `isle-rom`: read-only exact-US image validation in z64, v64 or n64 byte order, without writing normalized ROM data.
- `isle-native`: checked C11 arithmetic, caller-backed arenas and bounded spans.
- `isle-process` (`35mmisle/process.hpp`): an original stack-preserving `isle::Process`, implemented with a parked, joinable native worker and synchronous creator-thread handoff. Only the creator may resume, inspect, cancel or destroy it. `Yield::wait(0)` stays inline; `wait(N)` resumes the same stack after exactly N subsequent `resume()` calls. Cancellation unwinds and joins at cooperative boundaries, not during non-yielding callback execution. Successful waits/resumes do not allocate or copy callbacks. Native FP state is isolated across waits; that is not R4300 FP or Snap scheduling equivalence. Callers must honor the header's exception/unwind and lifetime rules and compile callback arithmetic with dynamic-FP-aware options.
- `isle::MessageQueue<T>` (`35mmisle/message_queue.hpp`): a bounded, construction-allocated, move-only ownership ring with blocking and nonblocking push/pop. `accepted` means admitted ownership, not completed work; failed pushes retain caller ownership. `close()` rejects new messages, wakes waiters and permits FIFO drain. Payload move/destruction must be nonthrowing, nonblocking and non-reentrant; prevent new access and join users before destroying the queue. This is not an N64 priority scheduler.
- `isle-vpk0` (`35mmisle/vpk0.hpp`): the original allocation-free decoder contract described below.

Link `isle-process` through CMake to inherit `Threads::Threads`; the header-only queue also requires native thread linkage. Existing process targets retain strict FP options (`/fp:strict` on MSVC; `-fno-fast-math`, `-ffp-contract=off` and `-frounding-math` on GCC/Clang).

### VPK0 decoding contract

`isle::vpk0::decode(std::span<const std::byte> input, std::span<std::byte> output) noexcept` returns a `Result` containing `error`, `bytes_consumed` and `bytes_written`; its explicit boolean conversion denotes success. The caller owns bounded, **nonoverlapping** input/output storage; decoding performs no allocations. The lowercase `vpk0` header declares the output length. Invalid/truncated headers and insufficient output capacity fail before writes; a body error can leave partial output, which callers must not publish. Publish output only on success. Success stops at the declared output length and reports the logical consumed bytes rounded up from bits; trailing input, such as subsequent ROM data, is allowed. Nonzero sample-method values share the format's nonzero-method behavior, not an invented unsupported-method rejection.

Linux Debug/Release and Clang ASan+UBSan checks passed. Private differential checks matched all three recognized streams in the exact supported ROM against the pinned independent reference: 1,026,256 output bytes and exact consumed lengths. Truncated streams were rejected without publication. This validates decoding, not typed asset relocation or game integration; no raw game payload is a public fixture.

## Opt-in real libultraship host probe (nonplayable)

`35mmisle-host-probe` links real LUS at `759b13401cd6903daa67a00a753066f791622e78`. It opens an SDL3/OpenGL window, submits original procedural colored rectangles through the real renderer and tears down host owners before exit. It does not boot the game, load a ROM, run the Snap scheduler, synthesize audio or establish graphics/photo fidelity.

The Linux development run used SDL3 **3.4.2**, OpenGL **and GLES2 development headers**, plus libzip, nlohmann-json, tinyxml2, spdlog, native threads and X11/EGL/Mesa support. GLES headers are needed by this dependency composition even though the probe selects desktop OpenGL (`USE_OPENGLES=OFF`). Supply these dependencies in a user-controlled prefix and/or through system packages. See the [Linux host CI recipe](.github/workflows/native.yml) for Ubuntu packages and an SDL3 source build pinned to [`683181b47cfabd293e3ea409f838915b8297a4fd`](https://github.com/libsdl-org/SDL/tree/683181b47cfabd293e3ea409f838915b8297a4fd); distro package versions are not a fully locked release composition.

```sh
PREFIX=/path/to/user/dependency-prefix
cmake -S . -B build/linux-host -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DISLE_BUILD_HOST_PROBE=ON \
  -DISLE_LUS_SOURCE_DIR=/path/to/read-only/clean/pinned-libultraship \
  -DCMAKE_PREFIX_PATH="$PREFIX"
cmake --build build/linux-host --parallel 2
ctest --test-dir build/linux-host --output-on-failure
```

The supplied LUS checkout must be clean at the exact pin; it is never modified. Omitting `ISLE_LUS_SOURCE_DIR` (or setting it empty) fetches the enforced immutable pin instead. CMake exports committed LUS bytes into the build directory, applies the exact-pin host-lifetime adaptation only there, enforces selected dependency commits and verifies the pinned stb header digest. The OpenGL shader support resources under `build/linux-host/host-renderer-resources` are pinned renderer resources, not game assets; retain that directory when running the development binary. Do not enable upstream automatic vcpkg, scripting/TCC, MPQ or DLL loading for this probe.

For the explicitly observed Linux software-rendering path (prefix libraries shown under `lib`; adjust for your dependency layout):

```sh
LD_LIBRARY_PATH="$PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
SDL_VIDEODRIVER=x11 SDL_VIDEO_FORCE_EGL=1 LIBGL_ALWAYS_SOFTWARE=1 \
timeout 90s xvfb-run --auto-servernum \
  --server-args="-screen 0 800x600x24 -nolisten tcp" \
  build/linux-host/35mmisle-host-probe --frames 240
```

For a visible Linux desktop window, use the same library/EGL/software environment without `xvfb-run`. `--frames` accepts 1–3600 (default 120); the process has an internal deadline and the example adds an external timeout. The probe writes its own host configuration/logs in LUS's application-data location, not game data.

The integration owner observed 240 real-renderer frames via Xvfb + Mesa llvmpipe + SDL3 EGL, colored geometry in a later frame and `HOST_OWNERS_RELEASED` before exit. The first captured frame was gray GUI startup; `FIRST_SUCCESSFUL_RENDERER_FRAME` means submission, **not pixel proof**. Default GLX context creation failed on that host; only the explicit EGL path is qualified by that observation. The new CI host job requires bounded submission and owner-release markers; its logs alone do not assert pixels. No Windows host-probe pass or Windows 11 qualification is recorded. Core MSVC synthetic CI is separate from host/game qualification.

[ADMISSION](docs/development/ADMISSION.md) records original-source boundaries, observed mechanism evidence and candidate-scoped import/release decisions. [Dependency evidence](cmake/dependencies.json) records selected pins and the observed Linux host composition; the enforced opt-in build controls are [Libultraship.cmake](cmake/Libultraship.cmake), and each configured host build writes `isle-host-dependencies.txt`. Neither is a complete shipped-composition approval. CPack still installs/packages **only the ROM checker and its original MIT license**, not the host probe, source bundles or a game.


## License and contributions

[LICENSE](LICENSE) is MIT for **original 35mmIsle contributions**, copyright 2026 35mmIsle contributors. It does not relicense Pokémon Snap, Nintendo/SGI material, decompilations, reference ports, dependencies or extracted payloads. Their rights and notices remain with their respective holders; missing or restricted grants block the affected import/release. Read LICENSING before proposing any imported source or generated content.

Development work is tracked in Beads. Architecture, porting decisions and validation requirements are documented under `docs/planning/`.
