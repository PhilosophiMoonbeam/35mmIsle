# Native component probes and experiments

Opt-in native probe instructions and their recorded evidence limits. Commands run from the repository root; see [BUILDING](../BUILDING.md) for configuration prerequisites and the host environment referenced below.

[Project README](../../README.md) · [Admission record](ADMISSION.md) · [Build prerequisites](../BUILDING.md)

## Opt-in real libultraship host probe (nonplayable)

### Private intro asset and original C fade

Configure the host above with `-DISLE_BUILD_GAME_ENGINE=ON` and optionally `-DISLE_SNAP_SOURCE_DIR=/path/to/read-only/pokemonsnap`. Then run the same explicit EGL/library environment with:

```sh
build/linux-host/35mmisle-host-probe --frames 120 --rom /path/to/private/pokemonsnap.z64
```

The loader verifies one bounded read-only ROM snapshot, decodes the selected bank, hydrates eight bitmap views and creates actual native C GObj/SObj state on a dedicated ordinary game-owner thread. After renderer/audio admission, an independent rational VI clock drives the bounded GTL policy: consume the update interval, drain surplus retraces, update once, then acquire a draw context when due. Presentation refusal retains the exact command lease; it does not manufacture an update or completion. Each context owns four wrapper commands over immutable asset lists. The selected RGB fade starts at game update 60 and runs twenty updates; alpha is unchanged. This remains an experimental stimulus, not the complete original intro timeline. Invalid ROM/layout/decode input fails without a procedural fallback.

The integration owner observed the original Nintendo 64 sprite and completed black fade through actual LUS/SDL3 EGL: 120 presentations, 122 C updates, 130 retraces/3 dropped and 121 admitted/retired graphics leases. Independent SDL-dummy audio executed 107 actual RSP tasks. C users joined on their creator; GPU/cache/host owners retired before asset storage and the game-owner thread were released. Captures remain private. The documented NTSC progressive rate 2250000/37609 Hz is a pacing candidate, not a physical measurement; genuine GAME canonical-memory feedback, scene reset/input integration, original cues, gameplay and Windows 11 runtime remain unqualified. The fully traced software run opened no DRM/audio nodes but retained 13 denied SDL input-node and two denied `/dev/udmabuf` opens; no device/hearing or all-process zero-device-attempt claim follows.

`--no-input` independently disables controller enumeration and keyboard/button callbacks while retaining real presentation; default input behavior is unchanged. It does not select or sandbox graphics/audio drivers. With the separately source-qualified offscreen composition, actual Linux execution completed 120 intro presentations/122 C updates; a private read-only swap observer showed the original sprite and completed black fade. All three launcher/descendant traces—intro, observed intro and four-frame procedural mode—had zero DRM/input/audio/udmabuf attempts or permission failures. This new evidence does not erase the historical denied-node run above or qualify physical input/gameplay/Windows 11.

### Actual native model presentation

Enable host, private engine, RSP and canonical RDP together using the configuration below. In the same compatible OpenGL/Vulkan environment:

```sh
build/host-feedback/35mmisle-host-probe --rom /private/pokemonsnap.z64 \
  --model-preview --cpu-device --no-input --frames 32
```

This mode samples the real pose-0 HD Pidgey, computes native GAME camera/model/light state and runs a new actual RSP/RDP task on persistent canonical memory for every presented frame. Completed RGBA16 pixels convert once into preallocated RGBA32 tiles; real LUS point-presents them at the native 320×240 canvas. The host waits for GL completion and invalidates the three exact texture-cache bases before changing pixel storage. A declined presentation retains its exact commands/pixels. The immutable ROM and typed owners survive through final RSP/RDP and GL/cache retirement.

The exercised Linux surface completed 32 new native tasks and LUS submissions. The first captured swap showed gray GUI startup; later private 640×480 default-framebuffer captures showed the original brown/cream textured model, changing poses and exact repeated images. Some sampled poses cross the image boundary. Seventeen invalid photo requests and twelve invalid host argument combinations rejected; finite negative animation time remains in the source domain. This is an authored 0..10 pose-sampling preview, not a complete scene, original timeline, camera controls, gameplay or Windows 11 runtime qualification. `--no-input` does not select drivers; `--cpu-device` alone does not prevent driver probes.

### Required feedback independent of presentation

Enable host, private engine, RSP and canonical RDP together, then use a compatible actual OpenGL/Vulkan composition:

