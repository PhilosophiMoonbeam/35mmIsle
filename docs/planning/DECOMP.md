# Authoritative Pokémon Snap Decomp Audit

## Scope

Research baseline and native-cutover contracts, not implementation or a portability inventory. Covers build authority, executable/data layout, process scheduling, input, ROM assets, rendering, photographs, persistence, audio and timing. **Observed** statements below come from the pinned upstream source; **Proposed** statements are 35mmIsle choices requiring the stated experiments. `[INFERENCE]` marks deductions not established by execution. No ROM extraction, rebuild, gameplay run or parity gate was performed.

## Evidence baseline

| Item | Authority and limitation |
|---|---|
| Upstream | `ethteck/pokemonsnap` commit `df8815e75f3d79618466e01e0827c6165b93d11c`; read-only snapshot `/tmp/35mmisle-planning-xaf_qkz0/snap`. `git ls-files \| wc -l` observed **1,398 tracked paths**. This is repository metadata, not completion percentage. |
| ROM | The supported input is the **US / North America** image named `pokemonsnap.z64`, SHA-1 **`edc7c49cc568c045fe48be0d18011c30f393cbaf`**. README, Splat and decomp metadata agree [B1][B2][B3]. The exact hash, not an inferred “Rev 0” label or a filename alone, is revision authority. No other region/revision support is established. |
| Reconstruction | Upstream calls itself WIP [B1]. Source exists for the engine and surveyed gameplay systems, but assembly entry/SDK sections and extracted RSP binaries remain [B2][B5][G1]. A matching-ROM build would establish reconstruction, not complete semantic recovery, host portability or rights clearance. No fresh matched-code/function percentage or residual-function-ASM total is claimed. |
| Local fork | `/home/bbferko/repos/pokemonsnap`, branch `hermes/final-stretch`, HEAD `b35d6de06edd43aa842fdd3d03175967dfecfbea`, has **102 tracked dirty entries** in the shared baseline record. Its generated/untracked inventory and graph are not upstream evidence. Preserve it unchanged. Reconciliation is an explicit gate: compare each intended fork delta against this pin and classify retain/rederive/reject with ownership and behavioral evidence before importing anything. |
| Rights | MIT authorization applies to original 35mmIsle work only. Upstream SDK header explicitly carries SGI proprietary/no-copy restrictions and Nintendo copyright [L1]; baseline metadata reports no repository-wide upstream license. Referencing source here is not permission to distribute code, SDK, extracted assets or microcode. Provenance/right-specific review blocks import/release. |

### Build/setup findings

Observed upstream recipe: `uv sync`; supply the exact ROM; `uv run configure.py --setup`; `uv run configure.py` to extract/disassemble; `ninja` to reconstruct [B1]. Python is `>=3.11`; dependencies include `splat64[mips]>=0.50.0`, `spimdisasm>=1.42.4` and `mapfile-parser>=2.13.0,<3.0` [B4]. Configuration downloads IDO static recompilers **5.3 and 7.1** plus asm-processor; it selects `mips-linux-gnu-` or `mips-elf-` binutils unless `CROSS` overrides [B6]. These are ROM-reconstruction tools, not the proposed native build.

Compilation assumes big-endian VR4300/O32 assembly, MIPS2 game compilation, IDO behavior, `BUILD_VERSION=VERSION_I`, `F3DEX_GBI_2`, `_FINALROM` and `NDEBUG`; source processing emits EUC-JP [B6]. Splat generates `pokemonsnap.ld` and `build/include/ld_addrs.h`; linker consumes `undefined_syms.txt` and generated undefined symbols. ELF is converted to a binary ROM and the Ninja success target checks `checksum.sha1` [B2][B7][B8]. Thus current `ROM_START`, `VRAM`, section-end symbols and data extraction are an address-based link contract, not a native resource API. Host support must not depend on running this setup against the dirty fork.

**Source versus remaining machine material:** the reviewed engine/game paths are C, but Splat still declares hand assembly for `entrypoint` and SDK `setintmask`, `exceptasm`, `bzero`, `writebackdcache`, `parameters`, `setsr` and `getsr` [B2][B9]. RSP boot/audio/triangle/line programs remain `textbin` and corresponding extracted data rather than recovered instruction semantics [G7][G8]. Generated data/ASM are reconstruction inputs, not evidence of a native C implementation. Remaining function coverage requires a pinned build/map/progress report; this audit makes no decomp-complete assertion.

