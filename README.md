<p align="center">
  <img src="assets/35mmisle-logo.svg" alt="A professional island photo-safari emblem featuring sculpted gold and ivory lettering, an optical camera-lens horizon framing a volcanic caldera and waterfalls, a winding 35mm film strip with exposed frames, parted jungle grass under a viewfinder, a half-buried spiral fossil stone, and a coastal cliff profile watching the bay." width="1000">
</p>

# 35mmIsle 📸

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

Debug presets are `linux-debug` and `windows-debug`. CTest executables exercise independently authored native memory, ROM identity, GTL clock/context policy, stackful processes, owned queues, VPK0, compiled-C continuations, typed sprites, packed-depth photo reductions, audio resources/readers/players/synthesis/effects, controller publication and checked save codecs/persistence/startup state; checks remain active in Release. The private game engine adds actual upstream C dispatcher, fade and camera/model computations with synthetic data. Public jobs never receive a ROM or extracted game content.
The optional RSP composition adds coprocessor and required-work lifetime regressions. These use original synthetic instruction programs and caller-owned memory, not a public microcode dump. The [published full Windows MSVC host/RDP run](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38834976035) passed the twenty-four CPU tests then present and five native CLI helps on Windows Server 2022; this is not Windows11/device execution or qualification of subsequent changes.

Current implemented surfaces are **not a playable Pokémon Snap executable**:

- `35mmisle-rom` / `isle-rom`: read-only exact-US image validation in z64, v64 or n64 byte order, without writing normalized ROM data.
- `isle-native`: checked C11 arithmetic, caller-backed arenas and bounded spans. `35mmisle/rng.h` adds explicit replayable seed state, the game's full-high16 LCG and guest-tick random projections, with defined signed-wrap/range arithmetic. Guest tick values are supplied by the caller; this does not substitute a host wall clock or qualify cross-domain timing.
- `isle-host-clock` (private host integration): bounded GAME-derived retrace/update/draw policy, exact rational time accumulation, owner/epoch/serial-qualified context leases and checked pause/reset/stop transitions. It preserves a pending blocking receive across interval changes and closes admission immediately on execution failure. Actual C game ownership and optional presentation remain separate; this is not full scheduler, input, AI or scene qualification.
- `isle-process` (`35mmisle/process.hpp`): an original stack-preserving `isle::Process`, implemented with a parked, joinable native worker and synchronous creator-thread handoff. Only the creator may resume, inspect, cancel or destroy it. `Yield::wait(0)` stays inline; `wait(N)` resumes the same stack after exactly N subsequent `resume()` calls. Cancellation unwinds and joins at cooperative boundaries, not during non-yielding callback execution. Successful waits/resumes do not allocate or copy callbacks. Native FP state is isolated across waits; that is not R4300 FP or Snap scheduling equivalence. Callers must honor the header's exception/unwind and lifetime rules and compile callback arithmetic with dynamic-FP-aware options.
- `isle::MessageQueue<T>` (`35mmisle/message_queue.hpp`): a bounded, construction-allocated, move-only ownership ring with blocking and nonblocking push/pop. `accepted` means admitted ownership, not completed work; failed pushes retain caller ownership. `close()` rejects new messages, wakes waiters and permits FIFO drain. Payload move/destruction must be nonthrowing, nonblocking and non-reentrant; prevent new access and join users before destroying the queue. This is not an N64 priority scheduler.
- `isle-vpk0` (`35mmisle/vpk0.hpp`): the original allocation-free decoder contract described below.
- `isle-continuation` (`35mmisle/continuation.h`): a C ABI over genuine stackful processes. Callback-created children rendezvous with the central owner; cross-cancellation joins before user storage can be recycled. The C trampoline fences control transfer: no C++ exception crosses a C frame. Intervening callback frames must not retain owning C++ automatic objects across wait, finish or failure; C resources require explicit object/scene ownership.
- `isle-sprite` (`35mmisle/sprite.h`, `35mmisle/sprite.hpp`): checked big-endian Sprite/Bitmap graph hydration with immutable bank ownership, canonical RGBA/CI/IA/I texel normalization and odd-row texture shuffle. Positive `actualHeight` determines stored planes; zero uses nominal height. Bare C views borrow their owning `Sprite`; renderer completion must precede owner release. This is not the original RDP DMA footprint or full Sprite renderer.
- Internal `isle-pidgey-resources`: immutable exact-ROM HD photo-Pidgey hierarchy, material/texture, vertex/display-list and pose-0 stream views. Installation writes 42 checked guest-addressed spans into caller-owned canonical memory, preserving unrelated bytes. Animation/render work belongs to the native producer below.
- Internal `isle-beach-world-resources`: checked immutable Beach world/block metadata and the complete first static terrain/sky display-list, vertex, texture and palette closure. Shared `rom::GuestView` preserves explicit guest/ROM/native address domains. Other blocks' geometry and dynamic material/road/spawn execution remain separate.
- Internal `isle-model-assets` / `isle-beach-actor-resources`: one constructor-only graphics closure shared with World; complete immutable Beach actor data for 17 models, 79 animation headers, 33 materials and 1,317 programs. The actual-ROM consumer compiles the full pool once and evaluates every header through the existing animator, including source zero-speed headers and texture track-time opcode12. Branch/CULL state, retained textures, NULL bindings and atomic installation are exercised. Native actor factories, behavior, sound/FX and combined-scene rendering remain separate.
- `isle-audio-bank` (`35mmisle/audio_bank.hpp`, `35mmisle/audio_sequence.hpp`): immutable, alias-preserving BE B1 sample graphs and S1 sequence catalogs, with retained input owners and checked offsets/extents. Signed envelope/detune/coefficient fields and partial ADPCM loops remain intact. S1 preserves declared payload length separately from even-DMA padding, including bounded aliases and gaps; all 37 original entries have been exercised privately. This is resource hydration, not compressed-event interpretation, sequencing, synthesis or an audio-ready signal.
- `isle-audio-sequence` (`35mmisle/audio_cseq.hpp`): allocation-free compressed-MIDI event decoding over retained S1 views, with value-copyable cursors, independent loop counters and atomic failure. All 112,733 qualified original public events across 37 actual sequences match: eleven terminal streams and twenty-six bounded 4,096-event prefixes. This is event decoding, not a CSP/MIDI voice player, ADSR, effects or BGM PCM qualification.
- `isle-audio` (`35mmisle/audio_synth.hpp`, `35mmisle/audio_sfx.hpp`, `35mmisle/audio_csp.hpp`, `35mmisle/audio_effects.hpp`): retained catalog, shared ABI1 synthesis, independent SoundPlayer and compact-MIDI sequence clients. One service owns up to 22 physical filters, 42 logical descriptors, three typed client calendars, a shared 44-slot modulation pool, stable RDRAM and a graph-wide task/PCM lease. Original ROM microcode performs decoding/resampling/mixing; no SDK producer body is imported. Finite SoundPlayer lifecycles, CSP scheduling/controller semantics, FX sends and static wet graphs have bounded actual-RSP qualification. A chorus candidate adds checked modulated taps and persistent resampling state; original-producer arithmetic and capture-admission counterexamples remain open, including an admitted `depth=50000` case. Complete BGM PCM, original AI scheduling and the short-envelope packed-rate counterexample remain open.
- Opt-in `isle-engine` (`35mmisle/engine.h`): native arena-backed GObj/process/SObj/camera/model ownership around thirty-seven reviewed GAME definitions. Typed source processes retain authoritative ordering, waits, function identity and joined retirement; native update context remains separate from GAME actor payload. Cameras provide stable caches, pure metadata preflight and atomic dirty matrix/reflection/normalization publication. Translate-only/kind-28 local matrices remain independent of RT/RTS placement. Explicit contextual RPY nodes retain full finite poses without pretending their reserved local fixed bytes are renderable; checked source54 fixed-W and source48 camera-basis math supplies renderer-owned payloads. Native C-libm seams do not establish original trig-bit parity.
- Internal actor controls (`actor_source.h`): six exact pinned GAME C helpers use the existing native process scheduler, one actual GObj payload and the application RNG. State/path/auxiliary ordering, real waits, source function identity and joined retirement are retained. Native binding validation rejects unrelated payloads before dereference. A separate API consumer exercised 61 native ticks and 59 source path waits; full actor factories/FSMs remain separate.
- Internal `isle-source-angles` / `isle-environment-sound`: exact-ROM signed-angle lookup and Beach's 53 sound descriptors over the real shared audio session. Source category replacement, hearing cutoff, camera-relative attenuation/pan, pitch and fixed-voice retirement are implemented without SDK bodies. An actual RSP consumer exercised category isolation/order, camera reversal and two parked native joins while retaining the detached voice: 57 tasks and 30,496 checked stereo frames. This is owned PCM proof, not physical hearing or actor-event/gameplay integration.
- Internal `isle-menu-animation`: one compiled evaluator with preallocated instance slots and explicit pool/slot/incarnation identity. Source opcode14 follows validated in-stream targets and resets clocks without restarting tracks, rates or receiver selection. Finite graphs use resource-certified bounds; cyclic owners specify fixed command/event capacities, with allocation-free preflight and atomic rejection of overflow or binary32 stagnation. Actual ROM looping wire exercised three cycles, ordered callbacks and retained-event failure atomicity. Actor header/force/pause, sound/FX dispatch and gameplay coordination remain separate work.
- Internal `isle-menu-model-producer`: one borrowed kernel shared by Title, World and actor models, with source28/54 and translate+44/48 recipes, inherited XYZ and renderer-owned contextual residency. Actual RSP/RDP preserves five Title models/292 nodes and eight primary/four secondary World blocks. Pure camera preflight and complete native-storage disjoint validation precede publication. Source NULL draws skip material/call emission while preserving valid I pre-only work. Actual actor qualification is described below; no complete gameplay renderer or product executable is implied.
- Internal `isle-actor-producer`: actual Beach camera3/14, callback-specific fog, source flag100, native visibility and link/tag policy over the same borrowed kernel. A six-slot actual-ROM GPU consumer exercised Snorlax54, two Pikachu48 instances, retained-texture1006, geometryless1005 and common1003, including local/placement/camera changes, atomic rejection, residency, real retirement/respawn and resource rebinding. Source distance LOD, scales and poses remain intact. Snorlax/Pikachu suppression changes visible RGB5 and depth with exact restoration. Detector-disabled geometry only; combined World/actor RAM, native factories and gameplay remain separate.
- `isle-focus-detector` (`35mmisle/focus_detector.hpp`): allocation-free two-context live-color history, full-16-bit 8×8 comparison, source 32-pixel threshold, ordered candidate/photo membership, cleanup and explicit deferred publication. Source conditions mark exactly 21 red center pixels without disturbing neighboring halfwords or hidden RDP memory. CPU contracts and a real native-identity API consumer pass; its color inputs are authored, not enabled-detector GPU capture or complete photo reconstruction.
- Internal `isle-pidgey-producer`: independently owned native pose-0 instances, photo-root transformation, source-faithful traversal and genuine dynamic material emission over shared immutable HD resources. Both score-style and normally lit preview paths execute actual textured/depth-writing RSP/RDP triangles; `--model-preview` presents new completed model frames through real LUS.
- Internal `isle-graphics-task`: shared checked canonical-memory, cold GAME image/BSS/boot installation and graphics-task construction for both genuine producers; production does not depend on test fixtures.
- Internal `isle-beach-world-producer`: native source-relative terrain transforms, camera-eye-following one-cycle sky, fog and integer-pixel scissor semantics over original immutable geometry. Twenty-one actual RSP/RDP tasks exercised isolated layers, camera/model/fog changes, clipped columns, exact restoration, transactional native-state/RAM rejection and complete retirement. This is not the full Beach scene or gameplay.
- Internal `isle-photo-lighting`: exact-ROM ambient/directional lights and normal photo-preview setup, with GAME-derived camera light direction. Three checked installation spans total 200 bytes; subsequent direction updates touch only three bytes. Actual isolated light changes altered color without changing depth.
- `isle-photo-feedback` (`35mmisle/photo_feedback.hpp`): allocation-free native wide/narrow reductions over completed 26-plane canonical memory. Preserves exact packed-Z/dZ equality, per-subject before-depth occlusion, inclusive center windows and integer area estimation.
- `isle-photo-score` (`35mmisle/photo_score.hpp`): allocation-free Oak CPU evaluation over directly borrowed completed feedback and immutable original ROM rule tables. Preserves subject priorities, sparse render/photo mappings, signs/eggs, pose/special/centering/companion bonuses and source rounding. Sixteen real RSP/RDP tasks fed actual scores; full photo reconstruction and independent Original math/score parity remain unqualified.
- Internal `isle-pidgey-feedback`: one genuine source-ordered GPU task for each 60°/30° camera pass, up to twelve independently posed native subjects, original two-half depth-copy list and completed-memory CPU reductions. Overlap, coincident occlusion, draw-order reversal, edge copies and twelve→one→zero reuse were exercised. Full world/items/effects/photo reconstruction and independent Original parity remain separate.
- `isle-save-image` (`35mmisle/save_image.hpp`): allocation-free inspection/sealing of the exact original save image and a versioned, baseline-bound native envelope. Independent legacy-hash math matched 86 actual GAME returns; native sealing reproduced a complete actual GAME image byte-for-byte. Structural validity does not establish photo/scene replay safety.
- `isle-save-store` (`35mmisle/save_store.hpp`): synchronous, checked two-generation local persistence with a stable session lock, immutable loaded snapshots and explicit publication/durability outcomes. Corrupt, future, conflicting or orphaned state requires non-destructive recovery; it never silently initializes a game. Linux cold reload, lock termination and rejected-write preservation were exercised. Windows implementation is not Windows 11 runtime or power-loss qualification.
- `isle-game-save` (`35mmisle/game_save.hpp`): checked GAME startup defaults and settings/progress/photo-field access over canonical save images. Only genuinely missing profiles initialize; corrupt, future and conflicting candidates remain preserved. Immutable owning snapshots survive mutation, session moves and destruction. A separate actual Linux API consumer committed two generations and cold-reloaded the second.
- `isle-game-input` (`35mmisle/game_input.hpp`): source-derived four-port read accumulation, publication acknowledgments, repeat/menu state and fixed-capacity SPSC acquisition transfer. Disconnect/errors retain GAME pad state; stale completions cannot cross service generations. Actual explicit sample-transfer/menu/reentry behavior is exercised; physical input and integrated host acquisition remain unqualified.
- Internal `isle-game-state`: one application-lifetime profile result/lock, deferred first open, selected controller, RNG, separate byte title flags and 32-bit scene notices. Scene retirement never reopens or replaces the profile.
- Internal `isle-scene`: stable caller-owned `Storage`, resource-independent `Services`, native command dispatch and generation-bearing render bindings. Composed pre-payload/joined observers retain command storage until actual caller joins; a native-address-reuse regression failed before the non-retired dispatch-owner correction and passes afterward. These joins do not certify GPU/effect completion.
- Internal `isle-scene-graphics`: typed startup and generic scene passes use the same canonical compositor. Whole immutable/mutable/task/framebuffer admission precedes writes; native views end before GPU-only execution, while raster owners remain pinned through real completion/readback. Optional LUS presentation owns copied completed pixels. Source-required sprite minification uses GPU sample rebasing without scale clamping or invalid derivative encodings; palette loads populate all four TMEM banks.
- Internal `isle-window`, `isle-new-game-scene` and `isle-oaks-lab-scene`: native font/raster/furniture, seven-character name entry and lab control/save/course processes over shared resources and owners. Visible sprite width is separate from padded CPU raster stride. The bounded actual Name→Oak flow below is exercised; full gameplay is separate.
- Internal `isle-world`: complete Beach primary/secondary block resources, native terrain/road ownership, source path/material clocks, movement, height/collision queries, transitions and GPU producer. A complete eight-primary/four-secondary native consumer exercised actual ROM RSP/RDP, visibility restoration and joined retirement. Its camera is diagnostic; player/Pokémon/course assembly remains distinct.