```sh
cmake -S . -B build/host-feedback -DISLE_BUILD_HOST_PROBE=ON \
  -DISLE_BUILD_GAME_ENGINE=ON -DISLE_BUILD_RSP_PROBE=ON -DISLE_BUILD_RDP_PROBE=ON
cmake --build build/host-feedback --parallel 2
build/host-feedback/35mmisle-host-probe --rom /private/pokemonsnap.z64 \
  --required-feedback-probe --decline-presentations --cpu-device --frames 64
```

This explicit diagnostic disables controller enumeration/input callbacks and retains both optional visual leases. A separate fixed required-work slot grants the native owner exclusive preparation of main's actual canonical RDRAM. Main runs a new ROM-bound RSP task on that same allocation, stages packets, flushes CPU/RSP writes and executes RDP work. Actual device completion precedes the owner-domain mapped color/hidden read; acknowledgement precedes main-only lease retirement and dependent preparation. `--frames` counts CPU consumptions, not presentations. No per-operation RAM snapshot, hidden reset or retained-capture replay occurs.

The Linux smoke completed 64 actual RSP tasks, RDP completions, CPU preparations/readbacks and backend retirements, with two retained visual leases and zero visual submissions. Six authored color predicates, guards, repeated-result identity and a persistent pixel-derived canary were checked against the same mapped allocation. This synthetic mode exercises persistent shared coprocessor ownership; the separate native model/photo graph is described above. Whole-process traces recorded zero DRM/input/sound/udmabuf attempts and permission failures under privately source-qualified Mesa; CPU-device selection alone does not prevent driver probes. Full scene/input/reset and Windows 11 runtime remain open.



[ADMISSION](ADMISSION.md) records original-source boundaries and exercised evidence. [Dependency evidence](../../cmake/dependencies.json), [Libultraship.cmake](../../cmake/Libultraship.cmake) and [SnapEngine.cmake](../../cmake/SnapEngine.cmake) record selected inputs. Configured builds write `isle-host-dependencies.txt` and, with the engine enabled, `isle-snap-engine-inputs.json`. CPack's `Runtime` component packages the ROM checker and, when enabled, the native application, selected runtime libraries, renderer resources and verified notices. Its final `share/35mmisle/runtime-composition.json` identifies every staged file/symlink except itself, selected source/toolchain/package inputs, original/staged DSO hashes and explicit publication holds. Native packaging requires Python 3.9+; Linux also requires `patchelf`, binutils and Debian ownership tooling. Reached non-dpkg Linux DSOs require a schema-2 receipt from `tools/runtime_composition.py capture-sdk` or `capture-deb-sdk`; notices alone do not establish identity. ROMs, extracted game assets, private source/evidence and relink kits are excluded.

Pinned dependency clones and the LUS committed-source export explicitly select canonical LF bytes, independently of host Git newline defaults. Notice hashes remain exact; platform newline conversion is not accepted as an alternative identity.

At `a859a09`, [all ten native CI jobs pass](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39956071193), including Linux/MSVC whole-product relinking and portable Runtime archive checks. Candidate payloads remain runner-local; this is not Windows 11 gameplay or redistribution approval.

For a **private** static GNU lightning companion, configure the full native build with `-DISLE_BUILD_RUNTIME_RELINK_KIT=ON` before its actual final link, then build `runtime-relink-kit`. Producing the kit requires CMake 3.27+ for same-generation File API queries; ordinary builds and the standalone recipient library retain their CMake 3.24 minimum. Default output is `private-runtime-relink-kit/<configuration>` under the build directory. Follow its `RELINK-INSTRUCTIONS.txt` for standalone library rebuild, genuine source modification, generated-code execution and whole-product relink. The kit contains GAME-derived application objects: **do not publish it or infer redistribution permission**. It is not installed by CPack, does not clear R1/R2, and does not transfer Linux execution evidence to MSVC/Windows 11.


## Shared native audio and checked output

Enable `-DISLE_BUILD_AUDIO_PROBE=ON` in a build with SDL3 3.4.2 or newer. This builds `35mmisle-audio-probe`, the actual RSP consumer and a checked SDL leaf; LUS is optional for the standalone CLI:

```sh
build/linux-host/35mmisle-audio-probe --rom /private/pokemonsnap.z64 \
  --tasks 400 --raw-out /private/new-audio-directory \
  --pitch-at 150 0.5 --pan-at 150 0 --stop-at 360 --replay-at 362
# Explicit virtual transport verification, not physical playback:
SDL_AUDIO_DRIVER=dummy build/linux-host/35mmisle-audio-probe \
  --rom /private/pokemonsnap.z64 --tasks 24 --play
# Combined real LUS presentation and native audio, with all three options enabled:
build/linux-host/35mmisle-host-probe --rom /private/pokemonsnap.z64 --audio --frames 120
```
`AudioService::create` requires an explicit fourth `effects::Config` argument: direct stereo AUX return or an immutable, creation-copied network of up to sixteen sections and 19,200 ring samples. Sections support ordered feedforward/feedback, return taps, optional pole filtering and optional `Section::chorus`. The chorus candidate retains phase, fractional carry, tap drift and distinct RS32 state; positive rate with zero depth still executes resampling. Creation checks worst-case staging/DMA geometry rather than imposing an arbitrary depth cutoff. Shared effects process each original 160-frame/final-partial outer block once even when client deadlines fragment voice rendering; tails survive source retirement. Reset requires quiescence and clears ring/filter/resampling/staging state without invalidating catalog storage. Static custom6/custom7 qualification does not extend to universal chorus arithmetic or full BGM fidelity. Earlier chorus collection claims were withdrawn; strict typed-argument/device-scope replacement remains incomplete. See ADMISSION.


`--help` lists bounded resource, quota, gain and chronological control options. Initial start precedes task 1; timed controls publish **after** the specified completed task for the next sound-client callback. The explicit quota is experimental, never host queue depth masquerading as guest AI occupancy. Raw output refuses an existing directory, contains private synthesized BE16 PCM/task metadata only, and writes success provenance last. No output directory means no raw files; without `--play`, the audio CLI opens no audio device.

`AudioService` retains the catalog and owns the shared synthesis graph. `Synth` exposes explicit resource, voice, parameter-time and binding identities; `SoundPlayer` owns its logical descriptors and one microsecond calendar, defaulting to ten descriptors at measured priority 5. Raw callback deadlines sort before their floor-16 DSP times; equal deadlines use newest registered client first. Quiescent `set_player_deadline` rephases only that raw deadline, not the client's envelope calendar. Successful command publication commits predicted CPU state; terminal execution grants the owner-qualified PCM lease, which the consumer must release before another task. Staged controls accepted during execution survive completion. No host-side final-clamp mixer or independent per-voice RSP services are used.

The standalone CLI intentionally selects one sound. Partial ADPCM loops and explicit player-volume overrides are supported; game-volume conversion stays outside synthesis. All twenty authored/copied loop cases remain byte-exact after the shared-service cutover, including observed decoder/resampler histories. These proofs alone do not qualify the complete original allocator, BGM, effects or AI scheduling.

The sink retains completed PCM on `WouldBlock`, copies each accepted block once through SDL, checks actual opening/removal/queue failures, and distinguishes final **stream** drain from DAC/hearing. It is owned directly by the host session, not falsely registered as LUS's legacy void-playback backend. Combined intro/audio preparation shares one immutable admitted ROM snapshot, releases it before presentation and retains typed resources through their consumer fences. Native producer versus original SDK producer matched the complete private reference schedule; exact scope, hashes and exercised transport/visual limits are in ADMISSION. Original multi-voice/BGM/effects/AI integration, hardware sound, full boot and Windows 11 qualification remain open.

## Canonical RDP memory experiment