## Findings

### Source map and boundary contracts

Evidence references below link to **full-commit-pinned exact spans**, including concrete entry points for architecture/roadmap work.

| Subsystem | Exact source entry points / observed behavior | Native boundary |
|---|---|---|
| Boot and service startup | `entrypoint` clears main BSS and jumps to `game_main` [B5]. `game_main → thread1_idle → thread5_main` initializes OS/VI/PI, ROM DMA, scheduler thread 3 (priority 120), audio thread 4 (110), controller thread 6 (115), waiting for each startup acknowledgment; thread 5 priority is 50. It initializes GTL, loads `app_render`, enters `start_scene_manager` [S1]. | Replace platform startup and MMIO, not scene/game initialization order. Distinguish these long-lived services from game-object processes. |
| Scheduler | `scExecuteBlocking`, `scAddClient` use completion/retrace queues [S2]; `scExecuteTasks` selects priorities and resumes paused graphics; `scHandleVRetrace` notifies clients [S3]. | Preserve visible ordering, buffer ownership and completion barriers. Hardware RSP/RDP preemption need not become host OS preemption. A no-op queue/cache layer is not a design. |
| Frame loop | `gtlMain` waits `gtlUpdateInterval` ticks, drains stale ticks, updates, increments `gtlFrameCounter`, conditionally draws, and waits for graphics before exit. `gtlDrawOne` renders and waits for its context completion before returning [S4]. Beach calls `gtlSetIntervals(1, 2)` [T1]. | Separate simulation cadence from display presentation. Offscreen photograph drawing must not accidentally execute a gameplay update; its completion must make CPU buffers readable. |
| Objects/processes | `omCreateProcess` supports kind 0 stackful threads and kind 1 callbacks; `omCreateProcessThreaded` chooses stack/thread ID; `omEndProcess` defers self-termination [P1]. `omUpdateAll` visits 32 object links first, then process priorities **11 down to 0**, skipping paused processes; `omRunProcess` starts a kind-0 thread and waits for its yield acknowledgment [P2]. | Ordered cooperative execution with preserved stacks, current-object/current-process globals, pause and deferred deletion. Do not map these to freely concurrent jobs or restart callbacks each frame. |
| Yield | `ohWait(n)` sends the process acknowledgment, stops the current OS thread, and resumes/decrements once per scheduled frame [P3]. | Proposed stackful cooperative contexts retain local variables and nested call stacks. Callback kind 1 remains synchronous. Platform service threads and cooperative contexts require separate contracts. |
| Scene/file/overlay model | `start_scene_manager`, `func_8009A8F0`, `func_8009AE0C` explicitly load world/app-level/course code and assets, and distinct camera-check/Oak/album/report/photo-check/gallery overlays [O1][O2]. `Overlay` stores **nine u32 addresses**, size `0x24`; `OVERLAY(name)` casts linker symbols [O3]. `dmaLoadOverlay` copies bytes to fixed VRAM and zeroes BSS [D1]. | Proposed native static code linkage plus explicit scene-state initialization/destruction and typed asset residency; do not DMA executable bytes into host memory or clear arbitrary native structs by N64 address range. |
| Controller | `contReadData` computes pressed/released/held events and accumulates edges; `contUpdateGlobals` consumes them and signed stick values [I1]. Default hold delay 30/interval 5, blocking event requests and retrace-driven `contMain` remain distinct from game consumption [I2][I3]. | Feed N64 logical inputs at simulation boundaries; preserve accumulation across draw skips, menu repeat, disconnect and device-availability behavior. Keyboard/gamepad mapping is upstream of this contract. |
| Printer/peripherals | `contIsPrinterAvailable` checks controller slot 4; printer status/commands block on their queue [I4]. Save sends `0xCC` before flash writing and `0x33` afterward, waiting while printer status is 8; gallery explicitly branches when printer is absent [V3][I5]. | Proposed initial native behavior is explicit printer-unavailable routing, not fabricated successful hardware commands. Physical-printer/export support remains a separately specified scope decision. Rumble/Pak adapters must report actual capabilities. |
| ROM/DMA/compression | `dmaCopy` chunks transfers at `0x10000`, waits completion and applies cache operations; `dmaDecodeVPK0` reads u16 bitstreams/Huffman structures/backreferences; `dmaReadVPK0ToBuffer` streams from ROM [D1][D2]. `N64SegVpk0.decompress` checks consumed compressed extent against Splat boundaries [D3]. | Distinguish ROM offsets, host addresses, decompressed bytes and typed resource objects. Decode source byte order explicitly; no host-u16 reinterpretation of big-endian bytes. Account for extent/alignment and asset relocation. |
| Graphics/task ABI | GTL table actually contains `gspF3DEX2_NoN_fifo` and `gspL3DEX2_fifo`; task selection, no-near-clipping substitution and in-list ucode reload are explicit [G2][G3]. Source payload filenames are **`gspF3DEX2H.NoN.fifo`** and **`gspL3DEX2H.fifo`**, extracted text/data binaries, not decoded RSP source [G1][G7][G8]. | Preserve task and `gSPLoadUcode` identity, triangle/line differences, segment rebasing and resets. Do not infer stock F3DEX2 compatibility from symbol spelling or compile define. |
| Custom graphics operations | `gSPMvpRecalc` emits `G_SPECIAL_1` with `gImmp21(..., 0, 1, 0)` [G6]; matrix types 41/42 follow with `G_MW_MATRIX` writes to packed integer/fractional MVP locations [G4]. Header defines `G_SPECIAL_1 = 0xD5` [G5]. | Interpreter must establish these exact semantics, including interaction with matrix state. Treating opcode `0xD5` as an ignorable unknown changes geometry and therefore photographs. |
| Live focus/shutter | `PokemonDetector_InitDetector` alternates snapshot contexts before rendering. `PokemonDetector_SaveRegion` copies center **8×8 RGBA16** after individual Pokémon draws; `PokemonDetector_FindPokemonInFocus` compares adjacent saved regions and accepts **≥32 changed pixels**, later qualifying object winning. Postprocessing matches queued render context to focus [F1][F2]. | This is color-content/draw-order detection, not raycast or an object-ID pick. Native rendering needs intermediate per-object color copies and correct render/snapshot/analysis/shutter latency. |
| Photograph capture/replay | `func_8009C8E4`, `makePhoto` capture level/block/time/camera and Pokémon/items/effects; maximum 60 course photographs and focus reordering/flag bits [F3]. `initObjectsOnPhoto` reconstructs world, creatures, items, effects and camera. `func_8009D65C` restores material/sky time, `func_8009D8A8` camera, `func_8009DEF0` model/index mappings [F4][F5][F10]. | A photograph is a scene snapshot, **not a stored screenshot**. All asset banks, animations and effects needed to replay past course photos must remain reconstructible across scene changes and save/reload. |
| Scoring | `score_Render`, `func_800A007C`, `func_800A0EA4`, `score_CalculateScore`, `score_AddSamePkmnBonus` perform offscreen reconstruction and visibility/size/pose/special/framing/group scoring [F6][F7][F8]. | Exact render-dependent counts, threshold order, integer rounding and subject selection are game behavior. A geometrically plausible photo image is insufficient. |
| Persistence | `UnkBigBoy` declares digest, offsets/progress/settings/name, 69 species photos/scores, four additional photos, 60 album snapshots/comments; declared size **`0x1F2A4`** [V1]. Flash wrapper reads 128-byte pages and writes/verifies regions of a ≤`0x20000` image [V2]. `SaveFile_VerifyMD4Hash` returns **true on mismatch**, despite its name; version validation, timestamp/hash/write and initialization are explicit [V3][V4]. | Encode/decode a canonical N64-layout image independently of native compiler structs; preserve invalid-save/reinitialization and success/failure polarity. Native durable storage replaces flash transport, not the photo/progress model. |
| Album/report/gallery | `func_800BF690_5C530`/`func_800BF710_5C5B0` store/retrieve per-species snapshots; `func_800BF7D4_5C674`/`func_800BF818_5C6B8` scores; album capacity 60, comments and delete sentinel [V5][V8]. Photo-check updates selected report photos/scores, additional slots and album [V6]. Gallery consumes additional/report/album snapshots and writes back/save [V7]. | End-to-end selection, replacement, reorder/delete/comment, comparison and persistence must share the same snapshot semantics; image caches cannot replace canonical photo data. |
| Audio | `auCreatePlayers` uses libaudio synthesis with DMA callback and custom FX, aligns samples around output rate/60; `auThreadMain` calls `alAudioFrame`, builds `M_AUDTASK` with `aspMain`, waits ticks/completion, submits audio and maintains SFX state [A1][A2]. | A PCM playback sink alone cannot interpret `Acmd` or synthesize banks/sequences. Preserve sequence/SFX lifecycle, pitch/pan/volume/priority/reverb, sample DMA relocation and producer cadence; replace the synthesis/task boundary explicitly. |
| Time/RNG/demo | `rand16`/`randFloat` use shared LCG `seed * 0x343FD + 0x269EC3`; `randT*` use low time bits [T2]. `IdleScript` contains seed/flags/controller samples; `initIdle` seeds and `updateIdle` injects each input then `ohWait(1)` [T3]. World time is `GlobalTimer`, advanced by skybox update and used for replay [T4][T6][F4]. | Preserve 32-bit wrap, RNG call order, float/rounding and separate simulation, audio and platform clocks. Demo replay is a useful deterministic fixture, not proof that all time-based randomness is deterministic. |
| Hardware integrity | Startup reads SP IMEM/DMEM into `gSPImemOkay`/`gSPDmemOkay`; `volcano_CheckIllegalCopy` sets `PFID_ILLEGAL_COPY` if either is false [S1][T5]. | Host absence of N64 registers must not trigger unintended progression behavior. Proposed validated-input/platform identity policy needs explicit review; neither literal MMIO nor silently deleting the branch is acceptable. |