A frozen private Linux scenario exercised the real logo, 3D title, existing-profile menu/reset, and title-only reentry through canonical RSP/RDP and LUS. It preserved live screen calibration during RAM reset, retained the original durable profile on cold reload, reused the same State/Storage after joined retirement, and rejected a second boot without destroying reentered content. Four actual window surfaces were inspected. Input was explicitly software-owned and audio used checked SDL dummy output: no physical-controller, hearing, independent Original numerical, or full-game qualification is implied.

A separate frozen Linux run executed real title/menu→name entry→Oak dialogue and route confirmation after correcting visible sprite width versus padded raster stride: 925 canonical RSP/RDP frames and 925 actual LUS presentations. Name/Oak captures show properly separated text and contiguous dialogue backgrounds; logo/title/menu captures are byte-identical to previously inspected surfaces. It confirmed the source course-0 request—not Beach gameplay. Checked SDL-dummy output accepted 1,266,416 PCM frames, including 142,880 during real graphics waits; input remained explicitly software-owned.

The independent native GPU sprite consumer also exercised signed/anisotropic minification, quarter-pixel sampling, clipped and empty geometry, texture borders/filtering, CI palette banks, mutable raster revisions, CUTOUT and blending. RGB5 samples and actual framebuffer/hidden coverage are checked separately: framebuffer bit 0 is coverage, not texture alpha. This proof covers the native-resolution non-AA renderer state, not independent Original equivalence or Windows runtime.