The opt-in `35mmisle-rdp-probe` runs original synthetic low-level RDP packets through pinned [paraLLEl-RDP](https://github.com/Themaister/parallel-rdp/tree/1cecd042b2619bc505c12bfdc713808386f2b54d), independently of LUS presentation and game CPU execution:

```sh
cmake -S . -B build/rdp -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF -DISLE_BUILD_RDP_PROBE=ON
CMAKE_BUILD_PARALLEL_LEVEL=2 cmake --build build/rdp --target 35mmisle-rdp-probe --parallel 2
build/rdp/35mmisle-rdp-probe --cpu-device --raw-out /path/to/new/synthetic-output-directory
```

This requires a real Vulkan loader/ICD, Vulkan 1.1 and enabled 8-/16-bit storage-buffer access. `--cpu-device` selects an enumerated Vulkan CPU device, not a null backend. The build acquires ten exact archive-digest-pinned renderer/tool inputs and builds its own pinned Granite `slangmosh` compiler; no installed shader-tool assumption. [ParallelRdp.cmake](../../cmake/ParallelRdp.cmake) compiles the standalone renderer subset, **not** `rdp-utils`, `alp-core`, Angrylion or MAME. Build-private provenance and complete upstream notices remain with the inputs. The probe is not installed/packaged.

The three stages clear a contiguous 64×48 color/13-depth/12-before region, draw five overlapping depth/coverage-sensitive triangles, then perform three GPU depth-as-color copies in two `0xC00`-byte halves each. After real timeline completion, checks inspect every region/hidden value and the original visibility predicates. One depth deliberately retains nonzero packed-dZ low bits; copied hidden coverage is not confused with depth metadata.

On the development machine, actual CPU Vulkan (`llvmpipe (LLVM 21.1.8, 256 bits)`, Mesa 26.0.8, Vulkan 1.4.335) completed all stages without semantic/backend errors. All **718,848** visible/hidden bytes across six snapshots matched a separately executed private CPU reference. This validates only the synthetic workload—not private-ROM raster frames, the RSP/custom-matrix frontend, full photo scoring, hardware GPUs or Windows Vulkan. See ADMISSION for exact reference/terms and limits. Output snapshots contain only this original synthetic workload; the CLI accepts no ROM or native frame.

## Private exact RSP task execution

`35mmisle-rsp-probe` executes the supported ROM's real boot/graphics/line microcode with independently authored matrix and geometry inputs. The game CPU remains native; no R4300 instructions execute.

```sh
cmake -S . -B build/rsp -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DISLE_BUILD_RSP_PROBE=ON
cmake --build build/rsp --parallel 2
ctest --test-dir build/rsp --output-on-failure --no-tests=error
build/rsp/35mmisle-rsp-probe --rom /private/pokemonsnap.z64 \
  --raw-out /private/new-synthetic-packet-directory
```

The exact archive-pinned [parallel-rsp](https://github.com/libretro/parallel-rsp/tree/9ab776a2ce28899769c4f911403234cac62fc799) library-only composition selects its MIT core license **but statically links GNU lightning under LGPL-3.0-or-later**. This is not an MIT-only binary or release admission. Original sources, complete notices, exact-input adaptations and provenance remain build-private; no SDK implementation/header, ROM, microcode, RSP runner or fuzzer enters the public source/package.

The original host service implements bounded two-way DMA, IMEM invalidation, SP status/semaphore/interrupt state and RDRAM/XBUS DPC packet delivery outside generated JIT frames. It borrows explicit word-swapped 8-MiB RDRAM and never truncates native pointers. JIT block-entry and packet limits bound execution. Currently admitted tasks are graphics/type 1/flags 4; DMA transport is synchronous, not cycle emulation, audio or yield/resume support. An observed FullSync packet is **not** GPU completion.

Actual GCC Debug/Release and Clang ASan+UBSan runs completed two cold fractional-matrix tasks, two cold graphics→line→graphics tasks and a warm graphics repeat after reload. The original viewport's downward screen Y, partial signed/fractional matrix writes and temporary line-scissor restoration are checked independently. Repeats compare semantic operands, not ignored NOOP/sync scratch words. Full-source sanitization retains one explicit Clang function-cookie exception at the foreign JIT-entry call; generated instructions themselves are uninstrumented. After correcting MSVC standard-token conformance with `/permissive-`, [all nine jobs passed at `3e478d80`](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38675299538), including native MSVC x64 Debug/Release ten-test suites and the combined renderer/compiler build. Hosted Windows Server 2022 is not Windows 11 or Windows Vulkan runtime qualification.

Enable both probe options to execute new RSP tasks directly on the real canonical renderer's persistent memory:

```sh
cmake -S . -B build/rsp -DISLE_BUILD_RSP_PROBE=ON -DISLE_BUILD_RDP_PROBE=ON
cmake --build build/rsp --target 35mmisle-rsp-probe --parallel 2
build/rsp/35mmisle-rsp-probe --rom /private/pokemonsnap.z64 \
  --cpu-device --render-out /private/new-canonical-directory
```

Actual CPU Vulkan completed fractional/partial-matrix and graphics→line→graphics tasks on one shared allocation. CPU preparation, actual RSP writes and subsequent RDP reads use the same word-swapped RDRAM; actual device completion precedes readback. Cold initialization occurs once, while each authored task explicitly clears its target through RDP commands. The earlier separate-memory after-SP replay matched **18,816** reference bytes; that historical comparison does not qualify the changed task/initialization contract. A separate actual native-camera consumer uploaded GAME-computed FOV30/60 matrices and observed the expected smaller raster footprint at wider FOV. These are authored workloads, not a textured GAME model, independent frontend oracle, photo scoring or W06 completion. No full ROM memory is dumped; see [ADMISSION](ADMISSION.md).