### Address, overlay and data-format risks

* **Observed overlapping identity:** camera-check, photo-check, album, report and gallery each link at `0x801DC8C0` and use `exclusive_ram_id: level`; main starts `0x80000400`, app-render `0x8009A8C0`, more-funcs `0x800BF080`, world `0x800E18A0`, app-level `0x80350200` [B2][O4][O5][O6]. `[INFERENCE]` An N64 address alone cannot uniquely identify a resource across those overlays. Native keys must include overlay/asset identity and lifetime, not just VRAM.
* **Observed size/ABI coupling:** `Overlay` address fields are u32, DMA implementation carries RAM address as u32 and casts pointers through u32, while its header declares `uintptr_t` for `dmaCopy` [O3][D1]. Native widening must migrate declarations **and callers**, separate address domains and retain encoded 32-bit addresses where they are file/command data; global search-and-replace `u32 → uintptr_t` corrupts formats.
* **Observed allocator trap:** `mlHeapAlloc` declares `u32 offset` and aligns with `((uintptr_t) heap->ptr + offset) & ~offset` [M1]. The complement is computed at 32-bit width before promotion; on a 64-bit `uintptr_t`, its zero-extended mask clears the upper pointer bits even though the pointer operand was widened. Proposed repair: convert the alignment mask to pointer width **before** complementing. The ABI gate must exercise real high-address allocations, not just search for casts.
* **Observed unsafe host-layout assumptions:** `PokemonPhotoData` mixes signed/unsigned bitfields; `PhotoData` starts with bitfields, contains an integer/float union and has declared offsets/size; live `UnkThing` adds host pointers after serialized data [F9]. `UnkBigBoy` has further bitfields and offset fields [V1][V4]. `[INFERENCE]` host bitfield order, padding, endianness and pointer size can break snapshots, MD4 and asset interpretation. Use explicit wire-field packing and independently validated runtime types, never `fwrite(native_struct)`.
* **Observed lifecycle coupling:** overlay loading zeros BSS, scene heaps are sized by linker-address subtraction, current-process self-delete is deferred and picture replay destroys/respawns objects [D1][T1][P1][P2][F5]. Proposed native linkage must reproduce reset scope and object/resource destruction order without wiping permanently resident shared state.
* **Observed extracted formats:** VPK0, RSP text/data, EUC-JP text and Splat-defined code/data/image/animation segments are different domains, not a homogeneous archive [B2][B6][G1][D3]. Proposed archive extraction must retain resource provenance, version, lengths and relocation information; original ROM validation precedes extraction. No Nintendo payload belongs in a project release merely because it was generated locally.