GAME audio endpoint inspection runs synchronously on the actual SDL owner through `isle_continuation_on_owner`; it continues the same continuation resume without a GAME tick, wait or cached readiness substitution. The checked SDL-dummy startup/reset/reentry scenario exercised these controls while audio also progressed during real graphics waits. This is owner-thread/control evidence, not physical output or hearing qualification.

Link `isle-process` through CMake to inherit `Threads::Threads`; the header-only queue also requires native thread linkage. Existing process targets retain strict FP options (`/fp:strict` on MSVC; `-fno-fast-math`, `-ffp-contract=off` and `-frounding-math` on GCC/Clang).

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

Omit `ISLE_SNAP_SOURCE_DIR` to fetch the immutable authoritative commit privately. A supplied repository may have a different HEAD or dirty worktree: preparation reads exact Git objects, never working files or an upstream build. [`snap-engine-inputs.json`](cmake/snap-engine-inputs.json) identifies seven immutable GAME source/header blobs, thirty-seven reviewed definitions, source/native hashes and narrowly versioned seam transformations. [`prepare_engine.py`](tools/prepare_engine.py) emits C/provenance only inside the build directory and verifies original native support hashes. No SDK implementation/header is compiled, and complete GAME bodies are not committed or packaged. This is private development admission, not blanket MIT or redistribution permission.

