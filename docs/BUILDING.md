# Building the native runtime and tools

Build requirements, private engine preparation, host probes and Windows dependencies. Run all commands from the repository root.

[Project README](../README.md) · [Admission index](development/ADMISSION.md) · [Running the application](RUNNING.md)

- [Original runtime and tools](#building-the-original-runtime-and-tools)
- [VPK0 decoding contract](#vpk0-decoding-contract)
- [Exact private C engine](#exact-private-c-engine)
- [Opt-in libultraship host probe](#opt-in-real-libultraship-host-probe-nonplayable)
- [Windows dependencies](#windows-static-runtime-host-dependencies)

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

Debug presets are `linux-debug` and `windows-debug`. CTest executables exercise independently authored native memory, ROM identity, GTL clock/context policy, stackful processes, owned queues, VPK0, compiled-C continuations, typed sprites, packed-depth photo reductions, audio resources/readers/players/synthesis/effects, controller publication and checked save codecs/persistence/startup state; checks remain active in Release. The private game engine adds actual upstream C dispatcher, fade and camera/model computations with synthetic data. Public jobs never receive a ROM or extracted game content.

### VPK0 decoding contract

`isle::vpk0::decode(std::span<const std::byte> input, std::span<std::byte> output) noexcept` returns a `Result` containing `error`, `bytes_consumed` and `bytes_written`; its explicit boolean conversion denotes success. The caller owns bounded, **nonoverlapping** input/output storage; decoding performs no allocations. The lowercase `vpk0` header declares the output length. Invalid/truncated headers and insufficient output capacity fail before writes; a body error can leave partial output, which callers must not publish. Publish output only on success. Success stops at the declared output length and reports the logical consumed bytes rounded up from bits; trailing input, such as subsequent ROM data, is allowed. Nonzero sample-method values share the format's nonzero-method behavior, not an invented unsupported-method rejection.

Linux Debug/Release and Clang ASan+UBSan checks passed. Private differential checks matched all three recognized streams in the exact supported ROM against the pinned independent reference: 1,026,256 output bytes and exact consumed lengths. Truncated streams were rejected without publication. This validates decoding, not typed asset relocation or game integration; no raw game payload is a public fixture.

### Exact private C engine

```sh
cmake -S . -B build/native-engine -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DISLE_BUILD_GAME_ENGINE=ON \
  -DISLE_SNAP_SOURCE_DIR=/path/to/read-only/pokemonsnap
cmake --build build/native-engine --parallel 2
ctest --test-dir build/native-engine --output-on-failure --no-tests=error
```

Omit `ISLE_SNAP_SOURCE_DIR` to fetch the immutable authoritative commit privately. A supplied repository may have a different HEAD or dirty worktree: preparation reads exact Git objects, never working files or an upstream build. [`snap-engine-inputs.json`](../cmake/snap-engine-inputs.json) identifies seven immutable GAME source/header blobs, thirty-seven reviewed definitions, source/native hashes and narrowly versioned seam transformations. [`prepare_engine.py`](../tools/prepare_engine.py) emits C/provenance only inside the build directory and verifies original native support hashes. No SDK implementation/header is compiled, and complete GAME bodies are not committed or packaged. This is private development admission, not blanket MIT or redistribution permission.

Camera APIs own type-3 payloads and up to two matrices. `isle_game_camera_peek_matrix` borrows immutable kind without computing dirty caches; `isle_game_camera_compute_all` publishes matrix/reflection/normalization only after every calculation succeeds. Kind8 uses fixed Y-up and dirties on eye/target/roll, not up-only edits. Model APIs retain translate-only/kind-28 local caches or explicit renderer-contextual poses, independently of model-level placement. Pure preflight precedes mutation; native retirement joins actual work. Fixed words upload directly into word-swapped RDRAM only where a valid local or renderer-owned payload exists. Full scene/photo, actor assets and Windows 11 runtime qualification remain separate.


## Opt-in real libultraship host probe (nonplayable)

`35mmisle-host-probe` links real LUS at `759b13401cd6903daa67a00a753066f791622e78`. Its default SDL3/OpenGL mode submits original procedural colored rectangles through the real renderer and tears down host owners before exit. With the private engine enabled, `--rom` exercises the actual intro sprite and selected C fade. With the audio option enabled, `--rom ... --audio` adds actual native dry SFX14/RSP synthesis and checked SDL output. These are independent bounded experiments, not full scene/game boot, original cue synchronization or graphics/photo fidelity.

The Linux development run used SDL3 **3.4.2**, OpenGL **and GLES2 development headers**, plus libzip, nlohmann-json, tinyxml2, spdlog, native threads and X11/EGL/Mesa support. GLES headers are needed by this dependency composition even though the probe selects desktop OpenGL (`USE_OPENGLES=OFF`). Supply these dependencies in a user-controlled prefix and/or through system packages. See the [Linux host CI recipe](../.github/workflows/native.yml) for Ubuntu packages and an SDL3 source build pinned to [`683181b47cfabd293e3ea409f838915b8297a4fd`](https://github.com/libsdl-org/SDL/tree/683181b47cfabd293e3ea409f838915b8297a4fd); distro package versions are not a fully locked release composition.

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

The supplied LUS checkout must be clean at the exact pin; it is never modified. Omitting `ISLE_LUS_SOURCE_DIR` (or setting it empty) fetches the enforced immutable pin instead. Every configure exports committed bytes and applies exact-context lifetime/stb adaptations in fresh build-owned staging, then synchronizes changed bytes only into live private sources. Identical headers and generated stb implementation retain timestamps; an inventory removes obsolete owned inputs without deleting generated-only files. Pin/clean-source/patch/digest checks are not skipped. Retain the pinned `build/linux-host/host-renderer-resources` shaders/licenses when running the development binary. Do not enable upstream automatic vcpkg, scripting/TCC, MPQ or DLL loading.

For the explicitly observed Linux software-rendering path (prefix libraries shown under `lib`; adjust for your dependency layout):

```sh
LD_LIBRARY_PATH="$PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
SDL_VIDEODRIVER=x11 SDL_VIDEO_FORCE_EGL=1 LIBGL_ALWAYS_SOFTWARE=1 \
timeout 90s xvfb-run --auto-servernum \
  --server-args="-screen 0 800x600x24 -nolisten tcp -extension GLX" \
  build/linux-host/35mmisle-host-probe --frames 240
```

For a visible Linux desktop window, use the same library/EGL/software environment without `xvfb-run`. `--frames` accepts 1–3600 (default 120); the process has an internal deadline and the example adds an external timeout. The probe writes its own host configuration/logs in LUS's application-data location, not game data.

The integration owner observed 240 real-renderer frames via Xvfb + Mesa llvmpipe + SDL3 EGL, colored geometry in a later frame and `HOST_OWNERS_RELEASED` before exit. The first captured frame was gray GUI startup; `FIRST_SUCCESSFUL_RENDERER_FRAME` means submission, **not pixel proof**. Default GLX context creation failed on that host; only the explicit EGL path is qualified by that observation. The Linux CI host requires bounded submission and owner-release markers; logs alone do not assert pixels. [Workflow 38766587623](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38766587623) passed all ten jobs, including the full native MSVC static-runtime LUS/audio/RSP/engine build, twenty tests and four CLI helps at `ffde8a3`. Hosted Server 2022 compiler/component evidence does not qualify Windows private-ROM graphics/audio or Windows 11 runtime.

### Windows static-runtime host dependencies

The full-host preset uses Visual Studio 2022 x64, static CRT dependencies and a frozen vcpkg source/registry/tool selection. In the **same PowerShell 7 session**:

```powershell
./tools/prepare_windows_host.ps1
cmake --preset windows-host-release
cmake --build --preset windows-host-release --parallel 2
ctest --preset windows-host-release
```

The script creates only its own checkout, refuses unowned/dirty/wrong-pin inputs, and verifies the vcpkg executable's size/SHA-256 before execution. It selects the installed VS2022 x64 developer environment and exports its actual compiler/SDK search paths, using the same environment mechanism as vcpkg. CMake repeats vcpkg verification before `project()` loads the toolchain. No floating bootstrap, overlays, automatic LUS vcpkg or binary cache substitution. The preset enables host, private engine, audio, RSP and canonical RDP; public CI needs no ROM. Compiler/CLI evidence and Windows 11/device runtime qualification remain separate; see ADMISSION.