### Photograph dependencies: blocking, not optional polish

There are **two separate render-to-gameplay paths**:

1. **In-course focus:** frame-context snapshot → draw world/Pokémon → per-object center-color capture → scheduler postprocess → focus publication → shutter snapshot. Comparisons operate on 8×8 u16 color regions, with ≥32 changed pixels and draw-order priority [F1][F2]. A final-frame screenshot loses intermediate changes; picking the nearest creature is a different game.
2. **Oak evaluation / saved-photo replay:** snapshot → reload course resources → rebuild models at recorded animation state, world/material/sky/effect time and camera → two offscreen passes → CPU visibility counts → score/subject selection → report/album/gallery persistence [F3][F4][F5][F6][F7][F8][V5][V6][V7].

`func_800A007C` partitions **`0x27000` bytes** into one 64×48 u16 image, one current-depth buffer, twelve per-Pokémon depth buffers and twelve pre-Pokémon depth buffers. First pass doubles vertical FOV; second uses normal FOV. `score_GfxSetupBuffers` fills current/per-Pokémon depths with **`0xFFFC`**. The scoring renderer copies current depth as texture/color into pre-Pokémon buffers, selects a per-Pokémon depth image, draws, restores current depth and draws again [F6][F7]. CPU comparisons include:

* per-Pokémon depth `!= 0xFFFC`;
* per-Pokémon depth `==` final scene depth **and** pre-Pokémon depth `!=` final scene depth;
* center rectangles of different sizes for wide-pass coverage, focus and unobstructed center;
* coverage-ratio estimates, visible-pixel thresholds and integer score rounding [F7][F8].