Camera APIs own type-3 payloads and up to two matrices. `isle_game_camera_peek_matrix` borrows immutable kind without computing dirty caches; `isle_game_camera_compute_all` publishes matrix/reflection/normalization only after every calculation succeeds. Kind8 uses fixed Y-up and dirties on eye/target/roll, not up-only edits. Model APIs retain translate-only/kind-28 local caches or explicit renderer-contextual poses, independently of model-level placement. Pure preflight precedes mutation; native retirement joins actual work. Fixed words upload directly into word-swapped RDRAM only where a valid local or renderer-owned payload exists. Full scene/photo, actor assets and Windows 11 runtime qualification remain separate.


## Opt-in real libultraship host probe (nonplayable)

`35mmisle-host-probe` links real LUS at `759b13401cd6903daa67a00a753066f791622e78`. Its default SDL3/OpenGL mode submits original procedural colored rectangles through the real renderer and tears down host owners before exit. With the private engine enabled, `--rom` exercises the actual intro sprite and selected C fade. With the audio option enabled, `--rom ... --audio` adds actual native dry SFX14/RSP synthesis and checked SDL output. These are independent bounded experiments, not full scene/game boot, original cue synchronization or graphics/photo fidelity.

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

The script creates only its own checkout, refuses unowned/dirty/wrong-pin inputs, and verifies the vcpkg executable's size/SHA-256 before execution. CMake repeats verification before `project()` loads the toolchain. No floating bootstrap, overlays, automatic LUS vcpkg or binary cache substitution. The preset enables host, private engine, audio, RSP and canonical RDP; public CI needs no ROM. Compiler/CLI evidence and Windows 11/device runtime qualification remain separate; see ADMISSION.

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



