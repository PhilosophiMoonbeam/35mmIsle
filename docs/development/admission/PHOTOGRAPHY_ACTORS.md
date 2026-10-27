# Photograph scoring and native actor integration

Extracted admission evidence and its original scope limitations. Commands are relative to the repository root.

[Admission index](../ADMISSION.md) · [Project README](../../../README.md)

## Contents

- [Native Oak CPU scoring over genuine completed feedback](#native-oak-cpu-scoring-over-genuine-completed-feedback)
- [GAME scalar angles and independent score-boundary composition](#game-scalar-angles-and-independent-score-boundary-composition)
- [Independent complete Original and native photo scoring](#independent-complete-original-and-native-photo-scoring)
- [Exact complete-photo feedback comparison](#exact-complete-photo-feedback-comparison)
- [Native actor prerequisites: pose, retirement and environmental sound](#native-actor-prerequisites-pose-retirement-and-environmental-sound)
- [Native actor controls, actual actor GPU and live focus CPU](#native-actor-controls-actual-actor-gpu-and-live-focus-cpu)
- [Shared native FX and actual live-focus cutover](#shared-native-fx-and-actual-live-focus-cutover)
- [Native common1003 ownership and canonical-pose cutover](#native-common1003-ownership-and-canonical-pose-cutover)
- [Composed native World, actor and FX execution](#composed-native-world-actor-and-fx-execution)
- [Native Magikarp and complete Beach interaction/item resources](#native-magikarp-and-complete-beach-interactionitem-resources)
- [Native scene triggering and source photograph eligibility](#native-scene-triggering-and-source-photograph-eligibility)
- [Native photo membership, scheduling and genuine Magikarp GPU focus](#native-photo-membership-scheduling-and-genuine-magikarp-gpu-focus)

## Native Oak CPU scoring over genuine completed feedback

`isle-photo-score` evaluates source-ordered selection and numeric scores without another renderer, global GAME state, table payloads or per-call allocation. `RulesView` borrows the admitted Snapshot's big-endian app_render image, ROM `[0x46270,0x5BF20)`, guest base `0x8009A8C0`; fixed-table extent and every used animation subrecord are checked separately. All four bonus/class table spans matched every pinned GAME initializer. Six source identities, data hashes and exact precision observations are recorded in `cmake/photo-score-inputs.json`.

Competing semantic/interface reviews found no score-formula defect but identified an avoidable count repack in the initial API. The clean cutover takes `FinalCounts` by reference plus two-byte `SubjectMap` metadata; no copied-count compatibility path remains. Render rank, original photo index and sparse feedback slot stay distinct. Active zero-visible actors retain their ranks; raw photo records are not compacted. Callers retain matching camera/yaw/animation/pose/special/level/flag/dash state and apply returned `photo_flags` only after successful evaluation.

The implementation preserves conditional tag clearing, signs/eggs/dash gating, eligibility, category priority/ties, Shellder's alternate ID, quantized main completeness versus unquantized companion fractions, and bonus/doubling order. Framing uses the isolated 4×4 center, not the visible 2×2 center. The raw-index selection classifier and mapped-index calculation deliberately remain different. Static inspection of only the known GAME score function established binary32 PI/TAU and single-precision fold/MUL/DIV/DIV/TRUNC; the earlier double-constant inference was discarded. Exact PI produces column12, whose next-row/class-table spill is read through a defined byte address rather than host array UB or clamping.

Eight genuine two-FOV photographs completed sixteen actual RSP/RDP tasks. The scorer directly borrowed each runner's completed narrow counts and the original ROM rules. All backend leases and native model/game owners retired, with zero arena growth.

| Actual native photograph | Selected original record | Total score | Pose points | Companion bonus |
|---|---:|---:|---:|---:|
| Single | 0 | 3560 | 1000 | 0 |
| Exact repeat | 0 | 3560 | 1000 | 0 |
| Two overlapping subjects | 1 | 3710 | 1000 | 30 |
| Coincident subjects | 0 | 3500 | 1000 | 0 |
| Intervening unscored mesh, sparse/nonidentity mapping | 0 | 80 | 0 | 0 |
| Actual model turned through PI | 0 | 2540 | 500 | 0 |
| Restored model | 0 | 3560 | 1000 | 0 |
| Empty geometry | — | 0 | 0 | 0 |

The coincident second actor had actual zero visibility and earned no bonus; removing only that candidate over the **same completed counts** left the CPU result unchanged. An intervening unscored draw need not preserve that zero or the first actor's packed-depth visibility. Two private consumers incorrectly assumed invariance across different GPU workloads; their failed binaries/logs remain preserved. Only the corrected prospective consumer supplies completion evidence; no production depth predicate, score rule or expected pixel value was weakened. The unscored original Pidgey mesh is a diagnostic occluder, not a claimed Gate asset.

Meaningful authored-table regressions cover selection/visibility/completeness/pose boundaries, exact flags, sparse mapping, twelve subjects, zero-visible ranks, companion fraction rounding, PI spill and transactional rejection. GCC Debug/full LUS, GCC Release and Clang ASan+UBSan each passed twenty-four component CTests. The actual GPU consumer used Debug, not a fully instrumented backend. Its complete trace had 38/38 terminal PIDs, all absent, zero known physical DRM/input/audio/udmabuf rows and zero permission denials; ROM identity remained unchanged. Private proof SHA-256: `2722a3e406a2e76641544894979b323a989d9b09e753ae448ab5b88b8beebbc4`.

This original experiment establishes native Oak CPU behavior over authored matching photo descriptors and genuine native model feedback. The later scalar-angle qualification below replaces its host-libm dependency. These bounded results alone do not establish full saved-photo reconstruction, Original score/pixel parity, natural gameplay, Windows 11 runtime or release qualification.

## GAME scalar angles and independent score-boundary composition

`isle-game-math` independently expresses authoritative GAME `src/sys/misc.c:15–76` at `df8815e75f3d79618466e01e0827c6165b93d11c`: continued-fraction coefficients/grouping, binary32 stages and comparison-based quadrants. The C API allocates nothing, does not change floating-point controls and does not interpose standard-library symbols. Photo scoring, menu effects and every admitted private GAME C `atanf`/`atan2f` route use this one implementation; deliberate host-side steering mathematics remains separate.

An independently authored public-call observer and predeclared corpus were frozen before inspecting that GAME math body for the native implementation. Actual mapped `atan2f` entries at `0x80019abc`, architectural argument/RM readbacks and PC/SP-matched real returns reproduced **28** native score discrepancies: signed-zero quadrants, the negative-axis sign and one-ULP pose-bin crossings. No SDK producer body/header, opaque object, private AI or guest snapshot was inspected.

The final prospective cohort observes the executing thread's actual entry/return MXCSR. The reference interpreter enables FTZ for RM0/1 and disables it for RM2/3 in this captured FS profile; host rounding can be applied lazily. Native comparison therefore selects rounding from actual guest FCSR and copies only observed stable FTZ/DAZ controls. Matching RM alone had left **217** scalar differences. No emulator-specific FTZ policy or output-fitting special case was added to production.

Matched FTZ also changes some authored camera-subtraction operands. Composition binds the **actual production subtraction bits**, full entry FCSR and observed denormal controls to actual public Original scalar observations. Conflicting returns or absent bindings fail admission; eight additional predeclared input pairs close the observed coverage gaps. It never substitutes the original query row when that row's operands differ.

| Final controlled observation / consumer | Actual result |
|---|---:|
| Public scalar calls; actual entries, returns and matched returns | 10,824 each |
| Raw scalar bit differences under matched context | 0 |
| RM0 production-score compositions | 2,712: 2,711 accepted and one agreed invalid-animation rejection |
| Score-field differences | 0 |
| Expected non-nearest scoring-contract rejections | 8,112 |
| Unexpected acceptance/rejection, protocol errors, duplicate events or profile differences | 0 |
| Fixture operand changes across all modes; explicitly rebound rather than ignored | 152 |
| Actual Original transport tasks / DMA rows / bytes | 353 / 91,735 / 20,166,728 |
| Command-fetch, ordered last-write PCM and custody disagreements | 0 |

The successful Original process exits normally after 30.337 seconds; its caller resumes, all protected inputs remain unchanged and no forbidden device endpoint/network call is recorded. The owned scalar caller consumes 232 additional actual GAME general-heap bytes. This is a controlled scalar mechanism, not an uninstrumented or natural gameplay trajectory. Separate explicitly synthetic duplicate-entry and missing-MXCSR fixtures both reject with exit2. A malformed extension rejected before collection is excluded, not relabeled successful.

GCC Debug passes all 59 component CTests; optimized GCC passes the scalar and score consumers; Clang passes scalar, score and effects consumers. The actual Linux product rebuilds. Real CPU-Vulkan effects execute their format/tenth-frame/retirement workload; the rebuilt photograph consumer completes 68 genuine GPU frames, four expected failed-GPU paths and 83 Original C OM/CopyInfo updates. Independent arithmetic, route-closure and composition reviews report no concrete defect within these scopes.

Private scalar qualification manifest SHA-256: `ad9c3656d015fddd196a96a0ac22c74bd2e2388ae4351b45a46be61cb4987c65`. Original `PI_2` macro value/type, universal underflow/exception-status behavior and physical N64 behavior remain unproven. This scalar cohort does not execute the complete Original scoring function; the separate complete-score cohort below supplies its own bounded evidence. Neither establishes universal score/pixel parity or Windows 11 runtime qualification.

### Exact scalar-angle portable Runtime products

Source `be7f372dcda8da37d1b30f8ff7783e1aa0122a42` passes all ten jobs in [CI 39747038144](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39747038144), including GCC/Clang Debug/Release, MSVC x64 Debug/Release, complete static-runtime Windows and Linux products, real Linux CPU-Vulkan execution and MSVC canonical RDP consumers. [Preview 4](https://github.com/PhilosophiMoonbeam/35mmIsle/releases/tag/v0.1.0-preview.4) publishes the unchanged hosted archives; the public tag resolves to that exact commit.

| Product | Bytes | SHA-256 |
|---|---:|---|
| Linux x64 `.tar.gz` | 9,881,709 | `24d7009f24a5edec40cce4df3b8b78518d64b0341af2fb235651ff89683204c6` |
| Windows x64 `.zip` | 5,372,776 | `bd658b64225c0a86bc8bb353aab4fec6f12d0ca6f942a874de63c00e1868303b` |

The published `SHA256SUMS` SHA-256 is `21b94f2ddf6699144d209285068549d895d83f8c4ac3d48b6d0d0a0978728e2a`; authenticated release metadata independently reports all three matching digests. Relative safe archive paths, applicable notices and absence of ROM/derived game archives were checked. Downloaded ELF and AMD64 PE identities are respectively `5af1e742bb59b48bd0d57f9aee370391f324a9957083197f8a595bdf0330caaa` and `46e6b9b9299a4d538dc97063d8c02c34e1cbaa22888f93eac2da6f68b8ea1e25`.

The exact downloaded Linux ELF passes `--help` and a separate uninstrumented ordinary-SDL-keyboard session with an unchanged whole, previously earned Save pair. Actual boot, Title, Beach attract and Continue-selection surfaces were captured. The session reached its declared 1,200-second deadline, sent normal WM close and returned 0 with no forced cleanup, errors or modified protected inputs; the pair remained byte-exact. A preceding unsupported-GLX startup is excluded. The successful session uses documented SDL3 `SDL_VIDEO_FORCE_EGL=1` and CPU rendering, not a product patch.

This session does **not** qualify Album/AB, a new photograph/Oak assessment or a fresh campaign for this binary. Those earlier results retain their source/cohort boundaries. Windows package/help/CI success is not Windows 11 ROM gameplay. Private portable-product qualification SHA-256: `e87c28ce81f4fca4e1a00169f8ca9fc09cbe441afa2da6285d499f1d2a55fec7`.

## Independent complete Original and native photo scoring

A fresh Original lifetime starts with empty saves and ordinary SDL/XTest input: New Game, naturally entered photographer name, Oak introduction, Beach tutorial photographs and normal Quit Course. No guest state, score, photograph, controller result or callback return is authored. The actual GAME replay initializer/configuration and yield-capable OM/GTL caller execute the complete `func_800A0EA4` → rendering/counting → `score_CalculateScore` path.

Independent pinned [CXD4](https://github.com/mupen64plus/mupen64plus-rsp-cxd4/tree/00906a92641c540a64e4e7505012157315314810) and serial [Angrylion](https://github.com/ata4/angrylion-rdp-plus/tree/31bdb1f0a79dd726017a38432540c6b5db0fa117) execute real opaque microcode and RDP work over live Original memory; no SDK producer body/header, opaque SDK/DSP object or full CPU/RAM checkpoint is inspected. Plugin forwarding wrappers observe public descriptors, actual invocations/interrupt callbacks and dispatch counters, not expected outcomes. These mixed-notice reference plugins remain private and are not relicensed or included in the portable release.

Each actual outer entry captures the complete typed 928-byte pre-call PhotoData, all 12 subject, six item and 32 effect slots, raw binary32 values, source order and sentinels. Post-call photo flags, all eleven ScoreData fields, selected rank/map/slot/species, both actual 159,744-byte feedback planes and reductions remain separate output evidence. Actual PC/SP-matched returns, camera restoration, wrapper copying, caller return, one-VI wait/resumption and subsequent VIs establish lifetime completion rather than just a returned C value.

Both FOV passes require real graphics-task ancestry, halted/broke RSP return, actually forwarded SP/DP interrupts, positive real RDP dispatch/triangle/full-sync work, nonempty actual subject depths and a drained pipeline at actual `gtlDrawOne` return. Attribution opens at actual `score_Render` entry, not after graphics have already executed. Earlier closed attempts with a late attribution window or unrelated-interrupt credit are excluded; independent review accepted both corrections before the fresh successful lifetime.

The fresh Original exits normally with **four qualified lifetimes**, zero pending observed frames, zero in-flight RSP/RDP calls, no observer failure and completed cleanup. Every launched asset, including lawful ROM, core, plugins, observer and contract, retains its launch SHA-256. Private Original qualification SHA-256: `7591af962bb56a17b9f3f3155ae9dbb5be508218e8f0836c3f9fb3f1f995586b`.

Four separate native consumers losslessly translate only those pre-call inputs into production Replay. Native independently reconstructs the world/subjects/items/effects, executes its actual GAME C render consumer with canonical RSP/CPU-Vulkan RDP, reduces its own completed memory and computes its own score. Original counts, feedback, ScoreData and expected results are never native inputs. Actual Source wait1 advances OM once; real Host Audio uses SDL's dummy device. Every consumer completes two actual GPU frames, restores its camera, joins Source, retires Replay/Frame and destroys the game with zero objects, cameras, models/nodes, matrices, sprites or processes remaining.

| Genuine photograph | Original total | Native total | Differing ScoreData fields |
|---|---:|---:|---:|
| 1 | 3,300 | 3,300 | 0 / 11 |
| 2 | 2,130 | 2,130 | 0 / 11 |
| 3 | 1,030 | 1,030 | 0 / 11 |
| 4 | 2,210 | 2,210 | 0 / 11 |

Selected input slots and post-call photo flags also agree in all four comparisons. Native completes eight genuine GPU passes. The public Replay API does not expose FinalCounts or a complete feedback-slot map, so this score-only consumer fabricates neither; its equality claim concerns actual complete scoring outputs. The separate read-only observer below supplies its own raw-word/count evidence. Fresh Native GameSession Dash state is identified separately; no Original save state is imported. The photos do not exercise every species, effect, equipment state, signed-zero case or rendering backend.

Private comparison manifest SHA-256: `d5cca808c67a6ebbc22bd2826dd3396c11d2826a9cf3696ccd4a8869ce8e3385`. This bounds the production `be7f372` implementation's independent complete-score agreement; it is not physical N64, universal score/pixel/FP, Windows 11 gameplay or downloaded-product campaign qualification.

## Exact complete-photo feedback comparison

A separate private Native consumer observes the existing completed-memory callback immediately after real wide/narrow feedback reductions and before lease release. Only the native replay translation unit and its shared score-render include are privately copied; production sources and APIs are unchanged. The observer copies exactly 159,744 guest-big-endian bytes per pass through canonical word-swapped32 addressing, plus the actual owned render-index→PhotoData-slot map. It never writes RAM, reads hidden/full RAM, retains a lease pointer or reads Original results/feedback/counts as Native input.

All four qualified Original pre-call photographs independently reconstruct, render, score and retire normally through this consumer. Exact comparison covers **638,976 u16 words** across eight completed passes: all 26 × 64 × 48 words per pass, including color plane0, inactive slots and every packed-depth word. There are **zero raw-word differences**, not merely equal scores or counts. Independently derived per-pixel GAME predicates and all seven reduction arrays also agree; Original derived arrays match its separately recorded arrays. Slot mappings and selected targets agree without reordering.

The original score-only harness, CMake recipe and ELF were preserved before observation was added; identity manifest SHA-256: `ee91b788a7c250275870265adf26ad9294bdf8da5ef665f91e3dea229fc09680`. A separate explicitly synthetic truncated-buffer fixture rejects with exit2; no observed buffer is edited. Independent observer/linkage and comparator/predicate reviews report no concrete defect within their source scopes. The temporary build hook is removed after execution.

Private feedback qualification SHA-256: `852f60a00604013a777c7ff1baae4eddfbe540d2b1b83b05f9fd8e17756e899b`. This is exact agreement for these eight actual public GAME feedback buffers, not hidden-coverage state, every focus/occlusion boundary, every photo/scene, physical N64 behavior, Windows 11 gameplay or universal backend/pixel parity.

## Native actor prerequisites: pose, retirement and environmental sound

The shared animator now publishes node-local transforms through `Runtime::set_node_transform` without a second pose cache, renderer quantization, program restart or hot allocation. Handle/lifecycle/extent/finite validation precedes mutation; unchanged binary32 fields do not dirty state. All 44 CPU contracts passed GCC Debug, GCC Release and Clang ASan+UBSan. A fresh actual 17-model/79-header consumer retained common1003's 59 source root-local Y decrements across real animation steps. This does not execute its native factory or path continuation.

Native object teardown composes pre-payload and joined observers through Services without replacing command ownership. Current/self deletion stays deferred; only actual caller joins certify abandoned source command frames. Independent review exposed an ABA counterexample involving a retained retired record, native-address reuse and an unrelated object's join. The native regression failed before the minimal live-registration guard and passed afterward; independent re-review accepted it. Native joins remain distinct from graphics/readback and effect fences.

`isle-environment-sound` implements the actual GAME category, cutoff, attenuation, pan, pitch, update, stop, cleanup and fixed-parameter rules over the existing audio session. `isle-source-angles` uses the authorized ROM's 4,097-entry angle table and exact source branch/grouping rules, not host `atan2`. Independent review caught an overstrict Y conversion bound: GAME short-circuits that cast when X truncates nonzero. The real-ROM boundary regression failed before correction; the corrected consumer accepts the affected native camera direction and still rejects actually consumed overflow without changing output. Separate planner/reviewer assessments accepted the minimal source correction and existing lifecycle design; source/listener/resource borrow documentation was clarified.

The fresh actual sound consumer passed category coexistence/replacement/stop and cutoff-before-replacement, then executed 57 real audio RSP tasks and accepted 30,496 checked stereo PCM frames. Camera reversal moved energy from right to left; after two actual parked continuations joined and the source model was removed, the fixed voice remained left despite another camera change. No physical sink/hearing, actor geometry/factory, full event routing or independent Original numerical parity is implied. Exact source/table and executable/archive identities, including the immutable rejected boundary root, are in [`environment-sound-inputs.json`](../../../cmake/environment-sound-inputs.json).

Fresh existing consumers also passed full World geometry—eight primary/four secondary blocks—and Startup→Title→Menu→Name→Oak: 925 canonical frames, 925 actual LUS presentations and 1,170,080 checked SDL-dummy stereo frames. Five actual window captures were inspected; Name retains three clean lines and inline A. Every new runtime tree was closed and stable before its sole physical/permission scan: zero known attempts and unchanged ROM. Identities and censuses are in [`snap-engine-inputs.json`](../../../cmake/snap-engine-inputs.json); local-pose evidence is in [`menu-animation-inputs.json`](../../../cmake/menu-animation-inputs.json).

These are actor prerequisites, not playable Beach or a product executable. Native actor ownership/control/factories, source player/items/detector/FX integration, combined-scene RAM layout, actor54/48 GPU, all-course gameplay, Windows11 private-ROM execution and release gates remain open.

## Native actor controls, actual actor GPU and live focus CPU

Six selected GAME C helpers now compile privately from the exact authoritative Git objects: state/random-state, auxiliary start/stop, path replacement and no-interaction flag wait. They use the existing native scheduler and one actual GObj payload; no SDK layouts, callback registry, guest function casts or second scheduler. A native binding marker prevents unrelated `userData` dereference. ASan reproduced the four-byte foreign-payload overflow before correction; fresh/reused-payload regressions pass afterward. The independent reviewer accepted the correction.

The detector CPU component implements two-context source history, full-16-bit 8×8 color comparison, the inclusive 32-pixel threshold, candidate/photo membership, cleanup-before-publication and the conditional 21-pixel red mark. A non-test API consumer exercised real native actor identity, 61 C updates, 59 source path waits and actual joins; authored color inputs produced the exact reduction and mark. This is not GPU focus capture or the complete common1003 FSM.

`isle-actor-producer` reuses the sole borrowed kernel, actual camera3/14, native attachments and complete immutable ActorResources. Callback-specific fog, flag100, native hidden flags, links/tags, residency, transactional admission and completed-work retirement are exercised. The private six-slot GPU consumer preserves actual source scales/initial poses and includes Snorlax54, two Pikachu48 instances, retained-texture1006, geometryless1005 and common1003.

Three integration prerequisites were corrected without changing source math or increasing the 16-million instruction budget:

- Source I/D/J NULL-draw guards precede material binding/calls; valid I pre-only work survives. The focused regression failed before and passes after.
- The caller supplies complete ordinary `srdpReset` state and the actual Beach global light callback. Typed SceneSetup plus own GAME headers establishes callback800A1A50→800AEC18; only the common 88 lighting bytes are installed.
- Actual Snorlax/Pikachu roots have two conditional depth-LOD geometry choices followed by EndDL: sufficiently distant objects legitimately draw nothing. Independent planner/reviewer decoding accepted this third outcome. The authored camera moved nearer while retaining the entire cohort, source scales, poses, fog, lists and matrices. This is a fixture correction, not a source54 workaround.

| Exercised component | Observed evidence |
|---|---|
| CPU integration | All 46 contracts pass GCC Debug, GCC Release and Clang ASan+UBSan. |
| Actor GPU | Complete six-slot consumer passed in 10.520s; Snorlax suppression changes 437 RGB5 pixels, Pikachu hide changes 23; both change depth/packets and restore exactly. Local/placement/camera changes, retained texture, rejection, pointer/epoch reuse, joined retirement/respawn and resource rebind pass. Three actual completed surfaces inspected. |
| Existing World | All eight primary/four secondary blocks pass in 8.650s after shared NULL guard. |
| Existing app | Startup→Title→Menu→Name→Oak passes 925 canonical/925 LUS frames, 1,169,152 checked dummy PCM frames including 123,632 during GPU waits. Five actual 640×480 surfaces inspected; course0 is a source request, not gameplay. |

All successful runtime trees closed before their sole initial observation: stable complete tasks, zero known physical/permission attempts, unchanged ROM. Rejected camera-alias, prerequisite and distant-fixture roots remain wholly rejected and immutable; none was replayed, rescanned, visually inspected or partially accepted. Temporary diagnostic pose/call dumps were removed after proof. Source/executable/archive identities and scope are in [`actor-control-inputs.json`](../../../cmake/actor-control-inputs.json), [`actor-producer-inputs.json`](../../../cmake/actor-producer-inputs.json), [`focus-detector-inputs.json`](../../../cmake/focus-detector-inputs.json) and [`menu-model-inputs.json`](../../../cmake/menu-model-inputs.json).

Published prerequisite `a72f332` built the Windows targets but failed three Windows CTests in [CI38905019507](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38905019507): the camera-OOM fixture assumed exhausting larger GObjs also exhausts smaller camera/matrix allocations. Explicitly consuming the remaining public arena tail corrected it. Actor integration `d8bb80e` passed [all ten replacement jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38915801831), including MSVC Debug/Release, renderer/compiler and full static-runtime LUS/audio/RSP/RDP/native engine. This is hosted Windows Server build/component acceptance, not Windows11 private-ROM execution or verification of later working changes.

Native actor factories/header coordination/full FSMs, actor/player/items integration, complete original detector snapshot/update queue/GObjs, combined World/actor/FX scene RAM, all-course gameplay, product entrypoint, independent Original parity and Windows11 private-ROM execution remain open.

## Shared native FX and actual live-focus cutover

The sole `EffectRuntime` replaces the Title-only class; no compatibility renderer or second pool remains. Immutable selection covers nine source groups, 73 scripts and 34 images; active slots remain common bank 3 plus course bank 0, not nine simultaneous slots. One 100-particle/10-emitter owner retains 16 encoded lists, application RNG/source-ID semantics, source interpreter/physics/children, local targets and joined detach/reset. Kind-2/flag-4 parent coupling remains explicitly unadmitted; actual selected descriptors use kinds 0/3. See [`effect-bank-inputs.json`](../../../cmake/effect-bank-inputs.json) and [`menu-effects-inputs.json`](../../../cmake/menu-effects-inputs.json).

Actual private resource consumption passed every group, borrowed alias/packing/Title identity and moved-owner rejection. The real RSP/RDP consumer passed six image formats, 48-pixel textures, tenth frame, independent completed texel/color/depth oracles, same-CI4-after-large-I8 reload and exact 16-entry palette isolation. Course renderer-only scripts are explicitly authored; actual intro scripts 0/3 remain separately exercised.

A throwaway non-test API example admitted every source bank and exercised 73 particle plus 73 attached-emitter source-script prefixes, eight update pairs each, with a real Runtime-issued identity/native DObj and 240 raw-world resolver calls. Functional native target services were supplied, but these prefixes invoked no local reads/writes; target/all-branches or whole-course parity is not inferred. The example source/build target was removed after smoke; its exact private frozen archive remains evidence.

The raw-world helper follows accepted native local S/R/T from leaf to root and root auxiliary RT/RTS, independent of flags, renderer caches and contextual matrix recipes. Actual Title synchronizes accepted animation poses before events/emission. Source column normalization remains in `EffectRuntime`. The complete real World consumer translates source-created particles across all seven primary entries without advancing velocity/RNG; the diagnostic `effect_position` surrogate is gone. This is not combined World+FX rendering. See [`source-effect-world-inputs.json`](../../../cmake/source-effect-world-inputs.json).

The complete six-actor GPU consumer supplies genuine per-callback center rows and canonical GObj Pokemon id/flags. Stable source DL/priority order merges the detector prefix; DL4 captures append to the previous context before prefix, then the prefix flips context and always saves a NULL baseline—even when actor saving is disabled. Actual GPU/readback and CPU analysis retire before the single shared 20-row pool is reused. Rejected preparation preserves RAM/native state, both histories, photo state and computed/published focus.

Independent planner/reviewer assessments rejected template-origin framing: the initial Pikachu animation hides nodes 1/2. Evaluated visible node 3/lane 0 geometry and source quantized matrices derive the authored placement, leaving source pose, scale, LOD, cohort, 32-pixel threshold and budget unchanged. Actual Snorlax/Pikachu rows each changed 64 full-16-bit pixels; flag 0x100/native hide restore Snorlax, exact reveal restores Pikachu, and preprefix/context/disabled-baseline/cleanup/deferred-publication gates pass. Failed framing roots remain wholly rejected, immutable and uninspected. See [`actor-producer-inputs.json`](../../../cmake/actor-producer-inputs.json) and [`focus-detector-inputs.json`](../../../cmake/focus-detector-inputs.json).

`CanonicalExecutor::MemoryLease` owns an explicit completed CPU write phase over actual persistent processor RDRAM. `finish_postprocess()` performs real consumer writeback; moving the lease transfers pending writes, released leases reject writers, and unfinished release poisons reuse. A separate six-task GPU consumer proves subsequent GPU consumption of retained CPU edits. The actual actor path marks exactly 21 red positions while preserving other pixels/hidden fingerprint. No abandoned preparation, remap, writable hidden memory or synthetic fence is used. See [`focus-capture-inputs.json`](../../../cmake/focus-capture-inputs.json).

Integrated Debug/Release/ClangASanUBSan each passed 49 CPU tests. Native startup passed 643 canonical/LUS frames, 1,257,040 dummy PCM frames and five inspected logo/Title/menu/Name/early-Oak surfaces; the source course-0 request remains a request, not gameplay. Every accepted runtime tree closed before its sole initial observation: stable complete tasks, no known physical/permission attempts, unchanged ROM. Published `e0d3b2f` passed [all ten hosted Linux/Windows jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38928147438), including MSVC Debug/Release, renderer/compiler and full static-runtime LUS/audio/RSP/RDP/native engine. This does not qualify later ActorOwner/source-pause/combined-scene work; Windows11 private-ROM/device execution and Original numerical parity remain unqualified.

## Native common1003 ownership and canonical-pose cutover

The common1003 baseline coordinates its actual factory, normal path/cleanup and hidden/retrigger FSM through six core plus nine owner-side pinned GAME C definitions. It owns one canonical source record per real native GObj, one shared animation Runtime and one shared FX owner. At this baseline, other configured factory/state/graph identities rejected admission; the subsequent Magikarp source/data cutover is qualified below. No guest callback cast, SDK object layout, second scheduler or animation clock is introduced.

The clean actual-ROM CPU consumer passes GCC Debug and Clang ASan+UBSan. It executes the 59 decrement-before-wait path writes and final wait, two natural hidden/retrigger cycles, actual OM Update/FIFO counter/end ordering, source command suspension/resumption, all 79 actual native header contexts, immediate/root sound-event precedence, raw-world attached emission and independent-particle cleanup. Duplicate retiring-sender delivery, unrelated native allocation, receiver cancellation, collection/reuse and final engine/audio destruction complete. Native identity retention prevents address reuse until the real joined object and its command borrows retire; it is not a GPU fence.

| Clean CPU consumer | Seconds | Closed terminals / created / absent, including tracer |
|---|---:|---|
| GCC Debug | 2.430761403 | 26 / 25 / 27 |
| Clang ASan+UBSan | 7.926716915 | 26 / 25 / 27 |

Both trees were stable and absent before their sole initial observation: unchanged ROM and zero known physical/permission attempts. Audio used an owned capture sink. Address/undefined-behavior failures remained fatal; leak scanning was disabled only because the complete tree was ptraced. Exact executable/archive identities and scope are in [`actor-owner-inputs.json`](../../../cmake/actor-owner-inputs.json). Temporary header-failure diagnostics were removed before these clean proofs.

Integration corrections preserve source behavior rather than manufacture broader acceptance:

- Actual header animation writes full local S/R/T even on translation-only render nodes. Native canonical locals now retain those finite channels; the T-only fixed cache consumes only position. Borrowed model44/48 paths select one explicit native-or-animation authority consistently. Raw-world FX composes canonical ancestry independently of the renderer's fixed recipe. World terrain seeds descriptor yaw and retains root pose through nonroad texture synchronization and translation-only block relocation.
- Independent planner/reviewer assessments retained safe eager RT/RTS placement admission. An authored oversized positive fixture—not a ROM spawn—exceeded consumed fixed translation bounds. Its exact input now proves rejection, joined collection, complete capacity recovery and accepted-placement atomicity. A separate representable fixture explicitly has no current World and verifies source ground fallback0, ignored spawn scale and initialization-scale semantics. No original GPU cohort or source pose changed.
- Selected C entrypoints require a live native callback. The Update consumer now suspends real processes with FIFO `-2` and runs the actual OM tick; it does not call selected C outside its contract or fabricate `omCurrentObject`.
- Exhaustive typed inventory of all nine groups/73 decoded FX programs finds no B7/B8/BF instruction or initial output flag8000. Impossible Original-asset target searches were deleted, not replaced with synthetic source scripts. Generic VM tests retain numerical aim/attraction, integrated local output, incarnation invalidation and stale-target behavior without callback-count or mock-echo acceptance. Original-bank local target execution is not claimed.

`EffectGraphicsResources` is the sole production cold FX packer. A removed throwaway actual-data API consumer covered all nine source groups using unique active slots, exact and partial/nested payload aliases, aligned bounded ownership, insufficient storage, duplicate bank0 rejection and moved-Snapshot invalidation. This CPU packing proof is not completed composed-GPU qualification.

The two-common composed scene uses generic `renderModelTypeDFogged` (`0x800a1608`), not a Pokemon photo-gated wrapper. Its correct source result is a real NULL baseline without a fabricated subject, even when common1003 appears in photo inputs. Genuine combined photo-subject qualification remains open until an actual gated native species FSM and its services are admitted. Separate published six-actor focus evidence does not discharge that requirement. Full player/items, other species/courses, product entrypoint, Original numerical parity and private Windows11 execution remain open.

## Composed native World, actor and FX execution

The shared compositor now accepts an explicit core RAM layout and a once-only completed-memory callback under the real executor lease. Each source lane joins World before Actor; global order remains 0, 2, 1, 3. One complete immutable/mutable/producer/task/framebuffer union is admitted before writes. The frame drains before producer/native ownership on success and exception paths; World callbacks retain the live native Owner through World retirement. A preparation lease, RAM reservation or native join never substitutes for actual graphics completion.

| Clean actual private consumer | Observed result |
|---|---|
| World + common1003 + shared FX | Twelve GPU frames; Debug 9.153 s, Clang ASan+UBSan 15.653 s. Real actor hide/restore, source normal cleanup, World/FX shift, guard preservation, native/graphics/audio retirement and genuine NULL-baseline/no-false-photo checks pass. Five saved surfaces match byte-for-byte across builds. |
| Existing full World | Unchanged eight-primary/four-secondary cohort passes in 11.095 s, including native descriptor yaw, nonroad texture/pose retention, placement/context/residency/visibility/rejection and real source particle translation. |
| Existing six-actor GPU | Unchanged source54/48/28 cohort passes in 15.606 s. Snorlax gate changes 437 RGB5 pixels, Pikachu hide changes 23; two actual focus rows each change 64 full-16-bit/RGB5 pixels and completed writeback marks 21 red positions. This is not additional native species FSM admission. |
| Existing application surface | Startup→Title→Menu→Name→early Oak passes 643 canonical/643 LUS frames, including 53 Name/229 Oak frames. Checked SDL dummy output accepts 1,256,112 stereo frames, 164,944 during GPU waits. Five actual 640×480 GL windows inspected; Name retains three instruction lines/inline A. Course 0 remains a source request, not gameplay. |
| CPU/platform builds | All 49 contracts pass GCC Debug, full canonical-renderer GCC Release and Clang ASan+UBSan. Hosted Windows and private Windows11 runtime are not inferred from these local runs. |

The initial FX visibility expectation was incorrect, not evidence for changing the renderer. Actual common bank 3/script 0 grows size 5→15 over six VM updates; creation performs the first update. At the unchanged diagnostic point/camera, update 2 encodes steps `0x8724`: negative [signed S5.10](http://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gsp/gSPScisTextureRectangle.html). Source clamping samples the actual transparent I8 corner, so pixels correctly remain unchanged. That exact checkpoint is retained. Exactly one subsequent genuine scene tick reaches positive steps `0x709e`; paired frames then change color while preserving triangle geometry, depth and focus-mark state. No source duration, position, scale, camera, asset, math, threshold or budget changed; there is no wait-until-pass loop.

Successful-surface inspection also exposed an invalid caller boundary: after moving World/camera outside OM, the consumer rendered before the existing priority-0 sky-follow process ran. It now verifies instantaneous FX translation/velocity/RNG first, then performs a genuine whole-scene native tick before GPU preparation. The corrected sky-follow surface was directly inspected; no direct sky surrogate or pose workaround was added.

Independent native-lifecycle review found no actionable issue. Graphics review found a real new-layout defect: catalog-backed Frame admitted core commands at `0x110000`, overlapping its fixed Title model payload. A real API regression failed before correction. Startup core ownership is now checked against fixed producers, framebuffers and fade storage in the first storage construction, before executor creation. The same central producer ranges feed Catalog planning. Six concrete overlapping plans reject while each same generic snapshot-only plan remains legal and retires; the actual consumer passes Debug 1.335 s/ASan 2.617 s. Default source startup and composed scenes pass afterward.

Every accepted tree was complete, stable and absent before its sole initial observation: unchanged ROM, zero known physical/permission attempts. Temporary particle/raster diagnostics and API packing scaffolds were removed after proof. Executable/archive identities, precise scopes and rejected-before/passed-after records are centralized in [`course-scene-inputs.json`](../../../cmake/course-scene-inputs.json); all byte-hashed support paths have explicit LF checkout rules.

The common-only scene deliberately retains generic callback `0x800a1608`; it cannot fabricate a Pokemon-gated subject. The later native-photo-controller section qualifies genuine combined Magikarp focus without changing this negative cohort. Player/items, complete photo reconstruction/queues, all-course gameplay, product entrypoint, Original parity and private Windows11 execution remain open.

## Native Magikarp and complete Beach interaction/item resources

Genuine Magikarp129 now shares the existing Owner, native scheduler, Runtime, World, RNG, FX and audio services with common1003. Six core and fifteen owner-side pinned GAME C definitions retain the distinct initial/state headers, source yaw expression, actual ten-wait ground path, first eligible emission, same-owner AddAtGeo sibling and animation-end scene-latch clear before cleanup. The private consumer also exercises nonzero normalized emission velocity and real source ID wrap, second-pass half-open search, other-link exclusion and occupied inclusive-max fallback. No source timing, scale, pose, diagnostic ceiling or pool changed.

The first integration build found a real C `Vec3f` typedef collision with native engine math. Clean `actor-control-v4` uses explicit `IsleGameVec3` declarations/signatures in the two selected source helpers; the conflicting typedef and old transformation profile are removed. Yaw retains the unsuffixed double [native PI choice](https://dlmf.nist.gov/3.12.E1) and final float publication. Original SDK macro value/type equivalence remains unproven; no SDK header was imported.

`BehaviorResources` retains all 71 graphs/212 live entries, eight weighted-state lists/16 live entries and the distinct fixed two-choice scene table, including its legitimate weighted NULL outcome. Source cmd58 and NULL-state termination consume only defined terminal fields. Original Snapshot aliases, finite mapped bounds, exact identity lookup and moved-owner rejection are qualified. Test-only pseudo interpreters and scalar-copy echoes were removed, not repinned. This data component does not admit graph callbacks, threshold dispatch, random selection or additional species.

One `model_assets::DataDecoder` now serves actors and all four Item families, using `Reader::remaining_bytes` for finite initialized bounds rather than duplicate sentinel walkers. Existing 17-model/79-header/33-material/1,317-program actor acceptance remains complete. Items retain four models, four materials, eleven programs and 22 installation spans; real animator execution visits all eight pester-ball/six apple images and finite splash programs with unchanged model1/texture0.4/splash0.9 speeds and root scale assignment0.1.

Two private Item assertions were wrong and their failed roots remain rejected. Own OM initializes NULL animation callbacks: the consumer now acquires with callbacks disabled, separately from per-stream callback metadata, without increasing event capacity or suppressing the production emitter. Own animation disables transient FINISHED within the same update: the invalid persistent-phase assertion was deleted, not replaced by another incidental phase check. The corrected consumer checks the real fume predicate `60.0f-animationTime<0.9f`, first cutoff at zero-based update66, finite terminal object-time freeze, image cycles and borrowed lifetime under the fixed256-update/64-command/zero-event envelope.

| Actual private CPU consumer | Debug seconds | Release seconds | Clang ASan+UBSan seconds | Closed terminals / created / absent |
|---|---:|---:|---:|---|
| Genuine native Magikarp source | 0.739089 | 0.265534 | 2.307477 | 25 / 24 / 26 |
| Complete behavior data API | 0.464979 | 0.140283 | 1.189881 | 1 / 0 / 2 |
| Complete Item/shared animator | 0.442216 | 0.149100 | 1.370300 | 1 / 0 / 2 |
| Unchanged all79 actor headers | 0.766638 | — | 2.836026 | 1 / 0 / 2 |
| Unchanged complete common source | 2.390488 | — | 7.320791 | 26 / 25 / 27 |

Every accepted root was stable and all tracer/tasks absent before its sole initial whole-tree observation: unchanged ROM, zero known physical/permission attempts. Address/UB failures remained fatal; leak scanning was disabled solely for ptrace and does not qualify leaks. Exact scopes/executable/archive identities are retained in the source manifests; no ROM/table/art payload is committed.

The unchanged actual GPU regressions also pass after the decoder cutover: twelve composed frames in Debug 11.327997 s, Release 10.156684 s and ASan 17.369143 s; each closes 44/43/45. Five saved surfaces are byte-identical across all three configurations and directly inspected. Full eight-primary/four-secondary World passes in 13.323909 s; the unchanged six-actor cohort passes in 17.502160 s, each closes 77/76/78. The latter retains Snorlax/Pikachu changes of 437/23 RGB5 pixels, two genuine 64-pixel focus captures and exactly 21 completed red writes with hidden memory unchanged. All 49 CPU contracts pass each full-renderer local configuration. Published `12931a8` passed all ten hosted Linux/Windows jobs; this does not qualify later private Windows11 execution.

Magikarp's actual initialization flags are `0x25`, not visibility-bypass `0x40`. Its genuine gated wrapper alone therefore does not supply source collision position, real player distance, camera visibility or photo eligibility. The following service cutover qualifies the next bounded source components, not a hand-built photo list or full collector. Native Item/Player services, other species/courses, complete photo reconstruction, playable product entrypoint and complete port acceptance remain open.

## Native scene triggering and source photograph eligibility

`MagikarpScene` implements own `55C110.c:10–65` over the existing Owner and actual native models. The helper uses current/current blocks, zero factory position/Euler and unit spawn scale, then copies the triggering root DObj position into the spawned AUX without multiplying by100, regrounding or rewriting the initial header/scale. The chooser retains the strict surface/distance gate, complete weighted NULL choice, shared RNG consumption before the active gate, and latch publication only after successful chosen spawn. Direct helper calls leave the latch alone; the genuine source state clears it after animation end and before cleanup.

Own source leaves `ObjectSpawn.path/behavior` unwritten although the Ground factory consumes them. The defined modern policy initializes native NULL path and behavior0. These are deliberate native values, not recovered source defaults or Original uninitialized-stack equivalence. One shared `Owner::add_one` now serves the scene and existing C AddAtGeo path; its cursor, both half-open native searches, occupied inclusive-max fallback and failure consumption remain unchanged.

`actor::photo` uses existing privately compiled own-GAME math for canonical AUX RT and explicit eye/at/up camera preparation. Collision offset is unscaled; flag1 alone adds root-local position times AUX scale. Visibility retains source depth planes, binary32 `228.506134f` projection, signed32 truncation and inclusive screen bounds. Hidden/untangible rejection precedes mutation; flag40 bypass precedes distance; visibility precedes the complete species whitelist. Explicit safe-domain errors preserve outputs/flags rather than clamp or invent invisibility.

One native `IsleActorSource.collPosition` is authoritative. `Owner::refresh_collision_positions` implements only the source global pass's position refresh/flag80 clear. Caller-owned priority-2 scheduling remains explicit; this is not the second-pass pair-collision, command26 or ground-walking manager. No dummy collision callback, full photo collector, payload reconstruction or source-context publication is inferred.

| Actual private source consumer | Debug seconds | Release seconds | Clang ASan+UBSan seconds | Closed terminals / created / absent |
|---|---:|---:|---:|---|
| Native scene helper/chooser + canonical photo predicate | 0.727000 | 0.290027 | 3.022388 | 8 / 7 / 9 |
| Unchanged full Magikarp source/allocator cohort | 0.732773 | 0.302867 | 3.198513 | 25 / 24 / 26 |
| Unchanged full common source/all79 native headers | 2.387418 | 0.645667 | 12.181451 | 26 / 25 / 27 |

The real scene API inputs distinguish trigger DObj from its unrelated AUX and player AUX from its unrelated root. Independent actual-data collision/camera oracles observe `(123.25,100.5,-100.625)` and source eligibility false in all three builds. That negative remains unchanged; no camera, endpoint, pose, scale or source threshold was tuned into a positive focus claim. Chosen capacity failure does not publish a latch; postcopy representation failure retains the genuinely spawned handle for explicit cleanup. Fixed 512 native updates complete the real source latch-clear/joined retirement.

All 50 CPU contracts pass Debug, Release and Clang ASan+UBSan, including independent collision, camera, integer-boundary, flag-order, ownership/error and complete species regressions. Unchanged twelve-frame composed GPU acceptance passes Debug 17.824820 s and ASan 30.062347 s, closing 44/43/45 and 47/46/48 respectively. All five surfaces are byte-identical to each other and the already directly inspected prior surfaces. The unchanged full six-actor GPU cohort passes in 27.718846 s, closes 77/76/78 and retains both 64-sample captures and exactly 21 completed red writes. These are correctness observations, not performance comparisons or additional native species admission.

Every accepted complete tree was stable/absent before its sole initial observation: unchanged ROM and zero known physical/permission attempts. Address/UB failures stayed fatal; leak scanning was disabled only under ptrace. Exact twelve-case scopes/executable/archive identities are in [`beach-magikarp-scene-inputs.json`](../../../cmake/beach-magikarp-scene-inputs.json); source numeric/service scope is in [`actor-photo-eligibility-inputs.json`](../../../cmake/actor-photo-eligibility-inputs.json). The following cutover qualifies genuine gated membership/scheduling/GPU focus; full Item/Player services and complete port acceptance remain open.

## Native photo membership, scheduling and genuine Magikarp GPU focus

`actor::photo::collect_membership` follows actual native link-3 head/next order, not Owner slots, payloads or render bindings. It validates identical live canonical source/model bindings, applies the source predicate until twelve accepted entries, stops before visiting the tail and publishes a fresh zero-focus/null-tail Photo only on success. Earlier flag100 visibility effects deliberately survive a later invalid node. A focus-only projection avoids claiming full photograph serialization or the source's invalid pose-tail behavior.

`PhotoScene` borrows the actual scene root0/link0/priority`0x80000000`, owns the genuine prefix and CopyInfo native objects, and schedules the position-only first pass as synchronous kind1/priority2. Actual actor distance8/state7/animation3 remain independent. CopyInfo publishes completed detector computation during object update, never in GPU readback. Full pair-flag200 collision, command26 and StepWalk are not supplied by this bounded cohort.

Actor preparation collects at the included DL5/priority `0x80000001` prefix before reading postprefix flags; preprefix gates/subjects retain their old values. One shared kernel stage precedes command publication. Prefix exclusion does not collect/flip; disabled actor detection still collects and captures the baseline. A separate renderer-API fixture explicitly routes one borrowed genuine Magikarp binding to DL4 to exercise both sides; this is not an Original Magikarp DL4 claim. The main phase retains its real DL5 source wrapper and flags `0x25`, without a flag40 bypass.

The genuine source helper, native player distance process and canonical camera/collision fields feed membership. The predeclared main guard is exactly 512 native updates with GPU checkpoints 1/2/3/9/10/11/16/32/64/128/256/512. Fixed companions bring the entire procedure to 518 native updates and 32 GPU frames: source FX pairs, native hide/restore, the separate prefix fixture and a three-update physical-slot-reuse tail. Existing legacy55 camera, trigger DObj `(0,1233,4000)`, shared default RNG procedure, two slots, source headers/poses/scales/speeds/LOD, 32-full16 threshold and command/event/GPU ceilings are unchanged. The independently observed postspawn RNG is 3357800067, not a searched seed.

Actual adjacent 8×8 full16 comparisons retain negative 25/27/0 results and genuine qualifying 34/51/52/49/45 results; RGB5 is reported separately. Independent last-qualifying-candidate then membership reduction agrees with actual context/identity/flags. Native CopyInfo publishes the preceding completed computation on the next tick. Conditional completed-memory writes verify all 21 source red positions, every untouched neighboring color/depth value, hidden-memory fingerprint and disjoint guard; changed-pixel count is not forced to 21 when a source pixel is already red. Both histories lose the retired identity before another preparation; ordinary1003 reuses the same pool/physical slot with a fresh incarnation and no inherited gated focus.

| Complete actual private consumer | Seconds | GPU frames | Closed terminals / created / absent |
|---|---:|---:|---|
| Genuine native photo/controller, GCC Debug | 8.415686 | 32 | 48 / 47 / 49 |
| Identical genuine scenario, GCC Release | 6.788329 | 32 | 48 / 47 / 49 |
| Identical genuine scenario, Clang ASan+UBSan | 17.638687 | 32 | 48 / 47 / 49 |
| Byte-unchanged original common/World/FX regression | 7.595871 | 12 | 44 / 43 / 45 |
| Byte-unchanged original complete six-actor regression | 13.568454 | — | 77 / 76 / 78 |
| Real native controller lifecycle/rollback API, GCC Debug | 0.683316 | — | 1 / 0 / 2 |
| Identical lifecycle/rollback API, Clang ASan+UBSan | 2.257158 | — | 1 / 0 / 2 |

All five genuine-scenario saved surfaces are byte-identical across the three configurations; the lossless five-state contact sheet was directly inspected. It shows the actual source Magikarp, marked focus, hidden subject, retired source and ordinary reused slot over real World geometry. These are canonical framebuffer observations, not LUS presentation, Original parity, continuous full-game cadence or performance comparisons.

The real late-error fixture creates an unbound native link-3 node after GPU drain. Production preparation rejects without publishing detector Photo/context/history/focus or changing sentinel RAM. Services remains sticky-failed. A newly exposed lifecycle defect prevented all-null observer detachment solely because failure was sticky; validated live-object detachment now succeeds without healing failure, while new nonnull observers remain rejected. Native parked-process/joined-storage regression and the actual failed PhotoScene teardown pass.

The first new GPU root is wholly rejected: the consumer incorrectly requested successful `Owner::reset` after deliberately failing Services. Production correctly requires reconstruction. The correction moves healthy World/Owner retirement before the negative fixture; no source policy, camera, pose, timer, threshold or capacity changes. That failed root is never replayed, rescanned, visually inspected or partially accepted.

The separate real API consumer checks occupied-hook/nonfirst-root constructor rejection, healthy explicit retirement followed by caller-root deletion after controller storage is freed, and actual root/prefix/CopyInfo external deletion with sticky failure and safe teardown. A fixed 2-MiB native arena and 65,536-allocation guard force genuine exhaustion; one recycled GObj admits the prefix but not CopyInfo, proving partial-constructor rollback with 13,670 other raw objects still live. Every case ends with zero native objects/processes/models/cameras/matrices/nodes. Two initial whole API roots are rejected because the consumer assumed synchronous audio drain; fresh roots use the existing real retrace/build/execute/submit protocol and fixed 240-retrace checked drain. Production policies are unchanged. The temporary consumer and build scaffolds were removed after complete proof; its source remains identified inside the frozen private archives.

Every accepted complete tree was stable and all tracer/tasks absent before its sole initial physical/permission observation: unchanged ROM, zero known attempts/errors. Address/UB failures remain fatal; ptrace-only leak-scan disable is not leak qualification. All51 CPU contracts pass Debug/Release/Clang ASan+UBSan. Exact identities and scopes are in [`course-magikarp-inputs.json`](../../../cmake/course-magikarp-inputs.json), with component contracts in [`actor-photo-membership-inputs.json`](../../../cmake/actor-photo-membership-inputs.json) and [`actor-photo-scene-inputs.json`](../../../cmake/actor-photo-scene-inputs.json). Full snapshot payload/queue/reconstruction, native Item/Player gameplay, additional species/courses, product entrypoint, Original equivalence and private Windows11 execution remain open.