**Proposed invariant:** scoring runs in a canonical 64×48 path independent of window size, widescreen, anti-aliasing enhancements and presentation interpolation. CPU-visible packed depth values and equality relationships must match the authority; ordinary floating-point GPU depth or point-depth queries are not proof. Backend choice (GPU-compatible canonical path, software reference or another faithful method) remains open until fixtures demonstrate parity. Screen appearance, ucode support and readback correctness are prerequisites to claiming faithful photographs, not separate optional features.

### Snap-specific differences from a conventional N64 display-list port

| Conventional shortcut | Why it is insufficient here |
|---|---|
| Call the original update function once per presented frame | Course setup uses update/draw intervals 1/2; game processes wait in update frames, offscreen picture draws add draws without updates, input edges accumulate [S4][T1][I1]. |
| Replace OS threads with per-frame callbacks or parallel jobs | Kind-0 scripts suspend nested stacks under an ordered object/process manager; self-delete and global current-process state rely on serial handoff [P1][P2][P3]. |
| Treat all F3DEX2 display lists as stock-compatible | H-suffixed extracted ucode, reloads, line mode, no-near-clipping selection, `G_SPECIAL_1` MVP recomputation and direct matrix-word edits require exact proof [G1][G2][G3][G4][G5][G6][G7][G8]. |
| Offer framebuffer screenshots and depth picking | Live focus requires intermediate 8×8 color tiles; scoring needs two full 64×48 multi-depth-buffer passes with packed equality, sentinel and copy semantics [F1][F2][F6][F7]. |
| Save thumbnails as the photograph | The save contains replayable object/world/camera/effect snapshots; album/report/gallery and evaluation reconstruct them after course transitions [F3][F4][F5][V1][V5][V6][V7]. |
| Hook native audio output and keep audio tasks unchanged | `alAudioFrame` emits RSP audio commands for `aspMain`; the port must provide synthesis/command execution, not only playback [A2]. |
| Flatten overlays into a single address map | Several mutually exclusive modules share VRAM, BSS clearing resets state, and assets/heap extents are linker-based [O4][O5][D1][T1]. |

## Reuse decisions

Decisions describe technical reuse only; legal clearance is separate and precedes import.