[ADMISSION](docs/development/ADMISSION.md) records original-source boundaries, private source candidates and exercised evidence. [Dependency evidence](cmake/dependencies.json), [Libultraship.cmake](cmake/Libultraship.cmake) and [SnapEngine.cmake](cmake/SnapEngine.cmake) distinguish selected inputs from complete shipped-composition approval. Configured builds write `isle-host-dependencies.txt` and, with the engine enabled, `isle-snap-engine-inputs.json`. CPack still installs/packages **only the ROM checker and its original MIT license**, not host/engine binaries, source bundles or a game.


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

This requires a real Vulkan loader/ICD, Vulkan 1.1 and enabled 8-/16-bit storage-buffer access. `--cpu-device` selects an enumerated Vulkan CPU device, not a null backend. The build acquires ten exact archive-digest-pinned renderer/tool inputs and builds its own pinned Granite `slangmosh` compiler; no installed shader-tool assumption. [ParallelRdp.cmake](cmake/ParallelRdp.cmake) compiles the standalone renderer subset, **not** `rdp-utils`, `alp-core`, Angrylion or MAME. Build-private provenance and complete upstream notices remain with the inputs. The probe is not installed/packaged.

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

Actual CPU Vulkan completed fractional/partial-matrix and graphics→line→graphics tasks on one shared allocation. CPU preparation, actual RSP writes and subsequent RDP reads use the same word-swapped RDRAM; actual device completion precedes readback. Cold initialization occurs once, while each authored task explicitly clears its target through RDP commands. The earlier separate-memory after-SP replay matched **18,816** reference bytes; that historical comparison does not qualify the changed task/initialization contract. A separate actual native-camera consumer uploaded GAME-computed FOV30/60 matrices and observed the expected smaller raster footprint at wider FOV. These are authored workloads, not a textured GAME model, independent frontend oracle, photo scoring or W06 completion. No full ROM memory is dumped; see [ADMISSION](docs/development/ADMISSION.md).

## License and contributions

[LICENSE](LICENSE) is MIT for **original 35mmIsle contributions**, copyright 2026 35mmIsle contributors. It does not relicense Pokémon Snap, Nintendo/SGI material, decompilations, reference ports, dependencies or extracted payloads. Their rights and notices remain with their respective holders; missing or restricted grants block the affected import/release. Read LICENSING before proposing any imported source or generated content.

Development work is tracked in Beads. Architecture, porting decisions and validation requirements are documented under `docs/planning/`.
