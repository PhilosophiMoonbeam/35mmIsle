# Original implementation admission

## Decision and evidence boundary

This record admits candidate-scoped original ROM/native/process/queue/continuation/codec/sprite services, build declarations and synthetic fixtures; pinned MIT libultraship integration; and the separately identified private authoritative C engine candidate below. Targets remain Linux x64 and Windows 11 x64. LUS/original services replace excluded restricted SDK implementation. Admission is a material/use decision, not game qualification, blanket source relicensing, completed dependency audit or release approval. No Ghostship, Zelda, modified-fork or SDK implementation transplant is selected. The full native game remains the objective; bounded experiments do not close W00–W15.

Original services use implementation requirements, public format/algorithm facts and read-only behavior/API research in [BASELINES](../planning/BASELINES.md) and [LUS / Ghostship](../planning/LUS_GHOSTSHIP.md), including [RFC 3174](https://www.rfc-editor.org/rfc/rfc3174) facts rather than its implementation. The default build excludes reference checkouts. Opt-in LUS and selected upstream GAME definitions have distinct identities and uses below. Original authored portions fall under the root MIT grant only where contributors own the rights; adapted game bodies are not classified as original-only.

The current complete-Player component result is recorded under [Complete original Player Root](#complete-original-player-root). Earlier checkpoints retain their original source identities and limitations; they are not relabeled as later evidence.

| Admitted candidate / implementation owner | Inputs and allowed use | Required ownership and containment | Falsifier |
|---|---|---|---|
| `include/35mmisle/rom.hpp`, `src/rom.cpp`, `src/rom_main.cpp`, `tests/rom_tests.cpp` — ROM identity owner | Original C++20 streaming normalization, SHA-1 identity implementation and CLI; z64/v64/n64 content magic; full normalized size/hash check | Caller owns input stream; bounded working memory (at most 128 KiB); read-only private file access; metadata-only output; no extraction, normalization output, repair, cache or game-byte dump | Wrong identity accepted; filename/header-only acceptance; missing byte-order normalization; unchecked read failure or partial tail; game bytes written/logged; source translation |
| `include/35mmisle/native.h`, `src/native.c`, `tests/native_tests.c` — native primitives owner | Original C11 checked size/alignment arithmetic, caller-backed arena and bounded span resolution; synthetic allocations only | Caller retains backing allocation; rejected operations leave outputs/cursor unchanged; real pointers use native widths; no SDK structures, copied allocator, game heap capacities or fake pointer sentinels | Overflow before checking; pointer narrowing; out-of-bounds pointer formation; state changes on rejection; claimed high-address proof without real allocation |
| `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/native.yml`, `.gitignore` — build integration owner | Original explicit-source C11/C++20, 64-bit build and synthetic CI declarations; existing standalone tools remain host-independent | Optional host dependency is explicit and separately scoped below; no ROM-fed public jobs, private caches or uploads; immutable action/dependency revisions require evidence | Hidden source/dependency import; fake game executable; payload packaging; unsupported platform success claim |
| `cmake/dependencies.json` and this record — admission/baseline owner | Original evidence inventory for the selected standalone LUS baseline | Descriptive metadata only; no vendored implementation and no substitute for the host build's actual pin/configuration controls | Observed tag resolution presented as a complete enforced lock; unresolved package/font/transitive composition called approved |
| `include/35mmisle/process.hpp`, `src/process.cpp`, `tests/process_tests.cpp` — process runtime owner | Original C++20 stack-preserving worker and synthetic behavioral fixtures; creator-thread-only owner API with synchronous handoff; `wait(0)` inline, `wait(N)` counts subsequent resumes | Construction-time callback/thread storage; no per-resume allocation/copy; owner cancellation at ready/parked boundaries unwinds and joins; non-yielding code is not forcibly cancellable; worker starts with `FE_DFL_ENV`, retains its native FP state across waits and cannot change owner FP | Restricted-source translation; resume allocation/copy; dangling execution/storage; swallowed control-unwind exception; unproved native stack/guard/toolchain or R4300 FPCSR/denormal/trap/native mapping and Snap scheduler equivalence claim |
| `include/35mmisle/message_queue.hpp`, `tests/message_queue_tests.cpp` — queue owner | Original bounded C++20 move-only ownership queue and synthetic behavioral fixtures; explicit accepted/full/empty/closed outcomes | Fixed construction-time `optional<T>` ring under one mutex; no per-operation queue allocation/type erasure or user callbacks; payload moves/destruction run under lock and must be nothrow, nonblocking and non-reentrant; close rejects new admissions, wakes waiters and permits admitted FIFO payloads to drain; exclude/join users before destruction | Unbounded growth; lost/duplicated ownership; payload contract violation; stranded waiters; acceptance confused with task completion; queue mechanism portrayed as N64 scheduling fidelity |
| `include/35mmisle/vpk0.hpp`, `src/vpk0.cpp`, `tests/vpk0_tests.cpp` — codec owner | Independently authored C++20 bounded decoder and synthetic streams; format/behavior facts researched read-only at authoritative Snap `df8815e75f3d79618466e01e0827c6165b93d11c`, `src/sys/dma.c:86-277` and `tools/splat_ext/vpk0.py:1-42`, not copied or translated implementations | Caller-owned nonoverlapping spans; no decoder allocations; header/capacity rejection before writes; body failure may leave partial output, publish only success; checked trees/bit widths/backreferences; logical rounded-up consumed byte count permits trailing ROM data; no raw game-payload fixtures | Restricted implementation ancestry; unchecked bit shift/tree/offset arithmetic; invalid memory/pointer formation; partial output published as success; rejecting all nonzero sample methods instead of preserving their shared format behavior; synthetic coverage portrayed as actual-ROM or game proof |
| `src/host_probe.cpp`, `cmake/Libultraship.cmake` — host owner | Opt-in real pinned LUS, immutable fetch references, procedural geometry and bounded event/render loop; explicit private intro and checked dry-audio modes separately admitted below | No playable/full-scene, original cue/AI or fidelity claim; scripting/TCC/MPQ/DLL-loader and automatic vcpkg disabled; preserve upstream notices; CPU texture owners outlive GPU completion and cache release | Mocked/null renderer success; floating dependency substitution; payload publication; host submission called game fidelity |
| `continuation.h`, `continuation.cpp`, compiled-C trampoline/fixtures — C boundary owner | Original C ABI over genuine parked/joinable workers, owner rendezvous and stable failure diagnostics | C longjmp fence skips no owning C++ automatic; exceptions retained/rethrown outside C frames; callback-created children use the owner; cancellation joins before storage reuse | Cross-thread jump, C++ exception crossing C, stale callback/user storage or stranded owner request |
| `sprite.h/.hpp`, `sprite.cpp`, synthetic fixtures — asset owner | Original checked BE Sprite/Bitmap hydration and RGBA/CI/IA/I normalization from format facts | Bank ownership retained; borrowed C views need a live asset owner through renderer completion; short planes use positive actualHeight, zero uses nominal; unsupported external/YUV state fails | Nominal-row overread, unbounded graph/palette/texture access, stale view, or normalization called full RDP parity |
| `engine.h`, `engine.c`, `engine_internal.h`, `engine_math.h`, `source_factory_resources.h`, `SnapEngine.cmake`, `prepare_engine.py`, `snap-engine-inputs.json` — engine owner | Original native C ownership plus thirty-seven reviewed GAME definitions; private development/synthetic builds only | Immutable full/source/native/support hashes; narrow declared seams; private generated bodies; native sprite/camera/model sizes and complete typed teardown; separate RT/RTS placement; real unpublished object/model/parked-continuation admission with consuming-side publication and joined cancellation; no SDK build/header/body | Mutable fork substitution, hash mismatch, unreviewed definition, cross-thread C jump, premature source/native publication or reuse, published complete GAME bodies or blanket MIT claim |
| `menu_animation.hpp/.cpp`, title/World/effect callers — instance-state owner | One existing evaluator/compiled pool over immutable admitted GAME programs; fixed mutable slots with unique pool/slot/incarnation identity | Resource IDs never identify mutable instances; fresh acquisition resets active state, ordinary null/bind/reset preserves source distinctions; owner releases retiring storage only after actual joined/native/graphics/effect fences; currently finite admitted grammar | Cross-instance state/event aliasing, stale-handle reuse, per-actor program/catalog copies, allocator growth in hot operations, boolean completion claims or unimplemented actor-loop support |
| `borrowed_model_kernel.hpp/.cpp`, title/World producer adapters — model-render owner | One shared bounded hierarchy/material/pose/upload kernel over scene-owned native models | Pure metadata peek during full preflight, computing native query only at commit; actual source order and per-lane scopes; incarnation/revision/RAM-epoch residency; title and World retain distinct camera/sky/fog policy | Third renderer, native GObj/payload ownership in the generic kernel, late-reject native/RAM mutation, stale hidden-slot material branches, fake retirement or recipe54/48 approximated as RTS |
| `intro_probe.cpp/.h`, `host_clock.cpp/.hpp` — bounded native owner/visual experiment | Exact private-ROM sprite, selected original C dispatcher/fade and independent GAME-derived GTL/context policy | No procedural fallback; dedicated C creator domain, bounded retraces, exact leased command bytes, actual GPU retirement and ordered owner teardown; not the full original intro, genuine GAME photo feedback or physical clocks | Stale/foreign retirement; update driven by presentation; premature C/GPU storage release; native gameplay/audio/photo qualification inferred from the sprite |
| `required_work.cpp/.hpp`, `rsp_render.cpp/.hpp`, `rsp_packets.cpp/.hpp` — required-work ownership | Original fixed-slot preparation/execution/CPU-read protocol; one persistent shared canonical allocation and actual RSP machine | Separate optional visual admission; typed owner/epoch/serial tokens; exclusive mapped CPU writer; actual RSP before CPU/RSP flush and RDP enqueue; real GPU idle; protected CPU reader; main-only lease retirement before reuse | Concurrent writer/RSP/GPU/reader access, reset/copy masquerading as persistence, capture replay masquerading as new execution, fabricated completion or early input release |
| `pidgey_resources.cpp/.hpp`, `pidgey-resource-inputs.json` — exact immutable HD model resources | Eight pinned GAME blobs plus exact admitted private US ROM; checked hierarchy/material/DL/vertex/texture/pose-0 views | Snapshot and backing storage outlive unmoved borrows; explicit disjoint guest-addressed canonical installation; no host-pointer casts, embedded asset payload or fake external material list | Dirty generated assets, wrong overlay mapping, unframed animation stream, write outside reservations, unretired input or resource loader called a model/photo renderer |
| `pidgey_producer.cpp/.hpp` — native model command producer | Exact immutable HD resources, source-derived pose-0/type-I/material/score-state semantics, admitted GAME transform math and public GBI | Native C model/camera owners; unbaked pose; complete-retirement input lifetime; pointer+epoch/revision residency; real original geometry and dynamic segment0E; actual RSP/RDP | Bind-pose substitution, wrong parent-space pre-list, bad matrix stack/clip words, no-op material, copied geometry, stale residency or model pass called full photography |
| `rom_view.cpp/.hpp`, `graphics_task.cpp/.hpp` — shared resource/task owner | Explicit immutable guest/ROM/native domains; common cold GAME image/BSS/boot and numeric RCP task construction | Checked disjoint installation, one canonical allocation and epoch, no production fixture dependency or per-frame immutable recopy | Stale/moved owner, narrowed host pointers, duplicate boot convention, hidden reset or changed Pidgey behavior |
| `beach_world_resources.cpp/.hpp`, `beach-world-inputs.json` — Beach immutable-input owner | Exact GAME world/block metadata and admitted ROM first static terrain/sky geometry, vertex, texture and palette closure | Borrowed Snapshot; bounded selected spans and reservations; signed GAME duration; other geometry/material/path/spawn execution excluded explicitly | Dirty/generated source, whole-bank copy, unchecked callback identity, out-of-range source or partial invalid installation |
| `beach_world_producer.cpp/.hpp` — native Beach geometry owner | Source Type-A-fogged terrain, proven one-cycle sky, native camera and translate-only model math | Validate every request/transform before atomic camera computation, native model commits and RAM writes; source scissor arithmetic; complete task/input retirement | RTS substitution, partial rejection mutation, raw quarter-pixel scissor, stale epoch/cache, copied surface or terrain called complete Beach gameplay |
| `photo_feedback.hpp/.cpp`, `photo_feedback_tests.cpp` — CPU photograph counts | Original native implementation of pinned GAME packed-depth reductions | Completed-memory borrow, all twelve slots, exact Z/dZ predicates/windows/integer estimate, no allocations or plane copies | Host depth tolerance/masking, stale lease, aliasing outputs or consumer counts called Oak scoring |
| `photo_score.hpp/.cpp`, `photo-score-inputs.json` — native Oak CPU evaluation | Completed genuine feedback, original photo metadata and immutable exact-ROM GAME rule views | Direct count borrow; separate render-rank/photo-index/slot mapping; retained Snapshot; source float stages, field widths, special branches and flags | Count repacking, dropping zero-visible ranks, reordering raw records, fabricated eligibility/bonuses, clamped angle endpoint, inferred Original parity or scoring called scene/photo reconstruction |
| `photo_lighting.cpp/.hpp`, `photo-light-inputs.json` — normal preview light owner | Exact private-ROM light/setup views and GAME-derived strict binary32 direction | Three disjoint immutable-input spans, three mutable direction bytes, lighting distinct from score state | Fabricated white light, wrong FRUSTRATIO, changed matrices hidden inside light proof, early Snapshot release |
| `pidgey_feedback.cpp/.hpp`, `pidgey-feedback-inputs.json` — complete bounded depth graph | Source-ordered real subject draws, exact private GAME GPU-copy list and two camera passes | Independent instance/matrix ownership, one task per FOV, actual completed leases through reductions; inactive B visible-only zero; shared immutable material | Shared scratch aliasing, copying final C as prefix, A copied instead of rendered, per-subject clears, CPU depth copies, hidden reset or full scene/parity inference |
| `model_preview.cpp/.hpp`, `host_probe.cpp --model-preview` — actual LUS surface | New native lit-model RSP/RDP frames converted into retained preallocated presentation tiles | Exact cache-base invalidation after GL completion, retained declined frames, native/asset storage outlives host/cache retirement | Recorded-image substitution, stale texture cache, first-swap startup mistaken for visible model proof or preview called playable game |
| `item_source.h`, `item_source_bind.c`, `item_source_bridge.c`, `item_owner.cpp`, `prepare_items.py`, `item-owner-inputs.json` — native Item owner | Fifty-three exact pinned GAME definitions, native source ABI and actual immutable Item/World/Actor/effect/audio resources | Original logical entries/cursor/overall-oldest ordering, real source waits and kind1 suffixes; unpublished real factory resources before side effects; retained canonical storage through command21/22, actual native join and all external fences | Counter rollback, hidden early replacement, fake Item AUX, constructor/diagnostic rejection turned into terminal fault, NULL-payload fumes treated as logical Items, expired source/model access |
| `actor_source_owner_bridge.c`, `actor_interaction_services.cpp`, `prepare_actor_control.py` — original-C interaction execution | Complete automatic/FIFO source switches and update over the existing canonical records/shared native queue | Eleven returning service leaves; full reachable callback/resource admission before custom consumption; original row/null/boundary/order semantics, ordinary suffix preservation and C-only fault/wait transfer | Second C++ interpreter/physics, guest function casts, fabricated Player/FX/SFX providers, swallowed native fault, partial row admission or NULL-current-World rejection |
| `item_flute.cpp/.hpp`, `game_audio.cpp` — source Flute owner | Caller-retained source state and actual shared Session epoch; real private RSP microcode/owned CPU PCM | Only accepted audio builds advance the epoch; natural source flags, retained index and exact source song/command order; task/PCM owners retire before drain | Query/Item tick/retrace masquerading as build, reset of source-retained state, authored PCM or dummy-environment policy called exercised SDL3/hearing |
| `item_producer.cpp/.hpp`, `borrowed_model_kernel.cpp`, `scene_graphics.cpp` — Item graphics owner | Existing borrowed kernel, actual source ItemOwner/native models, immutable Item resources and source global DL3 | Native visibility for native pose despite paused animator; actual native object/model association for every active binding before filtering; contemporaneous replacement baseline; preparation/GPU/readback/postprocess ownership separate from native join | Cached hidden pose, duplicate-model-only negative test, stale all-hidden baseline, payload/slot reuse before real fences, API-only cohort called source-owned execution |
| `engine.h`, `engine.c`, `engine_internal.h` — native model lifetime owner | Engine-minted nonowning `IsleGameModelRef` and bit-preserving canonical local/AUX stores | Same retained game/arena lifetime; allocation/reservation/reuse remints incarnation; resolve rejects retirement/reuse before consuming association; ordinary fixed/revision/dirty publication remains authoritative | Raw-handle ABA, ref retaining a retired pose, incarnation wrap or failed preparation mutation, padding comparison, signed-zero loss or reading RT's unused input scale |
| `actor_owner_source.h`, owner/C bridges and exact movement recipe — original-C movement owner | Thirty-two additional pinned common movement/heading/turn/target/path definitions and actual native World/Items/Player binding | Original arithmetic/order/waits and partial effects stay C; fresh checked borrowed reads, latest-placement masked stores, captured model/position space, one existing walker/interpolator, separate source-ground-found bit | Shadow transform/C++ movement interpreter, implicit target reselection, stale views across waits, ground `-9000` substituted for scalar false→0, whole-path rollback or changed infinite-wait/wrap semantics |

Admission itself is not execution evidence. Independently constructed public fixtures establish only exercised mechanisms. Exact contribution content is reviewed at integration; historical observations below do not automatically cover subsequent changes. No mechanism result establishes full game boot, fidelity, audio, photography, completion, Windows 11 qualification or release readiness.

## Native movement lifetime and original-C execution

`isle_game_model_reference` / `isle_game_model_resolve` identify one actual native
model attachment, including factory preparation/cancellation and physical pool
reuse. References allocate nothing, retain no object/model and add no retirement
fence. They expire with the retained game/arena lifetime; they do not survive an
arena reset. Native identity retention never promises a live retired model.

Canonical local/AUX no-change comparisons preserve consumed float bits,
including signed zero, without comparing structure padding or reading RT's
unused caller scale. An unchanged-engine CPU reproduction dropped requested
`80000000` to `00000000` in both records and kept AUX revision1. The repaired
native engine retained `80000000` and advanced AUX revision1→2. Its fresh stopped
CPU tree was 1 terminal / 0 created / 2 absent identities, with unchanged ROM and
zero physical-device attempts or permission errors. The repaired archive includes
the exact diagnostic caller; the earlier archive omitted its then-root-level
caller, which is separately retained with that limitation. The throwaway caller
was removed after the successful proof.

All 32 added helpers retain complete original C bodies through exact pinned
body/hash/occurrence-checked rewrites. Six engine controls remain separate from
owner-dependent C. Native leaves only bridge canonical storage, actual
initialization, World/terrain/path identity, existing interpolation and published
Player/Item services. `Pokemon_GetGroundAt` returns source false→0; callers of the
full ground query still receive `-9000/up/UINT32_MAX` with found=false. Valid
terrain at height−9000 remains found=true. Command19's nullable current World is
unchanged; ResetPathPos independently consumes a valid current descriptor.

The complete extended C interaction/movement consumer passed Debug, Release and
Clang ASan+UBSan: each closed tree had 274 terminal / 273 created / 275 absent
identities, stable traces, unchanged ROM and zero physical-device attempts or
permission errors. It exercises real native waits, interleaved placement/block
changes, captured target movement/replacement/retirement/physical reuse, source
pause/arrival boundaries, bounded authored native four-kind path data,
current-path versus captured-duration behavior, permanent-wait cancellation,
heading normalization/signed-zero fallback and one-RNG circle blocked/pause/
across-wait edges. Expected terminal-fault cases inspect an actual
prefault canonical borrow while its model remains alive; normal public queries
continue rejecting sticky failure. Traced sanitizer execution is not a leak
qualification. The semantic Player fixture and authored paths are not production
Player input or ROM trajectories.

All thirteen complete shared-engine consumers passed in each profile: 39 frozen
executions, 2,370 closed terminal tasks, identical 415-file source maps, stable
traces, unchanged ROM and zero physical-device attempts or permission errors.
The final 56 CPU contracts also passed per profile; the separate sanitizer CPU
suite exercised leak detection. Exact binary/archive hashes and bounded rejected
capture history are in `actor-interaction-inputs.json`'s
`native_common_movement_wave` receipt.

Native PI/TAU/PI_2/M_PI_F/SIGN definitions remain explicitly qualified modern
policy, not established original SDK macro/type/bit equivalence. No species
callback registry is widened by these helpers. A live priority2 collision phase
can overwrite priority7 diagnostic movement before rendering; isolated CPU
expectations are not valid whole-tick GPU expectations. The rejected combined
collision/movement capture remains wholly rejected, without replay or image use.

The genuine two-slot Magikarp consumer now completes 47 actual GPU frames in each
profile, preserving its complete source512 lifecycle, focus, FX and ordinary
reuse before a separate fresh actual-source single-actor movement phase. Each
original-C MoveXZ, Jump, Turn and Scale independently changes actual pre-marker
color **and** packed depth under a same-state component-only counterfactual and
restores exactly. Checks also cover source-bound fixed-matrix RAM upload and DL
addressing, independent local nodes, descendant-world coherence and real
native/GPU/readback/audio teardown. Original camera, spawner, trigger, two-slot
capacity, priority2 and budgets remain unchanged; no tuning or partial
failed-consumer acceptance supplies this proof.

All 21 known successful saved surfaces are byte-identical across profiles.
Fourteen earlier World/FX/genuine-Magikarp/collision surfaces are unchanged; seven
additional images cover movement and retirement. The actual Debug six-panel
initial/horizontal/vertical/turn/scale/restored surface was directly inspected.
These are native component results, not production Player, complete photography,
other species/courses, original hardware/numeric equivalence or private
Windows11 runtime qualification. Published `7fe551f` passed all ten real hosted
Linux/MSVC jobs in run38998122181, including full static LUS/audio/RSP/RDP/native
engine and eight opt-in consumer builds. That receipt qualifies its exact
movement revision, not later canonical-state/Player changes.

## Canonical C-compatible World movement

`IsleWorldMovementState` replaces the former C++ state and every caller outright.
The existing World algorithm borrows that one retained C record; explicit
`IsleGameVec3` component conversion replaces vector-type assumptions. One C/C++
initializer preserves the seven existing fields and defaults. C cannot
dereference its opaque block identity; native movement resolves actual retained
World membership before callbacks, sampling or mutation. The record does not
survive World replacement. A real separate same-resource World regression proves
foreign rejection leaves both Worlds, state, callbacks and native animation
unchanged, then verifies subsequent valid carry/transition.

The C11 header check and all 56 CPU contracts per Debug/Release/Clang ASan+UBSan
passed. Fourteen complete consumers per profile passed in 42 fresh frozen
captures: 2,601 terminal tasks, identical 416-file source maps, stable closed
trees, unchanged ROM and zero physical-device attempts or permission errors.
The direct full World GPU consumer additionally completed all eight primary/
four secondary resources, seven shifts, original sampling/admission/restoration/
residency/retirement checks; each tree was 77 terminal / 76 created / 78 absent.
All 21 saved surfaces/63 profile images remain byte-identical to the preceding
movement wave. Exact binary/archive/source-set identities are in
`world-inputs.json`'s `native_component.canonical_c_state_wave`.

This is the shared state prerequisite, not production original-C Player
execution, complete Beach gameplay or Windows11 private-ROM qualification.
Published `ef98b67` subsequently passed all ten hosted Linux/MSVC jobs in
[run39000000091](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39000000091).
That receipt qualifies this exact canonical-state revision, not the subsequent
Player dependency implementation below.

## Native Player dependencies and checked C context

The retained `IslePlayerSource` supplies one C state record, shared World
movement and per-game helper context. Strict payload discrimination remains
separate from helper lookup. Original C owns nonwaiting failure guards; native
C++ leaves return before checked transfer, and positive waits reject while
such a guard is active. Local Debug/Release/Clang ASan+UBSan suites pass
57 CPU contracts, including actual C callback/resume/cold-entry and ownership
failure paths. Sanitizer checks use fatal ASan/UBSan and leak detection.

The full native cohort passes 19 complete private consumers in each local
profile: 57 executions with identical 463-file recorded source sets. The four
new Player dependency consumers exercise:

| Consumer | Exercised behavior | Explicit boundary |
|---|---|---|
| UI/screen | 43 complete message/Icons/film C functions, 15 transition C functions; real ASCII/EUC-JP glyphs, scale/alpha/shadow/controls, FIFO and joined deletion, film bounds, source camera/viewport, solid/offset shapes, three delayed worker waits, actual RSP/RDP color/depth and graphics/readback retention | Authored film count is a roll-boundary diagnostic, not captured photos; no complete Player FSM |
| Photo | 24 complete C functions; 64 actual GPU Frames, two failed completed callbacks and 73 original-C CopyInfo updates per profile; exact Frame/ticket/context, ordered publication, short borrow, failed-work quiescence, World/Actor/Item/FX snapshot, 60-photo exhaustion/reset/selection, explicit big-endian codec | No production input-driven shutter, full replay/scoring scene or independent Original equivalence |
| Bump | Five complete C functions; genuine Magikarp129/factory802cac48 radius56, exact 90/60/30/5 boundaries, source tangible/flag20 gates, actual command4 FIFO, default dispatcher, queued then canonical Actor-field sender retention, joined deletion/reinit and audio-RSP/PCM drain | Explicit bounded C receiver, not the production Player interpreter; no graphics/device claim |
| Native camera | Actual Catalog camera index115/guest803a80d0, mask0x277/end1860; matrices3+14, Set/one Update, NULL/same-clip/new-skip, speed/tracks/flag2, independent analytic samples, fresh written-channel merge, typed errors and sequential owner retirement | Sole admitted title clip, not every course-camera program; no GPU/audio/device claim |

Every accepted case was frozen before launch. Its sole initial closure
established all traced tasks terminal and absent, stable trace identities,
unchanged private ROM, zero known physical-device attempts and zero permission
errors. The complete cohort has 3,297 terminal tasks, 3,240 created tasks and
3,354 absent tracer/task identities. Its recorded source-set SHA-256 is
`d131b6e8264bf52389604583e73ce863a08e0fef2db528e31dc69510ebb54419`
(sorted compact JSON path-to-SHA-256 map). All 21 previously saved surfaces
remain byte-identical across the three current profiles and the published
canonical-state baseline. All 71 explicit Photo color/depth, sixty-roll,
application-reset and failed-publication artifacts also match across profiles.
Original glyphs, the source-kind-1 signed-offset aperture and the native
World/Actor/Item/FX Photo surface were directly inspected.

Two failing-before/passing-after defects were corrected without weakening
assertions or deadlines: the UI generator had inserted a status return at every
matching closing brace instead of only the function suffix; the Photo harness
collected Items before retiring Actors whose canonical fields still held them.
The latter now attempts both real retirements before both collections and
checks every outcome. Failed captures remain rejected, never partially admitted.

Actor construction now prepares storage without publishing FX helpers;
`publish_effects()` performs their explicit native-lifetime publication.
Environment construction likewise prepares storage without publishing its
listener/list/update process; `initialize()` performs the original operation,
including distinct repeated initializations. Setting an Actor Player binding
does not initialize Environment state. Particle and emitter callbacks consume
their own native object's high-16 pause mask independently; a real
failing-before/passing-after owner regression covers frozen/resumed clocks,
particles and RNG.

Photo's concrete Frame methods occupy a separate static-library object, with
the actual renderer dependency declared when available. An RSP-enabled,
RDP-disabled full build and both complete core Actor/Photo collision and Actor
owner consumers pass without fake Frame implementations or per-executable
link workarounds.

The same source snapshot passes all four actual LUS-window startup scenarios:
Start-interrupted title/profile reentry, natural title completion/reentry,
live-VI reset/cold reload, and name entry through Oak's confirmed course-0
request. They close 459 terminal/455 created/463 absent tracer/task identities,
with unchanged ROM and zero known physical-device attempts or permission
errors. Rendering uses CPU Mesa; input is explicitly software-owned, and
actual audio-RSP PCM reaches a checked, drained SDL dummy stream. This is not
physical input, hardware/hearing qualification or Beach gameplay.

At this earlier checkpoint, the complete 106-function Player migration and its
39 one-argument callbacks remained private authoring, not an admitted Player
target. The later complete-Root component result below supersedes that status;
all reachable course callbacks and full Windows11/Linux gameplay remain open.
Published `a248e40` subsequently passed all ten hosted Linux/MSVC jobs in
[run39012104538](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39012104538),
including explicit compilation of all nineteen opt-in consumers on Linux and
the full static-runtime MSVC host. No ROM or private generated assets enter
public jobs. This receipt qualifies that exact dependency revision, not later
local canonical Actor/Item changes or private Windows11 gameplay.

## Canonical Actor/Item Player association

Published `05b2616` removes `PlayerState`, `PlayerBinding`, their setters and
the independent Course pause/execution fields. Actor and Item
owners require the same active `IslePlayerSource&`; all affected native and
diagnostic consumers use that association. Construction prepares storage
without publishing Root/model. Item initialization and actual consumption
require the published native identities. Current AUX, camera eye and
`cam_target` are consumed directly, not reconstructed from Player history.
The imported `Vec3f` type is the canonical `IsleGameVec3`, not a layout cast.

`IsPaused` gates source counter bookkeeping; the distinct execution predicate
is `IsPaused == 0 && D_80382D44_523154 == 0`. Neither Boolean transition
implicitly resumes suspended processes. Focus timer/direction come from Source;
interaction focus identity remains the Detector's published object even when
presence is false. Exact pinned Player cleanup clears prior pointer/latch
only when latch and identity match; it does not reset the focus timer.

Debug, Release and Clang ASan+UBSan each pass 57 CPU contracts and nineteen
complete private consumers. The 57 accepted cases share an identical
464-file source set, SHA-256
`d1a6cc7b947b7e9aa84a046c71fae1102ed31bd52733de7b2df26db0549385a5`.
Their sole initial closures establish 3,313 terminal tasks, 3,256 created tasks
and 3,370 absent tracer/task identities, unchanged ROM and zero known
physical-device attempts or permission errors. All 21 existing surfaces and
all 71 Photo files per profile remain byte-identical across profiles and both
preceding qualified snapshots.

One parent launch incorrectly precreated Photo's required-nonexistent output
directory. That entire early-admission failure remains rejected. The unchanged
binary passed in a fresh root after correcting only the launch precondition;
fifteen already accepted cases were neither replayed nor rescanned. This
cutover is not complete Root106 execution, production input-driven gameplay,
independent Original equivalence or private Windows11 runtime qualification.

That immutable revision passed all ten hosted Linux/MSVC jobs in
[run39014168220](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39014168220),
including the full static-runtime LUS/audio/RSP/RDP build and nineteen
then-current opt-in consumers. Later changes require their own evidence.

## C shutter, published focus and cart numeric resources

The C shutter transaction resolves the actual published ticket for
`TakenPhotoIndex`, borrows, executes original `makePhoto`, unconditionally
releases, and returns before Root checks failure. Published-focus queries read
the Detector's actual publication; presence clearing preserves its identity,
flags, context and staged work. Nullable Actor lookup returns OK/NULL for
genuine absence, but does not swallow foreign-identity, ownership or native
fault errors. Required Actor lookup retains its existing contract.

A completed Photo context can retain an Actor identity after native deletion
and join. Nullable lookup accepts that retained, fully joined identity as
absence without dereferencing its former payload. Unheld, foreign or corrupt
identities remain errors; required lookup remains strict. The regression uses
a real C continuation, deletion, join and identity hold. The whole Photo
consumer also exercises the C lookup against a subject verified in its actual
retained completed context. It does not assume that subject is currently
published as a live focus or claim a positive GPU-focus result.

Cart resources contain 43 nodes, eight materials, 78 model programs, twelve
material programs, two Beach camera programs and six paths. Only actually
reached path commands extend the shared animator; there is no second interpreter.
Independent Snapshot DATA decoding and private, provenance-pinned GAME C
numeric functions supply expected trajectories. Generated original C stays
under the build root; no SDK source/header/layout, ROM code or embedded asset
payload is admitted.

Ideal-double Hermite is not the source-numeric contract. At intro time28.5,
the camera-at parameters differ by two binary32 ULPs; the source's
tolerance-terminated inversion selects neighboring `2^-17` cells. Their Y
difference is `0.0343017578125`, exceeding the unchanged `0.002` threshold.
The source reference reproduces the actual native result. A separate executed
numeric probe also distinguishes wrong path binding, cubic coefficient and
control-point data. Production interpolation/inversion was not changed to
make this comparison pass.

At commit `3e47f6d61e2a625b3ab654c77cead4f5fec1c428`, all three profiles
passed 57 CPU contracts and twenty whole private component scenarios:
sixty cases over one 473-file recorded source set,
`6a68fd6c3402c0374a832db6be042015ae053d6e796d773e1749f4b2a938390a`
(SHA256 of compact, key-sorted JSON path→SHA256 records).
Their sole initial closures establish 3,318 terminal tasks, 3,258 created tasks
and 3,378 absent tracer/task identities, unchanged ROM, zero known physical
attempts and zero permission errors. All 21 prior surfaces/63 images and all
71 Photo files per profile/213 files remain byte-identical to the preceding
qualified snapshots. Current World and Photo surfaces were directly inspected.

This cohort includes the final cart behavioral assertions and the reviewed
joined-Actor fix. The already-qualified whole Debug Photo capture was retained
without replay or rescan; the other 59 cases used fresh captures. Earlier
frozen archives retain their original source identities, not this later
snapshot. Incidental default-value and immediate setter/getter assertions were
removed rather than repinned; full trajectory, skip, END and control behavior
remains exercised.

The cart probe deliberately tests retained rebinds; actual Beach outro creates
a fresh cart. Resource-clip completion is not the original intro's 290-iteration
wait/skip/blend lifecycle. At this immutable resource checkpoint, complete cart
ownership, Root106 execution, remaining species, full gameplay and private
Windows11 runtime remained unqualified. Later Cart Owner evidence is recorded
separately below.
Published `3e47f6d61e2a625b3ab654c77cead4f5fec1c428` passed
[all ten Linux/MSVC jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39021360675),
including explicit compilation of all twenty consumers without a ROM and the
full static-runtime Windows LUS/audio/RSP/RDP/engine build. This immutable
compiler/component evidence does not qualify subsequent camera changes or
private Windows11 gameplay.

## Native Item and complete C interaction execution

Current implementation extends the prior shared-source wave; earlier bounded
receipts remain historical, not claims that the new interpreters or Items are
unimplemented. Authority remains GAME
`df8815e75f3d79618466e01e0827c6165b93d11c`. Generated game bodies stay private.
No SDK producer/header/layout/disassembly or second C++ interpreter/physics is
admitted by this change.

`Pokemon_RunInteractions`, `Pokemon_HandleCommand` and `Pokemon_Update` execute
the actual automatic/FIFO source logic. Real World/Item/Audio/Detector/native
ownership supplies all eleven returning interaction services. Six compiled
common/Magikarp callbacks are not the complete Beach callback registry.
Caller-retained typed nonNULL graphs execute only with their full reachable
native callback/resource closure; all 71 cataloged graphs are not thereby
executable. Production Player and the remaining species stay open.

`items::Owner` compiles all 53 selected Item/helper definitions. Twenty logical
entries, cursor0–19/IDs300+i, source overall-oldest threshold ordering,
NULL-payload fumes, source waits and real command21/22 ownership survive the
cutover. Actual parked native workers/object/model storage, Services records
and Runtime admission are prepared without publication; source code consumes
them at its original sites. Capacity rejection does not replay/undo counters,
RNG, cache/list mutation or early deletion. Native terminal status is distinct
from retained diagnostic text; failed teardown still performs real joins and
retains semantic/native identities until genuine release.

The complete three-profile runtime matrix is recorded in
`cmake/actor-interaction-inputs.json` under `native_full_C_Item_wave`.
Every row below passed unchanged whole-consumer acceptance in GCC Debug,
GCC Release and Clang ASan+UBSan. Counts are terminal trace files / created tasks /
absent tracer-and-task identities; each profile has the same tuple.

| Complete native consumer | Whole closure in each profile |
|---|---|
| ActorOwner source CPU/lifetime | 28 / 27 / 29 |
| Canonical collision/source-lifetime CPU | 37 / 36 / 38 |
| Shared source command26 | 33 / 32 / 34 |
| Magikarp source owner CPU | 25 / 24 / 26 |
| Magikarp scene chooser CPU | 8 / 7 / 9 |
| ItemOwner, all seven scenarios | 37 / 36 / 38 |
| Actual C automatic/FIFO selectors and services | 212 / 211 / 213 |
| Flute, all nine actual RSP/PCM runs | 1 / 0 / 2 |
| Existing ActorProducer GPU/focus | 77 / 76 / 78 |
| Composed World/common-source GPU | 44 / 43 / 45 |
| Genuine two-slot Magikarp course GPU | 48 / 47 / 49 |
| Canonical pair-collision GPU | 45 / 44 / 46 |
| Actual ItemOwner mixed-owner GPU | 127 / 126 / 128 |

All 39 roots had one stopped-tree closure/initial scan: stable trace identities,
all tasks absent, unchanged ROM and zero known physical-device attempts or
permission errors. No rejected root was replayed/reopened/rescanned or partially
accepted. The CPU suites passed 56/56 in each profile. ASan/UBSan remain fatal;
only ptrace-incompatible LSan is disabled for traced private runs, so this is not
a leak-qualification claim. Fourteen saved genuine World/Magikarp/collision
surfaces were byte-identical across all three profiles; a current completed
World/FX surface was directly inspected. No camera, pose, source cohort,
threshold, timer, viewport or existing GPU frame budget was tuned.

Three exposed boundaries were corrected without weakening native contracts:
valid NULL-current-World command19 equality now uses status plus nullable output;
native-pose visibility reads current native HIDDEN even when animation is paused;
all-active render admission checks actual native object/model association before
hidden/pass filtering. The GPU negative oracle uses a unique foreign same-topology
model, and replacement comparisons exclude only the current replacement at the
same completed state. A migrated ActorOwner fixture now retains its actual joined
semantic owner until re-adoption/final22 delivery, rather than collecting and then
using an expired raw handle.

A separate actual initialized-bank investigation confirmed Item bank3/script0
velocity `(0,6,0)`, emitter lifetime20 and particle lifetime30. Over the
source-derived 52 updates, position-only publication matched the decoded-velocity
reference in all 850 observed particle trajectories and RNG progression; the old
zero-velocity override differed in all 850. The bank was not modified and no
authored velocity substituted. The throwaway probe was removed after its frozen
CPU proof.

Each Flute profile executes 828 actual audio RSP tasks with exact direct-song and
retained-index restart PCM plus nonzero muted-counterfactual differences.
Transport evidence is checked owned CPU `private_audio::Capture`, not an exercised
SDL3 device/stream; dummy-only environment policy alone proves no SDL transport.
Graphics use actual RSP plus qualified CPU Mesa/Vulkan RDP. These results do not
qualify physical devices/hearing, Original numerical parity, all species/courses,
production Player, complete photography/save flow, a playable full game or private
Windows11 execution. Hosted compile evidence remains tied to its named immutable
revision; current workflow additionally declares the four Item/C consumers.


## Private development input

The user confirmed a lawful private development ROM and authorizes extraction, reference/rebuild research and capture development for the native port. The integration owner observed normalized US SHA-1 `edc7c49cc568c045fe48be0d18011c30f393cbaf` and 16,777,216 bytes. This is supplied evidence, not a new ROM read by this author. Private ROM availability is not a missing prerequisite. Matching the identifier proves revision identity only, not acquisition rights. No private path or payload is needed in public records or synthetic fixtures.

This private-use authorization includes identity checks, extraction, game-fed development, reference rebuilds and comparison capture within the private environment; it is not a third-party redistribution grant or blanket MIT determination. ROMs, derived assets/archives, binaries, captures, caches, traces and save data stay private, outside public fixtures/CI/uploads. Actual source imports retain per-candidate provenance/use review under [LICENSING](../planning/LICENSING.md). Restricted SDK implementation is not imported into the port; LUS/original services replace it. No acquisition or upload service is requested.

## Superseded global block; actual import and release decisions

The former whole-roadmap external-permission prerequisite `bd_35mmIsle-v4u.1.3` is superseded. Original services, licensed LUS and authorized private ROM work proceed. Actual game-derived candidates remain scoped: thirteen definitions from authoritative `src/sys/om.c` and `src/AA0650.c` are now selected for private native compilation below. `src/sys/audio.c` blob `a617f2ee04ad0851e0db46d31a6cf034e1eb0355` and SDK `ultralib/include/PR/os.h` blob `5de240174869c0b672716ba5c5bbea4ad2e4ee91` remain unselected. The observed SGI proprietary notice and absent repository-wide grant do not prohibit every intended use, nor authorize redistribution. The modified local fork remains preserved and is not source authority.

For an actual proposed Snap routine, local-fork derivative, reconstructed table, RSP program or other imported material, **I1** records exact repository/full commit/path/span and content hash, modifications, authors/history, copied versus original portions, governing notices, generated inputs, destination and intended use/distribution/linkage. Private extraction/reference outputs stay contained under the admitted private use; placing them in a public source tree or package is a separate import/release decision. Snap behavior authority remains `ethteck/pokemonsnap@df8815e75f3d79618466e01e0827c6165b93d11c`, not the local fork.

**I2** requires a rights/use basis for the exact candidate: original authorship, an applicable dependency license, the admitted private development use, or a separately reviewed third-party import. Missing, incompatible, proprietary or ambiguous import rights require scoped rights-holder permission or qualified review, or rejection of that import. The user's direction authorizes this development scope but cannot grant third-party redistribution rights. Snap `ultralib/include/PR/os.h` and restricted SDK implementations remain excluded; renaming/translating them is not independent replacement. None of that blocks genuinely original services or licensed LUS work.

Any source/tooling/host or game executable release separately needs **R1/R2** composition review: exact source/dependency/artifact identities, embedded game/SDK/font/vendor content, linkage, source offers and notice delivery, plus payload/secret exclusion. An asset-free package is not automatically approved. The pinned MIT LUS host integration is admitted for development; its static-library behavior does not imply all dependencies are static or all embedded components are cleared for release.

## Frozen local-fork relevance screen

The comparison is authoritative Snap commit to the existing tracked working state at local HEAD `b35d6de06edd43aa842fdd3d03175967dfecfbea`; it excludes untracked files and leaves the reference worktree untouched. Of 178 changed paths, 9 deterministic exclusions leave **169 units** screened with a 24,000-character cap. There were **4 truncated units**, **21 flags**, **17 directly reviewed nontruncated units: 13 confirmed and 4 corrected**, and **148 unread/unflagged units** (not hand-read). **0 transplants** were selected. This is a relevance screen, not a complete inventory, correctness audit, source-completeness result or rights clearance.

The rubric was frozen before loading diffs. Unit: one tracked source/build/document delta from the authoritative commit to the preserved working state. Deterministic exclusion: extensions other than `.c`, `.h`, `.cpp`, `.cc`, `.cxx`, `.hpp`, `.s`, `.S`, `.py`, `.sh`, `.cmake`, `.json`, `.yaml`, `.yml`, `.toml`, `.md`, `.txt`, unless named `Makefile` or `CMakeLists.txt`. The independent boolean asked whether the diff explicitly showed proprietary/restricted notices or added binary/encoded game payload.

| Primary label and observable criterion | Raw verdicts |
|---|---:|
| `native_port`: all substantive changes are desktop/native ABI or host operations | 0 |
| `original_game`: all substantive changes are original N64 behavior/recovered definitions/reconstruction semantics without desktop runtime operations | 152 |
| `build_tooling`: all substantive changes are development tools, reconstruction configuration or documentation | 16 |
| `mixed`: substantive changes span at least two of native runtime, original game and tooling | 0 |
| `other`: empty, opaque, insufficient or outside the other categories | 1 |

Escalation selected `native_port`, `mixed`, `other`, top probability below 0.70, explicit-restriction probability at least 0.30, errors or truncations. There were zero judge errors. Raw labels are relevance evidence, not correctness or permission; only the rows below received direct nontruncated review.

The four unresolved, explicitly unadmitted paths are `continue.md`, `docs/decompilation-matching-notes.md`, `splat.yaml`, and `tools/symbol_addrs.txt`. The historical 102-dirty-entry observation in BASELINES is a different measurement, not replaced by this comparison. No row below selects code for import.

| File | Reviewed kind / disposition | Direct evidence |
|---|---|---|
| `src/29A190.c` | Original game; confirmed | Removes unused river BSS declaration. |
| `src/4A8160.c` | Original game; confirmed | Removes unused Rainbow Cloud BSS declaration. |
| `src/A74E0.c` | Original game; confirmed | Removes unused Beach BSS declaration. |
| `src/app_level/icons.c` | Original game; confirmed | Moves sprite/BSS declarations into generated-layout includes; no desktop operation shown. |
| `src/entry/entry.s` | Original game; confirmed | Deletes MIPS BSS-clearing entry without an evidenced desktop replacement. |
| `src/more_funcs/md4.c` | Original game; corrected from `other` | Changes global context definition to `extern`. |
| `src/rsp/aspMain.s` | Original game; confirmed | Deletes extracted audio-ucode wrapper, not a native synthesizer. |
| `src/rsp/gspF3DEX2H.NoN.fifo.s` | Original game; confirmed | Deletes triangle-ucode wrapper, not a native renderer. |
| `src/rsp/gspL3DEX2H.fifo.s` | Original game; confirmed | Deletes line-ucode wrapper, not a native renderer. |
| `src/rsp/rspboot.s` | Original game; confirmed | Deletes RSP boot wrapper without native execution. |
| `src/tunnel/world/world.c` | Original game; confirmed | Removes unused Tunnel BSS declaration. |
| `src/window/83D730.c` | Original game; confirmed | Asserts original 32-bit pointer/layout offsets, not native x64 admission. |
| `src/window/button.c` | Original game; confirmed | Removes a duplicate image-list structure declaration. |
| `docs/drawbitmap-window-donor-equivalence.md` | Tooling; corrected from `original_game` | Matching/reconstruction research, not desktop execution. |
| `src/window/845160.c` | Tooling; corrected from `original_game` | Adds explanatory sprite-traversal comment only. |
| `src/window/element.c` | Tooling; corrected from `original_game` | Adds explanatory sprite-traversal comment only. |
| `tools/splat_ext/vpk0.py` | Tooling; confirmed | Adds Splat extraction/linker-manifest processing. |

## Selected baseline and invalidation

[Selected dependency evidence](../../cmake/dependencies.json) identifies standalone LUS `759b13401cd6903daa67a00a753066f791622e78`, selected host pins and observed Linux package versions, not Ghostship's unaudited nested gitlink. [Libultraship.cmake](../../cmake/Libultraship.cmake) enforces the exact clean LUS git pin, exports committed bytes to a build-private snapshot, applies the scoped adaptation there, pins selected fetched dependency commits and verifies the stb digest. Each configured host build writes `isle-host-dependencies.txt` with selected inputs and patch/archive digests. Those controls are not a fully locked system-package composition or release approval. Package content identities, embedded font/header terms, transitive notices and final shipped composition remain candidate-specific release work. Default tools and original services remain independent of LUS; the opt-in host links it.

Reduced build options and host geometry are not Snap compatibility proof. `GBI_UCODE` was observed PRIVATE at the selected pin: consumers must explicitly agree with library/exporter definitions, vertex representation and runtime decoder. Earlier static inspection identified missing `G_MW_MATRIX` handling in the F3DEX2 moveword handler, no implemented Snap `G_SPECIAL_1` operation, and no L3DEX decoder entry; this was not a complete opcode audit or proof that no special operation is needed. Ghostship's SM64 flags, audio parameters and graphics-call assumptions are not Snap selections. Admitting a real host probe does not resolve those game-rendering obligations.

Any source, fixture, public algorithm input, API, pin, patch, package, option, ABI, target, embedded-data, input-use or linkage change invalidates the affected admission/proof and requires renewed provenance, compatibility, notice and behavioral review. Discovery of restricted ancestry, payload leakage or a violated containment rule suspends that material's admission. Historical research observations remain historical; unproved assumptions stay unproved.

## Historical subsequent orchestrator verification

Original source/build identities are frozen by [commit `d8042466625e1ca2314c9e9883fa24e5c93760b4`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/d8042466625e1ca2314c9e9883fa24e5c93760b4). The following checks were actually exercised after implementation; they do not retroactively change the research author's evidence above.

| Exercised surface | Observed result and scope |
|---|---|
| Local GCC 15.2, Linux x64 | Debug and Release configured/built without warnings; both synthetic CTest executables passed in each configuration. |
| Local Clang 21.1.8 | Debug ASan+UBSan configured/built; both synthetic executables passed without sanitizer findings. |
| Actual identity CLI | Exact private US z64 and in-memory v64/n64 equivalents accepted with the canonical digest; seven malformed/corrupt/size/magic cases and three argument/missing-input cases rejected. No private image was written as a fixture. |
| Read-only access | `strace` on the real identity command observed zero file-write/mutation opens or operations in the selected syscall set. |
| Native C/C++ boundary | A throwaway C++ consumer linked the C11 library and exercised real storage above 4 GiB, aligned allocations, span reads/writes, unchanged rejected state and reset. |
| Hosted native matrix | [Workflow 38651218858](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38651218858): all six Linux GCC/Clang and Windows MSVC Debug/Release jobs passed build, synthetic tests, actual CLI, installation and local tooling packaging. Windows also read/rejected a synthetic Unicode-path input. The Windows runner is Server 2022, not Windows 11 qualification. |
| Linux tooling package | Generated TGZ contains exactly the checker executable and matching original MIT license; the unpacked checker accepted the private US identity. SHA-256 `84409701b0b5361c13109355659aa67e580189560794cf414b6267fd8e45130b`. It is not a game package and was not published as a release. |
| Static review | Independent ROM, native-memory and build reviewers found no material findings; this is not legal clearance or game fidelity. |
| Documentation/tracker | Fifteen Markdown documents rendered and 102 local links resolved; all sixteen WID acceptance fields and thirty-four WID edges matched live Beads. Scoped lint passed eight affected records. Global lint reported four unrelated artwork-task template warnings, left untouched. |
| Reference preservation | All 169 screened tracked deltas matched their before-work fingerprints; no source transplant was selected. |
| Diagnostic limit | LSP was requested but no server is configured. Compiler, sanitizer, test and runtime evidence above is separate from LSP diagnostics. |

Private trace data and throwaway consumer code/binaries were removed. Public CI received no ROM, extracted resources or captures and uploaded no artifacts in this historical verification. None of these checks ran the native game, an original reference/oracle, renderer/photo scoring, synthesis/device output, persistence, progression or clean-machine game packages; none verifies this wave's process/queue/host changes. W00–W15 parent gates remain unclosed. The former global external-permission block is superseded; actual future import/release decisions and all implementation/qualification obligations remain.

## Subsequent original runtime and Linux host mechanism evidence

The integration owner supplied the following observations for the preceding runtime/host wave, committed as [`9a875dd58cd73042506b2aa4de58f32c4102d511`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/9a875dd58cd73042506b2aa4de58f32c4102d511). They are separate from the historical first-wave checks above and do not cover the subsequent VPK0 decoder or newly declared host CI job. This author did not rerun these checks.

| Exercised surface | Integration-owner observation and limit |
|---|---|
| Original runtime, Linux x64 | GCC Debug and Release and Clang ASan+UBSan each passed all four synthetic CTest executables then present. This covers native, ROM, process and queue mechanisms, not the later decoder. |
| Published runtime toolchain CI | [Run 38655451052](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38655451052) passed all six cells at `9a875dd`: Linux GCC/Clang Debug/Release and Windows MSVC x64 Debug/Release. It exercises the four original native/ROM/process/queue executables, CLI/Unicode-path handling and tooling install/package, not the later decoder or new LUS CI cell. Windows Server 2022 is not Windows 11 qualification. |
| Process/queue ownership and scheduling mechanism | Independent reviews found no material findings. A real combined consumer exercised exact scheduled waits, FIFO ownership transfer and close/drain. The process is a serialized native-thread stackful mechanism; the queue is a bounded ownership ring, not an N64 priority scheduler. No result establishes original Snap scheduler/FP equivalence or native stack/guard admission on every target. |
| Real pinned LUS host | Compiled and ran 240 frames through SDL3 3.4.2, Xvfb, Mesa llvmpipe and desktop OpenGL via explicit SDL3 EGL (`SDL_VIDEO_FORCE_EGL=1`, `LIBGL_ALWAYS_SOFTWARE=1`). The integration owner observed red/green/yellow/cyan geometry on dark blue in frame 20 of a private development video. No ROM/game assets were inputs. |
| Submission versus pixels | The first captured frame was gray GUI startup. `FIRST_SUCCESSFUL_RENDERER_FRAME` records successful procedural display-list submission, not visible pixels or game fidelity. The later visual observation is separate evidence; logs alone are not a pixel assertion. |
| Host ownership teardown | `HOST_OWNERS_RELEASED` was observed before exit: weak lifetime witnesses for window, GUI, control deck, resource manager and worker pool expired. This bounds those selected owners; it is not a universal leak/sanitizer or clean-machine package claim. |
| Platform/backend limit | Default GLX context creation failed on the development host; only the explicit software-Mesa/Xvfb/EGL path above is qualified by this run. There is no Windows LUS host-probe pass or Windows 11 qualification. The separate published runtime CI covers process/queue toolchains, not the later decoder or host. |

The host dependency setup required OpenGL **and GLES2 development headers** as well as SDL3 and the declared system packages, even though `USE_OPENGLES=OFF` selects desktop OpenGL. User-prefix preparation stayed outside the source checkout. Existing upstream warnings were not suppressed.

### Corrected immutable-pin host adaptation

`cmake/patches/lus-host-lifetime.patch` was regenerated from committed LUS bytes at the exact immutable pin, with normal three-line unified context, not from the mutable reference worktree. At this pin, `Ship::ControlDeck` is the owning base of `LUS::ControlDeck`; the adaptation therefore releases caches in the `ship/controller/controldeck` paths, not an assumed subclass location. The build exports fresh git-archive bytes into its own snapshot, unsets git worktree/index environment overrides, and applies the patch outside a containing worktree so paths cannot silently target the parent checkout. Reference checkouts remain read-only.

The scoped adaptation and original host owner release GUI/control-deck/resource/interpreter/backend owning references after final use, shut ImGui down while the OpenGL context exists, and reject actual SDL/window/context initialization failures rather than presenting a null backend as success. It is a development host-lifetime adaptation, not game behavior, general multi-context support or an upstream SDK transplant. The native probe mounts only the pinned renderer's OpenGL shader resources and preserves its MIT notices; no game archive is substituted.

### Current decoder verification and CI declaration

The root build includes explicit-source `isle-vpk0` and `vpk0-tests`, preserving existing libraries, native thread linkage and process FP settings. GCC Debug/Release and Clang ASan+UBSan each passed all five synthetic executables. Checks remain active in Release. The decoder uses caller-owned nonoverlapping storage without allocations; body failures may leave partial output, unpublished unless successful.

Strict GCC and sanitized Clang throwaway consumers each matched all three recognized private-ROM streams against authoritative `tools/vpk0_codec.py` at Snap `df8815e75f3d79618466e01e0827c6165b93d11c` (Git blob `ae2d7adcf9ec5ac93c3257533c6656af47f0aad4`): exact consumed lengths and SHA-256 output identities, totaling 1,026,256 output bytes per build, all sample method 1. Removing the final consumed byte rejected each stream without publishing output. The reference source and ROM were read privately; no payload fixtures were added.

Independent review found a valid zero-length-token incompatibility: distance validation ran before length was known. A zero-distance/zero-length token followed by literal `A` failed before the correction and passed afterward. The decoder now consumes zero-length tokens without forming an output pointer; nonempty copies still validate distance and extent before copying. Permanent synthetic regressions cover zero, forward and out-of-range distances on empty copies. Both consumer builds also exercised these cases. Own Clang fixture signed-byte warnings were corrected rather than suppressed.

At [`b5838c615871df8a9b46d0357bf22b47fad8b2dd`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/b5838c615871df8a9b46d0357bf22b47fad8b2dd), [run 38657047849](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38657047849) passed all seven jobs: Linux GCC/Clang Debug/Release, Windows MSVC x64 Debug/Release and the actual Linux LUS/SDL3/Mesa EGL procedural host. The five synthetic tests then present passed; the host required bounded 240-frame submission and owner-release markers, not pixel assertions. Actions are immutable-pinned; no ROM, capture or artifact uploads. System images/packages remain version-variable. Server 2022 CI is not Windows 11 qualification. CPack remains checker/license only.

The CI review identified Ubuntu libzip's exported `zipcmp`, `zipmerge` and `ziptool` targets as separate required packages; the successful host recipe includes them. Prior five-test CI does not cover the later continuation/sprite/engine contribution; its separate result follows below.

I1/I2 remain **candidate-scoped import/use decisions**, never a global stop on original services, the licensed LUS development integration or authorized private extraction/reference work. Full game implementation and W00–W15 behavioral/platform/release obligations remain open; none of this evidence is game completion.

## Native C engine, typed assets and private visual evidence

The selected source is authoritative Snap `df8815e75f3d79618466e01e0827c6165b93d11c`, not local working files. [`snap-engine-inputs.json`](../../cmake/snap-engine-inputs.json) is the exhaustive allowlist. Its initial thirteen definitions comprise twelve OM definitions from `src/sys/om.c` (blob `4ca82a764345116a8c924fb3449e4638e0720857`, SHA-256 `6908d51d68f90b27e3cc0fb795eea0602c7d5d713df1b0d6776719e906420f8c`) and the selected fade from `src/AA0650.c` (blob `d6263d3d6e1ff1f913e7620d402ed70c4d399acc`, SHA-256 `62e01867df443487283155a3c8d07d01c4922b7425c3788d2033ec4ef2ad43bf`). Camera computation expanded admission to twenty-one spans; the model/photo transform extension below brings it to twenty-six across the same four GAME source/header blobs.

Per-definition spans and original/native hashes, intended private use and exact transformations are recorded there. Seams replace OS start/receive with the native continuation resume, remove a matching-only self-assignment, const-qualify the borrowed sprite descriptor/add prepublication allocation failure, and explicitly preserve fade integer conversion. No complete body is public. The generator validates immutable full files, Git blobs, ranges and transformed contents before publication in the build directory; it neither invokes the upstream build nor compiles SDK headers/implementation. This admitted private/synthetic use supplies no blanket public game-body or game-binary redistribution grant.

| Exercised surface | Observed result and limit |
|---|---|
| Native C consumer | Eight tests passed under GCC Debug, GCC Release and Clang ASan+UBSan; strict standalone C11 `-Werror` compilation passed for original engine services and generated definitions. Actual upstream traversal executes native priority/current-object, nested waits, pause, callback child creation, cross-cancel/join, kind-0/kind-1 termination, deferred deletion, failure fences and cold reset. |
| Consumer-visible corrections | NULL/current-object ID lookup failed in a real compiled-C callback trace before correction. Exact intro hydration exposed a short-plane overread missed by synthetic-only review; corrected stored extent and permanent boundary/alias/lifetime/shuffle regressions pass. Neither defect was hidden by changing an acceptance threshold. |
| Actual original fade | Compiled C four-update helper produced RGB 191, 127, 63, 0 with alpha unchanged. This is the selected helper, not complete intro initialization or audio. |
| Actual private native visual path | Exact ROM snapshot → decoded bank → native Sprite/Bitmap graph → original C GObj/SObj/dispatcher/fade → real LUS textured commands. A 120-frame/119-update SDL3 EGL software-OpenGL run exited zero; the parent visually observed original centered sprite, fading RGB and completed black state. C users quiesced before GPU completion/cache/host release; asset owners released afterward. Private video SHA-256 `0d9e7502c89e2af4e242ebe9a3ea6cf115e80561ba49a81aa800b9d0aadefb85`. No capture/payload committed. |
| Review | Independent read-only engine/source-adapter and intro/sprite/lifetime reviewers reported no material findings. They ran no checks; parent compiler/runtime evidence is separate. |
| Original-game reference, not native boot | Private Mupen64Plus 2.6.0 cold start reached original title then fresh New Game/Options menu using actual XTest input. Parent inspected captures, verified 71 package and 37 selected/capture hashes, and rescanned 90 syscall traces: two ROM read-only opens, zero protected mutations. Video is 74.166667 seconds/2,225 captured frames, not an emulated-VI count or canonical hidden-buffer oracle. |
| Reference audio limit | Actual SDL2 disk/virtual PCM capture: 3,009,536 stereo frames at 44,100 Hz; measured peak −7.932200 and RMS −28.798817 dBFS under positive-full-scale 32767. Physical ALSA playback opens failed EACCES; no hardware output/listening or A/V synchronization claim. This does not satisfy W12. |

At [`2fe08ff9063e175a3bf623d4e6a5842d44ef0bda`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/2fe08ff9063e175a3bf623d4e6a5842d44ef0bda), [run 38667739718](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38667739718) passed all seven jobs with the actual private C engine enabled: Linux GCC/Clang Debug/Release, Windows MSVC x64 Debug/Release and actual Linux LUS/SDL3/Mesa EGL procedural host. All eight synthetic executables passed; hosted jobs received no ROM and uploaded no artifacts. The parent also exercised the preserved default renderer for 120 frames and rejected a four-byte Unicode-path snapshot under a live Xvfb display without a rendering fallback. A preceding displayless attempt stopped at SDL initialization and is not ROM-loader evidence.

These bounded H1 results close only `bd_35mmIsle-v4u.5.1` and `.6.2`. Full W04/W05/W06/W08, native title/menu, production clocks, photo/depth/coverage parity, audio, progression, Windows 11 and release qualification remain open.

## Canonical raw-RDP feedback discriminator

Competing planner/reviewer assessments selected an RDP-only memory discriminator before deciding the RSP frontend. Native game CPU execution stays native. Postpacking host OpenGL depth cannot reconstruct N64 packed-dZ/hidden coverage; neither a matching screenshot nor successful sprite submission proves photography feedback.

The admitted development candidate is [`ParallelRdp.cmake`](../../cmake/ParallelRdp.cmake), [`rdp-dependencies.json`](../../cmake/rdp-dependencies.json), [`prepare_rdp.py`](../../tools/prepare_rdp.py) and original `rdp_probe.cpp`/`rdp_fixture.cpp/.hpp`. Ten immutable repository/archive SHA-256 identities cover the MIT renderer/Granite/Volk, licensed Vulkan headers and build-only shader compiler closure. Materialized per-file notices and nested notice directories are preserved. `rdp-utils`/`alp-core`/Angrylion/MAME are **not** linked or installed with the port. The recipe builds the real pinned shader compiler rather than assuming a PATH tool. Its archive snapshots cannot inherit the containing project's Git identity.

Actual GCC 15 compilation exposed missing standard integer declarations in pinned glslang `SpvBuilder.h` and Granite `compile_time_hash.hpp`. The generator applies only exact before/after-SHA-256-bounded header additions in its private snapshot, records them in provenance and leaves reference checkouts unchanged. The original shader tool also needed its filesystem/path target linkage and the recursively preprocessed Granite `debug_channel.h` include. Review caught the missing shader input and Windows narrow-path bug; the snapshot/dependency closure and native UTF-16 entry point now address them. These are build/interface adaptations, not raster changes or warning suppression.

The original three-stage workload owns 8 MiB RDRAM and 4 MiB hidden storage. A contiguous `0x27000`-byte region contains 64×48 RGBA16 color, thirteen depth and twelve before planes. Clear checks cover every word, including the final thirteenth-plane pixel. Five quarter-pixel triangles exercise occlusion, edge coverage and packed depth: A `0x2000`/hidden `3`, nearer B `0x1001`/hidden `1`. Three real RDP texture copies each move two `0xC00` halves; RGBA-copy hidden coverage differs from Z hidden dZ. Every stage waits for actual backend timeline completion before CPU reads; validators check every region/hidden value, boundary guards and both original visibility predicates. Packet inputs contain no game/SDK source, ROM bytes, native pointers or VI presentation.

| Exercised surface | Actual result / limit |
|---|---|
| Headless Vulkan | GCC Debug built the complete shader producer/runtime. Actual enumerated CPU device `llvmpipe (LLVM 21.1.8, 256 bits)`, vendor 65541, device type 4, driver ID 13, Mesa 26.0.8, Vulkan 1.4.335; enabled 8-/16-bit storage access and timeline semaphores. Three stages/209 packets/460 words completed with zero backend or semantic errors. This is Vulkan CPU execution, not an inferred OpenGL software path. |
| Independent CPU comparison | Separately compiled/executed Angrylion `31bdb1f0a79dd726017a38432540c6b5db0fa117` privately under its old MAME noncommercial/source-disclosure terms. It is not a production dependency, MIT component or distributed executable. Actual RDP commands ran single-worker with offscreen lifecycle registration; unexpected VI presentation aborts. Three stages passed analytic checks and produced three SyncFull interrupts without reported errors. |
| Exact differential | Parent compared all six visible/hidden snapshots byte-for-byte: **718,848 bytes, zero differences**. Clear, triangle and copy hidden metadata matched, including B's visible nonzero packed-dZ bits and the copy's changed hidden meaning. No tolerance or expected-output weakening. |
| Failure paths | Existing raw-output directory rejected before overwriting; missing actual Vulkan ICD rejected instance initialization; incomplete CLI rejected. No null-device or empty-capture success. A Unicode/spaced Linux raw-output path was exercised. Windows native path handling and build require their separate hosted result. |
| Review | Independent analytic packet/depth/coverage reviewer found no material findings. Build/lifetime reviewer identified the corrected shader include and native Windows path defects. Static reviews executed no checks. |

Snapshot identities, useful only for this exact original workload:

| Stage | Visible RDRAM SHA-256 (`159,744` bytes each) | Hidden SHA-256 (`79,872` bytes each) |
|---|---|---|
| Clear | `60604ad8731cac733b3cc406fc6602e55919e1f4423bebb8931b53c5e3643005` | `1fedc174f214d680324e1193dd2c966560d422287326e989e3ebbb412cabb581` |
| Geometry | `35c8c4e57f968b3d3f07cd2674bd0132bf83baf7e291030e57e92fd980e60e2e` | `f5cfdc000f48ea83e54d84fffd8c44c32e34c61c00c0b00dd886c5f4579ae229` |
| Depth copy | `7ab65fa3e8286c95865e30f7d2278e115526387f7a33568db158547397ba6bb5` | `245e24786bb693ccf86bb1a95b91bab8feab8647c872d6c6c449b24e0e0ff381` |

These are bounded emulator-to-emulator/analytic results, not independent hardware validation, a private-ROM raster replay, custom `G_SPECIAL_1`/partial-matrix/in-list-ucode proof, full scoring or Windows Vulkan/Windows 11 qualification. CLI completion itself explicitly does not run or establish independent-reference parity; the separately recorded differential above does. No RDP/source/game package is selected.

At [`d27f329b6f61e01e64d2356af2f9eaa88863a5b4`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/d27f329b6f61e01e64d2356af2f9eaa88863a5b4), [run 38669853834](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38669853834) passed eight of nine jobs, including actual Linux CPU-Vulkan execution and all previous core/LUS jobs. The new Windows RDP cell failed during configure: automatic CRLF checkout changed the byte-hashed dependency manifest. The unchanged LF manifest SHA-256 is enforced; `.gitattributes` now preserves LF for byte-hashed CMake JSON and applied patches rather than accepting a second digest or weakening acquisition validation. Native Windows RDP build/CLI remain unproved until the corrected revision runs.

The correction at [`ff6870e9bd7f93edf6e41ce385c55092f13a0016`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/ff6870e9bd7f93edf6e41ce385c55092f13a0016) passed **all nine jobs** in [run 38670598190](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38670598190): the actual Linux CPU-Vulkan workload, complete native MSVC renderer/shader-tool build and UTF-16 output-directory rejection, plus the previous six core cells and Linux LUS host. This establishes the exercised Windows compiler/CLI boundary, **not Windows Vulkan execution or Windows 11 qualification**.

## Immutable audio-bank resources and producer decision

`audio_bank.hpp`, `audio_bank.cpp` and synthetic `audio_bank_tests.cpp` admit an original B1 wire-graph decoder, not SDK implementation or synthesis. Format facts come from authoritative `src/sys/audio.c:377–456` and `PR/libaudio.h` interfaces. All references remain explicit bank-relative/table-relative values until checked; native descriptors retain immutable bank/table owners, preserve aliases and signed fields, and keep runtime player/DSP state separate. Cross-type/overlapping records, relocated flags, negative counts, truncated coefficients/samples and invalid loop bounds fail without publishing an asset. Encoded sample bytes are retained, not decoded into prerecorded PCM.

Actual GCC and Clang ASan+UBSan private consumers both hydrated the exact supported ROM's BGM bank (42 instruments, 93 unique sounds, 84 waves) and SFX bank (one instrument, 400 slots, 306 unique sounds, 298 waves). Parent independently traversed the original wire graph and confirmed these identities/counts; no mutable extraction cache was authority. The selected looped SFX 14 closure matched: ADPCM book order 2/four predictors, loop 35,168–77,952/count `0xFFFFFFFF`, envelope 0/−1/6,000 µs, encoded extent 43,858 bytes. Sample content identity is SHA-256 `5bfc4fda60a4f28a9469d5a2462179b6a0512305fa156345d959e932627b25f5`; no bytes or audio were published.

Nine local tests passed in GCC Debug/Release and Clang ASan+UBSan. Bank regressions exercise aliased native identity, retained bare views through moves/input-owner release, signed extrema/indefinite decay, non-frame-aligned loops, exact final-coefficient/sample boundaries, oversized books, conflicting records, relocated input, optional slots, ADPCM alignment bytes and RAW16 loop/sample extent. At [`efc7610d331bd6ca09f3196410f1f673335fd52b`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/efc7610d331bd6ca09f3196410f1f673335fd52b), [run 38671436033](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38671436033) passed all nine jobs, including these nine tests under native MSVC x64 Debug/Release, actual Linux LUS host and CPU-Vulkan. No native sound producer, synthesis, device playback or Windows 11 qualification is implied.

An independent read-only bank-decoder reviewer found no material findings in signed arithmetic, alias/lifetime closure, shared-loop extent checks, record conflicts or allocation-failure publication. It ran no gates; compiler/runtime evidence above belongs to the integration owner.

Competing audio planner/reviewer assessments agree on the next implementation boundary: **native staged sound/player/voice producer → explicit eight-byte ABI1 commands/state → exact private `aspMain` coprocessor execution → owned PCM → checked transport**. The bounded first stimulus is actual looped SFX 14 with live pitch/pan and stop/release, not a fixed tone, recorded playback or captured-command replay masquerading as a producer. Separately licensed HLE is a private comparison candidate, not an MIT production dependency. Coprocessor execution does not supply `alSndp`/`alCSP` allocation, sequencing, envelopes, oscillators, effects or quotas.

The original retrace/AI/task phases remain distinct from gameplay and host presentation. Preserve lagged original AI occupancy, returned synthesis rate, bootstrap credit versus actual completion, and signed L/R averaging toward zero rather than temporal half-rate output. LUS's existing void playback can hide discard/prefix acceptance. The separately implemented checked leaf below retains complete PCM on backpressure and distinguishes SDL's accepted copy/stream drain from endpoint playback; it does not pretend to replace the legacy LUS bridge. Resource hydration and virtual reference capture alone do not close W12 or authorize full scene boot.

## Private exact RSP task execution — bounded W06 evidence

`isle-rsp` and `35mmisle-rsp-probe` use `parallel-rsp@9ab776a2ce28899769c4f911403234cac62fc799`, archive SHA-256 `e03ad7e3277a9f763aab0aff86da350cf93fecb66ef7f616298d84755fa6275d`. The explicit library-only JIT/SIMD composition selects the core's MIT license, **not** an MIT license for statically linked GNU lightning (LGPL-3.0-or-later) or other bundled material. Full original archive/notices and six exact-input adaptation hashes remain build-private. Public tooling contains original native adaptations; substantial GNU assembler matching bodies are read from exact private spans, not embedded publicly. No release/package approval follows.

Original `Machine` owns DMEM/IMEM/CP0 and borrows stable 8-MiB word-swapped RDRAM, never host pointers disguised as console addresses. Original CP0/DMA code replaces upstream plugin services. DMA validates all rows before transfer, preserves separately programmed pending latches, supports both directions/count/skip/alignment/same-bank wrap and yields for IMEM invalidation. SP status, semaphore and interrupt state are explicit. RDRAM/XBUS DPC delivery retains partial packets and calls the bounded host sink only after returning from generated code. Each compiled basic-block entry—including internal backedges—consumes budget. Compilation errors return explicit failure before generated-frame unwinding. FullSync counts observed commands; **no DP timeline completion is implied**. Current admission is graphics/type 1/flags 4 only, without cycle timing, single-step execution, audio or yield/resume.

### Exact private input reconstruction

One read-only admitted US ROM snapshot supplies boot/text/data. `gRspBootCode` is the actual 256-byte RAM buffer loaded from **ROM `[0xB70,0xC70)`**, SHA-1 `95f758ce017375682479347697d0904896b1e3c5`; it is not the linked `rspbootTextStart` alias. Reconstruct original main loading `[0x1000,0x46270)` → RAM `[0x400,0x45670)` and entry-cleared BSS `[0x45670,0x9A8C0)`. Thus declared 0x800-byte graphics data includes real adjacent line data, and line data includes 0x410 bytes of actual cleared BSS—not invented padding or adjacent `app_render` ROM. Task words, matrices, viewport, vertices and display-list words serialize explicitly in canonical order.

Independently authored geometry uses fractional nonidentity projection/modelview, custom `SPECIAL_1` recalculation, signed integer/fractional partial combined-matrix writes and in-list graphics→line→graphics reload. Actual successful runs:

| Case | Cold repeats | DMA reads | DMA writes | IMEM writes | Packets / words | FullSync | Final SP status |
|---|---:|---:|---:|---:|---:|---:|---|
| Fractional / partial matrix | 2 | 14 | 1 | 4 | 15 / 74 | 1 | `0x203` |
| Graphics → line → graphics | 2 | 27 | 3 | 10 | 24 / 114 | 1 | `0x203` |
| Matrix warm after reload | 0; one warm run | 14 | 1 | 4 | 15 / 74 | 1 | `0x203` |

### Findings, corrections and exercised gates

- Independent DMA/DPC review found completion incorrectly overwrote separately programmed SP/RAM pending latches, and retained single-step status could be ignored on warm execution. Three consumer-visible regressions failed before correction and passed afterward. JIT safety and initial fixture reviewers reported no material static findings; they did not execute gates.
- First actual fixture execution proved its analytic Y sign wrong. DMA-time private DMEM observation confirmed combined/partial matrix values; original viewport Y points downward. Corrected the independently derived expected coordinates without loosening quantization bounds. Deleted an assertion on an ignored FullSync operand rather than pinning its scratch value. Line execution temporarily narrows scissor to transformed endpoints; checks now bound it analytically and require restoration before graphics. Semantic repeat comparisons exclude ignored NOOP/sync operands.
- Full dependency-source Clang ASan+UBSan exposed unaligned x86 instruction-immediate/relocation stores and negative signed LUI left shift. Exact private adaptations use byte-preserving `memcpy` and representable signed multiplication, respectively; no alignment/shift sanitizer suppression. Standard case labels replace two GNU-only range labels for native MSVC.
- Clang's function-type sanitizer read eight bytes before a page-aligned foreign JIT entry. `addr2line` and disassembly identified its `0xC105CAFE` cookie read before the indirect call, not an emulated-memory read. Only that function-cookie check is excluded on `CPU::enter`; ASan and other UBSan checks remain on source. Generated machine instructions are uninstrumented. The upstream conditionally-supported `offsetof(CPU,state)` warning remains visible.
- GCC Release and fully source-instrumented Clang ASan+UBSan passed all ten tests and all five real private tasks after corrections; GCC Debug had also passed all ten tests and five tasks. Release output exercised a Unicode/spaces directory. At `9790c05c59a658f9ac3333c3f37f2e0ff06d3c36`, [run 38674141821](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38674141821) passed seven of nine jobs; both native MSVC RSP builds failed parsing the standard `xor` token in `rsp/vfunctions.cpp:590`. [Microsoft documents `/permissive-` as required](https://learn.microsoft.com/en-us/cpp/cpp/bitwise-exclusive-or-operator-hat?view=msvc-170). The dependency receives that conformance flag without changing/suppressing the operator or removing the failing target. At `3e478d80d34a30c8f456ed43430587b95a2c923c`, [run 38675299538](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38675299538) passed all nine jobs, including native MSVC x64 Debug/Release ten-test suites and the combined RSP/RDP renderer and exact shader compiler build. CI supplies/uploads no ROM or extracted payload. Hosted Windows Server 2022 is not Windows 11 or Windows Vulkan runtime qualification.

Raw private packet files and metadata are retained separately; no ROM/RDRAM/DMEM/IMEM dump is published. Independent RSP/frontend/hardware parity, private original game frames, full custom-matrix coverage and photography remain unqualified; W06 remains open.

### Actual emitted-packet Vulkan / independent CPU raster differential

The original two-flag `--render-out` implementation performed explicit after-SP replay of two validated captures. It admitted only an untextured, no-Z, shade-only 64×48 RGBA16 workload at `0x300000`. Each case owned separate RDRAM initialized to `0xA5` and hidden memory initialized to zero; this was **not** shared production memory or a coherency claim. Stable packet spans survived real renderer `idle()` and successful `vkDeviceWaitIdle()` before readback. The persistent shared-memory cutover below supersedes that implementation, not its historical evidence.

Actual llvmpipe CPU Vulkan completed both cases. Parent read the actual images: fractional/partial writes move the two shaded triangles; the reload case restores graphics after the temporary line scissor. Each case's entire 6,272-byte guarded color snapshot and 3,136-byte hidden snapshot matched separately executed `angrylion-rdp-plus@31bdb1f0a79dd726017a38432540c6b5db0fa117`: **18,816 bytes, zero differences**. Reference remains private under its old MAME noncommercial/source-disclosure terms, never port-linked/MIT or distributed. This is independent **RDP** agreement, not independent RSP, hardware, VI, scene or photo evidence.

| Synthetic capture | Guarded RDRAM SHA-256 | Hidden SHA-256 |
|---|---|---|
| Fractional / partial matrix | `7ebc63ef37ea36523899011d47e684b008bb6c00362737ec9e9453df06c01e4f` | `b390c072c2648db69f910c99c12c88bf034a8d554c6cc3aeae499fa3554c4abe` |
| Graphics → line → graphics | `897f940e1c1c180bc746dce95a688e11fd1415f99933e8233463f6929494189e` | `20fe53a3d52a7cf9b7d4b899a27d007d0e713efb969d1029def7fa73ed8d85d0` |

## Native one-voice producer, actual audio RSP and checked transport

At the bounded one-voice stage through `f5d0574475a64f550df187837f2fab7039539bc1`, original `audio_sfx.hpp/.cpp`, its synthetic ownership regressions, `audio_probe_fixture.hpp/.cpp`, `audio_probe.cpp`, `audio_transport.hpp/.cpp`, `audio_host_probe.hpp/.cpp` and `rom_snapshot.hpp/.cpp` were admitted for development use. The subsequent shared-service cutover is recorded separately below. No SDK producer body/header is imported, compiled or translated. Original coefficient data and actual `aspMain` remain private-ROM inputs; the existing RSP/GNU-lightning license boundary remains unchanged. Nothing new is installed or release-packaged.

The producer emits live ABI1 ADPCM/resample/envelope/interleave/save commands for one dry voice. It owns stable word-swapped RDRAM, retained immutable resources and bounded staged controls. Execution and PCM ownership are separate: one outstanding task or PCM lease, terminal failure poisons DSP state until quiescent reset, stale identities release nothing, and backpressure retains the completed lease. There are no service-time allocations, CPU ADPCM rendering, captured-command playback or per-task RAM snapshots.

Private original-ROM queries established float32 duration conversion, original eight-bucket rate coefficients/fraction 65535, low-bit-first float32 CPU gain exponentiation, and raw per-delta callback deadlines distinct from floor-16 DSP parameter time. Supplemental interrupted-ramp queries supersede the earlier provisional directional target clamp: before nominal completion the CPU estimate can overshoot its target; at completion it snaps to target without automatically restarting the DSP envelope. Actual retained 80-byte DSP state has eight independent gain lanes, not one CPU current-volume field. These observations qualify exercised inputs, not an exhaustive numeric-domain or unrelated resource-class proof.

A private consumer compiled the actual native production math and matched **308 unique original observations**: 61 durations, 64 interrupted CPU-current values and 183 packed rates, with zero mismatches. Parent independently verified 250 retained source/binary/input file identities across three original-oracle manifests, plus all 504 categorized artifacts and five provenance files from the 168-task actual DSP-state experiment. Captures and payloads remain private.

The original SDK producer was then executed with continuously retained actual RSP state, not an HLE snapshot loaded before every task. The full capture completed 1,770 tasks; the isolated comparison starts at reset task 373 and includes actual stock controls, loops, stops and four generations. Both producers use the same pinned RSP implementation: this is producer agreement, **not independent RSP or hardware parity**.

| Private comparison | Different PCM bytes | Different stereo frames |
|---|---:|---:|
| Before loop-history correction | 480 | 243 |
| After correction and current shared-snapshot/bulk-PCM cutover | 0 | 0 |

The defect decoded the compressed frame whose first sixteen samples were already restored from immutable loop history. Advancing the compressed source by nine bytes corrects the restart; no tolerance or expected-output weakening. The complete 1,397-task comparison covers 745,440 stereo frames / 2,981,760 bytes; both SHA-256 values are `b27d8a0e8c449532754bf341336df22987d8e14e90d15c126b1716ad5fb023fc`. Current z64, Unicode/spaced-path v64 and n64 inputs all reproduced those bytes. Original ROM SHA-256 remained unchanged.

### Partial loops, explicit player volume and final bus mixing

The native source now admits bounded partial ADPCM loop boundaries. Actual SFX32 loops over samples 356–20529: the restored aligned history skips four prefix samples, supplies the remaining twelve, and continues at compressed byte 207 (source frame 368). Decoder work rounds up to complete compressed frames while copying only logical samples. Every actual LOOP restore rebinds `SETLOOP` immediately; treating that shared microcode context as a task-lifetime cache caused a real address fault in the short-loop fixture and was corrected, not suppressed.

Direct player volume now overrides the resource default. The producer accepts that domain explicitly; game sample/global scaling remains in the game-facing session adapter and is not applied twice. A captured live volume change established a 1-ms gain transition; implementing an immediate step differed by 109 bytes in one task. The corrected full Sound32 calendar includes pitch 0.5/1/1.5, live pitch/pan/volume, four starts, releases and replay.

Finite repeats retire by saturating consumption after the enclosing task-relative 160-frame outer pull, not per decoder subchunk or individual wrap. Client subdivisions preserve that grid. At encoded exhaustion, the next outer pull is silent rather than consuming residual decoded frame history. The real final wet-to-dry `MIXER(0x7fff)` commands remain necessary even with zero wet input: their fixed-point destination scaling is not an identity. A diagnostic established identical mono, dry buses and envelope state before that omitted operation; restoring it removed the observed 1-LSB errors without an output tolerance.

| Additional private original/native comparison | Tasks compared | Stereo frames | Different PCM bytes |
|---|---:|---:|---:|
| Full SFX32 calendar, reset 373 onward | 1,777 | 948,320 | 0 |
| Twenty authored/copied partial-loop cases | 1,700 | 906,544 | 0 |

Both SFX32 streams have SHA-256 `cafb42b18c2b8ae57c54a8c278617674b284aa7288a90ce9c0e84d3c92964250`. The twenty-case concatenation has SHA-256 `4f87475e7cfbad514b84aae2d2130bcee61a38231412f9f3f9f1d017813389dd`; all 1,700 ADPCM 32-byte histories and resampler four-sample-history/fraction 10-byte windows also match. Those cases cover two nonconstant authored patterns, copied original resources, partial starts/ends, counts 0/1/2/infinite, controls, stop/replay and silence. They executed 10,960 actual native RSP tasks including idle calendar replay, with terminal `0x203` and no DPC. Full unrelated state-window equality, all possible loop lengths/counts/pitches and independent RSP/hardware parity are not claimed.

A separate byte audit rehashed all 319,035 original partial-loop capture files and all 275,233 mixed-priority query blobs without missing/hash mismatches, plus declared source/binary/input identities. Preferred clean original-loop harness snapshots are retained and verified. Four overwritten versions from the initial BGM-contaminated failed attempt survive only as identity records, not recoverable source/binary snapshots; that attempt is not a qualified reproducible oracle. Payloads and native/reference executables remain private.

After this cutover, full GCC Debug/LUS, GCC Release and source-instrumented Clang ASan+UBSan builds each passed all eleven tests. Release and sanitized public Sound32 runs each generated, accepted and stream-drained 332,800 frames across 520 tasks; their 1,331,200 PCM bytes matched exactly. The migrated combined LUS caller also completed 120 visible intro frames / 119 C updates and drained 69,120 audio frames, using one read-only ROM admission; the owned-display capture showed the centered sprite, RGB fade and terminal black frame. These remain software EGL and SDL dummy observations, not physical playback.

Published [`f5d0574475a64f550df187837f2fab7039539bc1`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/f5d0574475a64f550df187837f2fab7039539bc1) passed **all ten platform jobs** in [workflow 38704006390](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38704006390), including the full native MSVC static-runtime LUS/audio/RSP/engine build, CPU tests and CLI helps. This extends compiler evidence to the partial-loop/volume cutover, not valid-ROM or device execution on Windows 11.

Actual integrated Linux evidence:

- GCC Debug built full LUS, engine, RSP, producer, CLI and transport; all eleven CTests passed. Public raw CLI completed 400 tasks / 256,000 frames with pitch, pan, stop and replay; every SP terminal status was `0x203`, with no DPC packets.
- GCC Release and Clang ASan+UBSan engine/RSP/audio configurations also passed all eleven tests and actual 128-task / 81,920-frame checked-dummy execution with controls, stop and replay. Sanitized actual graphics/line execution passed all five tasks. Two original-code Clang diagnostics were corrected by a nonshadowing destination name and explicit checked signed-byte conversion, not suppressed.
- Public `--play` submitted and drained 15,360 actual native frames through SDL3 3.4.2 `dummy`, source 32,006 Hz to 44,100-Hz S16 stereo. Its raw source PCM exactly matched the no-play path. A separate actual-RSP consumer filled a paused 2,048-frame bound, observed repeated `WouldBlock` without changing accounting or retiring its lease, rejected an early build, resumed and accepted 3,072 frames exactly once, then flushed/drained while preserving the host SDL reference.
- The SDL leaf's separate private runs additionally exercised 48,000-Hz F32 conversion, abort, oversized input, post-finish rejection, unavailable driver, synthetic removal and event-filter rejection. Synthetic removal is not physical unplug or endpoint-liveness evidence. The selected leaf uses one enumerated physical endpoint and no Null fallback; its removal-watch lifetime relies on the audited SDL3 3.4.2 dispatch/removal fence.
- Combined real LUS/SDL3 EGL software rendering and actual native RSP-to-dummy audio completed 120 intro-asset frames / 119 original C updates, 107 audio tasks and 68,480 accepted/drained source frames. Parent inspected the owned-display video: centered original sprite, RGB fade and black terminal state. A syscall trace observed **one** `O_RDONLY` ROM open for both consumers. The 16-MiB snapshot was released before frames; audio watch/stream, C users, GPU/cache/host and asset storage retired in their required order.
- Unicode/spaced-path v64 and n64 combined-host runs also completed their 120-frame presentation/fade and stream/owner teardown with one read-only ROM open each; parent inspected both videos. The v64 trace's octal-escaped UTF-8 pathname initially defeated the private harness's literal matcher; decoding the existing successful trace fixed the evidence filter without rerunning the host or changing project code.
- Migrated RSP CLI executed all five original graphics/line tasks successfully for each z64/v64/n64 input. Eleven public audio argument/device/output rejection cases returned failure, preserved existing raw output and did not expose the private input pathname. An explicitly unavailable SDL driver failed admission without a real hardware attempt.
- A synthetic header-only image of the correct 16-MiB size failed whole-image admission. A separate actual-RSP run under a 4,096-byte file limit failed raw output and left no success metadata; it did not claim completion or open audio hardware.
- Independent transport/Session/CLI/host/snapshot source review reported no confirmed defects; it executed no gates. Runtime evidence above is separate from that review.

The checked SDL leaf is composed directly into the LUS host session, not falsely registered as its legacy void-playback backend. Host queue pacing and quota 640 are explicitly experimental; they are not original guest AI occupancy, VI timing or original cue synchronization. Physical audio opens previously failed EACCES and were not retried. No hearing, hardware-GPU, Windows audio, Windows 11, full title/menu/course, BGM/effects or complete W12 qualification follows.

## Frozen Windows host dependency path

`tools/prepare_windows_host.ps1`, the `windows-host-release` presets, `cmake/windows-host` manifest/registry and additive hosted Windows job select vcpkg source/registry commit `4da8ab8ef910293046e651287ff1115d9a4f14b0`, static x64 CRT/triplets and SDL3 3.4.2#1. Both registry reference and baseline are frozen; a baseline alone would not freeze registry selection. The source checkout must be script-owned, clean and exact; automatic LUS vcpkg, overlays, chainloading and binary cache substitution are excluded.

The upstream tool release URL is mutable, not an immutable supply-chain claim. Before execution, the script requires the selected `2026-03-04` Windows asset to be 6,591,048 bytes with SHA-256 `13a1c66b9c7578427b3eda7eba2332b73d4fb86706e053ad6426dad2f354cbc3`; CMake repeats source/tool verification before its first `project()` can load the toolchain. Primary evidence: [pinned tool metadata](https://github.com/microsoft/vcpkg/blob/4da8ab8ef910293046e651287ff1115d9a4f14b0/scripts/vcpkg-tool-metadata.txt) and [release/asset identity](https://github.com/microsoft/vcpkg-tool/releases/tag/2026-03-04).

At [`4a04519ae41fb147a7468ee29d6af8937570d5d0`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/4a04519ae41fb147a7468ee29d6af8937570d5d0), [workflow 38695294524](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38695294524) passed **all ten jobs**. The new native Windows full-host cell performed the verified initial provisioning and pre-project recheck, built LUS/SDL3/audio/RSP/private engine with MSVC 19.44.35229.0, passed all eleven CPU tests and launched all four CLI helps. The source-selected static packages included SDL3 3.4.2#1; Windows SDK 10.0.26100.0 targeted hosted Server 2022 10.0.20348. The six GCC/Clang/MSVC core cells, Linux LUS host, actual Linux CPU-Vulkan and native Windows renderer/compiler cell also passed. No ROM or artifact upload occurred.

This establishes native Windows compilation and exercised CPU/help entry points, not valid private-ROM, graphics/audio-device, Windows 11, hardware sound or release qualification. Native high-address pressure verification additionally observed RDRAM and consumer PCM storage above 4 GiB while actual RSP output remained byte-identical through repeated backpressure and stream drain. Full W12 and the platform/release parents remain open.

## Shared synthesis and pre-CSP ownership qualification

The frozen pre-CSP implementation (V8) replaced the former one-sound implementation without aliases or compatibility producers. Its original MIT code separates retained `SoundId`/`WaveId` catalog entries, 42 logical descriptors, 22 physical filter histories, three generic client calendars and a single task/PCM owner. One game SoundPlayer has ten descriptors and measured default priority 5; the two original 16-voice sequence contexts were not implemented by substituting SoundPlayers. The following runtime evidence qualifies V8, not the later CSP implementation described below.

Catalog packing preflights every admitted resource, alignment and address extent before its first RDRAM write; aliases retain the immutable bank/table owners. The service uses bounded CPU shadow banks and publishes one shared command graph. Successful publication commits predicted CPU state; terminal execution only changes the executing/PCM lease or poison state. Completion never restores old CPU state over newly staged controls. Foreign/stale task tokens cannot retire current ownership; reset requires quiescence and does not recycle task serials or the RDRAM allocation.

Each registered SoundPlayer owns a separately allocated pair of typed CPU checkpoints. Calendar metadata remains in the common banks; dispatch receives the selected bank explicitly. Shadow preparation copies common state and only registered typed checkpoints, without per-service allocation or shared mutable checkpoint aliases. Registration preflights storage, descriptor capacity and identities before publishing through the same active-bank index. Quiescent reset invalidates registrations before releasing typed storage; external handles cannot dereference a freed pair.

The native calendar compares raw deadlines before floor-16 parameter times and uses newest client registration only to break equal raw deadlines. Quiescent `set_player_deadline` changes the raw deadline without rewriting microsecond envelope history. The SoundPlayer uses authored attack DSP duration with a pitch-scaled callback deadline; release scales both domains. These are implemented native contracts; the older shared-bus collector does not independently admit their complete original clock semantics, for the reason below. Source-rate calculations use 32,006 Hz; the observed DAC is approximately 32,006.451 Hz, not proof of host/device timing.

Current GCC Debug with full LUS, GCC Release and Clang ASan+UBSan each passed all sixteen applicable CPU/coprocessor tests. Transactional regressions cover allocation-failed registration, a later client's failure after earlier progress, dirty-shadow retry, explicit shadow selection, lease-time staging and reset invalidation. The staged-control assertions inspect emitted left-zero/right-positive gain targets, not merely continued playback. Protocol acknowledgements in these tests are not actual audio execution evidence.

Current Release and sanitized standalone runs each generated and accepted/drained 332,800 frames across 520 ROM-RSP tasks with pitch/pan, stop and replay. Every terminal was `SP=0x203`, with no DPC packets. Their 1,331,200-byte PCM outputs exactly match the preserved identical scenario, SHA-256 `85308dc9199fbebfb11427575ce098d188e69f7254baead9a5ba68ba1786d7e2`. Owner-qualified task serials were contiguous from 1 through 520. The current frozen shared implementation also reproduced both complete qualified dry Sound14/Sound32 calendars: 3,174 actual tasks, 1,693,760 stereo frames and 6,775,040 byte-identical PCM bytes.

At that shared-service cutover, the combined LUS/SDL3 EGL caller completed 120 visible intro frames, 119 C updates and 108 actual audio tasks / 69,120 accepted/drained source frames. The integration owner inspected its private display capture: original centered sprite, RGB fade, then black. One read-only ROM open served both consumers; the snapshot, audio, C, renderer/cache and resource owners retired at their respective fences. SDL `dummy` drain is not DAC, hearing, original cue synchronization or full-scene proof.

All twenty unchanged authored/copied partial-loop cases also passed through the corrected shared service: 10,960 actual retained-RSP tasks including idle, 1,700 selected tasks, 906,544 frames and 3,626,176 PCM bytes. All 1,700 ADPCM32 and 1,700 resampler-history/fraction10 comparisons matched; aggregate PCM SHA-256 remains `4f87475e7cfbad514b84aae2d2130bcee61a38231412f9f3f9f1d017813389dd`. The earlier frozen shared revision received an independent 334-artifact / 54,383,976-byte parent audit with zero errors; subsequent revision evidence is retained separately. Unrelated opaque state bytes are not asserted equal.

An independent original-reference audit checked 612,506 inventoried regular files / 5,507,229,860 bytes across fifteen SoundPlayer, active-replacement and sticky-control captures, plus 594,015 actual-event artifact declarations. All 8,000 actual tasks and reconstructed PCM summaries matched; no inventoried hash/size mismatch or unavailable declared artifact remained. Nine historical mutable-path identity mismatches resolve to byte-exact retained version snapshots, not rewritten expected identities. This establishes recoverable versions, not independently proven historical before-edit timing. Older protected-input million-file checks remain metadata-only plus selected hashes. Same-core software comparison is not independent RSP or hardware validation.

A separate short-envelope numerical defect remains open. Public original/native queries expose one-unit packed-rate differences; three legal pan controls halfway through a 32-frame attack amplify the discrepancy into different CPU gain origins. Three authored high-amplitude ADPCM inputs produce actual PCM differences over retained 224-frame windows. Broad passing calendars do not discharge this counterexample. No fitted constant, blanket ULP adjustment, output tolerance or expected-output rewrite has been admitted.

**Reference-boundary correction:** the older shared-bus collector used undocumented SDK object offsets to discover callbacks, label clocks, select physical bindings, attribute per-source state and actively rewrite selected deadlines/player-list links. Its raw-clock, exact rephase, saturation-assignment and owner-qualified attribution claims are quarantined as public-interface-only evidence. Historical raw commands, command-addressed bytes, PCM comparisons and custody hashes remain recorded facts, not proof that those stimuli respected this admission boundary. No SDK producer body/header import was established by this finding. Replacement collection binds actual game construction to public registration and naturally invoked, documented callback arguments/returns without opaque-field access; unrelated public CSeq evidence is unchanged.

Full native allocation, shared-bus and client-policy differential closure is tracked separately in Beads. This cutover is not full W12 completion, BGM/effects/AI readiness, Windows 11 runtime qualification or release approval.

## Immutable S1 sequence resources

`audio_sequence.hpp/.cpp` extends `isle-audio-bank` with an original retained S1 catalog, not an event decoder or sequencer. Wire facts were checked against the lawful exact ROM and authoritative game-side resource calls: ASCII `S1`, a BE16 count, then eight-byte file-relative offset/declared-length entries. The actual catalog has 37 entries, not 38; there is no invented kind field or 32-bit count. Numeric bank labels differ from `AuSettings` field names: the BGM bank is at `0xAFEEE0`, while the existing proven SFX bank is at `0xBA6C20`. The game source's misleading load-comment is not ownership evidence.

Hydration checks the complete table and both raw/even-DMA extents before publishing an immutable asset. It retains bounded aliases, overlapping views, gaps and the caller's byte owner, without payload copies, relocation or CSeq validation. Declared length remains distinct from an odd-length DMA padding byte. No per-consumer player or DSP state is created.

GCC Debug/full LUS, GCC Release and Clang ASan+UBSan passed all fifteen applicable tests after integration. Separate optimized and source-instrumented private programs admitted the real ROM, decoded all 37 entries and compared every offset, length, declared payload byte and DMA padding byte against independently extracted metadata. Totals: 61,806 declared bytes and 61,828 DMA bytes. Each zero-copy view remained valid after both the ROM snapshot and caller byte owner were released; final catalog release retired the retained input. No compressed-event, BGM playback, device or hardware qualification follows.

## Native compact-MIDI event reader

`isle-audio-sequence` adds original MIT `audio_cseq.hpp/.cpp` and independently authored grammar/transaction regressions. It borrows only the declared S1 payload; the catalog or another immutable byte owner must outlive every reader/checkpoint. Sixteen track cursors and a reader-local, physical-offset-keyed counter overlay are value state, not shared mutable input. Each call returns one encoded event; callers must bound zero-time loop draining. Start/step failures publish no partial cursor, clock, counter or event change. No per-event allocation, decompression buffer or whole-reader speculative copy is required.

Executed original public interfaces, not SDK bodies or opaque object fields, establish MIDI durations, tempo, running status, slot/channel independence, lower-slot ties, track/final end and loop controls. CURRENT zero resets to INITIAL and falls through; CURRENT 255 repeats unchanged over the observed prefixes. Pattern bytes are physical, not recursively expanded. Encoded length zero copies exactly 256 bytes; bounded distance `FFFF` is supported despite the manual's smaller stated maximum. After decoded `FF 2D`, six loop payload bytes come directly from primary storage: mutation targets primary CURRENT, jumps anchor at primary payload end, and an active backup tail survives. `FF 2E` instead consumes its trailing bytes through ordinary pattern decoding.

All 37 reference inputs were byte-bound to actual ROM S1 entries. The native Debug, optimized and Clang ASan+UBSan consumers matched **112,733** meaningful original public events exactly, including delta, cumulative tick, MIDI fields, note duration and tempo. Eleven streams terminated; twenty-six comparisons stop at their recorded 4,096-event limits. The three complete native calendars are byte-identical. Native track provenance, undefined SDK padding, deliberate post-terminal calls and marker/SetLoc behavior are not asserted equal.

An additional private consumer matched 31 valid base authored calendars / 1,232 events and 80 complete followup calendars / 814 events. All 122 followup cases / 919 returned reference records are accounted for: 41 have explicit native-contract differences and one bounded failure has no reference event. Those differences include malformed/noncanonical encodings and **two valid unsigned-32 deltas outside the original signed-32 clock API domain**, not merely malformed inputs. Native admission uses checked five-byte/u32 VLQs, u64 clocks and at most 2,048 distinct physical loop counters; it does not reproduce unsafe overflow or unbounded overlong decoding.

Independent custody reduction checked 81,962 files / 2,670,709,691 bytes, all selected event/input bindings and 2,082 physical mutation-map rows, without a selected mismatch or unavailable artifact. Fifteen historical mutable-path discrepancies are recoverable from exact versions or a hash-exact reconstructed case list; recoverability is not independent execution-time proof. Static reader review found no actionable defect. Original execution, native comparison, audit and review are separate evidence.

This admits the standalone decoder, not CSP scheduling, MIDI voice policy, ADSR, reverb, BGM PCM, device timing or independent hardware parity. Payloads, original/native reference executables and actual event calendars remain private; the RSP/GNU-lightning license boundary is unchanged.

## Native CSP integration and review corrections

`audio_csp.hpp/.cpp` implements real typed sequence clients over the shared catalog/synthesizer: bank/program/key/velocity selection, compact-MIDI scheduling, duration offs, sustain, envelope phases, channel controls, tempo and the shared 44-slot oscillator pool. Typed resources remain outside the value checkpoint. Native admission counts at most 64 live note events plus inbox/published controls; fixed reader lookahead and poll state are separate. This is a deliberate checked native contract, not a claim of identical SDK pool capacity or reproduction of the original stopped-player callback nonreturn at saturation.

Independent planner and reviewer assessments found two concrete defects; additional original execution resolved a third. EOT now retains already-releasing notes, their authored endpoints and oscillator ownership, while retiring nonreleasing sources. Native `stopping` denotes draining, not an inferred original `GetState`. Sequence replacement restores ordinary instrument-derived defaults but preserves CC21 and signed extra-volume. Player-extra FX changes future starts/later explicit channel-FX edits, without immediately retargeting live notes. A private consumer linked against the prior 18-test library reproduced five failing assertions across these three defects; the corrected production code passed all 19 tests under GCC Debug/full LUS, GCC Release and Clang ASan+UBSan.

The effective constructor master `0x7800`, instrument-derived CC7, CC21=120 and effective extra-volume unity were measured separately from GAME bookkeeping. Gain composition, oldest-duplicate explicit off, phase-specific gain controls, tremolo retargeting over remaining attack/release, raw-pitch preservation and the serialized resample ceiling 65534 are implemented. Fading/rising control-estimator arithmetic remains a bounded model; the generic packed-rate counterexample above remains open. The actual native CSP/RSP comparison below is separate from CPU protocol tests. Full wet effects, original AI occupancy/cue timing, complete BGM PCM equality and Windows 11 execution remain unqualified.

Actual V9 native/RSP execution covered all 37 song windows: 31,419 tasks, 18,495 reader requests, 17,742 MIDI events and 13,673 source starts. Eight songs naturally drained; eighteen had bounded loop-bearing prefixes and eleven other bounded nonterminal prefixes. A separate three-client pressure run bound 22 physical filters from 42 logical starts and exercised twenty steals. These are functional coverage and owner-retirement observations, not whole-song or BGM PCM equivalence.

Differential execution of fifty admitted original clock/controller cases across two conservatively retained origin candidates found no mismatch in 348 public tempo observations, but exposed two additional native defects. CSP had passed perceptual public volume directly into linear `Gain15`; composition must finish integer controller/tremolo quantization and then square once at the synthesizer boundary. Explicit Stop must also retarget an already-releasing note to its own 50-ms endpoint, unlike EOT's tail-preservation rule. Both source corrections and consumer-visible command/deadline regressions passed all nineteen tests in three Linux configurations. No failed expected output was rewritten.

The corrected frozen V10b consumer completed **33,755 actual tasks**, all with `SP=0x203`, error zero and no DPC packets, and 162 owner retirements. Public volume 29,536 now produces linear gain 26,622 and left SETVOL 18,707, matching the original rather than the prior 20,754. All four explicit-Stop endpoints match original Stop/Free calls: 946,000 us at the 48,000-us tempo and 5,218,000 us in the 50,001-us case, each under both retained origin candidates. The fifty-case replay has 1,564 exact PCM tasks of 1,636 and unchanged 348 public tempo observations; 72 differing PCM tasks remain evidence, not tolerance-qualified passes. Exact preceding CC21→CC16 history resolves two cold-owner source/callback setup differences, but forty subsequent PCM tasks still differ by at most two sample units in that bounded case. The first residual includes multiple ENV sources; it is not attributed wholesale to packed-rate arithmetic. All 37 song windows ran again for 31,393 tasks; whole-song/BGM PCM equivalence remains open.

## Finite SoundPlayer lifecycle implementation

The SoundPlayer now implements the positive finite-decay classes used by 384 of the 400 actual SFX resources, alongside its indefinite class. Authored attack DSP duration remains unpitched; the attack callback is pitch-scaled. Decay and release scale both domains. Zero attack starts decay in the same callback. Controls do not rephase an existing decay endpoint; future release uses the latest pitch. Natural decay completion begins release before Stop/Free, rather than immediately retiring the source. Phase-specific volume controls use the current envelope level without confusing public perceptual volume with squared DSP amplitude.

Thirty-two independently owned original public lifecycles establish these policy facts, including real SFX383, SFX22's decay level 30 and SFX1's zero attack. Later mixed-client portions of the long original capture remain excluded; only its clean prefix and fresh bounded captures contribute admitted cases. Native transactional/lifecycle/command regressions pass under GCC Debug/full LUS, GCC Release and Clang ASan+UBSan. Static finite-policy review found no actionable defect; it ran no gates.

The finite after-replay completed **108,971 actual tasks / 58,137,264 stereo frames** across 544 conservative origin variants, with clean SP completion, 544 retirements, same-slot generation reuse, stale-handle rejection and quiescent resets. All 515 callback-bin-consistent candidates match policy; overall 543 of 544 match. The one inconsistent upper-origin candidate remains disclosed, not silently discarded or established as a native defect. Every predeclared lower-origin candidate matches complete captured PCM: **6,280 of 6,280 task pairs**. All candidates combined have 102,978 exact PCM task pairs of 106,760; 3,782 differences remain. After initial history, the lower candidates match 2,853 ADPCM-history and 2,913 resampler-ten-byte/ENV-history windows. INIT prehistory, additional resampler bytes and command-order differences remain preserved. The unique original raw origin is not proven.

Separate actual Release CLI runs exercised SFX383 at pitch .5, SFX22 at unity and SFX1 at pitch 1.5: 560 tasks / 358,400 stereo frames, all `SP=0x203`, error-free and without DPC packets or audio-device opens. CLI output alone does not expose logical retirement; that proof comes from the after-replay above. Finite zero-time or zero-level decay remains explicitly unsupported because no actual resource or admitted execution establishes those transitions. These results are not full audio, AI scheduling, hardware, Windows 11 or release qualification.

## Original-compatible image codec and checked native persistence

`save_image.hpp/.cpp` independently implements the canonical 127,652-byte image, all sixteen version bytes, fixed relative offsets and the original digest convention. RFC 1320 byte padding and little-endian bit length are retained, but every padded compression word and stored state word is big-endian. Ordinary MD4 or input-only word swapping is incompatible. Native release execution matched all 86 actual GAME hash returns and reproduced the complete GAME-sealed initialized image byte-for-byte. Twelve resealed malformed offsets accepted by the original validator are deliberately rejected by native structural validation. Unknown bytes, float/bitfield representations and final hash-covered padding are preserved; this is not photograph replay or progression validation.

The fixed 128-byte native envelope includes codec/version, generation, canonical ROM/source identities, zeroed reserved fields and SHA-256 over header metadata plus exact payload. It requires exact EOF and a valid inner image; SHA-256 is integrity, not authentication. Sealing preflights before changing only the raw GAME-time word and digest. The codec allocates no storage and publishes only borrowed immutable views; it does not invent a new-game initializer or depend on SDK/GAME hash bodies.

`save_store.hpp/.cpp` provides alternating `save.a`/`save.b`, no HEAD file, a stable lifetime `save.lock`, immutable owned loaded snapshots and synchronous generation-checked commits. Complete temporary writes are read back, validated and flushed before installation into the inactive slot. Linux acknowledges file plus directory sync; Windows uses checked wide-character native installation, reopened verification and file flush, reported only as Win32 acknowledgment. After any installation ambiguity, writes freeze and evidence remains. Rejected existing slots, conflicting generations or orphan temporaries block automatic writes even when another valid recovery candidate exists. No fallback/reset/migration/promotion silently consumes damaged evidence.

An actual native Release consumer published generations 1 and 2 in a Unicode profile and cold-reloaded each in separate processes. A stale-generation commit preserved both slots. A competing process failed the session lock; killing the holder released it without destructors and cold reload retained generation 2. Truncated and future-version newest slots each exposed the older read-only candidate, rejected commits and preserved both slot byte identities. These observations are local Linux filesystem evidence, not power-loss survival, Windows 11 execution, complete save UI integration or W10 photo fidelity.

Original hash/layout collection used exact pinned GAME code and public architectural/debugger interfaces, not SDK implementations. Final run05/06 verified loaded GAME bytes and unchanged protected identities; no external device/write open succeeded. Earlier failed automatic camera/GPU/input probes are preserved and excluded. A failed software-graphics `udmabuf` allocator probe remains disclosed in the admitted final runs; no blanket no-hardware-attempt history is asserted. No original Flash write/durability or persisted Flash save was measured. GAME-derived images, authored inputs, command/PCM captures and reference executables remain private.

## Subsequent published cross-platform component gates

At [`43442de5c94a25844303dc74e9af644db808f803`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/43442de5c94a25844303dc74e9af644db808f803), [workflow 38754821969](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38754821969) passed all ten jobs: six GCC/Clang/MSVC Debug/Release cells, the Linux LUS/EGL host, actual Linux CPU Vulkan/RDP, native Windows renderer/compiler and full Windows static-runtime LUS/audio/RSP/private-engine composition. The full Windows job passed nineteen CTest executables, including CSP, save-image and actual Win32 save-store mechanisms, plus four CLI helps. Observed toolchain: MSVC 19.44.35229.0 x64, SDK 10.0.26100.0, hosted Server 2022 10.0.20348. This is compiler/component execution, not private-ROM audio/graphics, Windows 11, physical devices, power-loss survival or a playable game. Later static wet-network work is not covered by this commit.

## Static shared wet-network implementation

Original MIT `audio_effects.hpp/.cpp` adds an explicit creation-owned program: direct stereo AUX return or up to sixteen ordered sections over at most 19,200 ring samples. The static network emits signed stereo downmix, shared mono-ring transfers, feedforward/feedback, optional pole books/history, return accumulation and the existing stereo AUX mixers. Zero return gain does not remove feedback or filtering. CPU shadow state owns the cursor and pole initialization; four-plane staging preserves original 160-frame/final-partial effect invocations across client callback fragmentation. Build failure does not publish predicted state. Quiescent reset clears ring/history/staging; source retirement does not erase shared tails.

The frozen V11 implementation passed all **twenty** tests under GCC Debug/full LUS, GCC Release and Clang ASan+UBSan. Independent dataflow and transaction reviewers found no actionable defect; their static review is separate from execution:

| Actual-RSP qualification | Executions | Exercised result |
|---|---:|---|
| Continuous original/native wet-component comparison, four custom6/custom7 impulse/baseline cases | 1,040 | Exact 1,108,864 PCM bytes, 17,739,904 chronological logical ring-write bytes, 61,568,000 outer-boundary ring bytes and 33,280 per-invocation pole-history bytes. Literal graph ordering, books, gains, flags and active full counts match. |
| Public service/Synth/SoundPlayer integration and independent nonzero oracles | 17,741 | 4,320 paired task comparisons: no input, PCM, ring or pole-history differences under real silent-client deadlines 16/32/48, alone/together. Seven quotas, repeated wraps, stop/tails, reset and task/PCM fences; 75 service owners retired. |
| Parent-run independent arithmetic smoke | 29 | Fourteen nonzero exact PCM/ring cases across quotas 16/160/496/512/528/544/1,024, with actual executing/PCM resource-retirement checks. |

All executions completed with `SP=0x203`, error zero and no DPC packets. The parent independently verified 704 delivered files / 379,645,003 bytes without mismatches. Original component reconstruction recovered every initial ring byte, with no unknowns or conflicting task-pre observations: 20,800 bytes for custom6 and 38,400 for custom7. Custom6's nonzero initial tail remains intact; it is not presented as a zero-state impulse response. Its section1 self-overlap covers 64 bytes, with ordered input/output writes checked. Cursor rotation changes physical wrap segmentation, not logical transfers or DSP operation order. Existing per-SAVE capture files describe task-final addressed memory, not intermediate writes; corrected boundary attribution and new actual-RSP transfer tracing establish chronological equality without altering expected bytes.

The original capture's scratch ALVoice storage capacity/type authority remains unestablished. Qualification uses literal coprocessor commands and addressed bytes, not opaque producer fields, natural constructor proof or SDK producer parity. Captured voice-prefix inputs do not establish Nativevoice parity. Custom7 uses the measured 4-physical/6-virtual/2-per-CSP profile, not an inferred successful default22 original profile. The private reference consumer hydrates a broader opaque canonical ROM-main context; CPU bytes remain inert and its full loaded extent is not claimed consumed. Only declared RSP text/data/boot and explicit command-addressed spans are grounded; no SDK producer body/header is translated, linked or interpreted.

Chorus types3/4 are not replaced by static graphs. They, full mixed BGM PCM, original AI occupancy/cue timing, hardware/hearing, Windows 11 and full W12 qualification remain open. At [`ffde8a3562265cd46d841575906378b4b59cbee4`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/ffde8a3562265cd46d841575906378b4b59cbee4), [workflow 38766587623](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38766587623) passed all ten jobs, including twenty actual MSVC CTest executables and four CLI helps in the full static-runtime host composition. This subsequent compiler/component evidence remains hosted Server 2022, not Win11/private-ROM/device qualification.

## Deterministic game RNG and supplied-tick projections

`rng.h/.c` extends `isle-native` with caller-owned 32-bit replay state and no global state or allocation. Each seed-based operation advances the modulo-2^32 linear congruential generator once and uses all sixteen high bits, not the C library's fifteen-bit `rand()`. Binary fractions remain exact. Range scaling preserves the original signed product wrap and truncation toward zero, including zero, negative and overflowing ranges. Tick-based operations use only the low eight bits of an explicitly supplied guest `osGetTime`-domain value and do not advance the LCG. Time-range conversion preserves binary32 rounding under the caller's FP mode.

Requirements derive from authoritative GAME `src/sys/misc.c:110–146`, blob `84be12339c2f266dcfb5e2ae7ccd6a73cbe2a46b`, not SDK implementation. Private source-compiled arithmetic comparison used those exact GAME definitions, independent scalar interface declarations and supplied tick values. `-fwrapv` expresses MIPS low-word arithmetic for the private source oracle; the original native service uses defined unsigned arithmetic and in-range signed conversion instead.

| Actual native API consumer | LCG results and resulting seeds matched | Tick-projection results matched | FP modes |
|---|---:|---:|---:|
| GCC Release | 3,670,016 | 14,336 | 4 |
| Clang ASan+UBSan | 3,670,016 | 14,336 | 4 |

Every high16 output and low8 tick value participates; both consumers reported zero differences, with no sanitizer diagnostics. Existing native regressions cover mixed-call advancement, full-bit outputs, signed wrap, division direction and binary32-versus-integer scaling boundaries. All twenty component tests passed under GCC Debug/full LUS, GCC Release and Clang ASan+UBSan. This is native arithmetic and private source-compilation evidence, **not** Original R4300 execution, original time-service mapping, photo replay, gameplay RNG call-order or full W04 qualification. No GAME body or SDK implementation/header is published.

## Native GTL owner and independent presentation

`host_clock.hpp/.cpp` implements the bounded GAME policy from pinned `src/sys/gtl.c:618–790,967–988,1006–1036`, not SDK scheduling code. Four retraces are buffered; overflow drops newest. An update consumes its interval individually, drains surplus and advances once, without catch-up. Draw cadence, blocking/opportunistic context acquisition and pause acknowledgement remain distinct. Owner/epoch/serial-qualified leases reject foreign, stale and duplicate terminals. Failed execution closes admission immediately, including during pause. Reset requires stopped, resolved work and changes epoch without recycling lifetime serials.

Two failing-before/passing-after regressions cover immediate failure closure while paused and a live interval shrink during a pending blocking receive. The latter must finish its already-entered receive before reconsidering the interval, as the original C loop does. Existing regressions cover bounded backlog, intervals larger than queue capacity, context pressure, skipped draws without skipped updates, pause/resume, cancellation, frame wrap, reset and rational-clock overflow boundaries.

The actual `IntroProbe` now constructs, updates and destroys its C world on one ordinary creator-owned thread; SDL/LUS, events and audio output remain main-thread operations. A start barrier excludes asset/renderer/audio initialization from VI time. Immutable prefix/texture display lists are shared; each context owns four wrapper commands and a captured primitive color. Declined presentation retains its exact lease, and serial—not context index—orders pending frames. Actual OpenGL completion precedes retirement; packet FullSync and pinned LUS's empty `FinishRender` are not treated as GPU fences. Shutdown stops/joins C users while retaining assets, releases GPU/cache/host owners, then releases scene storage and joins the owner thread.

The integration owner exercised an actual software EGL window and privately inspected the original Nintendo 64 sprite followed by the completed black fade:

| Bounded native run | Observed result |
|---|---|
| Presentation / C execution | 120 presented frames; 122 actual C updates |
| VI policy | 130 retraces; 3 dropped |
| Graphics ownership | 121 admissions; 121 terminals |
| Independent SDL-dummy audio | 107 actual RSP tasks; 68,480 source frames |
| C fade | Update 60: RGB=(242,242,242); update 79: RGB=(0,0,0); alpha=255 unchanged |

The complete process tree was traced. Disabling Xvfb's GLX extension and explicitly selecting client EGL avoided DRM/audio-node opens; SDL controller discovery still attempted thirteen denied input-node opens, and the software graphics loader attempted two denied `/dev/udmabuf` opens. These attempts remain disclosed and are not retried. No all-process zero-device-attempt, physical playback/hearing, hardware GPU or Windows 11 claim follows.

Native pacing uses the documented ideal-oscillator NTSC progressive rational `2250000/37609` Hz, not rounded 60 Hz or presentation callbacks. [VI timing documentation](https://n64brew.dev/wiki/Video_Interface) and the [timing derivation](https://github.com/meauxdal/N64-Refresh-Rate-Reference/blob/main/N64_Timing_Reference.md) are model evidence, not physical measurements. The supplied-nanosecond Count-domain helper uses exact `floor(ns*3/64)` per the public [46.875 MHz time-service contract](https://ultra64.ca/files/documentation/online-manuals/man-v5-1/n64man/os/osGetTime.htm). It does not establish native GAME time-call ordering.

A separate genuine Original study retained nineteen captures and 1,977 audio-RSP executions: seven-retrace SP/DP message diversion distinguished blocking (2 updates/1 draw) from opportunistic (7 updates/1 draw), with bounded tick overflow, interval changes, pause and end-fence observations. Modal emulated VI period was 811,092 Count cycles; observed `osGetTime` return/Count offset was 37. Neither value is adopted as physical timing. Graphics remained HLE. Reset entered but faulted when the no-accessory transport reached an unconstructed rumble-credit queue; no successful reset or fabricated credit is claimed.

At the native-clock wave, all twenty-one component tests passed under GCC Release, GCC Debug/full LUS and Clang ASan+UBSan. [Commit `9c93b6e`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/9c93b6eba3f205172051cad13d490d3585244493) also passed all ten hosted jobs: the full Windows Server 2022/MSVC 19.44.35229.0 x64 static-runtime host passed twenty-one CPU tests and four native CLI helps in [run 38787370975](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38787370975). This is not Windows 11 or device execution. Genuine GAME canonical-memory feedback, complete input/scene-reset/conditional-exit integration, original AI/cue timing and full W07 remain open.

## Native chorus candidate and corrected capture custody

The original native effects implementation now accepts optional per-section chorus parameters. Persistent CPU phase/carry/tap drift and separate RS32 history produce modulated LOAD/RESAMPLE transfers inside the existing shared wet graph. Positive rate with zero depth still executes resampling; zero return gain does not remove its state. Creation bounds phase advance and worst-case aligned DMA/staging geometry. Ordered feedback commits, pole/book separation, aliasing, publication rollback, tail retention and quiescent reset remain explicit. Source review found no new actionable implementation defect; this is not a universal original-producer arithmetic verdict.

Frozen V12 Release and exact-source sanitizer consumers exercised the real shared service with multiple clients, steals, callback fragmentation, wraps, tails and repeated teardown/reset. A separate numerical replay reported 10,500 Original/native component task pairs using 21,000 own actual-RSP executions. Independent acceptance re-executed 300 reference/native pairs: exact bounded PCM, ordered logical transfers, input/ring/pole state and meaningful RS10. Entire task-boundary RS32 equality is not established. These numerical observations remain candidates under the collection limitations below, not clean original-producer qualification.

Capture-admission corrections are binding:

- The earlier seventeen-run recorder inherited whole 8 MiB checkpoints and private emulator AI-prefix collection. Its exclusive public-interface capture claim was withdrawn; sealed evidence remains private and unchanged.
- Subsequent raw/custom captures removed those paths but emitted unused argument registers/stack words regardless of API arity. Their exclusively-public-argument collection claim was rejected. Combined captures also mislabeled ambient `a3` on three-argument `alSynStartVoice`. Hash equality and numerical replay do not cure these scope defects.
- An independent audit verified 775,080 unique files/4,860,995,086 bytes without hash errors and separately exercised the bounded DSP consumer. Emulator-only traces did not cover Xvfb's thirty denied DRM probes; all-process zero-device-attempt claims were withdrawn.
- New signature-bounded collectors fail closed on unknown APIs, read only declared scalar bytes and omit ambient void-return residue, producer-code windows, full-RAM/private-AI paths. Parent-reviewed sources/binaries were hash-bound before launch. The first new raw run completed 2,600 actual RSP tasks but attempted a denied `/dev/udmabuf` open; its device qualification failed, the remaining seven profiles were not launched and no retry occurred. At that earlier stage the typed combined replacement was unlaunched. Strict replacement qualification remains incomplete; subsequent rejected pilot evidence is recorded below.

Arithmetic debt also remains: a fresh canonical counterexample at admitted `depth=50000` produces Original `23356` versus model `23355`; larger rejected depths are structural-only observations. The earlier ordinary `depth=2500`/N16 section-dependent discrepancy remains open, not silently discarded with the capture correction. Fresh replacement comparisons naturally cover 48/64/80/160-frame outer chunks, not clean 16/32 parity. Strict `>` rather than `>=` at the model's half-period boundary was discriminated, but does not resolve universal producer arithmetic. No fitted constants, tolerance, special-case input or expected-output rewrite is applied. Full chorus, mixed BGM PCM, AI/device and W12 qualification remain open.


## Historical capture-replay required-work qualification

The initial host fixed required-work slot was independent of its two optional visual contexts. Typed owner/epoch/serial tokens qualified publication, CPU borrowing and retirement. Closing woke capacity/result waits before C joins; executing GPU work and borrowed CPU memory were not declared ceased by cancellation. Main released its actual mapped backend lease between retirement acknowledgement and capacity reuse. Producer-owned captures survived early-close and remained live until final renderer/cache/backend teardown. The subsequent cutover removes those retained captures and adds exclusive actual-memory CPU preparation.

The former `CanonicalCaptureExecutor` kept one Vulkan context/device/processor on its creating main thread. Its memory lease exposed actual word-swapped RDRAM and hidden spans, not a copied image. It validated its bounded packet contract before mutation, waited for real processor idle and `vkDeviceWaitIdle`, checked maps/guards and poisoned failed backends. Frame-context advance recycled Granite command pools after the previous CPU lease was released. This was persistent execution ownership, **not persistent canonical GAME memory**: each capture initialized RDRAM/hidden memory. The class/API/target and replay route have since been removed, not retained as compatibility aliases.

The actual Linux host smoke deliberately retained both visual leases while the original C owner reached its blocked draw-context boundary. Two genuine ROM-bound RSP matrix executions produced stable retained captures. Main performed 64 real canonical executions; the native owner checked six independently authored triangle/color samples, untouched/hidden guards and repeated color/hidden identity, then acknowledged 64 CPU borrows. Main retired all 64 actual backend leases. Three C updates, two visual admissions/retirements and zero visual submissions were observed; the required channel was quiescent before backend destruction, followed by asset/thread release. The earlier run failed its CPU predicate because two selected pixel centers lay outside the authored partial triangle; source-derived matrix/vertex analysis selected interior centers without learning expected colors from raster output. That failed run is retained and exercised failed CPU acknowledgement plus ordered teardown.

Private evidence manifest SHA-256 is `029b5bac02f6e3b8367be4d537e2341d6eab394598d442bd7eaa9180a6ad93b4`. The distinct transport uses [Mesa 26.0.8](https://docs.mesa3d.org/relnotes/26.0.8.html), upstream commit `60e95b787857afbc9a00b693b91c0d9c8923a430`, with a reviewed private six-file patch: disable optional llvmpipe udmabuf export, skip global DRM enumeration under explicit software selection and directly select surfaceless CPU swrast. GBM/GLX/WSI platforms are disabled; real LLVMpipe OpenGL and Lavapipe Vulkan remain. Thirty-four distribution packages were extracted privately without system installation. Actual SDL2 offscreen EGL produced all 4,096 specified RGBA8 pixels exactly. Whole-launcher/descendant traces of that execution and the failed/passing native SDL3/Vulkan runs recorded zero `/dev/dri`, `/dev/input`, `/dev/snd` or `/dev/udmabuf` attempts and zero access denials. The generic EGL force-software warning and successful-compute compilation stalls remain recorded; stdin `/dev/null` terminal-query failures are not a physical-device claim. Old denied captures remain failures, unchanged; no new Original collection is admitted by this transport proof.

GCC Release, full-LUS GCC Debug and Clang ASan+UBSan each passed all twenty-two CPU tests; five actual Linux CLI helps also passed. [Commit `1618ed7`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/1618ed71dee53815edcdffbf0fa0c39af6043bb1) passed all ten hosted jobs in [run 38796577921](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38796577921). Its Windows Server 2022/MSVC 19.44.35229.0 x64 static-runtime build includes LUS, audio, RSP, canonical RDP and the native engine; all twenty-two CPU tests and five CLI helps passed with SDK 10.0.26100.0. This compiles/links the integrated feedback path but does not execute its ROM/graphics/audio path on Windows. The diagnostic is not a C GAME photo consumer or continuation wait, native scene boot, photo fidelity, physical timing/device/hearing proof, Windows 11 runtime or release admission. Genuine scoring still needs writable persistent canonical memory, real photo/world/model reconstruction, pointer/resource relocation, admitted depth/texture-copy commands and original wide/narrow continuation semantics.

## Canonical Player FIFO and native Source updates

`isle-player-commands` binds one active canonical Player Source to its actual
Scene Services. The binding introduces no queue, interpreter, mutable Player
mirror or receiver registry. Attachment and dispatch plumbing do not allocate;
normal enqueue retains the existing arena-backed command-pool behavior.
Receivers, callbacks, Source and Services remain borrowed until their real
native joins and semantic/graphics consumers finish. Detach does not manufacture
those joins.

The typed C bridge invokes `isle_game_commands_checked` directly. A local
`isle_player_source_call` fence catches checked handler failure; the returning
C++ backend then closes the actual dispatch before C propagates the first
error. Cleanup remains available after sticky Source/Services failure.
Consumed/current sender credits settle once; an ordinary live suffix remains
owned until real receiver cancellation or retirement. Sends and link broadcasts
use the existing registered-sender rules, including NULL sender selecting the
current emitter. Builtins remain native interpreter behavior.

The private witness generator admits three complete original definitions:
`mainCameraHandleCmd`, `mainCameraUpdate` and the genuinely empty reticle
handler `func_80356118_4F6528`. The empty body is original behavior, not a stub.
Pinned own-GAME commit/blob/range/body/declaration/selection hashes, counted
rewrites, generated C/header identities and seventeen native support identities
are checked. The blink value comes from the exact own-GAME
`app_level.h:136` declaration, not a guessed SDK constant. Parent replay
independently reconstructs all three complete bodies and matches the actual
compiled private include/header. These test-private exports do not duplicate
production Root symbols.

`isle_game_object_set_source_update` replaces either updater form with an
actual one-argument callback; NULL callback disables updates. Object is explicit:
NULL never selects the current object. Owner/object/fault/retirement guards
precede mutation, and entity userdata/payload remain unchanged. The Actor source
installer now uses this API after its existing typed payload validation.

Independent planner/reviewer assessments found no concrete production defect.
All three consequential acceptance gaps were added to the real consumer:

- A successful builtin prefix precedes a faulting C handler that appends a
  self-sent suffix. Distinct retired prefix/current/original-suffix senders
  retain the correct credits. First failure closes only consumed/current
  credits; original and appended suffixes survive until actual receiver erase.
- NULL-object replacement rejects inside a real C update. Replacement of a
  separate live camera and the retiring object rejects during a real retirement
  observer. The unchanged camera subsequently consumes its queued original
  blink, and deleted receiver storage joins before reuse.
- A prepared inactive Source passed from a canonical C handler cannot nominate
  the failure fence or enqueue a blink. Its complete record remains unchanged;
  canonical failure closes current credit and retains the real suffix until erase.

Ordinary cases also exercise disabled-update FIFO retention and original-C
restoration, interleaved builtins/custom commands, actual paused process
execution, direct/link delivery ordering, NULL-current sender identity, retired
sender retention, custom/builtin self-deletion, joined address reuse and nested
C failure-fence restoration. Terminal faults use separate native game/Source
scopes. The consumer's seven scopes hydrate actual admitted-ROM resources,
Session and save state; they claim neither audio-device output nor hearing.

| Local profile | All 57 rebuilt CPU tests, seconds | Complete FIFO consumer, seconds |
|---|---:|---:|
| GCC Debug / full LUS build | 3.48 | 0.553 |
| GCC Release | 0.79 | 0.164 |
| Clang ASan/UBSan | 11.63 | 1.611 |

All 22 opt-in consumers compile and execute in each profile: 66 accepted whole
cases over one 496-file source set,
`f3812489a23864006a22d11fa73817b6d45c4d819ecaf2da159a8278bddea81d`
(SHA256 of compact, key-sorted JSON path→SHA256 records). The accepted reviewed
Debug FIFO case is reused without replay/rescan; the other 65 use fresh scopes.
Sole initial closures establish 3,555 terminal tasks, 3,489 created tasks and
3,621 absent tracer/task identities, stable traces, unchanged ROM, zero known
physical attempts and zero permission errors. The previous 63 surface images,
213 Photo files and twelve Cart files remain byte-identical to their preceding
qualified snapshots.

The opt-in executable is `player-commands-rom-smoke`; arguments are a private
admitted ROM and an existing empty private profile directory. It is not a
ROM-bearing CTest or a game entrypoint. Public CI explicitly compiles it without
a ROM; the preceding published `49d8546` independently passed
[all ten Linux/MSVC jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39028838977)
with the then-existing 21 consumers. That prior result does not qualify the
subsequent command revision.

Published `588dc41` passed seven jobs in
[run 39030932112](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39030932112);
three engine-enabled Windows configurations rejected
`src/menu_scene_bridge.h` at the strict support hash guard. It was the only one
of the seventeen command dependencies without an explicit LF attribute.
The attribute is corrected; no source hash, generator guard or output identity
is weakened. A real isolated Git checkout of that published revision with
`core.autocrlf=true` and only the corrected attribute blob matches all seventeen
support hashes and runs the actual strict generator successfully, reproducing
the exact generated C/header digests. This exercises checkout/importer behavior,
not an MSVC compiler or Windows runtime. Corrected revision
`d936b02b5a726db93c2bca7f4a0dc7583a9bc45a` subsequently passed
[all ten Linux/MSVC jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39031651154),
including explicit compilation of all 22 opt-in consumers and the full static
LUS/audio/RSP/RDP/engine closure. Subsequent renderer-prefix changes and private
Windows11/game/device execution require separate qualification.

This admits the FIFO/updater component, not execution of the complete original
main-Player command3–10 handler, full reticle/Root creation, prefix-time
Photo/camera/FX/UI dispatch, all 106 Root definitions, remaining species,
complete gameplay, private Windows11 runtime or original/device parity.

## Native renderer prerequisites: fragments, prefix capture and processes

The 496-file native snapshot
`e5056805a97576506086a9696c3d2619c5257f9b8e094029720ea240b6c44639`
adds prerequisites for the original source-controlled camera pass, not the
complete renderer or Player Root.

`actor::Producer` publishes complete raw per-object callback ranges without
EndDL and an explicit NULL-object Detector-prefix marker. Empty hidden,
filtered and geometryless objects remain represented and validated. Cold
staging/published vectors swap only after success; rejected production preserves
prior fragments, RAM and history. Actual Services DL registration/order and
aggregate-versus-fragment triangle/color/depth/capture equivalence are exercised.

`Photo::capture_prefix` executes the complete original C camera/Pokémon/Item/FX
capture before history staging, acquiring real actor identity holds immediately.
`seal_history` validates the exact committed context/generation and membership,
adopts additional history-only holds and then leases history. Rejected unsealed
captures remain exactly retireable. Shared rows cannot overwrite queued work;
postprocessed reuse requires the bound Frame to be completed and quiescent.
The Frame outlives its ticket and cannot be begun, retired or reused until ticket
retirement. Unsubmitted tickets are synchronous single-attempt capabilities;
retirement consumes them even when a rejected attempt shares a planned generation.

History staging advances its own generation exactly once on success.
Appending does not advance it; retain/publish/release require the exact existing
generation. Real focus tests reject future/stale tokens and held mutation without
changing publication, then publish a genuine replacement with its new token.
The unreachable-in-practice generation-overflow guard is not runtime-qualified.

`isle_player_source_add_process` preserves actual native one-argument C callbacks,
kind, priority, returned process and owner. Eleven complete native scopes cover
creation during FIFO dispatch, real wait/pause/resume/self-end, receiver
cancellation/join, wrong Source rejection and created-child cleanup after a later
C failure. All 44 private Root creators lower to this checked API; independent
replay preserves every complete original body. That authoring still is not
compilation or execution of the full 106-definition Root.

Competing Photo assessments found a real first-error publication defect:
CopyInfo could publish a retained completion or return OK without one, and its
C++ leaf overwrote the original error. An actual native/GPU run fails before the
fix. Publication now passes the canonical sticky-error gate before any mutation;
the returning C leaf does not assign Source status. Both retained-completion and
no-completion regressions pass after exact cleanup. Two earlier whole scopes
are rejected because new fault inputs failed native validation too early.
Corrected inputs exercise actual C Item packing rejection and real bounded
Actor encoding failure after prefix capture; no validator is weakened.

| Complete actual consumer | GCC Debug seconds | GCC Release seconds | Clang ASan+UBSan seconds |
|---|---:|---:|---:|
| Actor aggregate/raw-fragment GPU and native DL order | 14.890974 | 7.352548 | 65.093740 |
| Prefix Photo/history/GPU/CopyInfo and failure retirement | 10.699296 | 8.889479 | 20.291409 |
| Checked C process/FIFO ownership, eleven scopes | 0.554276 | 0.158603 | 1.573075 |
| All 57 CPU contracts after default-target rebuild | 3.50 | 0.80 | 11.87 |

All 22 explicit opt-in targets and default CPU executables build per profile.
The complete current cohort has 66 accepted cases: 65 fresh executions plus the
exact-source/binary passing-after Debug Photo scope, never replayed. All 3,588
terminal tasks and 3,654 tracer/tasks are absent before each sole initial scan;
traces are stable, ROM unchanged, physical attempts/permission errors zero.
All 57 support manifests reach an unchanged digest fixed point. Fatal address/UB
checks remain enabled; ptrace-only leak-scan disable is not leak qualification.
All 63 previously inspected surface images, 213 Photo files and 12 Cart files
remain byte-identical, checked only at known positive paths without inventory,
trace rescans, parameter searches or repinning.

The Photo consumer executes 66 real GPU frames, two actual failed completion
callbacks and 79 original C CopyInfo updates per profile. These results do not
admit original global camera/FX/UI dispatch, full Root/initUI/World factory
closure, remaining species/courses, gameplay, Original capture parity, private
Windows11 runtime, hearing or hardware. Published
`c079c6d490b0b73a3476fb542ef12bc00fac9ddc` subsequently passed
[all ten Linux/MSVC jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39034124226),
including the full static-runtime LUS/audio/RSP/RDP/engine closure and explicit
compilation of all 22 opt-in consumers without a ROM.

## Checked World callbacks, complete vectors and SourceControl

Published `6c7e3ca`'s 496-file native snapshot is
`3ec8feddd13afeaf16faa9cf49a41eebc19943b1f9433582a34f03095dc9d6dd`.
All World and Movement callbacks now return `bool noexcept`. Failed callbacks
return before checked C++ propagation; subsequent callbacks, sampling and
dependent publication stop without undoing completed source side effects.
Movement initialization/update return success explicitly. Native update thunks
latch failure and return normally. `World::retire()` reports cleanup completion,
not restored health: failed removal retains remaining objects/resources/context,
and retries resume without repeating completed removals or clearing the first
error. Destruction fails closed only when required explicit teardown remains
incomplete. Root adapters must still supply nested plain-C Source fences and
return through all C++ frames before an outer checked transfer.

The existing strict private engine recipe now admits 41 complete definitions.
Four unmodified pinned `vector.c` bodies supply normalize/add/difference/Euler
rotation; the already admitted arbitrary-axis function is reused once, not
duplicated. Native float3/float3x3 values and exact prototypes preserve source
reciprocal arithmetic, signed zero, squared underflow/overflow, aliasing and
axis/order semantics. A separate actual CLI links the real native engine and
executes all five functions: magnitude13, normalized `(5,12,0)`, difference/add
reconstruction, Y-axis quarter-turn `(5,3,-2.00000024)` and aliased unit-axis
invariance. Temporary source/object/executable are removed. This is not Original
libm bit-parity or full Root linkage.

`Pass::control` runs synchronously before generic camera emission under the
actual exclusive preparation lease. Its noncopyable `SourceControl` borrows
bounded command/list emission, RAM, epoch and real targets; it cannot outlive
dispatch. A controlled pass explicitly draws at most once or supplies its own
commands/skips. The NULL-control path remains unchanged. Later UI sprite views
and actual camera parameters are consumed at explicit draw, not snapshotted
before an earlier source control.

The actual UI consumer hides a genuine native film object before initial
gather, then changes its visibility and invokes the real C sprite leaf under a
Source fence. Dispatch-time re-gather restores that exact object's membership
and meaningful pixels without changing depth. An actual full-screen black GPU
fill remains visible rather than being overwritten by generic clearing. A
separate real Frame rejects a nested C native-metadata failure: first Source
status/side effects survive, C++ sink scope is destroyed, C fences are restored,
no token/completion is fabricated and exact failed-Frame retirement succeeds.
Explicit component recovery resets only the local C-fence status after proving
native Game fault was never set; it is not production fault healing.

The first whole UI scope is rejected: its fixture incorrectly expected changing
a sprite's hidden attribute to alter Services object membership. Production
filters hidden native objects, not those scalar sprite attributes. The corrected
fixture exercises actual object visibility and exact membership; no production
filter, pixel threshold or error/lifetime guard is weakened. Rejected scope
artifacts are neither replayed, inventoried nor partially admitted.

| Complete actual consumer | GCC Debug seconds | GCC Release seconds | Clang ASan+UBSan seconds |
|---|---:|---:|---:|
| UI/Screen plus source control, late gather and nested C failure | 5.969765 | 5.289869 | 11.340114 |
| World/Actor/Item course composition | 6.534028 | 5.856542 | 11.938022 |
| All 57 CPU contracts after complete default rebuild | 3.48 | 0.79 | 11.83 |

All 22 opt-in targets and default CPU executables build in each profile.
All 66 component cases pass with one exact-source/binary prequalified Debug UI
scope reused without replay and 65 fresh executions. Before each sole initial
scan, all 3,702 terminal tasks and 3,768 tracer/tasks are absent; traces stable,
ROM unchanged, physical attempts/permission errors zero. All 57 support
manifests reach an unchanged fixed point. All 63 prior surfaces, 213 Photo files
and 12 Cart files remain byte-identical at exact known positive paths.
Address/UB failures remain fatal; ptrace-only leak-scan disable is not leak
qualification.

This admits callback/math/control prerequisites, not the complete original
camera functions, global camera/FX/UI scheduling, all 106 Root definitions,
genuine first-World factory/graph closure, other courses, full gameplay, private
Windows11 execution, Original capture parity or physical devices.
That immutable revision subsequently passed
[all ten Linux/MSVC jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39036742866),
including the full static-runtime LUS/audio/RSP/RDP/engine closure and explicit
compilation of all 22 then-current opt-in consumers without a ROM. Later
species/helper changes require their own evidence.

## Genuine Butterfree/Doduo and native object helpers

The 500-file source snapshot is
`d8a794c9928889297f4bb8a39c39add4a6a9b36fa3776bd059b4844d5eca4570`
(SHA256 of compact, key-sorted JSON path→SHA256 records). Strict
`actor-control-v7` materialization compiles all thirteen complete pinned GAME
definitions: eleven Butterfree/Doduo callbacks and two original void factories.
Original bodies remain private. Immutable typed bindings include direct,
graph, auxiliary and self-cycle dependencies; missing closure rejects before
native ownership/RNG mutation.

Butterfree12 uses ordinary `Pokemon_Spawn`: complete ground factory first,
including forced animation/events, process creation and source resets, then
only the original late AUX-Y overwrite. Ground `pos1` and cached target fields
are not replaced. Doduo84 uses `Pokemon_SpawnOnGround`, not the ordinary leaf.
Both factories accept the active prepared canonical PlayerSource before its
object/model or FX publication. The same native actors survive later actual
component Player/FX publication and execute original initial states only under
the real OM. This proves that ordering capability, not original Root entry.

The clean four-argument animation seam preserves explicit start/force;
canonical RNG supplies randFloat, seven shared scalar unions supply miscVars,
and masked native object flags remain distinct from tangible visibility.
Doduo's hidden-but-tangible wait, real player-distance command16 emergence,
original hit timer/path cleanup and joined slot reuse execute without forged
state jumps. Butterfree's path, queued hit, Jump/Fall auxiliaries and actual FIFO
pause/resume execute. Object fnUpdate continues draining FIFO while attached
processes pause; the source counter belongs to that independent update.

Checked C Player helpers pause/resume every actual attached process kind without
pausing fnUpdate; a NULL target means actual OM current object, never an
invented Player fallback. Link-ID lookup preserves native priority/insertion
order, unsigned IDs including main-camera−1 and legal NULL absence. Invalid
links reject without changing outputs or the first failure. Real mixed-kind
processes, current-object self-pause tails/resumption, independent fnUpdate and
lookup/fence boundaries pass all three CPU profiles. All four private full106
Root call sites are migrated and independently source-replayed; that draft
remains uncompiled and does not count as original Root execution.

| Complete actual consumer | GCC Debug seconds | GCC Release seconds | Clang ASan+UBSan seconds |
|---|---:|---:|---:|
| Original-C Butterfree/Doduo and owned audio-RSP drain | 1.228293 | 0.637213 | 2.837121 |
| All 57 CPU contracts after explicit all23 and default rebuild | 3.51 | 0.81 | 11.73 |

All 23 explicit opt-in targets and complete default CPU executables build in
each profile. The current cohort has 69 accepted component cases: 68 fresh
executions plus the exact-source/binary passing Debug species scope, reused
without replay or rescan. Sole initial closures establish 3,760 terminal tasks,
3,691 created tasks and 3,829 absent tracer/task identities before physical
inspection. Traces are stable, ROM unchanged, known physical-device attempts
and permission errors zero. All 57 support manifests reach an unchanged fixed
point. Exact known positive paths retain all 63 prior surface images, 213 Photo
files and 12 Cart files byte-for-byte.

Four initial whole scopes remain rejected. Consumer corrections replace an
empty-span assumption with actual owners in the fixed-capacity render span,
an incorrect counter-pause expectation with real attached-process observations,
and an out-of-range100000 placement with a representable far fixture. Native
filtering, scheduler policy, fixed16.16 validation, source thresholds and source
fields are not weakened. Rejected scopes are not replayed, inventoried or
partially admitted.

The new species consumer performs no GPU/render submission. Audio proof is
actual private RSP execution and owned CPU PCM/drain, not an exercised SDL
stream or hearing. Address/UB failures remain fatal; ptrace-only leak-scan
disable is not leak qualification. These results do not admit complete original
camera routing, all106 Root bodies, the remaining first-entry25/16/131/1004
families, all courses, gameplay, independent Original parity, private Windows11
runtime or hardware. CI explicitly builds all23 consumers without a ROM;
the preceding hosted receipt above remains tied to `6c7e3ca`.

Published `244c950` passed seven hosted jobs, including Linux all23 compilation
and canonical GPU execution. Three Windows configurations rejected the newly
hashed species header after automatic CRLF conversion; strict verification
correctly stopped before compilation. Four explicit LF rules extend the existing
checkout policy for the new species files. Actual Git checkout filters with
`core.autocrlf=true` reproduce all four qualified hashes without CRLF. No source
or expected hash is changed, normalized or bypassed. The native500 snapshot and
its local runtime receipts remain exact. The correction at
`c85ca4f808c9a38705c0e37ab982a67236158cdb` passed
[all ten Linux/MSVC jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39041010654),
including native MSVC Debug/Release, the full static LUS/audio/RSP/RDP/engine
composition and all23 opt-in consumer builds without a ROM. That receipt does
not qualify subsequent original-camera renderer changes or Windows11 runtime.


## Independent Detector and framebuffer contexts

Original Detector context flips at its DL5 prefix; graphics context selection
is independent. A blink skips the prefix, so the two IDs cannot be coupled.
`Photo::enqueue` now binds Detector history/generation to the actual executing
Frame and submission without requiring its framebuffer index to equal the
Detector context. Concrete Frame identity, token, error/busy state, prepared
phase, generation, queue and failed-submission retirement checks remain.

The actual Photo consumer selects framebuffer context from each concrete
Frame's completed ordinal, not from the predicted Detector flip. Its former
incidental equality assertion is deleted. Before the production fix, this
complete real-GPU case returned failure after actual teardown; its sole
closure has 81 terminal tasks, 80 created tasks and 82 absent tracer/task
identities. That rejected scope is not partially admitted or replayed.

After the narrow guard correction:

| Local profile | All 57 CPU tests, seconds | Complete Photo consumer, seconds |
|---|---:|---:|
| GCC Debug | 3.50 | 10.996 |
| GCC Release | 0.81 | 9.079 |
| Clang ASan/UBSan | 11.91 | 20.269 |

Each complete execution includes 64 actual GPU Frames, two completed-callback
failures and 73 original-C OM CopyInfo updates. All 21 opt-in targets compile
in each profile. The three Photo cases share the 487-file source set
`03ceb188106dc20966ee8f8ab0ac6633e03866c395e2a966dae63fdb258811ae`.
Their sole initial closures establish 486 terminal tasks, 483 created tasks
and 489 absent tracer/task identities, stable traces, unchanged ROM, zero
known physical attempts and zero permission errors. All 71 known positive
Photo files per profile/213 files remain byte-identical to the preceding
qualified snapshots.

This fixes context ownership in the existing Photo component. It does not
establish original prefix-time full snapshots, source-controlled camera/FX/UI
dispatch, whole Root106, complete gameplay or Windows/device/original parity.
The following 63-case component cohort retains its separate immutable source set.

## Original Cart control and native rendering

`isle-player-cart` compiles all four complete original `render_player.c`
functions, source lines 6–18, 20–33, 35–46 and 48–55, from OwnGAME pin
`df8815e75f3d79618466e01e0827c6165b93d11c`, blob
`a87c007297022d577f629d5850a8ec76ac69a845`.
Commit/blob/body/declaration/selection hashes, counted rewrites, generated
include/header hashes and nine native support identities guard materialization.
Parent independently replayed every complete transformed body and matched the
actual compiled include. Original C controls guards, independent model/material
ordering and final current-cart publication; there is no replacement Cart FSM.

Each Init publishes a new object35, LINK9/priority `0x80000000`, actual default
FIFO update, J-fogged DL3 metadata, 43 kind-28 nodes/eight materials and the real
kind-1/priority-1 C animation process. Runtime, Services and native factory
cohorts are prepared outside OM; actual C creation does not grow the arena.
Fresh speed1 is observed through advancing native transforms and interpolated
material output, not default getters. Outer NULL preserves the unrequested
domain; consumed mixed streams and subsequent ChangeAnimation retain their
independent speeds. Actual object flag2 advances tracks without pose writes;
node controls remain separate. Per-object callbacks continue to identify A
after a later Init makes B current.

C callbacks run only after the C++ step returns. Callback failure, self-deletion
plus failure and rejected Runtime reentry close the real event lease before
the first checked failure propagates. Individual retirement does not clear
`cartGObj`. Joined collection still waits for actual queued-source, caller,
current-pointer, event and graphics/preparation/readback holds. The consumer
retires a queued sender before the receiver's real default update, then proves
physical-slot reuse only after the remaining holds close.

The shared BorrowedKernel emits actual J-fogged source-root/native-pose lists.
All active bindings validate before hidden/pass filtering; a corrupt hidden
binding rejects without producer RAM publication. Real Services DL3 reordering
changes actual triangle-packet order. Raw per-object fragments, including
zero-length lanes, reproduce aggregate RSP packets, color and depth in both
orders. Independent canonical RAM mappings exercise A→B→A residency. Two real
producers sequentially reuse the same workspace, with a second admitted source
camera sample; returning the unchanged first producer restores its camera and
matrix payload under the actual renewed epoch. This latter witness does not
separately establish overwritten-material restoration.

Actual intro and fresh-outro cart geometry reaches canonical RSP/RDP using
source camera initialization, admitted clip trajectories, World fog and
lighting—not tuned camera, pose or scale inputs. Hidden, pass-filtered,
reveal/residency, intro END control2, retained-rebind suppression and fresh-outro
controls are exercised. Positive intro/outro surfaces were directly inspected.
The added clear pass is an explicit component framebuffer control, not a change
to original main-camera flags5 or evidence of the complete course renderer.

Qualification command wall seconds, not gameplay performance:

| Local profile | All 57 CPU tests | Complete Cart C/RSP/RDP consumer |
|---|---:|---:|
| GCC Debug / full LUS build | 3.585 | 7.313 |
| GCC Release | 0.911 | 6.280 |
| Clang ASan/UBSan | 11.789 | 12.203 |

All 21 opt-in consumers compile in each profile. All 63 actual whole-component
cases use the identical 487-file source set
`577fae35a03af479a061977efdd7694bf2ab13fed42be291986eda41e523a719`
(compact, key-sorted JSON path→SHA256). The accepted Cart Debug case is retained
without replay/rescan; the other 62 use fresh scopes. Sole initial closures
establish 3,549 terminal tasks, 3,486 created tasks and 3,612 absent tracer/task
identities, stable traces, unchanged ROM, zero known physical attempts and zero
permission errors. All 21 previous surfaces/63 images and all 71 Photo files
per profile/213 files remain byte-identical to their preceding qualified
snapshots; all four new Cart files per profile/twelve files agree across profiles.

This admits the four-function Cart prerequisite, not the original Beach intro's
290-wait/skip/inclusive camera restore or fresh-outro 1200-wait continuation,
their FX/audio/Player-command services, whole Root106, remaining species, full
gameplay, private Windows11 execution or original/device/hearing parity. The
published Source sprite revision `4c009ee` separately passed
[all ten hosted Linux/MSVC jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39025174041);
its twenty-consumer compiler proof does not qualify these subsequent additions.

## Canonical Source sprite execution

`isle_game_object_sprite` returns the actual native list head, with the same
owner, membership, fault and cleared-output conventions as camera ownership.
Checked C-only Source borrows use the actual active context and its real
execution fence; another record cannot supply a replacement failure frame.
Move, scale, tint and attribute operations mutate the native sprite itself.
Signed coordinate narrowing retains the low sixteen bits without undefined
or implementation-defined conversion; scale enables only the source SCALE
attribute, while attribute updates preserve unrelated bits.

All Root sprite SDK calls, SObj sprite-field accesses and `data.sobj` reads
are migrated in the complete 106-body private authoring. Independently replayed
source identity is
`4f4aaa84e4e5cd46a1526b97327ed526db47d1c2e785419f847beee47f56fcaf`;
the original 64 context helpers, 79 calls and 39 one-argument callbacks remain.

Debug, Release and Clang ASan/UBSan pass all 57 CPU contracts and separate
actual programs running the complete original `Pause_HideUI` C function,
source lines 1647–1676. Its real kind-0/priority-9 process controls nine native
sprites, yields for twenty updates and self-ends on update 21. Every update
checks original positions, alpha write/clamp, visibility, unrelated channels
and live counts; actual object deletion closes the joined native ownership.
The 475-file source set is
`1fe7b5fc8619398e45bab3b119f10b0836527a199a051e38167de7be1dd95102`.
Each sole closure contains two terminal tasks, one created task and three
absent tracer/task identities, stable traces, unchanged ROM, zero known
physical attempts and zero permission errors.

The sprite records and initial state are explicit owned CPU inputs. No RSP/RDP
pause-menu surface, whole 106-body Player owner, gameplay or Windows runtime
is inferred. Earlier camera and sixty-component receipts retain their separate
source identities.

## Native camera computation and enumeration-free presentation

Native type-3 camera ownership uses the existing bounded arena and recycled nodes, not SDK camera/object layouts. Empty, sprite and camera payloads have distinct teardown; unsupported payload types fail rather than acting as an unimplemented DObj path. Two same-kind matrices are permitted as in GAME. Configuration invalidates only dependent computed matrices; unchanged poses avoid repeated normalization. Read-only float/fixed borrows remain stable until removal, with no per-update allocation or redundant big-endian matrix cache.

The additional exact GAME spans are three declarations from `include/sys/mtx.h` (blob `4f0706f10e48d4df55f1e9bf625ede7c9fd7f6c3`) plus fixed-combine macros, the immutable sine table, `hal_mtx_f2l`, `hal_look_at_f` and `hal_perspective_fast_f` from `src/sys/matrix.c` (blob `b6c98ec45939cd9e59ef16d7e44868eee5dc250d`). Every source/adapted span hash and reviewed change is in the manifest. The original C math is compiled privately; camera ownership is original native implementation, not a claim that unadmitted GAME camera/render/animation bodies now execute.

Actual GCC Debug standalone API execution created a camera with photo near10/far25600 and eye(0,0,100), computed FOV30 then60 with normalization5, mapped eye/target correctly and widened projected geometry. Matrix updates did not grow the native arena; object deletion retired all camera/matrix ownership. Permanent engine checks additionally passed geometry/fixed reconstruction, selective caching, invalid/foreign-thread access, exhaustion/reuse, typed cleanup and the two-matrix boundary. No real RSP upload, textured model, render-list emission, photo reconstruction or scoring is established by these CPU results.

Returning `isle_game_object_camera` / `isle_game_camera_object` now expose the
canonical bidirectional attachment without allocation or a second mapping.
Wrong-kind, detached, deleted, foreign-thread and faulted access reject with
cleared outputs; raw handles still expire at removal, including pool reuse.

`isle_player_camera_set_viewport_rect` uses the original binary32 midpoint,
quarter-pixel scaling, signed truncation and low-sixteen-bit lane semantics.
It preserves unwritten lanes, resets source depth and delegates native
admissibility without clamping. Nonfinite or out-of-int32 intermediates reject
before conversion or mutation. Owner validation precedes coordinate validation.
All five private Root viewport sites and all camera-owner reads use real native
APIs; the initial pose publishes once, after all source assignments, avoiding
invalid intermediate eye/target/up combinations.

After these additions, all 57 CPU contracts pass Debug, Release and Clang
ASan/UBSan. Three separate actual native programs execute the exact C
viewport/border, initial-pose and whole-main-object-removal fragments, compute
real matrices3/14, preserve a distinct UI camera and close all native resources.
Their 475-file source set is
`63e4c9fe26124cd9377ef91bc9547734eb65dd3b99ef5c2870c02a75203eca4f`.
Each sole closure has one terminal task, no created tasks and two absent
tracer/task identities; ROM hashes remain unchanged, with zero known physical
attempts or permission errors. The projection witness uses independently read
pinned GAME sine-table entries341/1365, not ideal continuous trigonometry;
its expected matrix element is exactly `0.4950297772884369`. Production
projection mathematics was not changed.

The programs supply explicit logical-screen320×240 inputs; they do not implement
VI/window policy, camera render registration, command/prerender dispatch or the
complete106-body Player owner. Full Root remains private, replay-checked
authoring—not a compiled gameplay claim. The earlier sixty-case archives retain
their own source identity and are not relabeled as this later source set.

The independent `--no-input` flag suppresses SDL gamepad initialization, ControlDeck construction/initialization, input callbacks and ImGui automatic controller enumeration without disabling rendering. Audio/graphics driver selection remains independent. Actual separately source-qualified Linux SDL3/OpenGL execution completed 120 presentations/122 C updates and 121 visual admissions/terminals. A private source-bound observer read the real default 640×480 front framebuffer before swaps1/60/100, restored GL read/pack state and forwarded every real SDL swap: swap60 showed the original sprite; swap100 was black. Actual GL strings were LLVMpipe LLVM21.1.8/Mesa26.0.8; no Null renderer or fake presentation was used.

Whole `strace -f` transcripts for unobserved intro, observed intro and four-frame procedural mode contained respectively 70/70, 70/70 and 68/68 traced/terminal PIDs, zero DRM/input/audio/udmabuf attempts and zero EACCES/EPERM. Repeated `--no-input` failed before SDL startup. The generic Mesa ForceSoftware warning remains in raw logs. Private evidence manifest SHA-256 is `1cc8c47baf47bb5253855a354856c6c5b247b0382743a988f3e7a46e91e85624`; copyrighted surface captures remain private. This does not retroactively qualify older denied-node runs, physical input/audio, full native scene/gameplay, pixel parity or Windows 11 runtime.

## Persistent shared CPU/RSP/RDP memory

`CanonicalExecutor` now owns one processor-backed, host-visible RDRAM allocation and a persistent actual RSP machine borrowing those same words. CPU preparation receives an exclusive mapped span; no host vector is imported or copied into a separate renderer image. Actual RSP execution stages bounded packets before any GPU enqueue. CPU and RSP writes are flushed together; an intervening read invalidation cannot discard them. Real processor/device completion precedes read-only CPU access. Failed execution or abandoned preparation poisons the instance; no rollback, fabricated completion or automatic RAM reset is supplied. RSP destruction precedes backing-storage destruction.

The fixed channel now covers preparation request/mapping/publication, exclusive CPU writing, queued/actual execution, completed CPU borrowing, acknowledgement and actual backend retirement. Close cannot revoke a live writer or reader. Cancelling after CPU writing has ceased acknowledges that ownership before capacity can return. Token epochs/lifetime serials and creator-thread-only actual leases remain distinct from optional visual contexts. Channel regressions exercise these transitions; they do not substitute for backend execution.

Actual Linux host execution completed **64 CPU preparations, 64 new RSP tasks, 64 RDP completions, 64 owner-domain CPU reads and 64 actual lease retirements**, while both optional visual leases remained held and zero presentations were submitted. The immutable private ROM lived through final required retirement. The owner verified six authored color samples, untouched guards, stable color/hidden digest `4938304128900258360`, actual allocation identity and a persistent pixel-derived canary before dependent preparation. Hidden memory retained the renderer's one-time cold initialization; later tasks did not clear it or reconstruct complete RDRAM. Three actual C updates and 590 retraces/583 drops were observed; these are bounded clock-policy observations, not game timing fidelity.

A standalone actual camera consumer created the native C camera, computed pinned GAME perspective/look-at/fixed words and uploaded them directly into this same canonical memory. Two actual RSP/RDP tasks drew one authored triangle:

| Camera FOV | Actual colored pixels | Actual pixel bounding box |
|---|---:|---|
| 30° | 153 | (23,16)–(39,32) |
| 60° | 28 | (28,21)–(34,27) |

Both used normalization5, preserved the same allocation/canary, avoided arena growth during updates and retired all owners. These results establish actual matrix upload and changed raster geometry, not a GAME DObj, texture/material producer, animation, photo reconstruction or scorer.

GCC Debug/full LUS, GCC Release and Clang ASan+UBSan each passed all twenty-two component tests. Real private-ROM RSP CLI runs passed five cold/reload/warm tasks in each configuration; the combined Debug CLI additionally executed two new canonical RSP/RDP tasks. Generated JIT instructions remain outside sanitizer instrumentation. The host's complete trace contained 106/106 traced/terminal PIDs, zero known DRM/input/audio/udmabuf attempts and zero EACCES/EPERM under the already source-qualified private Mesa composition. This is a scoped known-node scan, not universal descriptor-policy qualification. Private proof SHA-256: `a00a10bcfb958ffec5631f18464ed4851129a5080261fa92943c190a98a9379c`. Historical failed Original captures remain rejected; full GAME scene/photo, physical devices, Windows 11 and release gates remain open.

## Exact HD Pidgey resource loading

`HdResources` borrows the admitted immutable ROM and decodes native fixed-capacity views: 17 live nodes plus sentinel, 15 pre/draw payload pairs, 23 display lists/971 commands, 602 vertices, 21 texture/palette views, the GAME material and selected photo/pose-0 records. Sixteen framed model-animation streams are retained, not evaluated. The selected render entry is Pidgey16, scale2.0, HD model; pose0 has no material animation. Node9 uses payload7. Its real `DE000000 0E000000` call remains an explicit dynamic-material producer requirement, never an empty substitute.

The manifest records eight complete GAME blob/SHA-256 identities, independently verified by the parent against the pinned commit. Splat mappings resolve the tree to ROM `0x15E5F0`/guest `0x801D2180`, with 18×44=`0x318` bytes; the payload is at `0x15E578` and immutable gfx ends there, not at the next bank. Older scout arithmetic, offsets, list count and vertex-size annotations are corrected, not accepted as authority. Resource payload bytes remain solely in the private ROM.

Actual GCC Release and Clang ASan+UBSan consumers decoded the graph, read all vertices/commands and installed 42 disjoint spans/36,752 bytes through XOR-3 canonical addressing. Every installed byte matched its borrowed source, every other 8,351,856 RAM byte and 128 outer guard bytes remained unchanged, and wrong-size installation failed before mutation. No compiler/sanitizer diagnostics occurred. Private proof SHA-256: `818ab451c4af694bf46738011a3ce37b88f5d4f00d0cfd595dfb645c75350022`; manifest SHA-256: `113ba64f737dc6c1d0a6c275f336c97ea9acc9d90266124e97cbb3f87c049d67`. This is exact resource ownership/installation, not animated pose evaluation, native model task production or GAME photo fidelity.

## Content-stable private LUS preparation

Fresh committed-byte export, exact-base lifetime patch and exact stb acquisition/generation adaptations now occur in disposable staging on every configure. The live build receives only changed file bytes/types. A prior-input inventory owns removal of obsolete source inputs, while generated-only artifacts survive; it is not a trusted source cache. The pinned archive has only regular blobs; unexpected staged symlinks fail closed. External source checkouts remain untouched and pin/clean/context/digest failures remain fatal.

Two actual full-host configure/build pairs compiled zero unchanged LUS/stb objects; only four shader external-project bookkeeping steps remained, with its compiler reporting no work. The second pair then ran four real LUS frames under explicit input-disabled/source-qualified software rendering and observed complete host-owner release. The prior identical-source configurations rebuilt roughly seventy objects. This establishes the exercised Linux incremental path; a cross-platform build and bad-source/patch rejection remain separate checks, not inferred runtime evidence.

## Subsequent rejected Original collector pilots

Two fresh source-bound pilots used exactly one authorized launch and one external final qualification each, after every traced descendant ceased. Neither made a forbidden DRM/input/audio/udmabuf attempt or received a permission denial, but **both transport verdicts remain rejected and immutable**. The raw impulse completed 2,600 audio tasks; the combined impulse stopped after 190, before recreation/endpoints. Neither the remaining seven raw cases nor combined zero was launched.

The strict audit incorrectly treated no-follow `S_IFLNK` observations as contradictions of followed regular-file bindings; it also rejected valid strace deleted-memfd result syntax. Raw had six unbound semantic loader-directory spellings. Combined additionally lacked one actual regular libGLX executable binding and classified its own exact failure-evidence file outside the run subdirectory. These findings do not authorize broad filename/device/ioctl grants or numerical corpus admission. Replacement source policy/compositions are developed in distinct closed-gate roots; old helpers/manifests are not edited, retried, requalified or relabeled as passing.

## Native type-1 model and real pose-0 HD task execution

Native C models attach only to empty GObjs, allocate their actual hierarchy size from the bounded arena, reuse best-fit retired blocks and preserve parent/child/sibling relationships. Fixed-matrix borrows remain stable; unchanged transforms and flags-only changes avoid recomputation. Object removal, deferred self-deletion and game destruction retire every owned node/matrix. Model/camera counts remain distinct and complete.

Five additional private GAME spans cover `mtx.h:4–20` and `matrix.c:979–999,1148–1179,1289–1305,1307–1371`. Fixed kind-28 math preserves low-word multiplication, arithmetic shifts, scale×256 truncation and explicit fixed packing without undefined conversions or sanitizer suppression. The angle factor is not a guessed PI expression: the exact compiled GAME function at guest `0x8001DA08`, ROM `[0x1E608,0x1E9FC)`, loads binary32 `0x4422F983` from ROM `0x449B0` and uses MUL.S→TRUNC.W.S. The manifest records this observation, source/support hashes and adaptations. Float root baking uses local×photo-world, with model scale2.0×0.1, never yesterday's already-baked root.

The native producer samples the actual pose-0 model streams before baking, preserves geometry-less nodes, executes pre-lists under parent transforms and emits the seven source-derived conditional pushes/pops. The real node9/payload7 segment0E material selects the original first CI8 image; original geometry, palettes, TLUT/load-block commands and vertex-cache/ST behavior are retained. Perspective LOAD→normalization→look-at PROJECTION MUL follows GAME. Lighting stays deliberately off for this score-style workload; rainbow-like normal-byte shade is not normal lit-preview color fidelity.

Actual execution caught a defect absent from both source-only reviews: the first task completed its texture/state commands but emitted zero triangles because the four frustum-ratio values were reversed. Independent native fixed-matrix projection placed the model in view; an authored MVP-force discriminator did not fix it and was not retained. The source-selected original GAME score list at ROM `0x5A330..0x5A350` established exact FRUSTRATIO1 wire order. Correcting only those values produced the following six real tasks, without diagnostic command injection:

| Input | Colored / changed-Z pixels | Textured Z triangles |
|---|---:|---:|
| Initial time0, FOV30 | 1,177 | 303 |
| Identical repeat | 1,177 | 303 |
| Animation time10 | 565 | 289 |
| Changed world position/yaw | 1,159 | 312 |
| FOV60 | 297 | 314 |
| Restored original input | 1,177 | 303 |

Each task executed 27 actual TLUT loads and 27 texture loads. Repeat and restored inputs matched complete color/Z planes exactly. All six used the same persistent allocation, preserved a canary and every immutable input byte, grew the arena by zero and retired the actual completed-memory lease. Private coarse/cropped score-style surfaces were inspected; payloads/captures remain private. GCC Debug/full LUS, GCC Release and Clang ASan+UBSan each compiled the producer and passed all twenty-two component tests; the six-task RSP/RDP raster run itself was Debug, not a fully sanitized GPU execution.

The complete model trace contained 38/38 traced/terminal PIDs, zero known DRM/input/audio/udmabuf attempts and zero EACCES/EPERM under the source-qualified software composition. Private proof SHA-256: `9ee54ff8df3884c1fe2a131f2ea87df1466d8ac8cc8ec1cbd5841d95bcb3c4cc`. This is genuine native pose/resource/material/task execution, not independent Original/hardware pixel parity, normal preview lighting, 13-plane/two-draw/depth-copy CPU feedback, complete photo reconstruction, full native game, Windows 11 or release qualification.

## Native support byte identities on Windows

[Run 38818215978](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38818215978) passed seven jobs but rejected all three Windows engine configurations during preparation: `native support identity mismatch: src/engine.c`. The native support files had newly enforced raw-byte hashes without explicit checkout line-ending attributes. No compiler failure or admitted-source mismatch was bypassed.

The four hash-bound native support paths now explicitly use `text eol=lf`, matching the existing manifest/patch policy. A disposable Git checkout with `core.autocrlf=true` preserved all four exact manifest hashes and successfully prepared all twenty-six pinned GAME definitions. The preparer still hashes raw bytes and rejects differences; it does not normalize inputs or accept alternate identities. This proves the exercised checkout/preparation path, not hosted Windows compilation or Windows 11 execution.

## Native packed-depth photo reductions

Original `photo_feedback.hpp/.cpp` implement GAME `src/app_render/score.c:243–318` reductions without importing an SDK body or serializing planes. A checked borrow addresses all twenty-six 64×48 halfword planes in completed word-swapped RDRAM: F/color, C/scene depth, twelve A/subject depths and twelve B/before depths. CPU access remains inside the actual completed-memory read lease; wide counts survive by value before narrow reuse. Clear depth is exactly `0xFFFC`; visibility requires `A != 0xFFFC && A == C && B != C`, preserving every packed-Z/dZ bit. Inclusive windows and integer area estimation match the GAME expressions.

Permanent regressions cover packed low-bit differences, coincident before/current occlusion, first/last subject slots, reversed halfword pairs, all center-window boundaries, dense/empty planes, same-buffer reuse and rejected extents/counts preserving outputs. GCC Debug/full LUS, GCC Release and Clang ASan+UBSan each passed all twenty-three tests. The actual CPU-Vulkan RDP probe also completed clear, geometry and depth-copy stages with zero semantic/backend errors; the reducer consumed the real completed GPU-written/copied planes and matched independent analytic counts for all twelve slots. This probe deliberately reuses its synthetic geometry across wide/narrow reductions; it is not the genuine two-camera-pass GAME model graph or Oak scoring.

## Actual normal photo-preview lighting

`photo_lighting.hpp/.cpp` borrow the exact private-ROM ambient/directional light union, global pre-render list and normal photo-preview setup. The manifest records ten pinned GAME blob identities, independently verified by the parent. Three disjoint installation spans total 200 bytes; only the three directional bytes are mutable. The original camera calculation uses strict binary32 horizontal normalization, Y=1, full normalization and signed-byte truncation. The producer restores ordinary reset → global pre-render → camera matrices → normal camera setup → fog/material/model ordering. Normal FRUSTRATIO6 remains distinct from score FRUSTRATIO1; score rendering still disables lighting.

A private actual consumer checked every byte of 8 MiB RAM plus guards: only the 200 installation bytes changed, and every directional update stayed within its three-byte set. Four signed X/Z axes produced ±89 with Y89. Seventeen invalid cases rejected before mutation, including nonfinite horizontal inputs, zero/overflow/underflow direction, invalid extent, Snapshot aliasing and non-nearest rounding. Unused eye/at Y did not affect direction, and the complete immutable ROM Snapshot remained unchanged.

Nine real native model tasks then executed on one persistent canonical allocation with actual RSP/RDP completion and lease retirement. The baseline camera fit the entire coarse 64×48 bird; parent visual inspection observed original brown/cream texture coloration rather than the score pass's normal-byte colors. Every task retained three canaries, all 36,752 immutable resource bytes and zero arena growth; each executed 27 TLUT and 27 texture loads.

| Actual comparison | Different color pixels | Different depth pixels |
|---|---:|---:|
| Score versus lit preview | 559 | 0 |
| Exact preview repeat | 0 | 0 |
| Diagnostic −X light with fixed matrices | 511 | 0 |
| Restored diagnostic direction | 0 | 0 |
| Score repeat after previews | 0 | 0 |
| Preview restored after score | 0 | 0 |
| Native camera changed to +X | 671 | 677 |
| Native camera restored | 0 | 0 |

The isolated light experiment changed only direction bytes after preparation; it is a diagnostic, not an Original caller-parity claim. Baseline output covered 560 color/depth pixels and 328 textured-Z triangles. All model/game/backend owners retired. Private proof SHA-256: `09e7a99f97e36e43134295e1d7bd7ba6d84339e9978df7295e1a4e441d3a0d8a`; resource manifest SHA-256: `975e030ded932c4b460b95bd277220b235f6376056762d15736d940ea40268ba`. These results establish bounded native preview lighting, not independent Original pixel parity, full scene/photo reconstruction, a presented gameplay surface, Windows 11 execution or release readiness. No payload or screenshot is published.

## Genuine native two-pass photo depth graph

Competing planner/reviewer assessments selected one scene owner, independently posed native model instances and one actual task per FOV. The hybrid keeps the existing main/runtime addresses, adds twelve bounded model sublists and distinct matrix slots inside `[0x100000,0x110000)`, and retains the separate FIFO reservation. All callers use explicit instance indices; no old single-instance aliases remain. Immutable model/material resources install once per canonical pointer+epoch. The original 37-command GAME copy list, private ROM `[0x5A180,0x5A2A8)`, installs at `0x10E800`; its SHA-256 and three independently verified GAME source identities are in `pidgey-feedback-inputs.json`. No list payload is public.

Each scored actor performs **prefix C→B GPU copy → independent A geometry draw → PipeSync/C-depth restore → cumulative C geometry draw**, both draws writing F. Unscored actors draw only to cumulative C. Native instances retain distinct unbaked poses, matrices and revision caches; source order is never distance-sorted. GPU clears F and contiguous C+A12 once per pass. Only inactive B visible bytes are CPU-zeroed; active B is completely GPU-copied and inactive B hidden metadata remains untouched. This removes overwritten CPU zeroing without claiming that every pre-GPU intermediate byte matches the original whole-block `bzero`.

The runner freezes bounded descriptors and validates both cameras/poses before canonical writes. It completes the wide 60° pass, reduces under its actual completed-memory lease, retains counts by value, releases, then executes/reduces narrow 30° on the same allocation. World-up, aspect4/3, near10/far25600 and viewport Z lanes are preserved. No per-pass arena growth, whole-RAM copy/reset, executor reset, per-subject task serialization or captured-command replay occurs.

The actual Debug RSP/RDP consumer completed **28 tasks**, including the integrated runner. Independent single-subject C planes matched every one of the 3,072 packed halfwords in the later subject's B prefix, through both original 64×24 LoadBlock halves. Each isolated A matched its independently rendered actor. Repeat and restored inputs matched all twenty-six visible planes exactly. Reverse order changed prefix histories; unscored occluders changed C/B without receiving A/count slots. Twelve→one→zero cleared abandoned slots and replaced every count.

| Native narrow case | Subject slot | Wide pixels | Narrow pixels | Visible pixels |
|---|---:|---:|---:|---:|
| Single A | 0 | 136 | 560 | 560 |
| Single B | 11 | 159 | 647 | 647 |
| A then B | 0 | 136 | 560 | 72 |
| A then B | 11 | 159 | 647 | 630 |
| Coincident second actor | 11 | 136 | 560 | 0 |

Maximum occupancy executed 7,906 actual triangle packets, 24 depth-copy rectangles and 363,478 packet words in the narrow pass under the unchanged one-million block-entry/1,048,576-word limits. A separate four-task boundary consumer exercised real geometry at the last column (nine pixels), last row (sixteen pixels) and both sides of the row23/24 seam; every copied packed prefix halfword matched. Seventeen unsupported requests rejected before canonical access; finite negative animation time remained accepted, as required by the GAME domain.

The initial private smoke incorrectly expected 48 rather than 24 copy rectangles **per pass**; another used an entire asset-bank range instead of an actual selected installation span and incorrectly rejected finite negative animation time. Those harness failures remain preserved, not relabeled successful; only corrected prospective consumers supply completion evidence. No production guard, packed-depth predicate or expected pixel value was weakened.

GCC Debug/full LUS, GCC Release and Clang ASan+UBSan each compiled the producer and passed twenty-three component tests. The runner/real GPU graph execution itself used Debug, not a fully instrumented GPU backend. Independent source review found no actionable defect; that is separate from execution. The graph trace contained 38/38 traced/terminal PIDs, zero known DRM/input/audio/udmabuf rows and zero permission denials under the previously source-qualified software composition. Private proof SHA-256: `633879cc066cd62469bd3ccac757416ca7bd012ecae716232868506983b27ef0`. This is genuine bounded native photo-depth feedback, not complete `PhotoData`/world/items/effects reconstruction, Oak scoring, independent Original/hardware parity, Windows 11 or release qualification.

## Native model frames presented through LUS

The public `--model-preview` mode now composes the native camera/model/light/material producer, actual persistent RSP/RDP completion and real LUS/OpenGL presentation. RGBA16 pixels convert once into three preallocated 64×16 RGBA32 tiles. GL completion and exact texture-cache-base invalidation precede mutation; a declined draw retains its commands/pixels. Host/cache retirement precedes native model/game/resource destruction, with the immutable Snapshot retained through final canonical work.

Actual Linux execution completed **32 new native tasks and 32 LUS submissions**, with 31 changes between adjacent native plane digests, one stable allocation, unchanged canary and zero arena growth. Private default-framebuffer captures were inspected: swap1 was gray GUI startup, not visible model evidence; swaps11/21 showed distinct original textured model poses, and swaps11/31 matched exactly. Some authored poses cross the image boundary. The first-submission diagnostic was clarified accordingly; command submission and GL completion are not first-swap pixel proof.

Twelve invalid host argument combinations rejected before host initialization; actual help exposes the mode. All required/canonical leases and host/model owners retired. The complete trace contained 110/110 traced/terminal PIDs, zero known DRM/input/audio/udmabuf rows and zero EACCES/EPERM. Generic force-software warnings and shader stalls were retained. Private surface proof SHA-256: `4cc3f471617fdd0a1e05767127c4ee1443a2011425901491a0e45fa32da7f391`. This is an authored pose0 preview, not native scene boot, original timing, camera controls, gameplay, hardware graphics, Windows 11 or a game package.

## Subsequent hosted component acceptance

The exact-LF correction at `919dcd288d12f96124f5b45326e9010e1f820afb` passed [workflow 38819643683](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38819643683), including the three previously rejected Windows configurations. The lighting/CPU-reduction publication at `9888c97127d7f7f16a1f0cd5aa29cff4a938f299` passed [workflow 38820198806](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38820198806), including full native MSVC static-runtime LUS/audio/RSP/RDP/engine compilation, twenty-three CPU tests and CLI helps. These hosted Windows Server 2022 results do not qualify the later multi-subject/presentation changes, private-ROM Windows execution or Windows 11.

The later graph/model-presentation/owner-barrier publication at `8e46a0cacdb34d43d560c820bb28798149d5e9ea` passed [workflow 38823313702](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38823313702), all ten jobs, including full static-runtime native MSVC compilation and twenty-three CPU tests. This qualifies those component builds, not later Oak changes, private-ROM Windows execution or Windows 11.

## Actual fast-feedback visual-retirement race

An actual short required-feedback smoke exposed a distinct shutdown boundary: `StopGame()` guaranteed C teardown, but `offer_terminal()` only queued visual acknowledgements. The main thread could inspect final statistics before the actual owner called `policy.poll()` to consume those terminals. The prior longer successful workload did not eliminate this race.

`WaitForVisualRetirement()` now waits for the real owner's stopped state after C quiescence, returned visual borrows and actual required-backend retirement. It neither increments counters nor manufactures GPU completion; early calls with unresolved ownership reject. Native/asset storage still survives through renderer/cache retirement.

| Actual short workload | Required CPU/RSP/RDP cycles | Visual admissions | Consumed visual terminals | Exit |
|---|---:|---:|---:|---:|
| Before explicit owner barrier | 4 | 2 | 0 | 1 |
| After explicit owner barrier | 4 | 2 | 2 | 0 |

The corrected run retired every real required-memory lease, presented no optional frames and released host/intro owners. The clarified model mode and existing procedural mode also completed bounded execution. Twenty-three component tests passed after the fix. This ROM-dependent host race has retained failing-before/passing-after smoke evidence, not a new ROM-fed public CTest. Private regression proof SHA-256: `77913a637672f08fd355d48e0c027f0b163dcc0ac601d6c0613d88ba58ff6b1a`; preserved failure record SHA-256: `52c5c062dfb1f5480712b05178b1c816cb76c6c547b6e79184446caa3a778e23`.

## Rejected closure-bound Original pilots and exact parser cause

Both subsequent single-launch/single-final-qualification Original pilots remain rejected and immutable. Raw completed 2,600 audio tasks with the entire required executable closure; combined stopped at 229 tasks before recreation/endpoints. Both had exactly one consuming-side `ftruncate` pathname parse rejection, with zero forbidden-resource/ioctl/unknown-provenance/permission/unsupported-async findings. No numerical corpus or producer-parity admission follows.

Independent competing assessments traced the defect to actual strace `clone3({input} => {output}, size)` grammar: the argument splitter treated `>` in `=>` as an unmatched angle close, and the sharing decoder treated both structures as one input body. That conservatively denied all memfd credits before the adjacent successful memory operations. The finding is a manual source/literal-metadata diagnosis, not a replay or requalification of failed traces. Fresh prospective source corrects grammar only; helper-prefix classification does not become resource authority, and ordinary-file/device/ioctl/code restrictions are not weakened. New source/native-OS evidence and separately authorized new roots are required before another pilot.

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

This establishes native Oak CPU behavior over authored matching photo descriptors and genuine native model feedback. Standard-library float `atan2` still needs independent Original return/bin-boundary qualification. Full saved-photo/world/items/effects reconstruction, original score/pixel parity, scene/UI/gameplay, Windows 11 runtime and release qualification remain open.

## Fresh clone-update transport results

Fresh strict-grammar sources passed seventeen detached groups, complete source/alias/retained-identity checks and actual fail-closed gates. An authored native pthread fixture observed successful `clone3({input} => {output}, size)`, credited both zero/nonzero memfd truncations, and rejected anonymous credit after an ordinary file replaced the descriptor. No helper permission was relaxed or inferred from a memory object's name.

The separately authorized fresh raw3-impulse pilot completed one launch and one external final qualifier, both exit0: 2,600 audio tasks, 2,542 graphics tasks, three recreations and a complete ceased tree. Every rejection category was zero; required executable closure and protected identities matched. Final transport manifest SHA-256: `e9784ea03685239afdba2e895f4baae02b69f4de3c8a2e9078065bbe88f80448`. Transport closure is not numerical producer parity.

The fresh combined pilot remains rejected: a live fatal stopped execution before its endpoints, and its operator incorrectly interrupted the sole final qualifier after thirty seconds. No final closed audit exists and no retry is authorized. Its retained **later nonfinal** snapshot has no fatal categories; that does not identify or erase the stopping condition. Competing source reviews identified missing first-fatal persistence plus demonstrable incomplete-prefix/creation-context false-fatal paths on fresh authored inputs. Prospective corrections require independent proof and new bindings; no failed Original trace is replayed or requalified.


## Native Beach geometry and atomic reflection/camera state

The private engine now admits thirty-one GAME definitions from six exact Git blobs. Reflection-roll uses `hal_look_at_reflect_roll_f` and its actual GAME vector helper; translate-only models use unchanged `hal_translate`, `src/sys/matrix.c:1020–1049`, SHA-256 `67bbcbdb9e9418cc00592645f11fed9741069fd8bd1f278aa9124a62d9aaef5e`. Existing RTS consumers explicitly select their original kind. No SDK body/header/layout or new math convention is introduced.

`rom::GuestView` replaces the model-specific view without an alias. `isle-graphics-task` supplies one checked memory/cold-image/boot/task implementation to Pidgey and Beach. First-block and sky hydration installs forty merged spans, 65,740 bytes, leaving 8,322,868 unrelated RDRAM bytes and 128 outer guards unchanged. Actual Debug, Release and ASan+UBSan consumers rejected wrong extents, overlap, invalid reservations and moved owners before writes; admitted ROM bytes remained unchanged.

Known GAME callback `0x800E1CA4` was mapped only through the pinned world overlay and bounded own-GAME instructions, not source-order inference or a nonexistent symbol-map entry. ROM `[0x5F454,0x5F530)` has SHA-256 `dadc126a6530bb3add03af5e5952c4eb5cfa481296efb63573e64dda965f5989`; its cycle/render/geometry commands establish the one-cycle sky semantics. No SDK callee was followed. Terrain uses source relative position times100; sky follows the native camera eye.

Review caught two defects before acceptance: a valid terrain setter could commit before an invalid sky transform rejected, and raw quarter-pixel scissor sums disagreed with GAME's separately divided integer-pixel bounds. The correction shares model validation and stages every dirty camera result before publishing any cache, revision, normalization or validity state. Individual camera getters retain their behavior. Both model preflights and all request checks precede atomic camera computation; same-owner commits and RAM writes follow only success. Meaningful CPU failure/recovery tests observe retained borrows, not structure padding. Invalid enum values are constructed by an actual C11 client, avoiding an out-of-range C++ enum conversion.

Twenty-one new real RSP/RDP tasks used one persistent canonical allocation, genuine immutable geometry and native transforms. Isolated layers, camera eye/target/roll/FOV, block reference, fog color/distance, clipped columns and exact restoration were exercised:

| Actual diagnostic frame | Triangles | Z triangles | Colored pixels | Written-depth pixels | Distinct colors |
|---|---:|---:|---:|---:|---:|
| Terrain and sky | 478 | 462 | 3014 | 1704 | 482 |
| Terrain only | 462 | 462 | 1704 | 1704 | 393 |
| Sky only | 16 | 0 | 1838 | 0 | 93 |
| Source-rounded horizontal scissor `[1,63)` | 477 | 461 | 2920 | 1646 | 487 |

Scissor columns0/63 retained background and clear depth; column62 contained genuine color. Rejected requests preserved native model/matrix/reflection values and revisions as well as all RAM. Immutable spans, unrelated reservations, canonical address stability and final native/task ownership passed. Private render proof SHA-256: `b7d1d1efdc0f150761a72ebe96bcf38307050c799a76e96d59f5baee93916acc`.

Four current-engine RSP/RDP tasks independently isolated reflection bytes: generated UV/color changed while geometry/shade/depth stayed fixed; actual roll changed geometry/color; restored inputs reproduced output exactly. The initial diagnostic's literal zero light count stalled RSP processing; the corrected public-interface black directional/white ambient setup uses count24, without a budget increase. The failed binary/logs remain separate and unchanged. Private camera proof SHA-256: `258380241b6409b98c98cfb3342720edb28c1b39cc20ca33ab77ba0e3fe2fee8`.

The shared cutover also passed sixteen newly executed two-FOV Pidgey/feedback/Oak tasks with every previously recorded score restored, sparse mappings and zero-visible membership intact, zero arena growth and complete retirement. Private regression proof SHA-256: `77df5f20b0329eaa359b518518f2c528f610568651bc71d98b9168a0c36d83ce`. Each of the three graphics workloads had 38 terminal traces, all observed PIDs absent, zero known physical-node rows and zero captured EACCES/EPERM rows. This literal closure observation is not Original descriptor/code qualification.

GCC Debug/full LUS, GCC Release and Clang ASan+UBSan each passed twenty-four CPU component tests; the defined-C enum boundary then passed all three engine executables. Actual GPU diagnostics used Debug with previously qualified CPU Vulkan, not a fully instrumented backend. Other eleven blocks, five dynamic-material blocks, paths/spawns/actors, complete Beach service/scene/gameplay, original first-frame/pixel parity, actual Windows11 execution and release remain unqualified.

## Durable first-fatal evidence and remaining live-context defect

Prospective source corrections passed detached completeness/retained groups, real owned pthread/memfd controls, thirty-six durable-checkpoint cases and a real C child that observed the checkpoint before termination. Both fresh launch/controller gates rejected without changing any artifact before Main source binding and profile-specific authority. The corrected controller retained launch diagnostics, used a literal terminal PID census before its sole qualifier, and preserved explicit1540/1500-second deadlines.

Both newly authorized pipelines then stopped on live evidence and their sole qualifiers finalized rejection. No timeout, retry, failed-trace replay or numerical admission occurred. Exact first-fatal objects retain the adjacent successful anonymous memfd creation/truncation plus36/37 named missing-child snapshot dependencies. Source review proves a sufficient defect: ordinary exec makes the broad-hazard set nonempty, and the global gate disables pending classification, so valid anonymous syntax falls through to filesystem parsing. Other unretained vetoes remain unknown. Rejected roots/helpers/manifests are immutable; candidate-specific explained context and zero-authority live output are being developed only in new source artifacts. No known peer hazard, asynchronous mechanism or final-closure requirement is waived.


## Hosted native Beach/camera component acceptance

Commit [`bafca1214c6d92d88465bf15370e5c13cb965bd0`](https://github.com/PhilosophiMoonbeam/35mmIsle/commit/bafca1214c6d92d88465bf15370e5c13cb965bd0) passed [workflow38834976035](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38834976035), all ten jobs: GCC/Clang/MSVC Debug/Release, real Linux LUS/software presentation, actual Linux CPU Vulkan/RDP, Windows renderer/compiler, and full native MSVC static-runtime LUS/audio/RSP/RDP/engine composition. The full Windows job built the new resource/producer/camera code and passed twenty-four ROMless CPU CTests. This is hosted Windows Server build/component evidence, not private-ROM Windows or actual Windows11 runtime qualification.

## Native startup input, save state and fixed-Y-up camera

The original adapters `game_input.hpp/.cpp` and `game_save.hpp/.cpp` implement source-derived service contracts recorded in `game-input-inputs.json` and `game-save-inputs.json`. Input retains explicit acquisition/publication acknowledgments, four physical ports, error-state retention, source menu/repeat arithmetic and stale-generation rejection. Save initialization applies only to genuinely missing profiles; checked field mutation uses the existing canonical codec and transactional Store. Immutable owning snapshots survive mutation, moved/destroyed sessions and subsequent durable commits. Neither adapter imports an SDK layout/body or establishes full startup readiness.

Parent GCC Debug/Release and Clang ASan+UBSan builds passed the three affected CPU executables: engine, GAME input and GAME save. A separate Linux API consumer performed two acknowledged durable commits, moved its owning session, cold-reloaded generation2 and retained earlier snapshots; it also transferred an explicit acquisition, acknowledged publication, derived a Start menu edge and rejected a pre-reentry completion. The first own consumer incorrectly expected a pressed edge from the first menu query despite source zero-BSS complement state. That failed root is preserved; the corrected consumer first runs the actual idle menu query. Production input code was not changed to satisfy the mistaken expectation. Physical acquisition and complete scene/UI integration remain unqualified.

The thirty-second private GAME definition is unchanged `hal_look_at_roll_f`, `matrix.c:442–506`, SHA256 `43b040ab705e9d17292c7be17c229ba6142777606b2f3695050137fe9c2f9604`. Source `render.c:2294–2300` forces fixed Y-up for kind8; the native cache dirties on eye/target/roll, not up-only mutation. A new canonical consumer completed seven real tasks: four reflection-isolation tasks, then kind8 initial/rolled/restored tasks using source projection-load/normalization/view-projection-multiply and a nonidentity native model, without LookAt upload. Geometry, color and depth changed on roll and restored exactly. Runtime receipt SHA256 `63c61913c06a0ef143d1e343d5f252a484643ca29d62853a74b8f82ecff50fc8`; ROM unchanged,75 terminal traces/74 decoded created tasks/75 absent PIDs, zero known physical-device rows and zero EACCES/EPERM rows. This is bounded authored workload evidence, not Original scene parity or a general resource audit.

## Looping animation and contextual model matrices

The native engine now admits thirty-seven own-GAME definitions across seven immutable source/header blobs. The three additions are `hal_mtx_f2l_fixed_w`, `hal_scale_f` and `func_8001EA90`; exact spans, hashes, C-libm substitutions and strict FP policy remain in [`snap-engine-inputs.json`](../../cmake/snap-engine-inputs.json). No SDK producer/header/layout or `guMtxCatF` body is imported. Contextual RPY nodes store full finite accepted poses without claiming an ordinary local fixed matrix. The one borrowed kernel owns source54 payloads and source48 camera-basis row replacements, preserves source-consumed XYZ transitions, and borrows one retained camera state from application Services.

The existing compiled animator implements source `SET_ANIMATION14`, including retained tracks/rates/receiver, clock reset, model `-2` ordering and material-only clock behavior. Both creation entrypoints require explicit execution-capacity policy: finite-only resource certification or fixed command/event limits. Scalar binary32 preflight rejects overflow or stagnation before publishing state or replacing borrowed events.

| Exercised surface | Observed result and limit |
|---|---|
| CPU integration | All 43 contracts passed GCC Debug, GCC Release and Clang ASan+UBSan, including contextual math/residency and cyclic budget/error boundaries. Independent source14 and native-math reviews found no actionable defect. Authored cases are not actual actor GPU proof. |
| Actual cyclic ROM wire | Initial Snorlax root stream `0x8017ebdc` completed three 25-frame cycles at speed 75 with six ordered callbacks. An eight-command limit rejected atomically while retaining four prior events and clocks. The same consumer retained the actual 292-node, 116-stream Title timeline and exact reentry. This uses an authored one-node API hierarchy, not actor geometry or factories. |
| Existing GPU consumers | Fresh actual five-model, 292-node Title and full World RSP/RDP consumers—eight primary and four secondary blocks—preserved source-pose, placement, binding, visibility, residency, guards, leases and real retirement. New actor54/48 assets were not substituted into these proofs. |
| Actual application surface | Startup→Title→Menu→Name→Oak completed 925 canonical and 925 LUS frames with 1,181,264 checked SDL-dummy PCM frames. Five actual 640×480 window captures were inspected; Name retained three clean text lines and inline A. Confirmed course 0 is a source request, not Beach gameplay. Input is explicitly software-owned, not hardware/LUS-controller proof. |
| Rejected-frame regression | Independent review found a computing World camera getter before late rejection. A new valid dirty-perspective/capacity case failed before correction. Pure `isle_game_camera_peek_matrix`, alias/context/list/model/capacity admission and a single later atomic compute fixed it; the fresh full World consumer passed. The failed root remains immutable/rejected, never replayed or partially accepted. |

Every new runtime root received a stable initial whole-tree closure before its sole cached-text scan: no live traced tasks, zero known physical/permission attempts and unchanged ROM. Exact executable/archive identities, terminal censuses and scopes are centralized in [`menu-animation-inputs.json`](../../cmake/menu-animation-inputs.json) `integration.source14_execution` and [`menu-model-inputs.json`](../../cmake/menu-model-inputs.json) `integration.contextual_matrix_source_status`.

The preceding published shared-instance commit `0b023a78` separately passed [all ten CI jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38887703815), including MSVC Debug/Release and full static-runtime LUS/audio/RSP/RDP/engine. That result does not qualify this later contribution, private-ROM Windows execution or Windows11. Native actor factories/behavior, full source-header coordination, actor GPU, player/gameplay, product entrypoint, Original numerical parity and release remain open.

## Shared model-resource closure and complete Beach actor data

World and ActorResources now use one constructor-only typed reader and relational graphics closure. Complete TIMG tuples, Segment14 bindings and branch latches survive PUSH/NOPUSH, both conditional paths and early CULL returns. Source lanes remain independent at entry; node/pre-list/material/draw ordering discharges local dependencies without guessing cross-object texture state. Incomplete obligations prevent installation.

The actual actor consumer hydrated all 17 models, 79 headers, 33 materials and 1,317 programs, then compiled the pool once and evaluated every header through the shared animator. This includes geometryless1005, common1003, all real cycles, source NULL bindings, five legitimate NULL-image descriptors, and source1006's dominating same-lane texture setter. It also proved atomic conflict/lifetime rejection and isolated selected-byte installation; old World dynamic buffers still correctly reject actor overlap.

Two real integration defects were corrected: a strictly-positive speed guard rejected the source's zero-speed headers; the shared compiler rejected texture opcode12 despite own-GAME support. The latter's interpolation regression failed before correction and passed afterward. The complete actual consumer passed in 0.756s. A fresh full World RSP/RDP run passed the shared-reader cutover in 8.023s. All 44 contracts passed GCC Debug, GCC Release and Clang ASan+UBSan. Exact identities and closed-tree evidence are in [`beach-actor-inputs.json`](../../cmake/beach-actor-inputs.json) and [`model-asset-inputs.json`](../../cmake/model-asset-inputs.json); the failed initial actor root remains rejected and immutable.

Hosted contextual-matrix [CI38895105871](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38895105871) passed seven jobs but failed three Windows configurations: the new byte-hashed header lacked an LF checkout rule. An actual `core.autocrlf=true` checkout reproduced the failure; adding only the missing `.gitattributes` path restored the unchanged strict preparer's 37-definition output. Published resource commit `e0626a3` then passed [all ten replacement CI jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38899109904), including MSVC Debug/Release and the full static-runtime LUS/audio/RSP/RDP/native-engine build. This is not private-ROM Windows11 execution or verification of later working changes.

The native actor cutover uses a planner/reviewer-selected hybrid: native ownership/factory/animation/retirement adapters plus selected pinned GAME C control bodies, not foreign SDK layouts or a second scheduler. Full actor behavior, source sound/FX/detector/player services, actual actor54/48 GPU, combined-scene RAM layout, product entrypoint, Original numerical parity and Windows11 ROM runtime remain open.

## Native actor prerequisites: pose, retirement and environmental sound

The shared animator now publishes node-local transforms through `Runtime::set_node_transform` without a second pose cache, renderer quantization, program restart or hot allocation. Handle/lifecycle/extent/finite validation precedes mutation; unchanged binary32 fields do not dirty state. All 44 CPU contracts passed GCC Debug, GCC Release and Clang ASan+UBSan. A fresh actual 17-model/79-header consumer retained common1003's 59 source root-local Y decrements across real animation steps. This does not execute its native factory or path continuation.

Native object teardown composes pre-payload and joined observers through Services without replacing command ownership. Current/self deletion stays deferred; only actual caller joins certify abandoned source command frames. Independent review exposed an ABA counterexample involving a retained retired record, native-address reuse and an unrelated object's join. The native regression failed before the minimal live-registration guard and passed afterward; independent re-review accepted it. Native joins remain distinct from graphics/readback and effect fences.

`isle-environment-sound` implements the actual GAME category, cutoff, attenuation, pan, pitch, update, stop, cleanup and fixed-parameter rules over the existing audio session. `isle-source-angles` uses the authorized ROM's 4,097-entry angle table and exact source branch/grouping rules, not host `atan2`. Independent review caught an overstrict Y conversion bound: GAME short-circuits that cast when X truncates nonzero. The real-ROM boundary regression failed before correction; the corrected consumer accepts the affected native camera direction and still rejects actually consumed overflow without changing output. Separate planner/reviewer assessments accepted the minimal source correction and existing lifecycle design; source/listener/resource borrow documentation was clarified.

The fresh actual sound consumer passed category coexistence/replacement/stop and cutoff-before-replacement, then executed 57 real audio RSP tasks and accepted 30,496 checked stereo PCM frames. Camera reversal moved energy from right to left; after two actual parked continuations joined and the source model was removed, the fixed voice remained left despite another camera change. No physical sink/hearing, actor geometry/factory, full event routing or independent Original numerical parity is implied. Exact source/table and executable/archive identities, including the immutable rejected boundary root, are in [`environment-sound-inputs.json`](../../cmake/environment-sound-inputs.json).

Fresh existing consumers also passed full World geometry—eight primary/four secondary blocks—and Startup→Title→Menu→Name→Oak: 925 canonical frames, 925 actual LUS presentations and 1,170,080 checked SDL-dummy stereo frames. Five actual window captures were inspected; Name retains three clean lines and inline A. Every new runtime tree was closed and stable before its sole physical/permission scan: zero known attempts and unchanged ROM. Identities and censuses are in [`snap-engine-inputs.json`](../../cmake/snap-engine-inputs.json); local-pose evidence is in [`menu-animation-inputs.json`](../../cmake/menu-animation-inputs.json).

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

All successful runtime trees closed before their sole initial observation: stable complete tasks, zero known physical/permission attempts, unchanged ROM. Rejected camera-alias, prerequisite and distant-fixture roots remain wholly rejected and immutable; none was replayed, rescanned, visually inspected or partially accepted. Temporary diagnostic pose/call dumps were removed after proof. Source/executable/archive identities and scope are in [`actor-control-inputs.json`](../../cmake/actor-control-inputs.json), [`actor-producer-inputs.json`](../../cmake/actor-producer-inputs.json), [`focus-detector-inputs.json`](../../cmake/focus-detector-inputs.json) and [`menu-model-inputs.json`](../../cmake/menu-model-inputs.json).

Published prerequisite `a72f332` built the Windows targets but failed three Windows CTests in [CI38905019507](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38905019507): the camera-OOM fixture assumed exhausting larger GObjs also exhausts smaller camera/matrix allocations. Explicitly consuming the remaining public arena tail corrected it. Actor integration `d8bb80e` passed [all ten replacement jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38915801831), including MSVC Debug/Release, renderer/compiler and full static-runtime LUS/audio/RSP/RDP/native engine. This is hosted Windows Server build/component acceptance, not Windows11 private-ROM execution or verification of later working changes.

Native actor factories/header coordination/full FSMs, actor/player/items integration, complete original detector snapshot/update queue/GObjs, combined World/actor/FX scene RAM, all-course gameplay, product entrypoint, independent Original parity and Windows11 private-ROM execution remain open.

## Shared native FX and actual live-focus cutover

The sole `EffectRuntime` replaces the Title-only class; no compatibility renderer or second pool remains. Immutable selection covers nine source groups, 73 scripts and 34 images; active slots remain common bank 3 plus course bank 0, not nine simultaneous slots. One 100-particle/10-emitter owner retains 16 encoded lists, application RNG/source-ID semantics, source interpreter/physics/children, local targets and joined detach/reset. Kind-2/flag-4 parent coupling remains explicitly unadmitted; actual selected descriptors use kinds 0/3. See [`effect-bank-inputs.json`](../../cmake/effect-bank-inputs.json) and [`menu-effects-inputs.json`](../../cmake/menu-effects-inputs.json).

Actual private resource consumption passed every group, borrowed alias/packing/Title identity and moved-owner rejection. The real RSP/RDP consumer passed six image formats, 48-pixel textures, tenth frame, independent completed texel/color/depth oracles, same-CI4-after-large-I8 reload and exact 16-entry palette isolation. Course renderer-only scripts are explicitly authored; actual intro scripts 0/3 remain separately exercised.

A throwaway non-test API example admitted every source bank and exercised 73 particle plus 73 attached-emitter source-script prefixes, eight update pairs each, with a real Runtime-issued identity/native DObj and 240 raw-world resolver calls. Functional native target services were supplied, but these prefixes invoked no local reads/writes; target/all-branches or whole-course parity is not inferred. The example source/build target was removed after smoke; its exact private frozen archive remains evidence.

The raw-world helper follows accepted native local S/R/T from leaf to root and root auxiliary RT/RTS, independent of flags, renderer caches and contextual matrix recipes. Actual Title synchronizes accepted animation poses before events/emission. Source column normalization remains in `EffectRuntime`. The complete real World consumer translates source-created particles across all seven primary entries without advancing velocity/RNG; the diagnostic `effect_position` surrogate is gone. This is not combined World+FX rendering. See [`source-effect-world-inputs.json`](../../cmake/source-effect-world-inputs.json).

The complete six-actor GPU consumer supplies genuine per-callback center rows and canonical GObj Pokemon id/flags. Stable source DL/priority order merges the detector prefix; DL4 captures append to the previous context before prefix, then the prefix flips context and always saves a NULL baseline—even when actor saving is disabled. Actual GPU/readback and CPU analysis retire before the single shared 20-row pool is reused. Rejected preparation preserves RAM/native state, both histories, photo state and computed/published focus.

Independent planner/reviewer assessments rejected template-origin framing: the initial Pikachu animation hides nodes 1/2. Evaluated visible node 3/lane 0 geometry and source quantized matrices derive the authored placement, leaving source pose, scale, LOD, cohort, 32-pixel threshold and budget unchanged. Actual Snorlax/Pikachu rows each changed 64 full-16-bit pixels; flag 0x100/native hide restore Snorlax, exact reveal restores Pikachu, and preprefix/context/disabled-baseline/cleanup/deferred-publication gates pass. Failed framing roots remain wholly rejected, immutable and uninspected. See [`actor-producer-inputs.json`](../../cmake/actor-producer-inputs.json) and [`focus-detector-inputs.json`](../../cmake/focus-detector-inputs.json).

`CanonicalExecutor::MemoryLease` owns an explicit completed CPU write phase over actual persistent processor RDRAM. `finish_postprocess()` performs real consumer writeback; moving the lease transfers pending writes, released leases reject writers, and unfinished release poisons reuse. A separate six-task GPU consumer proves subsequent GPU consumption of retained CPU edits. The actual actor path marks exactly 21 red positions while preserving other pixels/hidden fingerprint. No abandoned preparation, remap, writable hidden memory or synthetic fence is used. See [`focus-capture-inputs.json`](../../cmake/focus-capture-inputs.json).

Integrated Debug/Release/ClangASanUBSan each passed 49 CPU tests. Native startup passed 643 canonical/LUS frames, 1,257,040 dummy PCM frames and five inspected logo/Title/menu/Name/early-Oak surfaces; the source course-0 request remains a request, not gameplay. Every accepted runtime tree closed before its sole initial observation: stable complete tasks, no known physical/permission attempts, unchanged ROM. Published `e0d3b2f` passed [all ten hosted Linux/Windows jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/38928147438), including MSVC Debug/Release, renderer/compiler and full static-runtime LUS/audio/RSP/RDP/native engine. This does not qualify later ActorOwner/source-pause/combined-scene work; Windows11 private-ROM/device execution and Original numerical parity remain unqualified.

## Native common1003 ownership and canonical-pose cutover

The common1003 baseline coordinates its actual factory, normal path/cleanup and hidden/retrigger FSM through six core plus nine owner-side pinned GAME C definitions. It owns one canonical source record per real native GObj, one shared animation Runtime and one shared FX owner. At this baseline, other configured factory/state/graph identities rejected admission; the subsequent Magikarp source/data cutover is qualified below. No guest callback cast, SDK object layout, second scheduler or animation clock is introduced.

The clean actual-ROM CPU consumer passes GCC Debug and Clang ASan+UBSan. It executes the 59 decrement-before-wait path writes and final wait, two natural hidden/retrigger cycles, actual OM Update/FIFO counter/end ordering, source command suspension/resumption, all 79 actual native header contexts, immediate/root sound-event precedence, raw-world attached emission and independent-particle cleanup. Duplicate retiring-sender delivery, unrelated native allocation, receiver cancellation, collection/reuse and final engine/audio destruction complete. Native identity retention prevents address reuse until the real joined object and its command borrows retire; it is not a GPU fence.

| Clean CPU consumer | Seconds | Closed terminals / created / absent, including tracer |
|---|---:|---|
| GCC Debug | 2.430761403 | 26 / 25 / 27 |
| Clang ASan+UBSan | 7.926716915 | 26 / 25 / 27 |

Both trees were stable and absent before their sole initial observation: unchanged ROM and zero known physical/permission attempts. Audio used an owned capture sink. Address/undefined-behavior failures remained fatal; leak scanning was disabled only because the complete tree was ptraced. Exact executable/archive identities and scope are in [`actor-owner-inputs.json`](../../cmake/actor-owner-inputs.json). Temporary header-failure diagnostics were removed before these clean proofs.

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

Every accepted tree was complete, stable and absent before its sole initial observation: unchanged ROM, zero known physical/permission attempts. Temporary particle/raster diagnostics and API packing scaffolds were removed after proof. Executable/archive identities, precise scopes and rejected-before/passed-after records are centralized in [`course-scene-inputs.json`](../../cmake/course-scene-inputs.json); all byte-hashed support paths have explicit LF checkout rules.

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

Every accepted complete tree was stable/absent before its sole initial observation: unchanged ROM and zero known physical/permission attempts. Address/UB failures stayed fatal; leak scanning was disabled only under ptrace. Exact twelve-case scopes/executable/archive identities are in [`beach-magikarp-scene-inputs.json`](../../cmake/beach-magikarp-scene-inputs.json); source numeric/service scope is in [`actor-photo-eligibility-inputs.json`](../../cmake/actor-photo-eligibility-inputs.json). The following cutover qualifies genuine gated membership/scheduling/GPU focus; full Item/Player services and complete port acceptance remain open.

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

Every accepted complete tree was stable and all tracer/tasks absent before its sole initial physical/permission observation: unchanged ROM, zero known attempts/errors. Address/UB failures remain fatal; ptrace-only leak-scan disable is not leak qualification. All51 CPU contracts pass Debug/Release/Clang ASan+UBSan. Exact identities and scopes are in [`course-magikarp-inputs.json`](../../cmake/course-magikarp-inputs.json), with component contracts in [`actor-photo-membership-inputs.json`](../../cmake/actor-photo-membership-inputs.json) and [`actor-photo-scene-inputs.json`](../../cmake/actor-photo-scene-inputs.json). Full snapshot payload/queue/reconstruction, native Item/Player gameplay, additional species/courses, product entrypoint, Original equivalence and private Windows11 execution remain open.

## Complete native collision manager and command26 sender lifetime

`Owner::update_collisions` replaces the refresh-only API without an alias. The existing source logical tail-order list drives the complete pass: refresh every canonical collision position and clear process80, then visit each flag200 actor against later flag200 actors. Outer coordinates remain cached across its inner loop; each inner coordinate snapshot is current when visited. Contact is strict three-dimensional distance minus the float radius sum below zero. Command26 sets process80 before enqueue, once per endpoint per pass; suppression does not suppress later displacement.

The source checks the inner actor's flag800 twice. Native code preserves that behavior, not its unreachable branch: inner800 moves both endpoints by half penetration even when outer800 is clear; otherwise outer800 moves only the outer endpoint by full penetration. Both half-moves use their pre-pair snapshots. Collision position is recalculated after every attempted walk, including ordinary blocking. The source same-Z fallback can move both actors toward positive X; the unsuffixed-double `0.00001` comparison, strict tangency and cached three-actor order are independently exercised.

One semantic `IsleActorGround` retains height, normal and unsigned surface; `currGround` is canonical actor state. `StepWalk` queries destination directly into it before filtering, then probes old AUX plus source-normalized velocity times signed radius into a separate result. Default forbidden surfaces and consumed-only borrowed float-list replacement retain source order/float comparison/zero-or-first-match termination. Fixed-height uses exact equality; ON_GROUND publishes destination height only after acceptance. Blocked walks retain destination ground but not placement edits. Native RT/RTS rotation/scale/root-local pose are preserved; zero/subnormal normalization and finite signed radius are not replaced with invented normals or nonnegative validation.

Absent current block, descriptor or map preserves real source `-9000/up/UINT32_MAX` ground; the spawn factory's separate height-zero fallback remains separate. Existing World traversal now bounds consumed cycles to its finite node extent rather than adding a second prewalk. An unchanged pure native self-cycle program timed out before correction and exits with the checked default afterward; an unvisited cyclic sibling remains legal. Actual GDB inspection also found a NULL current-block descriptor dereference; the central World guard now preserves the source fallback. These source-undefined/missing-input safeguards do not establish Original numerical parity.

Command26 uses the same successful-enqueue credits as21/22, not another identity cache. Actual receiver delivery or queue cancellation releases its credit; joined sender address and canonical physical slot remain held until all credits close. Existing `collect_joined` first-held-slot failure and any preceding successful collection remain unchanged. The real API consumer exercises seven address/slot/new-incarnation reuse proofs, multiple senders/receivers, FIFO deletion, NULL-graph target/state invariance, genuine process pause/resume and failed admission. Each configuration executes eight actual audio tasks, 9,120 checked PCM samples and two stop retraces under the unchanged maximum240 drain protocol.

| Complete actual private consumer | GCC Debug seconds | GCC Release seconds | Clang ASan+UBSan seconds |
|---|---:|---:|---:|
| Native command26 FIFO/lifetime API | 0.792618 | 0.511728 | 4.092021 |
| Native complete pair-manager API/order/ground/scheduling | 0.797287 | 0.420287 | 4.129873 |
| New actual terrain/collision GPU, six frames | 7.935782 | 11.275420 | 17.778145 |
| Byte-unchanged genuine source focus,32 GPU frames/518 updates | 9.511950 | 12.453462 | 23.971321 |
| Preserved full common source/all79-header CPU cohort | 3.961742 | 1.216943 | 12.270462 |
| Preserved full Magikarp source/allocator CPU cohort | 1.624908 | 0.437555 | 4.017640 |
| Preserved scene helper/chooser/analytic predicate CPU cohort | 1.396583 | 0.423478 | 3.787628 |
| Byte-unchanged original nongated12-frame composition | 12.494804 | — | — |
| Byte-unchanged original complete six-actor GPU cohort | 18.641631 | — | — |

The new GPU fixture is explicitly an API experiment over three genuine factory actors, not admission of the original nonNULL-graph collision state. Its procedure was fixed before observation: four native updates/six GPU frames, legacy55 camera/default shared RNG, existing initial `(0,1233,Z)` points with Z4000/4100/4175, radius100, immutable empty forbidden list and actual source-process suspension after one real distance/state/animation update. No flag40 bypass, source header/scale/LOD rewrite, endpoint/camera search or source32-threshold change. Root priority2 produces first endpoint Z3937.5 and actual terrain Y400; later endpoints land on the real sloped terrain. Independent native positions and collision fields are checked before rendering.

Actual completed memory records 516 changed RGB5 pixels after the collision tick and 57 when the first native actor is hidden; these are observed differences, not a claim that every pixel change excludes World animation. Repeated rendering and hide/reveal restoration preserve every color/depth halfword exactly. All four saved collision surfaces are byte-identical across the three configurations and their lossless contact sheet was directly inspected. All five original genuine-focus surfaces also remain byte-identical to the previous inspected proof; negative 25/27/0 samples, positive source32 comparisons, CopyInfo timing, 21-position marking and cleanup/reuse remain intact. The original common-only and full six-actor consumers are not replaced or edited.

Three initial new roots are wholly rejected because the consumers omitted required World camera/effect-shift services. Production correctly rejected them. New consumers share complete real camera/FX translation and ordered World retirement while Owner is alive; no guard suppression or no-op callback was added. Failed roots are never reopened, replayed, rescanned, visually inspected or partially accepted.

All 52 CPU contracts pass GCC Debug, GCC Release and Clang ASan+UBSan. All 23 accepted private trees were complete, stable and absent before their sole initial physical/permission observations: unchanged ROM and zero known attempts/errors. Fatal address/UB checks remain active; ptrace-only leak-scan disable is not leak qualification. Exact per-root counts, executable/archive identities and scopes are in [`actor-collision-inputs.json`](../../cmake/actor-collision-inputs.json). Compiler/hosted evidence remains tied to its published revision. Complete interaction graphs/native Item/Player services, other species/courses, snapshot reconstruction, product entrypoint, Original equivalence and private Windows11 execution remain open.

## Shared source lifetime, native spatial queries and optional ceilings

The Actor-only sender matrices are removed. The same `scene::Services` registration now owns one native identity hold and semantic queued/in-flight/field counts for every participating source owner. Registration borrows the actual canonical field addresses; assignment adopts the new sender before releasing the old. A persistent active command token survives manual delivery and canonical-field adoption, with exactly-once normal-return/current-token/proven-caller-join settlement. Ordinary FIFO suffixes are preserved. Retiring receivers close their own fields to break cycles, not other live receivers' fields; source21/22 ordering remains observable. Joined collection requires all source borrowers and the independent graphics boundary, and preserves first-held-slot failure with earlier collection progress.

Actual integration exposed an unregistered-sender hole: successful enqueue retained no semantic identity, the native address was recycled, and delivery adopted the replacement object as the old sender. The permanent real-native regression fails before correction and passes afterward. Source-owned custom queues now reject nonNULL unregistered senders before allocation/acquisition/publication; built-ins never consume that identity and unrelated menu owners remain opt-in. Source lifetime tests also cover persistent target adoption/replacement, cycles, cancellation, parked cross-object dispatch and actual caller join. No C++ unwinding is required to release a transfer token.

`actor_spatial.h` supplies status-returning, nonwaiting distance and neighbor leaves. Distance uses actual AUX when enabled, otherwise actual root position; NULL arguments and live NULL second models produce `FLT_MAX`, while invalid first models/retired handles reject. Neighbor traversal uses actual native link3 across owners and excludes only self: full 3D source direction away from each neighbor, strict distance100, ordered distance-weighted accumulation and reciprocal-multiply normalization. Hidden, intangible and paused actors still contribute. Canonical `dirToNeighbors` is not a renderer or pair-membership cache; failures preserve outputs, and C++ returns before any C fault.

World now exposes both global and local optional-ceiling queries through the existing single bounded height traversal. Global absent-map output is the source default; local absent-map output is untouched. Present-map missing-block/descriptor, translated/sloped maps, consumed bad branches, ignored siblings and retirement are covered on actual native Worlds. Beach remains correctly NULL: pinned initialization installs only its height map, and neither source setup nor decoded metadata supplies a ceiling pointer. No pointer, decoder or constant-false provider is invented.

The shared height normal had normalized swapped `(A,C,B)` using component division. Source normalizes `(A,B,C)` with left-associated float squares/reciprocal multiplication, then swaps axes. An unchanged pure probe over plane `(1,4096,1.5)` fails against published code with bits `397ffffe/39bfffff/3f7ffffe`; corrected code passes with `39800000/39c00000/3f800000`. Permanent regressions cover rounding order, reciprocal arithmetic, signed zero and subnormals. Temporary probe sources/objects/binaries are removed; this native source policy does not claim Original floating-point equivalence.

Typed `IsleActorInteractionGraph` replaces the untyped field. `BehaviorBindings` translates a complete requested callback/graph/weighted callable-data closure once, retaining ordered rows/float values/NULL semantics and exact native callback identities. Dependency cycles terminate; an unresolved transitive or later executable callback rejects the whole constructed owner. Terminal/unknown-nonexecutable callback words never become callable guest addresses. Actual CPU-only execution covers the three compiled common callbacks, their real dependency cycle, missing path callback, terminal-only graph and moved-Snapshot rejection. It inventories all 71 graphs/212 rows/eight weighted lists, including 72 distinct graph callback identities and 10 command16 rows; empty registry rejects 70 nonempty graphs/all eight weighted lists and admits only the source terminal graph. This is callable-data admission, not completion of the still-unimplemented full C interpreters/factory/service closures.

| Complete actual private consumer | GCC Debug seconds | GCC Release seconds | Clang ASan+UBSan seconds |
|---|---:|---:|---:|
| Expanded source FIFO/lifetime API | 0.801978 | 1.313741 | 5.282498 |
| Complete native collision manager API | 0.765843 | 1.289254 | 5.539011 |
| Fixed six-frame/four-update terrain GPU | 8.844602 | 21.201201 | 32.982240 |
| Unchanged genuine32-frame/518-update focus GPU | 11.254877 | 23.439143 | 42.049227 |
| Full common/all79-header source CPU | 6.032968 | 1.724496 | 16.084282 |
| Full Magikarp source/allocator CPU | 2.209269 | 0.805652 | 5.902123 |
| Full scene helper/chooser/analytic CPU | 2.258422 | 1.129828 | 5.181765 |
| Unchanged original common12-frame GPU | 22.697284 | — | — |
| Unchanged original six-actor GPU | 12.116747 | — | — |
| Actual CPU-only native callable-data closure | 0.408491 | — | — |

The expanded FIFO consumer proves ten actual address/slot/new-incarnation reuse cases per configuration, including arbitrary custom commands and earlier collection before the first held slot. Each run executes nine actual audio RSP tasks, 10,176 checked PCM samples and two stop retraces. All three full original GPU consumer source files are byte-unchanged. All four pair and five genuine surfaces remain byte-identical across configurations and to previously directly inspected images. The preserved six-actor consumer retains 437/23 RGB5 visibility differences, source 64/64 focus rows, actual 21-position/21-changed completed-memory marking, hidden/depth protection and retirement.

Three parent qualification roots are wholly rejected. The first CPU-only data run inherited a generic required-GPU scope: its premature acceptance was rescinded, derived inventory deleted, and fresh identical code/parameters ran under explicitly NONE graphics/audio. Two six-actor invocations separately lacked the original CLI arity and required existing empty capture directory. Fresh roots correct only those prerequisites; no production source, binary, cohort, pose, camera, timer, threshold or budget changes. Rejected roots remain immutable after final rejection, with no subsequent access or partial data used in accepted results.

All 53 CPU contracts pass GCC Debug/Release/Clang ASan+UBSan. All 24 accepted complete trees have sole initial closure/physical-permission observations, stable traces, absent tracer/tasks, unchanged ROM and zero known attempts/errors. Fatal address/UB checks remain enabled; ptrace-only leak-scan disable does not qualify leaks. Exact executable/archive/source scopes and both failing-before/passing-after bugs are recorded in [`actor-interaction-inputs.json`](../../cmake/actor-interaction-inputs.json). Full C interaction execution, all source callback/services, actual Item/Player simulation, snapshot/reconstruction, other species/courses, product executable, Original parity and private Windows11 execution remain open.

## Original camera callbacks and native source render phases

Thirteen complete definitions from pinned own-GAME Player/camera/light code compile through the checked C importer. Camera command handlers are promoted from the reference fixture into the production command library, without duplicate definitions or aliases. Native backend leaves return before C failure transfer; re-entry uses nested plain-C fenced calls. No SDK implementation, guest CPU execution or C++ gameplay state machine is introduced.

`player::Renderer` retains scheduling metadata for 32 real camera/helper records, not copied camera poses. Main, returned UI and actual ActorOwner −6/−7 helpers preserve native scheduling priorities, insertion ties and distinct scheduling/projection identities. Visibility is resolved at each dispatch; sprite views are gathered after earlier callbacks. A secondary Services lifecycle observer runs before the primary semantic owner without replacing it. Native join is not GPU completion: camera identity reuse also waits for actual Frame/readback and every Photo ticket.

SourceControl separates camera initialization, producer preparation, bounded packet emission, secondary-lane camera restoration and finalization. Borders retain source ONE_CYCLE/NOOP; blink retains FILL/NOOP followed by ONE_CYCLE/AA-ZB restore. The original light updater computes three signed bytes in native OM; their later upload does not recompute direction from a changed camera. Cold reservations precede first preparation. Sprite command storage occupies 0x780000–0x7d0000, outside the actual Actor assets that correctly rejected the initial 0x150000 arena.

Global composition uses actual Services DL3/4/5 order across World, Actor, Item, Cart and the genuine PhotoScene prefix object. Raw fragments exclude owned continuation slots; unpushed continuations restore camera state after provider changes or actual sky resets. Source composition does not copy hot geometry or append families in a substitute order. The separate three-field diagnostic join remains self-contained because its real consumers re-prepare provider lists before submission. The actual prefix's hidden flag is projected before membership collection/history staging, not merely used to omit GPU commands afterward.

Four source cursors preserve GPU order 0→2→1→3 and the literal lane 1→3 reload appended to lane 0. Per-camera combine returns to current lane 0; final-GTL emits real FullSync/EndDL without an invented normal-program restore or extra camera. Frame finalization validates that tail and avoids a duplicate sync. Actual ROM programs 2/8, their full cold regions, reload packets and reset commands remain strict Snapshot-backed inputs; a selected program 8 task really executes 8. Explicit LINE3D diagnostics use admitted Cart vertices, not a fabricated original line asset. No genuine source-selected ROM line asset has yet been qualified; absence of such assets is not established.

The production Photo Controller uses the sole Photo pending registry. Actual prefix capture precedes history commit; request seals during preparation, before submission. Enqueue adopts the real Frame/token; completed memory, completed poll and later OM CopyInfo remain separate authorities. Detector context/generation is independent of framebuffer context and submission serial. Mutable lighting and immutable effect assets retain the existing ownership convention; collision guards were not weakened to admit the first incorrect fixture.

The 508-file native snapshot `aa0acc082a81eee141323ca5d6a53778dad44db6e565af656b26a10ba0e96417` builds all 24 opt-in consumers and default targets, then passes all 57 CPU contracts in GCC Debug, GCC Release and Clang ASan+UBSan. All 72 complete actual component cases share that snapshot: 71 fresh runs and one already-accepted exact-source/binary Debug Photo proof, never replayed. Their 5,090 terminal tasks, 5,018 created tasks and 5,162 tracer/task absences were established before each tree's sole initial physical/permission observation. Traces are stable, ROM unchanged and known attempts/errors zero. Fatal Address/UB checks remain enabled; ptrace-only leak-scan disable is not leak qualification.

The camera consumer executes normal→FIFO blink→normal, real provider/prefix/FX/UI geometry, hidden-prefix history behavior, Photo publication/lifetimes, actual source failures, independent RAM/epochs, native camera reuse, task 8 and all eight auxiliary-lane combinations in both combine and deferred-finalization paths. Exact known-positive checks preserve all preceding 63 images, 213 Photo files and 12 Cart files; 60 new camera output/report files match the first accepted Debug GPU baseline across all profiles. Normal and blink surfaces were directly inspected: the source blink blacks the scene while independently eligible UI remains visible. No expected output was repinned or obtained from a rejected tree.

All 125 byte-hashed native support paths across 58 manifests have explicit LF policy and retain the exact qualified bytes through actual Git index filtering with Windows `core.autocrlf=true`. Hosted MSVC evidence remains tied to its published revision. This is component evidence, not original initUI/Player 7/full 106-definition execution, complete gameplay, private Windows11 runtime, LUS presentation or hardware/hearing parity.

Camera flags `0x10`/`0x40` remain explicit unsupported mid-frame task boundaries until real native multi-task transactions implement source process/reset semantics. A hidden prefix does not authorize a successful ProcessImage drop: pinned code nonblocking-enqueues the current existing Detector context regardless of a new prefix capture. That retained-snapshot request path remains separately tracked and fail-closed. Neither limit is an external SDK blocker.


## Complete ordinary first-entry behavior and factory closures

The actor materializer selects 130 complete pinned own-game C definitions: the preceding 68, 60 for ordinary Lapras/Pidgey/Pikachu and two for sign `1004`. The callback registry retains 68 genuine callbacks, 20 graphs, six initial roots and weighted list `0x802CDE30`. Every executable dependency is admitted before factory allocation or RNG/process mutation; alternate singleton factories remain rejected. No C++ gameplay state machine, SDK layout alias or unknown-callback fallback was introduced.

The exact first eight ROM rows `[12,84,25 behavior 4,1004,16,16,16,131]` execute through their original factories before Player object/model and FX publication. The same objects subsequently run their own original initial callbacks under native OM. Native model/path/root-motion, weighted, flute/apple, flock/link-command and cleanup cases then execute. Lapras behaviors 2/4/5 retain their nonreturning shared-signal waits; real command 24 transitions unlock the appropriate cohorts, without inventing the original sender. Native scene reconstruction and later actor reuse distinguish scene-owned state from process globals or per-context resets.

Four mutable source counters initialize once per retained Actor `CourseState`, from actual ROM words at `0x564E6C..0x564E77` and `0x5650D0`; Pikachu's immutable temporary vector comes from `0x566088`. Item consumption reuses the genuine Item owner and original deletion path. Commands use the actual checked native queues and link order.

Pinned `562370.c:10–54` and actual ROM definition/setup tuples establish that actor `1004` is the progress sign: factory `0x802CA418`, initial state `0x802CA300`, seven nodes and header `0x802CD920`. The ending/camera/audio/Cart closure belongs to separate actor `1001`, factory `0x802CAB14`/initializer `0x802CDA10`; it is not a first-entry sign dependency. Real native Movement, not injected progress values, exercises the sign's `0.92` recognition and `0.99` clear/nonreturning-end boundaries.

Actual integration found and repaired four defects without weakening guards or expected behavior: Pidgey `802C85E0` requires its real `arg0` parameter; sign C requires the pinned own-game flag-4 value; the consumer's last-ID lookup selected alternate singleton Pidgey instead of the ordinary World table; native initial-state dispatch still routed newly admitted families to Doduo. The latter failed the retained source-state regression before the real callback routing was corrected. Both rejected runtime trees were wholly closed and observed once, never replayed or used as accepted output.

The 508-file snapshot `4596925a0b9079c7b1b01c7d2b70f0ed781e45c4b36b882694106791236956fb` builds all 24 opt-in consumers and default targets and passes all 57 CPU contracts in Debug, Release and Clang ASan+UBSan. The complete new first-entry consumer passes all three profiles: 603 terminal tasks, 600 created tasks, 606 tracer/task absences, stable traces, unchanged ROM and zero known physical attempts or permission errors. Each run performs four actual audio-RSP tasks and owned PCM drain. Fatal Address/UB checks remain enabled; ptrace-only leak-scan disable is not leak qualification.

The complete regression matrix accepts all 72 source-identical component cases: 69 fresh executions and the three exact-source/binary CPU proofs above, never replayed or rescanned. There are 5,634 closed terminal tasks, 5,562 created tasks and 5,706 tracer/task absences, stable traces, unchanged ROM and zero known physical attempts or permission errors. Exact checks preserve all 63 preceding images, 213 Photo files, 12 Cart files and 60 camera output/report files. Only previously declared positive paths were read; no expected output was repinned or taken from a rejected tree.

All 125 strict SHA256 support inputs retain the exact qualified bytes through actual staged Git filtering with Windows `core.autocrlf=true`; each has explicit LF attributes and no CRLF bytes. Descriptive nonhashed save-support entries are not counted as byte-hashed inputs. Existing source/hash checks were not weakened.

This snapshot qualifies the complete ordinary first-entry component path, not execution of the complete 106-definition Player Root, later species/course closures, full gameplay, private Windows11 runtime or hardware/hearing parity. Its published revision `23f4c05` subsequently passed [all ten hosted Linux/MSVC jobs](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39054672591), including all 24 then-existing opt-in consumers without a ROM. That receipt does not qualify subsequent Root changes.

## Complete original Player Root

`isle-player-root` now compiles the entire pinned `player.c` definition set:
92 Root bodies, two canonical engine-support bodies, ten renderer bodies and
two command bodies. Exact ownership covers all 106 definitions once; the old
partial command-reference importer and duplicate Actor focus-cleanup body are
removed. Generated GAME C remains build-private. Thirty-seven returning native
leaves compose the existing domains without a second gameplay FSM, opaque SDK
layout or SDK implementation import.

The retained Root owns genuine Player7, reticle25 and pause26 identities. Cold
World/Actor/Item/UI/Photo/Cart construction precedes original publication; all
eight initial World factories execute before the Player model is published.
Physical port-zero software input is published once before OM, after which
the original idle and pause functions own their canonical mutations. Save
report mapping uses the actual 69 slots. The complete original Beach music
callback adopts its ROM-initialized state; an absent song preserves the
original rejected-request/nonfatal behavior. Original C Detector Enable and
Disable replace a C++ game-state assignment.

Source drawing retains late message and transition cameras in the same actual
task. Pinned `sys/om.c:404–430,1633–1640,1736–1755` establishes **tail insertion
after equal camera priority**, not head insertion. Native viewport lanes and
projection remain unmodified; only physical scissor is target-clipped, using
the original separate integer `/4` arithmetic in `sys/render.c:2073–2092` and
`2721–2741`. Source-controlled secondary restoration also reapplies the current
pass scissor without clears or redundant full-target emission. The defined
`SP_OVERLAP` attribute is inert in the original draw contract and is preserved;
external, undefined, stale and immutable-layout guards remain strict.

ProcessImage retains the original two-entry FIFO of context IDs, not copies of
enqueue-time pixels/generations. Eligible completion resolves current context
metadata against the real shared GPU rows. Queue membership is independent of
GPU/shutter/history capabilities; callback removal does not discard queued IDs.
These paths and the signed drawn-frame counter are implemented; their expanded
standalone renderer/Photo cohort still requires refreshed runtime/parity proof.
Flags `0x10`/`0x40` remain real multi-task implementation work, not SDK blockers.

### Initial Debug component qualification

The 519-file source snapshot
`4b884cd4fbef2e1ff65d5e8594f8a6a7b526fef66c97dcd636009390794dbe40`,
binary `db6876436daa0438a2c7b899cf1c149d05f059964f083179647d10b38e925fd3`,
passes the independent real Scene camera contract and every genuine Root case:
fresh tutorial with three qualifying photographs, original zoom/pause/resume,
interrupted idle, completed idle and the ordinary no-song third music loop.
The run took 110.513670 seconds. Its sole complete closure established
335 terminal tasks, 334 created tasks and 336 tracer/task absences, stable
traces, unchanged ROM and zero known physical-device attempts or permission
errors before the initial bounded artifact observation.

Camera regression checks all eight original viewport lanes, bounded clears,
exact one-cycle raster scissoring, narrow secondary restoration and actual
poll/readback/native destruction in both contexts. FILL-cycle scissoring is
four-pixel-granular, so it is not used as an exact-scissor oracle; expected
edges and colors were not repinned. Root visibility observes RGB blackening,
not primitive alpha: the original iris legitimately emits primitive alpha
zero. Both idle terminal images contain 76,800 RGB-black pixels.

Original terrain/Pidgey, HUD, message, pause and resume surfaces were directly
inspected. Photo serialization and qualifying counters advance through original
C. Native deletion, semantic collection, GPU/readback retirement and owned PCM
drain all close independently. Renderer detachment retires the real main-camera
receiver before collecting FX identities held by its canonical fields; no
synthetic field clear, OM retry, fake fence or leaked-graph success is used.
Every earlier failed tree remains wholly rejected and closed, never a source
of positive outputs or expected-value changes.

### Three-profile Root qualification

The final Root input snapshot contains 519 files, SHA-256
`6e6a821134d808ab20aedc74c189caf7b8c13a8c055b755b9d48a60ca4338336`.
The independent Scene camera contract and all four genuine Root scenarios pass
in each profile:

| Profile | Executable SHA-256 | Runtime seconds |
| --- | --- | ---: |
| GCC Debug | `30316d58880d2da6e643386e4af7a42bfc7cec831f65c79b3968ea93a8c47839` | 124.065495 |
| GCC Release | `888bc46b2ab4c2e6a047a81a14f17b80b940fbb7bce4fac380b28241df1fe8e9` | 83.670132 |
| Clang ASan+UBSan | `2c952ef21aa074ac454ad0fdd6f14f95ceefa6810c7005e6a4417a4dd01789bf` | 235.092938 |

Each sole complete closure establishes 335 terminal tasks, 334 created tasks,
336 tracer/task absences, stable traces, unchanged ROM and zero known
physical-device attempts or permission errors. After closure, one initial
bounded observation reads 68 declared artifacts per profile. All 59 binary
raster/depth/Photo positives match exactly across the three profiles; the
previously inspected twelve Root RGBA surfaces also remain byte-identical.
PCM and chronology are actual execution evidence, not repinned raster oracles.
Fatal ASan/UBSan checks remain enabled. The ptrace-only LeakSanitizer disable
and generated uninstrumented JIT do not establish leak or JIT-instrumentation
qualification. No accepted or rejected capture tree is reopened afterward.

### Build, change record and remaining boundary

GCC Debug/full LUS, GCC Release and Clang ASan+UBSan each build all default
targets and all 25 opt-in consumers, then pass 57 CPU tests. Final Root-only
raster-witness/capacity changes are subsequently rebuilt and executed in all
three profiles above. Refreshed older-component parity remains in progress.
The expanded actual Photo Debug consumer passes 66 GPU frames, two intentional
completed-callback failures and 79 original-C CopyInfo updates. Its initial
71-file observation matches 70 prior hashes. A separate fresh immutable public-before
build reproduces all 71 old hashes, without reopening any earlier capture.
The reset record differs only at wire offsets `0x1ac`, `0x1ad`, `0x1af`:
effect0 primitive R/G/A changes from `88/34/12` to `96/38/24`; blue is 255 in both.
Both records have `textureID == -1`; all other 925 bytes are exact.

Pinned `47380.c:714–728` initializes absent effects by writing only `textureID=-1`;
`:766–775` copies the complete record, including inactive payload; `:1438–1441`
skips inactive effects before reading that payload. The expanded failure/FIFO
scenario genuinely recreates Photo/PhotoScene, retaining a different inactive
context color. This is Source-ignored residue, not a consumer-visible regression.
Both strict raw hashes remain recorded, with this exact semantic adjudication;
there is no 71-file raw-parity claim, golden repin, synthetic color/clock reset
or zeroing of original inactive payload.

The subsequent 527-file Debug integration compiles both new original
application-manager and Beach-lifecycle Source providers, all default targets,
all 25 consumers and 57 CPU tests. Its expanded Renderer passes all eight
actual GPU scenarios: 413 terminal tasks, 412 created tasks, 414 tracer/task
absences, stable traces, unchanged ROM and zero known physical-device attempts
or permission errors. All 20 prior Renderer positive files remain exact.
The failure was an invalid observer demanding 3D Source lighting during genuine
empty/presentation-only work; queue/counter observers now verify their own
actual completed GPU memory, shared-region classification and FIFO invariants.
Ordinary 3D Source-light checks and all expected images remain unchanged.

The earlier 519-file Root materializer binds 32 support inputs across 59
manifests. Its 149 strict support paths and both new generators have actual Git
clean-filter proof: Windows `core.autocrlf=true` restores exact admitted bytes
from CRLF inputs without changing the index or objects. The new integration
adds two providers, migrates the canonical application latch/idle storage,
and extends Root to 33 support inputs; its refreshed platform proof is separate.
This is not a new hosted-MSVC or private Windows11 runtime result.

This checkpoint adds complete Root and Beach-music ownership, canonical
Detector controls, message/fade task composition and the Source queue/counter
cutover. It corrects inert-attribute rejection, intentional zoom-overscan
rejection, secondary pass-scissor loss, equal-priority camera order and
receiver-before-sender collection. Permanent raster/lifetime consumers retain
the real failure boundaries; no temporary viewport/RAM diagnostic remains.

Full original Beach intro/outro and end routing, complete reachable
species/course execution, independent Original equivalence, Windows11 gameplay
and hardware/hearing qualification remain open. The application-ingress
qualification below extends Root integration, not full-game completion.
Public CI receives no ROM or extracted assets; the latest hosted receipt remains
the separately identified immutable `23f4c05` result.

## Original application through native Beach ingress

The real `35mmisle` target composes original C manager decisions with actual
Boot/Title, Name, Oak and Beach scene owners. Application scheduling owns real
Input publication, GTL update/draw claims, completed GPU/Photo access and ordered
native retirement; it does not replace gameplay with a native FSM.

Four new strict original-C providers own 32 Snorlax/Kangaskhan, 51 Meowth/common
block-change, 38 Eevee/Chansey and 22 Scyther/special definitions. Exact compiled
native pointers, original decoded tuples, complete graphs/weighted lists and
real resource capabilities admit all 17 factory families and actual alternate/
singleton factories. Disabled actor `1005` creates then pauses its real animation
process through the original NULL-header path. No header is fabricated.
Cold immutable FX-bank preflight precedes original active-slot reset and later
source-ordered bank installation; no emitter or particle is published early.

The source snapshot of 567 declared files has aggregate SHA256
`189ff9d5c73fab22d0bf2cb8862c24f2d540363137510b47ab1bbd5880bd2c9e`.
Its actual application consumer reaches five original-manager-selected entries,
including 16 genuine Beach updates, eight Beach canonical frames and 15 observed
native-camera changes. All 817 admitted canonical frames reach actual copied
LUS presentation and receive their real terminals; all six 640×480 window
surfaces were visually inspected. Audio accepts 1,073,456 SDL-dummy PCM frames.

The consumer withholds a real owned-controller completion while GTL owns an
update: eight real retraces and 4,224 accepted PCM frames progress before the
claim resumes. Holding a completed Beach GPU/Photo frame similarly permits
eight retraces and 4,320 accepted PCM frames. A stop requested during that hold
cannot complete retirement until the caller releases the real frame. Immediate
stop before the first pump also terminates without replaying a one-shot clock
action. Native/GPU/Photo/manager owners retire before Input cancellation and
actual Audio stop/drain; Host ownership closes afterward.

The sole accepted whole-tree closure records 216 terminal tasks, 215 created
tasks, 217 tracer/task absences, stable traces, unchanged ROM and zero known
physical-device attempts or permission errors. One bounded initial observation
reads nine predeclared artifacts; its retained report and six window images are
outside the capture. Both earlier failures remain rejected/frozen with no
artifact observations. No capture tree is reopened after initial observation.

The actual run exposed a premature FX semantic-sender registration check:
the real publisher returns a native scheduling object before Renderer registers
that camera. Adoption now validates its exact factory identity and canonical
receiver, performs actual registration, then obtains the real field credit
before the original C store. Native service and Renderer failures retain their
first precise context instead of a generic rejection.

The preceding 566-file product snapshot builds the real executable and passes
57 CPU tests; the new consumer and all 25 prior opt-in consumers subsequently
compile. This is not a refreshed three-profile or hosted-MSVC result. A fresh
installed executable initially failed to load non-system `libzip.so.5`; actual
dependency installation plus executable-relative runtime paths now permits
its CLI under an empty environment. Notice delivery, installed renderer runtime
and release composition remain separately tracked; no binary is published.

This result proves original application ingress, asynchronous ownership and
actual Beach rendering. It does not prove a complete course, every species
state, post-course UI/scoring, all seven courses, independent Original pixel/
numerical equivalence, hardware/hearing behavior or Windows11 gameplay.

## Shared Host progress and post-course native interfaces

The 572-file Debug snapshot has aggregate SHA256
`207a38257864c50a1a1f523251b553aad6d50ed9d5a66540346dd19d6623775a`.
The aggregate hashes sorted compact JSON of source path → SHA256. All default
targets, the actual application consumer, all 25 preceding opt-in consumers
and the new post-course interface consumer compile; 57 CPU tests pass.
This receipt does not refresh Release, sanitizers or hosted MSVC.

Production main and the application consumer share one native Host-progress
owner: actual monotonic rational VI, Audio/event servicing, exact outstanding
Input acquisition/completion and genuine GTL retrace offers. It never pumps
Runtime/OM, publishes an extra Input snapshot or releases a display frame.
The actual original Boot → Title → Name → Oak → Beach-ingress run retains
817 submissions, presentations and terminals, with 1,139,856 accepted
SDL-dummy PCM frames. Withheld Input permits eight actual retraces and 4,224
PCM frames; a retained completed GPU/Photo frame permits eight retraces and
4,368 PCM frames before genuine release. Immediate pre-pump stop and ordered
native/Photo/Manager/Input/Audio/Host retirement pass. All six actual 640×480
copied LUS windows are inspected from their sole admitted initial observation.

The application capture closes 216 terminal/215 created tasks and establishes
217 tracer/task absences, unchanged ROM, stable traces and zero known physical
attempts or permission errors. Nine predeclared artifacts are observed once,
retained outside the capture, and never reopened or rescanned.

The independent native-interface consumer uses admitted ROM resources,
authored canonical mutation records and genuine OM updates, not gameplay
phase injection. It exercises stable saved-photo/comment projections through
direct session writes/removal/reset, exact reserved-byte preservation, COW
snapshots, independent photo/score/comment operations, flag-only mutation,
stale/foreign identities, report/sign mapping, four temporary slots and clamped
pending grants sharing the consume-once course notice. No operation implicitly
commits a durable save.

Returning Text/Frame/Layout leaves expose explicit pending events and original
wait counts; the caller acknowledges only after servicing them. The consumer
checks actual native button visibility after cancellation during incremental
creation, both with a different replacement and a same-table retry. Built
prefixes retain ownership; an incomplete prefix is not a completed table.
This capture closes three terminal/two created tasks and four tracer/task
absences, with unchanged ROM and zero known physical/permission attempts.
Only its two predeclared output artifacts are observed once. No GPU, replay,
captured-photo, audio-output or complete post-course-scene claim follows.

Independent planner/reviewer assessments select a C-only same-update owner
rendezvous for future genuine replay barriers, with real Host progress and
drain-before-C-exit cancellation. That integration remains open. Bounded GPU
failure draining and graceful Source update abandonment have separate review/
acceptance obligations; neither proves a score, thumbnail, completed course,
full game, Original equivalence, hardware/hearing behavior or Windows11 play.

### C-only abandoned update

The subsequent 572-file snapshot
`d51ea03225fd45dd0c1f90b978c0a3d755ba61ad4877816e583d4cb8c2f3ef70`
rebuilds all default targets and 27 opt-in consumers, passes 57 CPU tests, and
runs the native engine consumer directly. A typed stackful C process can end
its invocation with a distinct stopped outcome, then the returning owner-side
C dispatcher abandons the rest of that OM traversal. No C++ frame is discarded,
execution fault created, scene result supplied or GTL update credited.

Idle, synchronous, untyped native and nested Player nonwaiting calls reject.
Independent review exposed the nested-fence admission hole; the new actual
regression failed before the guard and passes afterward. It proves no suffix
or lower-priority process runs, later ticks remain stopped, the original Game
fault remains clear, and idle object deletion/destruction/recreation succeeds.
Runtime's Window path abandons its claimed update after this outcome. Genuine
replay cancellation and normal quit during a retained GPU barrier remain
separate end-to-end obligations.

### Pollable genuine GPU failure

The 577-file Debug snapshot
`4f61315bd64302555308304d60cae828257caf73920f49e04be785c8275526dc`
builds all default targets and 29 opt-in consumers, passes all 57 CPU tests,
and directly exercises native hierarchy-to-sprite packet composition.
Independent review found a real hierarchy boundary defect: Window's `0x4000`
RGBA-inheritance exclusion leaked into the raster producer's unsupported bits.
The actual packet regression fails before the correction and passes after it;
prepared copies consume the bit, while native locals retain it.

`canonical-gpu-failure-smoke` executes a genuine nonempty RGBA4 LoadTile
rejected by the pinned CPU-Vulkan backend. Asynchronous execution observes two
pending polls before the original backend exception, with a maximum measured
poll of 73,077 ns. The failed token is consumed, exposes no memory lease, and
leaves a quiescent poisoned executor which rejects new preparation. Separate
blocking retire, execute and outstanding-submission destructor paths drain
without external polling. A fresh executor renders the exact same 64 red
RGBA5551 pixels in cold and warm tasks; neighboring RAM guards remain intact.
Diagnostic text is recorded, not pinned as an incidental wording test.

The sole initial trace observation closes 192 terminal tasks / 191 created
tasks / 193 absent tracer-task identities, preserves the ROM, and finds zero
known physical-device attempts or permission errors. Only its two predeclared
stdout/stderr artifacts are read. This is genuine backend-failure ownership
evidence, not device-loss, prolonged-drain, hardware or Windows qualification.

The same snapshot passes actual application ingress with 817 canonical frames,
817 LUS presentations and 1,158,320 accepted SDL-dummy PCM frames. All 216 tasks
terminate; six actual windows are inspected. The complete Beach attempt still
returns original Oak13 with no photographs and is rejected without artifact
reads. Independent planner and reviewer identify premature completed-Photo
retirement on image release before original CopyInfo can publish it. The
subsequent lifetime correction retains CPU Photo/history through Source updates
until next draw preparation or teardown; its course acceptance remains pending.

### All-course resources and post-course prototypes

The 583-file Debug snapshot
`5daf05bff5c1fa5252c01e82ad5fb1ab8b3ec3fb69901a2557470b4d378035f1`
builds all default targets and 29 opt-in consumers and passes 57 CPU tests.
Obsolete tests pinning unsupported jumps/camera transfers and a single
contiguous terminal grammar are deleted, not re-pinned. Actual native
`world::Resources(snapshot, course)` construction and complete bounded graphics
closure succeed for all seven original courses. Primary/secondary groups are
Beach8+4, Tunnel6, Volcano6+1, River6+4, Cave5+1, Valley7+2 and Rainbow1.
Ground/ceiling trees and exact callback identities are decoded; invalid course7
rejects. This is immutable resource admission, not all-course World execution,
Rainbow two-cycle rendering or live gameplay.

Actual original-ROM CameraCheck and PhotoCheck banks also pass all reached
prototype previews: 20 and8 roots, two genuine cold/warm GPU tasks per bank,
byte-identical cold/warm images and protected-RAM guards. The unused
`801E5840` declaration is not a Sprite in the ROM and has no reached Source
reference; removing it corrects inventory, without weakening hydration or
omitting a used root. The 77-task trace closes with unchanged ROM, zero known
physical attempts and zero permission errors. Its four predeclared artifacts
are read once; both actual 320×240 prototype surfaces are inspected. The
temporary preview target/source are then removed.

The actual native post-course interface consumer passes again after migrating
its hidden-Window observation to real native root attachments, rather than the
render projection which correctly prunes hidden hierarchies. Its three-task
trace closes cleanly. Application ingress also passes: 817 actual frames/
presentations, 1,142,608 accepted SDL-dummy PCM frames and 216 closed tasks.
All six actual window images remain byte-identical to the inspected 577-file
surfaces. Full-course/photo-replay acceptance remains separate.

At that checkpoint, the shared material-domain certifier had two independently
reproduced defects despite the passing suite: a large rounded seek consumed
image10 beyond a certified ten-entry prefix, and an inactive positive-wait cycle
failed to return within three seconds. Their subsequent correction and actual
regression evidence are recorded below, not inferred from this earlier cohort.

### Rounded domains, actual sky and three-photo Source evidence

The 588-file Debug source
`dfe8832ff353cd55794fbccb52955245537236f95d4856b2c1a0ad1faabe04d7`
builds all default targets and 28 opt-in consumers and passes all 57 CPU tests.
Private native extraction now admits 43 checked definitions, including genuine
camera12 no-roll reflection; RT27 retains its original scale-free transform.
Real sky models join each course's authoritative catalog instead of a
materialless stand-in. Actual private-ROM resource construction succeeds for
all seven courses again; Tunnel/Cave have no sky and are the only ceilings.
Beach's 79 animation headers, 33 materials and 1,317 decoded streams also admit.

Material extent authority is explicit: declared bounded pointer allocations
remain independent of playback inference; derived extents require exact
binary32 first-update sampling capabilities or the timing-independent STEP
proof. Every rendered pose still checks actual image/next-image/palette indices
before writes. Independent review caught a valid live Lapras LINEAR stream
being incorrectly routed through STEP-only inference; matching its already
authoritative allocation fixes admission without widening the solver.

The actual rounded-seek reproducer now certifies eleven entries while the real
Runtime consumes image10 at skip33,554,432/speed0.5. The inactive positive-wait
cycle returns with a one-entry prefix in approximately 1.3ms rather than
exceeding its three-second deadline. Both direct consumers pass; their
throwaway sources are removed. No expected value is re-pinned.

Actual application ingress passes 817 canonical submissions/terminals and LUS
presentations with 1,127,264 accepted SDL-dummy PCM frames, real backpressure
and held-frame stop. Its sole trace closure has 216 terminals/215 created tasks,
all 217 tracer/task PIDs absent, unchanged ROM and zero physical/permission
hits. No capture artifacts are read; this is not new visual parity evidence.

The full-course run reaches actual CameraCheck9 with three photographs,
three qualifying photographs and three shutter presses; original outro,
ending sound/cart wait/cart animation terminal and one verified retirement
callback occur. It still fails the strict gate: follow-up/stick and two course
window captures are absent. The rejected trace closes once with 471 terminals,
470 created tasks, 472 absent PIDs, unchanged ROM and zero physical/permission
hits. No artifacts are read from the permanently closed rejected tree.

Competing planner/reviewer assessments identify an observer/input-policy
mismatch, not a native tutorial coroutine failure: original instant messages
copy complete bytes into `Msg_PrintedText` while leaving `Msg_IsPrinted=false`.
The consumer now recognizes the exact active tutorial identities plus their
complete displayed bytes, preserving original flags/timers and the full gate.
Its corrected course run, retained replay World/GPU operations, C Window
thumbnail services and complete CameraCheck/PhotoCheck owners remain separately
subject to actual acceptance. Full-game/hardware/Windows11 acceptance is false.

### Actual seven-course replay preview and precise target ownership

The 599-file Debug source
`9f1227a75feb6fd0c67eb202a2882a44850cd5696cc8162b089c9d645b964aa6`
builds all default targets and 30 opt-in consumers; all 57 CPU tests pass.
The real `photo-replay-rsp-smoke` uses the shared canonical executor and actual
GAME RSP microcode, not a screenshot or mock renderer. All seven reconstructed
World previews complete with their 320×210/134,400-byte canonical RGBA5551
backing. Nine bounded 104×78 surfaces are read once and visually inspected:
seven World views and paired authored-Pidgey/terrain-only views. The pair differs
in 346 canonical bytes. These authored records are not captured gameplay.

The consumer rejects a stale GPU admission without releasing current ownership,
submits another actual preview, cancels it, drains real GPU/readback ownership
and joins all native objects/processes/models/cameras. Normal Frame ownership
cannot overlap Replay admission; no normal-display/GTL submission is invented.
The single trace closure records 39 terminals/38 created tasks, all 40 tracer
and task PIDs absent, unchanged ROM and zero physical/permission hits. Only the
eleven prospectively bounded artifacts are read; the tree is permanently closed.
Private executable SHA-256:
`b4ab754d26e9c8eb613457a5034c994c57c8f1123763a591298c3a9c2a006e93`.

Actual execution exposed three preflight integration errors before that pass:
the harness duplicated immutable main/BSS in its external ownership plan;
shared reset validation assumed every target was 320×240 instead of the actual
320×210 preview or 64×48 scoring allocation; Replay supplied its pre-render
guest identity where the reset contract requires a physical address. The
consumer excludes only the exact already-owned main/BSS range, every reset
caller now declares its actual owned 16-bit extents, and Replay translates the
validated KSEG identity. Numeric reset commands, physical/alignment/immutable
asset guards and complete owner-range rejection remain unchanged. Each rejected
capture closes once and permits zero artifact reads.

The 595-file canonical Photos/GameSession interface consumer also passes its
real per-record lease/mutation-exclusion checks and native Window/OM operations.
Its trace has three terminals/two created tasks, four absent PIDs and zero
physical/permission hits. The 595-file full course reaches CameraCheck9 with
three qualifying photographs and all five tutorial/message/stick predicates,
but the strict gate still rejects initial tutorial-window capture absence.
That rejected run has 471 terminals/470 created tasks, 472 absent PIDs and zero
physical/permission hits; no artifacts are read. The consumer now holds authored
Z/A until actual first tutorial presentation, without changing Source state,
timers or the full gate. Its new complete-course acceptance remains pending.

The new actual C Window GPU consumer reaches execution but rejects its RGB5
sample oracle; its 45-terminal trace closes with zero physical/permission hits
and zero artifact reads. Mutable thumbnail pixel qualification therefore remains
open. General two-stage scoring, Source sound/wait obligations, complete
CameraCheck/PhotoCheck owners, full-game/hardware and Windows11 acceptance also
remain open; the successful preview does not promote any of those claims.

### Complete original Beach and genuine post-course boundary

The same immutable 599-file source completes the strict full-course consumer.
The program's expected exit2 is accepted only after every original Beach
predicate and the genuine unavailable CameraCheck9 boundary pass; it is not
normal application completion. Execution takes 414.636s on the qualified
software composition. Actual Source produces four photographs, three qualifying
shots, four shutter presses, 8,106 movement changes and two stick updates.
Intro, tutorial, real input release, follow-up message/clear, actual stick input,
ending sound, outro cart wait/animation terminal and verified retirement all
occur. All three prescribed tutorial/travel/outro windows are captured and
visually inspected; the first visibly contains the original “Press Z to aim”
message. Only authored controller timing changes: Z/A follows actual first
presentation. Original state, timers and the strict acceptance gate are intact.

There are 5,233 actual canonical submissions/terminals/LUS presentations and
12,684,944 accepted SDL-dummy PCM frames across Boot/Title/Name/Oak/Beach.
The sole trace closure records 474 terminals/473 created tasks, all 475
tracer/task PIDs absent, unchanged ROM and zero physical/permission hits.
Exactly ten prospectively bounded artifacts are read once; the tree is closed
permanently. Private executable SHA-256:
`60de0a3b8b5d4a4c2b0a060980fb3e31150349baacc5ced773268fa04e600caa`.

After actual Player/Music/GPU/Photo/Source-domain/ending retirement, the original
manager publishes serial6, typed operation10, argument9. The retained Runtime
error remains3 with its exact unavailable-native-owner message. No artificial
level-end callback, result injection, spawn filtering, swallowed request or
synthetic time completes the course. CameraCheck/PhotoCheck execution, the
remaining playable courses, full-game and Windows11 qualification remain open.

### Sticky Frame failure drains actual GPU ownership

A rejected cold reservation can latch `Frame` failure after real GPU submission.
`poll()` now drains only its own normal submission, releases the actual completed
mapping and preserves the first error. It does not call completed work, copy an
image, advance the presentation ordinal or publish success. A failure latched
inside completed work follows the same rule. Replay retains sole ownership of
its separate Source admission/token and must release its exact binding itself.

The original Frame translation unit was reconstructed byte-for-byte against its
recorded SHA-256 and linked with the unchanged real consumer objects. It emitted
the actual GPU/mapping ownership failure and stranded cleanup; its verified
owned process group was terminated after 94.788s. The rejected tree has 200
terminal traces/199 created tasks/201 absent tracer-task PIDs, zero physical or
permission hits, and zero artifact reads. No successful before-run teardown is
claimed.

The corrected 617-source build passes default targets, 31 explicit consumers
and 57 CPU gates. Its actual photo consumer completes 68 GPU frames, four
genuine failed frames and 83 original C CopyInfo updates in 19.977s. Cases cover
throwing callbacks, late cold reservation and nonthrowing callback-latched
failure, exact failed-ticket retirement, retained Actor ownership and no failed
image publication. All 364 traces terminate, 363 created tasks are traced and
365 tracer/task PIDs are absent; ROM unchanged, zero physical/permission hits.
Exactly three prospectively bounded artifacts are read once. This proves the
shared Frame lifecycle, not post-course execution, all courses, Original
parity, physical devices or Windows11.

### Returning Window C services: actual GPU qualification

The 617-source C-owned Window consumer passes callback replacement/cancellation,
synchronous and nested no-wait fences, live Layout selection, native SObj marker
state and independent thumbnail topology. Camera exercises two 52×39 children
and the 140×105 large prototype with eight bitmap pieces; Photo exercises two
independent 52×39 children. The actual RSP/RDP executes six Camera and five Photo
frames, including cold/warm draws, every child revision and held-result guards.

Every framebuffer pixel is checked against independent normal Source GUI
combiner/blender arithmetic, not raw-texel equality. All rows, piece boundaries,
siblings and untouched guards are covered. Actual graphics borrowing rejects
mutation and unregister; after real release, immutable descriptors are restored
on the same native objects without discarding their marker state. Borrowed
raster mutation is a logical state error, distinct from invalid parameters;
the C veneer preserves that distinction without another borrow flag.

Execution takes 4.963s. All 83 trace files terminate, 82 created tasks are traced
and 84 tracer/task PIDs are absent; ROM unchanged, zero physical/permission hits.
Exactly 25 prospectively bounded artifacts are read once; four representative
canonical readbacks are visually inspected. These are authored mathematical
pixel stimuli, not captured photographs, replay scoring, complete CameraCheck/
PhotoCheck scenes or LUS window captures. Original/hardware/Windows parity is
not claimed. The separate C-owned Replay consumer remains under qualification.

### C-owned Replay: actual scoring, preview and stop

The 618-source build shares one mandatory `world::ModuleState` between live
World and Replay. Actual WORLD loads restore initialized data/zeroed BSS;
logical create/destroy resets road events without resetting the module clock.
Sky speed reaches the actual texture process; terrain and sky yaw read the same
clock. Constructor consumers are migrated without aliases or private fallback
records. Runtime now invokes separate World/Window generation observers before
binding their consumers. Default targets, 31 explicit consumers and 57 CPU
gates pass, including scheduled clock/sky/road retirement regressions.

The actual C-owned Replay consumer scores an authored Pidgey record as species16,
total2500, with original sound85 and exact Source waits60/1. Terrain scores zero
through both GPU stages. Its real preview callback consumes the 134,400-byte
BE5551 backing and updates an actual Window thumbnail before acknowledging.
A submitted Host stop uses the nonreturning C stop fence, not scene completion.
All six actual GPU admissions/releases balance; the real initial display is
copied, presented and released by creator-thread native progress. The consumer
executes 67 direct OM updates and accepts 212,192 SDL-dummy PCM frames. It does
not claim GTL cadence or captured-gameplay photos.

Execution takes 7.970s: 78 terminal traces, 77 created tasks, 79 absent tracer/
task PIDs, unchanged ROM, zero physical/permission hits. Five prospectively
bounded artifacts are read once; the actual C preview is visually inspected.
Separately, all-seven actual GPU previews pass after this World cutover in
7.092s, including the 346-byte subject-versus-terrain difference and genuine
submitted cancellation. That closure is 39/38/40 terminal/created/absent with
zero hits; eleven artifacts are read once and nine surfaces inspected. Neither
consumer proves seven playable courses, Original parity, hardware or Windows11.


### Closed course catalogs and genuine Controller curve — 634c

Source snapshot `4dee8937bc78358d2211d7390ff63bee346b49257e4415dec0c4384d97a073ed`
contains 634 files. Default targets and 31 opt-in consumers build; all 57 CPU
tests pass in 3.54 s. Exact private Rainbow providers compile all 77 actor and
21 lifecycle definitions. Their native service bindings and executable Rainbow
scene remain unqualified.

One concrete `actor::Resources` and `actor::BehaviorResources` now select closed
Beach0/Rainbow6 catalogs. Obsolete Beach-only owners are removed, without aliases.
Beach scalars/chooser are explicit `ActorSceneResources`; one `ActorSceneState`
owns counters/path and borrows the actual lifecycle uint16 Magikarp latch.

| Immutable catalog | Beach0 | Rainbow6 |
|---|---:|---:|
| Models / materials | 17 / 33 | 5 / 14 |
| Model + texture programs | 1,201 + 116 | 311 + 63 |
| Animation headers | 79 | 26 |
| Initialized definitions | 22, including retained zero row | 7 genuine records |
| Automatic definition origins | None added | 3 |
| Graphs / weighted lists | 71 / 8 | 20 / 1 |
| Actor animation paths | 0 | 1 |
| Cart programs / paths | 92 / 6 | 100 / 6 |
| Environmental descriptors | 53 | 9 |

The actual private CPU client checks exact original setup/header/singleton and
automatic-origin identities, Mew weights, both Rainbow cart clip tuples, sound
order, and complete word-swapped Actor/Cart installation. It evaluates original
Controller header `8034B590`, node1, through the shared animator: time55 yields
`(0, -255.0000457763672, 0)`, Y bits `C37F0003`, matching an independent pinned
Source binary32 derivation from CATROM descriptor `800F5664` and its admitted
control/timestamp/polynomial data. No mapping extension or skipped path opcode.

The previous 634b data run failed on missing retained path storage. Its root is
rejected permanently, with zero artifacts read. Corrected client exits0 in
0.5593 s: one terminal trace, no created children, tracer/task absent, ROM
unchanged, zero physical-device attempts or permission errors. Two prospectively
bounded logs are retained once after the sole initial qualification. Its frozen
binary is `a9ab6c021d03859322fe454b99283c3053b08bbe4f340e4a31ca3ae844177f38`.
The source archive additionally contains the private authored client.

The actual original-C interaction/FIFO/movement/fault consumer also exits0,
in 1.1235 s. Addressed command28 changes the real destination's Source AUX Y
without changing its requester; genuine queued sender credits balance. Original
FIFO−1 cleanup clears the canonical Player focus pointer/latch and releases its
independent credit before collection. The earlier failure was the fixture's
unregistered Player focus field and raw pointer store; production registration
and credit guards remain unchanged. All 276 traced tasks terminate, 275 created
children are accounted for, tracer/tasks are absent, ROM unchanged, zero physical
or permission hits. Two bounded logs are retained once. Binary:
`cdbd34008c999f91af9ec278a3d66282201e7173dff625e4ab41ebd83d32e997`.

These are real immutable-data, curve, native-C FIFO/reference and owned audio-RSP
PCM proofs. They establish neither Rainbow ActorAI/lifecycle execution, Rainbow
cart GPU output, completed CameraCheck9/PhotoCheck15 scenes, Original/hardware
parity, audible output nor Windows11 gameplay.

The same 634c snapshot passes fresh canonical all-seven-world GPU preview and
C-owned Replay runs. All nine preview surfaces and the separate C-owned thumbnail
were visually inspected after qualification; the authored subject changes 346
bytes relative to identical terrain input. Preview exits0 in 6.0261 s, with
39 terminal traces, 38 created tasks and 40 absent tracer/tasks; eleven bounded
artifacts are retained once.

C-owned Replay exits0 in 7.0093 s: actual Pidgey16 score2,500, original waits60/1,
134,400-byte preview delivery, terrain score0 and submitted Host STOPPED—not
scene completion. Six real GPU admissions/releases balance; 67 direct OM ticks,
one canonical frame copy/Host presentation and 183,664 SDL-dummy PCM frames.
All 78 traced tasks terminate, 77 created tasks are accounted for and 79
tracer/tasks are absent; five bounded artifacts are retained once. Both runs:
ROM unchanged, stable terminal trees, zero physical-device attempts or permission
errors. Direct OM/service proofs do not establish GTL cadence or complete
CameraCheck9/PhotoCheck15 scenes; their full original application run remains
separate.

### Original Gate collision geometry and Item regression — 634d

The next 634-file snapshot builds default targets and 31 opt-in consumers and
passes all57 CPU tests in 3.52 s. The contextual Item importer verifies its new
closed transformed-C digest against unchanged pinned original Source hashes.

A separate private client decodes the actual Rainbow singleton8034AF20 and
eight-node tree800EDF78, with complete ROM provenance. Its prepared CPU solver
hits the front post at (223,200,−50), reflected velocity(0,0,−140). The open center
misses while preserving the original negative-result output mutation:
position(11,22,33), velocity(0,0,2). Broadphase misses preserve outputs; current
translated and rotated instance poses affect queries; invalid model indices and
moved-from Snapshot provenance reject without output mutation. Run exits0 in
0.4179 s: one terminal trace, zero created tasks, two absent tracer/tasks and two
bounded artifacts retained once. This proves the real collider, not Item
callback installation, Actor AI, Source lifecycle, GPU output or gameplay.

The existing actual original-C Item consumer separately passes all seven
scenarios: complete physics/reference behavior, physical/fume/threshold
exhaustion, and faults with empty/live/collected ownership. Native and actual
audio-RSP retirement complete. Run exits0 in 2.2240 s: 39 terminal traces,
38 created tasks, 40 absent tracer/tasks and two bounded artifacts retained once.
Both trees are stable, ROM unchanged, with zero physical-device attempts or
permission errors. Context-bearing callback execution and cross-Game rejection
still require their own physical-behavior proof; this regression does not claim
them.

### Context-bearing Item collision and Window regression — 634f/634g

The corrected post-course integration builds all default targets and 31 opt-in
consumers; all57 CPU tests pass in 3.67 s. The original Window consumer then
passes eleven canonical GPU frames and its whole-frame independent GUI oracle,
all thumbnail revisions, real borrower rejection and strict sticky-failure
cleanup. Four completed Camera/Photo surfaces were visually inspected. Its
5.0401 s run qualifies83 terminal traces, 82 created tasks and84 absent
tracer/tasks; 25 bounded artifacts are retained once. This proves execution
regression coverage, not completed CameraCheck/PhotoCheck scenes.

The separate contextual Item client executes genuine original-C movement with
the actual prepared Gate solver. It proves real post reflection, open-center
miss followed by normal gravity/flight, current-instance pose changes, paired
binding preservation after half-null and reentrant replacement attempts, and
wrong-Game NULL rejection. Owner reconstruction and successive actual Games
retain no prior callback. Moving only the Gate Snapshot causes a real returning
provenance error: C faults the Game before hit flags or the normal-flight tail;
terminal clear does not heal that failure. Source credits, native processes,
Runtime instances, actual RSP/accepted PCM, audio clients and output drain join.
The authored throw target respects original `Items_InitItem`'s +100 Y offset;
no Item pose or velocity is fabricated to satisfy the assertions.

This 1.4741 s run qualifies16 terminal traces,15 created tasks and17 absent
tracer/tasks; two bounded artifacts are retained once. Both accepted trees:
stable, ROM unchanged, zero physical-device attempts or permission errors.
The original OM supports one active Game; successive scene/Owner incarnations,
not concurrent Game isolation or a second live foreign-Game token, are proved.
Neither run establishes Player FSM, full Source lifecycle, fullgame,
Original/hardware/hearing parity or Windows11 gameplay.

### Typed course admission and faithful single-image UI — 635a

The 635-file snapshot builds all default targets and 31 opt-in consumers; all
57 CPU tests pass in 3.85 s. Its sorted source-map SHA256 is
`ba0e460c8132a970eeaab6eb75891429c5589534d9bdc8ae74f69255f01a27a0`.
The original Rainbow Actor objects now share the native Actor archive. All
existing callers use the closed typed course context. One compiled factory
authority validates exact tuples and partially written helper provenance;
Meowth's actual required behavior is borrowed, never fabricated. Original Source
hashes remain unchanged. Complete immutable admission and capacity proofs run
once cold; live Source entry retains identity, phase, health and Snapshot guards.
Actual Rainbow activation and gameplay still require the real lifecycle/services.

Independent planner/reviewer inspection found that the former unconditional
two-plane Element allocation could not hold PhotoCheck's three original flags-0
Elements in its unchanged `0x32000` heap. Elements now allocate one image unless
Source requests `use_alt_image`; Raster and Producer represent absent alternates
explicitly, without writable aliases or fake compiled resources. The exact
simultaneous `204×48`, `204×120`, `192×47` regression exercises drawing, upload,
27-tile preparation, borrowing rejection, storage retention and final reclamation.
Real dual-image behavior and no-alt primary-descriptor restoration remain.
Declared absent ground/ceiling maps likewise retain NULL in live/replay views,
without a synthetic plane or suppression of malformed-present-map errors.

| Actual fresh run | Seconds | Terminal / created / absent tracer+tasks | Artifacts retained once |
|---|---:|---:|---:|
| Original native Actor owner | 3.3012 | 32 / 31 / 33 | 2 |
| Original C automatic/FIFO interaction | 1.7494 | 276 / 275 / 277 | 2 |
| Window canonical GPU regression | 9.1703 | 83 / 82 / 84 | 25 |
| All-seven-world GPU preview | 9.5416 | 39 / 38 / 40 | 11 |
| C-owned Replay/score/stop | 11.0783 | 78 / 77 / 79 | 5 |

Every accepted tree is terminal and stable, ROM unchanged, with zero
physical-device attempts or permission errors. Four Window surfaces, nine world/
subject/terrain surfaces and the separate Source-produced thumbnail were
visually inspected from the once-retained bytes. Window's independent whole-frame
oracle passes all eleven frames and strict borrower-failure cleanup. Authored
subject reconstruction still changes 346 canonical bytes. C-owned Replay retains
actual Pidgey16 score2,500, original waits60/1, six GPU admissions/releases and
submitted STOPPED—not scene completion; 67 direct OM ticks and297,936 accepted
SDL-dummy PCM frames do not establish GTL or audible output.

The prior full application run observed a genuine Pidgey roll mark, live Oak
slot22 and Manager PhotoCheck admission, then a native failure. It was rejected
with zero artifact reads; neither its generic message nor the UI heap defect
identifies the executed failpoint. A bounded native-only first-failure record now
retains checked caller/expression/line/result before C unwind, without success
telemetry or treating STOPPED as an error.

### Completed original post-course behavior — 635a

The separately qualified full application exits successfully after Boot → Title →
Name → Oak → full Beach → CameraCheck9 → PhotoCheck15 → constructed Oak13
admission. The final Oak owner is admitted, not rendered or interacted with.
The run takes 500.5617 seconds; all 591 terminal trace records, 590 created tasks
and 592 absent tracer/task identities qualify once, with stable files, unchanged
ROM and zero physical-device attempts or permission errors. Its 15 prospectively
bounded artifacts total 7,517,721 bytes and are retained once; the capture tree
is closed permanently.

All 5,818 canonical GPU submissions terminate and release; completed frames are
presented through LUS. SDL dummy accepts 14,866,208 PCM frames. The full Beach
contract retains four photographs, three qualified shots and four real shutter
edges. Camera marks roll 0, species 16, revision 2; Photo scores 3,300 and writes
Report slot 8 with the same canonical photograph bytes and identity. Source skip
remains zero. Actual ordered assessment, replacement `216×72` summary Element,
retired-Element unavailability without fault, original results and all real
scene/native/Input/GPU/audio/Host owner joins pass.

Seven actual Host surfaces were inspected. Beach tutorial/travel/outro and marked
Camera selection are meaningful; the assessment is explicitly early phase 0.
The intro capture is mostly black, and the summary capture precedes completed
guidance/final totals and contains overlapping text/thumbnail strips. Independent
planner/reviewer assessment identifies premature capture permissions and an
original/native zero-step bitmap-iteration mismatch. Neither timing alone nor
the stored Report score proves final screen correctness. Fresh phase-correct
visual qualification remains required; old surfaces are not relabeled.

Live Rainbow, all seven playable courses, Original/hardware parity, audible
output and Windows 11 gameplay remain unqualified.

### Returning native Source fault ownership

A real native Actor precondition invoked through an actual canonical Player
root reproduced the missing C return boundary: the Source call failed to return
its execution error or reject its suffix. Engine fault transfer now honors the
already-active Player/post-course nonwaiting C frame after disarming real
command-fault cleanup; the first Game diagnostic and sticky failure remain.
The actual `player-source-tests` executable fails before and passes after the
correction. Its regression observes rejected Source code remaining unexecuted,
subsequent admission rejected and the owning C++ destructor running normally.
Equivalent post-course and live Rainbow failure-path qualification is separate.

### Completed post-course screens and bounded sprite arenas — 638d

The 637-file full-application snapshot builds all default targets and 31 opt-in
consumers and passes 57 CPU tests. The actual original route completes Boot →
Title → Name → Oak → full Beach → CameraCheck9 → PhotoCheck15 → constructed
Oak13 admission. The fresh capture exits 0 in 437.970614046 seconds. All 589
terminal tasks, 588 created tasks and 590 absent tracer/task identities qualify
once: stable files, unchanged ROM, zero physical-device attempts and zero
permission errors. Its 15 prospective artifacts total 7,541,427 bytes, retained
once; the tree is permanently closed.

There are 5,972 actual canonical GPU submissions, terminals, caller releases and
LUS presentations; SDL dummy accepts 13,317,072 PCM frames. Beach retains its
four real photographs, three qualifying shots and original tutorial/travel/
ending contract. Camera marks roll 0, species 16, revision 2; Photo evaluates
3,300 points and stores Report slot 8 with identical canonical photograph bytes
(digest `1506459116649097628`). Original results are 9 → 15 → 13, with one real
retirement callback per scene. Native/GPU/Photo/Source/Manager/Input/audio/Host
owners join; the constructed return-Oak13 owner is not rendered.

Capture permission now observes the actual returning Source acknowledgment
helper, not a guessed update count or a surrogate phase. Camera's final intro
wait is pinned line 1039: readiness at Source update 162 excludes frame 81 and
captures completed Host presentation 82 before A. Photo consumes/releases the
actual save acknowledgment at line 2197, then waits at final guidance line
2302: readiness at update 533 excludes frame 533 and captures presentation 534.
No Source FSM, clock, skip flag or photo record is forced. Four post-course Host
surfaces were inspected: completed Camera prompt, marked selection, explicitly
early phase-0 assessment, and completed final summary. The prompt is readable;
the summary has clean layout, one Report species and 3,300 points, without the
earlier overlapping text/thumbnail strips. Historical premature captures remain
historical, not relabeled as completed screens.

Native Sprite emission preserves Source zero-step repetition, pre-draw wrap,
full-row admission, actual bitmap height, row stride and post-scale expansion.
Per-Sprite identical texture/band/tile setup is reused; no cache crosses Sprite
or pass boundaries. Equal per-camera slices caused a separate first-Photo-frame
overflow: its repeated 16×16 backdrop and 8×8 stripe share demand unevenly among
four actual cameras. Post-course cameras now share the same existing physical
`0x590000..0x5a0000` command arena. The compositor preflights fixed-size pool
ownership and appends only actual successfully prepared lists; it neither grows
RAM nor premeasures/replays late Source controls. Exact-identical ranges share;
other sprite ranges must be disjoint, including skipped passes. Consumed
prefixes and core/framebuffer/Replay reservations remain protected. Failed
preparation submits no task; the whole frame is not a RAM-rollback transaction.
Fractional edge and derivative arithmetic parity remains a separate obligation.

A separate actual direct Photo15 fixture qualifies original initialization,
four cameras, three Source updates and three GPU/Host frames, then holds the
third image across genuine STOPPED retirement. All 81 terminal/80 created/82
absent task identities qualify, with the same zero-device/permission conditions;
five retained artifacts total 934,648 bytes. Its early-fade image was inspected;
it is not final-UI proof. There are three genuine submissions/terminals/releases,
147,376 accepted dummy PCM frames and complete native/Input/audio/Host joins.
The official empty-roll fixture never resumes Manager or invents a scene result.

Full-run binary SHA-256:
`12fd1c1c6daf9f4f0dc4319959a868d2a5047787497144f7bc4e87c8bc584979`.
Frozen source/executable archive:
`c799d2801e64a884bbee6824a26ad10c20c6469629293e2cd190ce9e3c2fc28f`.
Canonical sorted source-SHA map:
`5898ad8eb524cb344e20a5955b94452b9dc45832d21aacc9f886e80c5b6a810f`.
These are native Linux software-renderer proofs, not independent Original
producer/hardware parity, audible output, seven playable courses or Windows11
runtime qualification. Rainbow's original Retry reaches healthy retirement;
two remaining native processes still prevent its required final Actor detach.

The separate shared-arena UI/Screen GPU regression also passes after establishing
visible objects through actual Original Icons/film operations. Its 244 terminal,
243 created and 245 absent tracer/task identities qualify once; ROM unchanged,
zero physical/permission hits, three selected log/report artifacts totaling
10,098 bytes. Across 53 completed frames, interleaved A/B/A lists, real empty
draws, skipped controls, both framebuffer contexts and late Source visibility
retain the disjoint reference's GPU color/depth/rectangle output. Exact-fit
allocation succeeds; an eight-byte-short pool and crossing/contained ranges
reject without submission, with overlap rejected before callback side effects.
Actual audio executes 218 RSP tasks/116,400 stereo frames and joins its retained
PCM drain. This component proof does not claim audible output or game completion.

### Terminal native joins and retained Photo contexts — 638f/638g

Rainbow retirement now drains its actual Photo FIFO before checking remaining
controller requests. After commands detach, `Services::retire_objects()` joins
the remaining Services-owned native objects outside OM dispatch, before Actor's
unchanged zero-process/model/reference retirement guard. This includes the
environment publication object and original Retry fade worker: Source clears
the fade's busy flag three updates before deleting its object. Retirement does
not invent those updates, loosen Actor guards or destroy callback owners early.
The terminal sweep closes new object/process admission; destruction retains
the existing semantic-record and Source-storage release order.

The actual UI/Screen consumer rejects the sweep during a genuine native callback,
then joins a live publication process and original delayed black-fade process
at the safe point. Native object/process/model/sprite/camera counts reach zero
without advancing the game clock. Its 53 completed GPU frames and 225 audio RSP
tasks produce 120,128 stereo frames. All 245 terminal/244 created/246 absent
tracer/task identities qualify once, stable with unchanged ROM and zero physical
or permission hits; three retained artifacts total 10,300 bytes.
Binary SHA-256: `de0ffae2bc19f8be6d98ee46dee935c01e3e1d24b23eb20faaaf48dcbb09accb`.
Archive: `df638c96519f4993b89ae5beb8a466acc0611028505dac1a5e1299cda0202274`.
Sorted source-SHA map:
`bae39f438146a87aebd2532716a6948f4d2988e2265eaabd24c955a9efec92c0`.

Original `PokemonDetector_Create` resets focus, membership, FIFO and context
selection but deliberately retains serialized Photo headers. A fresh course
therefore must not require those application-owned headers to be zero merely
because its native Detector history is fresh. Binding removes only that
incorrect zero-header condition; it preserves exact Source ownership,
uncreated/unstaged native-history checks, task completion, generation, tickets,
GPU joins and retention checks. Retained headers do not mint a native Image or
committed capture generation. Actor identity pointers are still reset.

The actual Photo GPU consumer preserves both prior headers across a genuinely
fresh Detector, verifies generation zero/context selection one, and performs new
real captures. Its 68 GPU frames and four separately reported failed GPU frames
retain queue, prefix, ownership and roll-exhaustion coverage, including 83
original C `OM_CopyInfo` calls. All 364 terminal/363 created/365 absent identities
qualify once, unchanged ROM and zero physical/permission hits. Three retained
artifacts total 13,544 bytes.
Binary: `eabb20e88efc3f86c04046c4bc445b51883d9be35e3ad78eb1fb50982a2de563`.
Archive: `ebdb2f841d7faaf79fc43573c78f836802f2ac96efc8d425d354b82cc78ed395`.
Sorted source-SHA map:
`1a2c20301272e7dc99e014888d9d315ba4014b4952a3fd8257d70bd7ba453845`.

### Original Rainbow Retry and fresh-owner shutdown — 638g

The real application Manager follows Boot → Title → Name → Oak6 → Rainbow6.
After Name, the private driver uses the official selected/unlocked-course
operations to author progress in RAM; it does not play the unlock sequence or
commit that fixture. Original intro and controls run, actual pause input selects
Retry, and Source returns reason6/result6. The old Actor reaches DETACHED,
native storage and entry-domain receipts close, and Manager constructs a fresh
Rainbow6 owner. Its original intro and controls run with retained application
Photo headers. A held completed frame blocks STOPPED retirement until released;
shutdown joins Runtime/Manager/native/Input/audio/Host owners without fabricated
Source completion or result.

The two course owners each present 154 frames before their selected captures;
the first ultimately presents 188 frames. All application entries together
submit, complete, release and present 1,079 canonical GPU frames, with 2,036,400
accepted SDL-dummy PCM frames. Both actual course-window images were inspected:
visible world, reticle and film UI on initial entry and re-entry. This does not
qualify natural Mew/course completion, played progression, hardware, audible
output, all Rainbow behavior or Windows execution.

All 239 terminal/238 created/240 absent tracer/task identities qualify once:
stable, unchanged ROM, zero physical-device attempts and zero permission errors.
Six retained artifacts total 2,012,949 bytes.
Binary: `926dd7925e7e018afee69f2b609549c7b0d8217cc6ab280d01b6cd7af49ea259`.
Archive: `7587b3c30d38d667c3ca086be491697cb689680d717e06ef943002d1c02ed922`.
Sorted source-SHA map:
`bf50e62f0131ac20365c3df59a06dcfb838473d2d755f852dd289b302214ef39`.

### Full course/post-course regression after retirement fixes — 638g

The latest real application repeats the complete original Beach → CameraCheck9
result15 → PhotoCheck15 result13 → constructed Oak13 route after both terminal
Services cleanup and retained-header changes. There are 5,972 canonical GPU
submissions/terminals/releases/LUS presentations and 13,314,912 accepted dummy
PCM frames. Source captures four photographs, marks the same Pidgey record and
stores Report slot8 with 3,300 points and unchanged canonical digest
`1506459116649097628`. The actual final acknowledgment gates and completed
Camera/Photo captures remain intact. All seven selected Beach/post-course
images were inspected, including tutorial, travel, outro, completed Camera
prompt, marked selection, explicitly early assessment and final summary.
Oak13 is constructed, not rendered.

All 590 terminal/589 created/591 absent tracer/task identities qualify once:
stable, unchanged ROM, zero physical/permission hits. Fifteen artifacts total
7,541,469 bytes, retained once with the tree permanently closed.
Binary: `049fb7c5d477b40277ec6ba4a19390a2520057d86a2bb0e80d6dd3979536c9fe`.
Archive: `3de95a6946192dee50743d504042a5d386f5d0404fd250ff47d25a7295bc17bf`.
Sorted source-SHA map:
`1a2c20301272e7dc99e014888d9d315ba4014b4952a3fd8257d70bd7ba453845`.

### Exact pinned matching-ROM linker authority — 638g

A separate private build exports only
`ethteck/pokemonsnap@df8815e75f3d79618466e01e0827c6165b93d11c`,
uses its frozen dependency lock and configure/setup flow, compiles all 2,519
MIPS targets, and passes the pinned ninja ROM-checksum gate. Explicit hashes of
the freshly built ROM also match both authoritative baselines:
SHA-1 `edc7c49cc568c045fe48be0d18011c30f393cbaf`;
SHA-256 `a1d5d816db7f8557ee04c35a011326d058b2c1fbca76b57b352b1d705a1ec1cc`.
The resulting complete 33,385-line/2,173,956-byte linker map has SHA-256
`545866faf5fb44abf47d2f455ff7cad75580dc15320c357ac8dadc3359cb1a00`.
It supplies 16,484 unambiguous named numeric identities. The staged five-course
inventories' nonempty function definitions all have map identities; three
unused static `nullsub2` bodies have no emitted symbol and require no invented
guest address. Source-local duplicate names remain file-scoped.

All 9,377 terminal/9,376 created/9,378 absent tracer/task identities qualify once:
stable, unchanged input ROM, zero physical-device attempts and zero permission
errors. Four prospectively selected artifacts total 2,360,799 bytes, retained
once. No file in that closed tree is reopened; numeric recovery uses only the
cached map outside the tree. The ROM, Source archive, assets and full map are
not public release artifacts.
Launcher: `55c7fbd1ea54f7136be26db4d347db9c823ba9d88a1b5ffe67193b354b45ba62`.
Frozen launcher/source archive:
`38ff8f06e2e456d5759d552f5e232cdc26efa04b57a85360583266d0bfa44a54`.
Sorted source-SHA map:
`6e76fb176ab8a3beaa77c821831b3495a50d89821d61daf9f7484322e4701cf4`.
Pinned upstream Git archive:
`76847d28b697babe8fcde1e32ae0cf7ec0e46d746d505ca07cfc59c14daf1155`.
This is exact Original MIPS compilation and numeric provenance, not MIPS
execution, native gameplay, complete executable course catalogs or Windows
qualification. It removes the numeric-authority prerequisite without guessing
function addresses or using the dirty adjacent fork.

### Clean-checkout hosted build boundaries — 17332ff

Published commit `17332ff4821c1f37838900ed540549e3924b9516` is the first
public cohort containing the actual native application. Hosted run
`39124821957` passes Linux full-product compilation, application/interface
consumer compilation, actual product help, Release-active CPU tests and bounded
real renderer submission/teardown. Its standalone canonical-RDP job also passes.
These are hosted Linux build/consumer receipts, not additional ROM gameplay or
Windows qualification.

The broader matrix reveals two distinct build defects. Windows basic and
full-host configuration reject `include/35mmisle/continuation.h` at exact native
support admission: the per-file attributes list omitted transitive hashed inputs,
so checkout changed their line endings. Four GCC/Clang Debug/Release basic
builds fail to link Rainbow callbacks because their real provider object target
was incorrectly confined to the RDP configuration.

Text checkout now has one canonical LF policy with Git's binary detection,
replacing the obsolete path-by-path list; no input hash check is weakened.
An actual private Git clone/read-tree/forced checkout with `core.autocrlf=true`
passes all 615 support identities across 72 manifests and 260 unique files.
The formerly failing header retains SHA-256
`10fb58f5f6cebe32ea8662cb7a4f1bfc8e287d88b6355bdd6b30cba22e7f83c9`.
This exercises real Git checkout behavior on Linux, not a Windows runner.

The existing real Rainbow provider object target now belongs to the native
ActorOwner scope, independent of optional RDP support. No callback stubs,
conditional no-ops or fake supported-course results are introduced. The corrected
actual hosted configuration (`linux-debug` with GAME engine and RSP enabled,
RDP disabled) builds all targets and passes 57 CPU tests in 3.60 seconds; both
actual ROM-checker and coprocessor CLI help paths execute successfully. The
smaller unextended preset separately passes 26 tests but is not substituted for
that affected configuration. The full LUS product/default and 31 opt-in consumer
gate also passes, with 57 CPU tests in 4.07 seconds. Fresh hosted Windows/matrix
qualification remains pending; the earlier failed run is not relabeled as passing.

### Hosted actual application on Windows x64 — 69b6bae

[Run 39126580987](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/39126580987)
passes all ten jobs at exact public commit
`69b6bae0cf68ca56ff0e34be1afdc4f047d46d3c`. This is the first hosted
Windows build containing the actual application, not only earlier components.
Full-host job `117672874399` uses MSVC C/C++ 19.44.35229.0, SDK 10.0.26100.0,
static-runtime dependency configuration and the observed Windows Server 2022
10.0.20348 runner.

MSBuild produces `build/windows-host-release/Release/35mmisle.exe`, the actual
application-runtime ROM consumer and post-course-interface consumer. All 57
ROMless CPU tests pass in 2.18 seconds. The product's real `--help` path executes
successfully after the five component CLIs, without requiring a ROM or opening
graphics/audio/input devices. Windows Debug/Release basic builds also pass,
including their real CLI/Unicode input, installation and local tooling-package
checks. GCC/Clang Debug/Release, standalone canonical RDP and Linux LUS product
jobs pass; the Linux product retains its actual renderer submission/teardown
smoke.

The previous CRLF-admission and non-RDP-provider failures are corrected and
independently exercised; the failed `17332ff` run remains historical. This run
does not contain a private ROM, execute Windows gameplay, establish Windows11
device compatibility, qualify the remaining five course owners or authorize a
binary release. Compiler/runner versions are observed, not newly pinned inputs.

### Source stationary spline intervals and remaining-course data consumers

Actual shared `DataDecoder` admission in 638l rejects Tunnel header
`D_802F01F0_5ED2C0` at path `0x80141c5c`: the previous timestamp guard
requires strict increase. The failed capture is permanently rejected with zero
artifact reads. Fresh typed inspection 638m establishes CATROM, five timestamps
`[0, 0, 0.1786330044, 0.6666010022, 1]`, zero tension, identical first-segment
endpoints and an all-zero first speed polynomial. This is a stationary segment,
not a malformed header or an inferred asset boundary.

Independent planner/reviewer assessments agree with pinned `src/1F960.c:258–309`:
segment search uses strict `<`; nonlinear parameterization never divides by
timestamp width. At parameter zero its first midpoint is 0.5, the residual
tolerance terminates that iteration, and the resulting uniform parameter is
0.125. No zero-parameter shortcut, duplicate removal or epsilon is equivalent.
The decoder now retains nondecreasing timestamps. Native evaluation rejects
decreasing intervals and selected zero-width LINEAR intervals, while nonlinear
zero-width intervals use the unchanged Source solver. Finite-value, exact
endpoint, declared coefficient and position-control bounds remain enforced;
terminal spline-velocity overreads remain explicitly rejected.

The permanent World runtime regression drives real opcode-13 model transforms:
stationary and distinguishable-midpoint CATROM cases, positive parameters,
terminal positions, LINEAR equality/duplicate skipping, selected zero-width
rejection and stationary velocity. The default LUS product, 31 opt-in consumers
and all 57 CPU tests build/pass; CPU execution takes 3.56 seconds. Existing
upstream/private-Source conversion warnings remain visible.

Fresh 638o also decodes the exact actual Tunnel path, checks its stationary
position/velocity, samples its bounded positions, and compiles/binds/steps the
actual eight-node initialization `0x802f025c` with Source header `0x802f01f0`
and its actual speed. It qualifies once: one terminal task, no created children,
two absent task/tracer identities, stable unchanged ROM, zero physical-device
attempts and permission failures; three retained artifacts total 1,811 bytes.
Executable SHA256:
`7a7e92e94edffe4f710ab0ff260caeb19a24996267006337d70e1f713998bb85`.
Source archive:
`9c566f8c9cf8d40148bada0b3a23c9d0a3c3be7ec39779fcf6823a92a410fce4`;
Source map:
`54206252f3d9a0a049899464b51bd8c04414d3579cf228b655fa2f87e8f41f48`.

Independent 638n cases close these complete shared data-consumer sets:

| Course | Models | Materials | Model streams | Texture streams | Receiver-qualified headers | Paths |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Volcano | 20 | 59 | 1,318 | 254 | 91 | 4 |
| Cave | 18 | 68 | 1,201 | 316 | 84 | 0 |
| Valley | 20 | 23 | 1,136 | 85 | 96 | 0 |

Each performs actual bounded `DataDecoder` parsing, Source/all-program material
extent admission, hierarchy decoding, zero-obligation graphics closure and every
selected header's real native animator bind/immediate update. Each qualifies
once with the same task/device/ROM invariants as 638o. Cave retains four header
identities under both genuine initialization-qualified bindings; Valley's
Source-used signed list `0x802ecb00` has two live rows. Common1003 is included.

Tunnel and River cases remain rejected with zero artifact reads. The separate
638o numeric diagnostics establish why: Magnemite's declared four-slot image
allocation contains a NULL hole at slot1, while its complete STEP pulse selects
slots2/3 and the initial renderer selects slot0; a contiguous prefix therefore
overapproximates actual consumption. River's LOD channel uses non-STEP cubic
commands, for which unrestricted seek/update-domain inference correctly requires
an explicit sampling capability or independently proven Source allocation.
Neither failure is suppressed or relabeled as readiness.

These are shared numeric/data-consumer proofs, not activated remaining-course
Actors, Original callbacks, custom-render effects, GPU presentation or gameplay.
Volcano's custom renderer identities are retained; shape classification alone
does not implement their ordered effects. No Windows gameplay or full-game
completion is established by this cohort.