| Adopt / adapt / reject | Decision |
|---|---|
| **Adopt as authority** | Pinned upstream symbols, source behavior, ROM hash, course/state-machine ordering, photograph fields and scoring formulas. Adopt the ROM-rebuild workflow as a reference/evidence producer, not native deployment. |
| **Adapt** | Object/process manager to stack-preserving cooperative host contexts; retain callback processes separately. Scheduler/queues to explicit frame/task completion and host-owned services. Overlay loads to native state lifecycle plus typed asset residency. |
| **Adapt** | ROM extraction/VPK0 and resource relocation into owner-supplied-ROM tooling; explicit endian-aware snapshot/save codecs; renderer with verified custom ucode and canonical color/depth readback; libaudio/Acmd producer to verified host synthesis. |
| **Reject** | Wholesale dirty-fork import, generated-file-count completeness claims, blanket pointer/segment identity macros, native struct dumps, executable DMA, freely concurrent game processes, no-op hardware APIs, ignored custom opcodes, nearest-object focus, score substitutes and screenshot-only saves. |
| **Reject as proof** | Stock microcode naming, a successful boot, matching visible screenshots, PCM sink availability or successful ROM rebuild as evidence of complete native compatibility. |

## Risks / open decisions and resolving experiments

These are proposed Beads work-unit contracts, not task-state tracking. No experiments were run in this research wave.

| Work unit / boundary | Concrete resolving experiment and observable result |
|---|---|
| Baseline/fork/rights admission | Freeze authorized ROM/source/tool inputs and compare fork deltas against upstream per intended import. Produce provenance/rights decisions and behavioral rationale. No user checkout modification, SDK/payload redistribution or unexplained fork behavior enters the native baseline. |
| Native ABI/address model | Decode representative model/texture/animation/VPK0/photo/save fixtures and trace `ROM → decoded bytes → relocated runtime object → Gfx` identity through two overlays sharing VRAM. Exercise real high-address heap allocations and the `mlHeapAlloc` complemented-mask trap [M1], 64-bit host pointers and canonical 32-bit wire fields; exact round-trip bytes and no truncated address/ambiguous overlay identity. |
| Cooperative process backend | Trace nested `ohWait(1/N)`, priority ties, insertion, pause/resume, end-current-process and delete-current-object against original ordering. Native trace must preserve suspension-local values, frame of resumption, current globals and deferred destruction; no concurrent access to game globals. |
| Ucode/interpreter admission | Record tasks and in-list loads for triangle/line/no-near-clipping paths plus matrix types 41/42. Resolve `G_SPECIAL_1`/matrix-word behavior against the extracted authoritative ucode in an authorized reference environment. Match geometry/state transitions; catalog unsupported commands as blockers, not silent fallbacks. |
| Live focus color parity | Fixture overlapping, translucent, similar-color and rapidly moving subjects. Compare each object's 64 u16 center pixels, context IDs, focus result and shutter snapshot—not only final image. Match ≥32 rule, draw-order winner and frame latency. |
| Photo depth/score parity | Replay fixed snapshots with occlusion, clipped/edge subjects, equal-depth surfaces, group bonuses, special signs/eggs and pose thresholds. Compare all wide/normal depth buffers and count arrays, selected subject and every `ScoreData` component. Any equality/threshold disagreement blocks faithful photo implementation. |
| Snapshot residency/lifecycle | Capture in all seven courses, leave course, replay from camera-check/report/album/gallery, save, restart and replay again. Match animation/model/material/effect/sky/camera state without stale object pointers or missing course assets. |
| Save codec/storage | Reference valid, blank, version-mismatched and digest-corrupt images; round-trip packed bitfields, signed sentinels, floats, settings, names/comments, all photo categories and relative offsets. Verify actual MD4 byte/word convention rather than assuming a library-compatible standard representation. Native save failure must report failure and preserve last durable data; schema migration is a separate explicit decision. |
| Audio producer | Capture `Acmd`, bank/sequence offsets and PCM for BGM transitions, SFX priorities/pitch/pan/reverb, mono/quality settings and scene changes. Establish host command/synthesis coverage and audio-clock sample behavior; output device latency must not alter game-process time. |
| Clocks/RNG/integrity | Replay `IdleScript` seed/flags/input at authority cadence; record RNG sequence, process events and global/photo time under different presentation rates. Define unsigned 32-bit LCG wrap and controlled time source for replay. Establish validated-ROM handling of SP-integrity flags without host MMIO or accidental illegal-copy progression. |

## Acceptance constraints

* Authority remains the pinned upstream and exact US ROM hash; upstream WIP/completion and local-fork reconciliation remain explicit rather than inferred from generated inventory.
* Native scene transition resets match original overlay semantics; runtime pointers never share the encoded ROM/VRAM/segment address type accidentally.
* Cooperative process traces match order, suspension, pause and deletion; changing display frame rate cannot change update-count waits or input edges.
* Canonical live-focus color copies, photo reconstruction, packed-depth counts, subject selection and scoring pass parity fixtures before photographs are declared faithful.
* Complete behavior chain is **course → focus/shutter → 60-photo capture limit → selection/evaluation → report replacement/progression → album/comment/reorder/delete → gallery → durable save → restart/replay**. All seven courses and their special subjects/effects participate; a single-course screenshot is not end-to-end acceptance.
* Audio synthesis, temporal behavior and persistent data are native implementations with explicit completion/failure semantics, not transport-only mocks.
* Original MIT files remain distinct from unlicensed/proprietary/vendor material and user-owned extracted assets. Import/release is blocked pending rights review.

## Commit-pinned evidence index

All links point to `ethteck/pokemonsnap` at the full authoritative SHA. Range references in tables are shorthand for these exact source spans.

[B1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/README.md#L9-L21
[B2]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/splat.yaml#L1-L58
[B3]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/decomp.yaml#L1-L18
[B4]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/pyproject.toml#L1-L19
[B5]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/entry/entry.s#L1-L26
[B6]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/configure.py#L32-L179
[B7]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/configure.py#L576-L595
[S1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/main.c#L20-L163
[S2]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/sched.c#L75-L98
[S3]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/sched.c#L776-L853
[S4]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/gtl.c#L699-L850
[P1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/om.c#L627-L744
[P2]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/om.c#L1757-L1856
[P3]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/oh.c#L104-L114
[O1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_render/46270.c#L9-L130
[O2]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_render/46270.c#L238-L350
[O3]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/include/sys/dma.h#L9-L31
[O4]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/splat.yaml#L353-L431
[O5]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/splat.yaml#L10150-L10289
[I1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/cont.c#L267-L336
[I2]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/cont.c#L422-L489
[D1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/dma.c#L23-L84
[D2]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/dma.c#L86-L299
[D3]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/tools/splat_ext/vpk0.py#L1-L42
[G1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/rsp/gspF3DEX2H.NoN.fifo.s#L1-L17
[G2]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/gtl.c#L53-L57
[G3]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/gtl.c#L297-L445
[G4]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/render.c#L499-L527
[G5]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/include/PR/gbi.h#L94-L120
[F1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_level/pokemon_detect.c#L11-L157
[F2]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_level/pokemon_detect.c#L160-L241
[F3]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_render/47380.c#L623-L835
[F4]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_render/47380.c#L1039-L1133
[F5]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_render/47380.c#L1654-L1723
[F6]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_render/score.c#L53-L228
[F7]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_render/score.c#L230-L326
[F8]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_render/score.c#L333-L674
[F9]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/include/common_structs.h#L229-L277
[V1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/more_funcs/more_funcs.h#L18-L65
[V2]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/more_funcs/5D500.c#L29-L204
[V3]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/more_funcs/5BF20.c#L13-L125
[V4]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/more_funcs/5BF20.c#L618-L683
[V5]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/more_funcs/5BF20.c#L167-L334
[V6]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/photo_check/993F80.c#L245-L284
[V7]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/gallery/9FEC10.c#L129-L184
[A1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/audio.c#L814-L858
[A2]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/audio.c#L909-L1014
[T1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/beach/55C110.c#L160-L191
[T2]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/misc.c#L110-L146
[T3]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_level/player.c#L287-L337
[T4]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/world/block.c#L100-L103
[T5]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/volcano/main.c#L401-L412
[L1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/include/PR/os.h#L5-L23
[B8]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/configure.py#L280-L302
[O6]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/splat.yaml#L5292-L5315
[G6]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/render.c#L1-L9
[G7]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/splat.yaml#L244-L247
[G8]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/splat.yaml#L321-L323
[F10]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_render/47380.c#L1245-L1272
[V8]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/more_funcs/5BF20.c#L151-L165
[M1]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/ml.c#L11-L28
[I3]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/cont.c#L741-L778
[I4]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/cont.c#L619-L640
[I5]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/gallery/9FD510.c#L443-L452
[T6]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/world/block.c#L232-L243
[B9]: https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/splat.yaml#L121-L143
