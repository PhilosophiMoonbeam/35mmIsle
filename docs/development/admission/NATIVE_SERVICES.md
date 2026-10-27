# Native services, movement, Player, and Item ownership

Native service ownership, movement, Player and Item execution, and their bounded historical evidence.

[Admission index](../ADMISSION.md) · [Project README](../../../README.md)

## Contents

- [Native movement lifetime and original-C execution](#native-movement-lifetime-and-original-c-execution)
- [Canonical C-compatible World movement](#canonical-c-compatible-world-movement)
- [Native Player dependencies and checked C context](#native-player-dependencies-and-checked-c-context)
- [Canonical Actor/Item Player association](#canonical-actoritem-player-association)
- [C shutter, published focus and cart numeric resources](#c-shutter-published-focus-and-cart-numeric-resources)
- [Native Item and complete C interaction execution](#native-item-and-complete-c-interaction-execution)
- [Original-compatible image codec and checked native persistence](#original-compatible-image-codec-and-checked-native-persistence)
- [Deterministic game RNG and supplied-tick projections](#deterministic-game-rng-and-supplied-tick-projections)
- [Canonical Player FIFO and native Source updates](#canonical-player-fifo-and-native-source-updates)
- [Native renderer prerequisites: fragments, prefix capture and processes](#native-renderer-prerequisites-fragments-prefix-capture-and-processes)
- [Checked World callbacks, complete vectors and SourceControl](#checked-world-callbacks-complete-vectors-and-sourcecontrol)
- [Genuine Butterfree/Doduo and native object helpers](#genuine-butterfreedoduo-and-native-object-helpers)
- [Complete native collision manager and command26 sender lifetime](#complete-native-collision-manager-and-command26-sender-lifetime)
- [Shared source lifetime, native spatial queries and optional ceilings](#shared-source-lifetime-native-spatial-queries-and-optional-ceilings)
- [Original camera callbacks and native source render phases](#original-camera-callbacks-and-native-source-render-phases)
- [Complete ordinary first-entry behavior and factory closures](#complete-ordinary-first-entry-behavior-and-factory-closures)
- [Complete original Player Root](#complete-original-player-root)

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

## Original-compatible image codec and checked native persistence

`save_image.hpp/.cpp` independently implements the canonical 127,652-byte image, all sixteen version bytes, fixed relative offsets and the original digest convention. RFC 1320 byte padding and little-endian bit length are retained, but every padded compression word and stored state word is big-endian. Ordinary MD4 or input-only word swapping is incompatible. Native release execution matched all 86 actual GAME hash returns and reproduced the complete GAME-sealed initialized image byte-for-byte. Twelve resealed malformed offsets accepted by the original validator are deliberately rejected by native structural validation. Unknown bytes, float/bitfield representations and final hash-covered padding are preserved; this is not photograph replay or progression validation.

The fixed 128-byte native envelope includes codec/version, generation, canonical ROM/source identities, zeroed reserved fields and SHA-256 over header metadata plus exact payload. It requires exact EOF and a valid inner image; SHA-256 is integrity, not authentication. Sealing preflights before changing only the raw GAME-time word and digest. The codec allocates no storage and publishes only borrowed immutable views; it does not invent a new-game initializer or depend on SDK/GAME hash bodies.

`save_store.hpp/.cpp` provides alternating `save.a`/`save.b`, no HEAD file, a stable lifetime `save.lock`, immutable owned loaded snapshots and synchronous generation-checked commits. Complete temporary writes are read back, validated and flushed before installation into the inactive slot. Linux acknowledges file plus directory sync; Windows uses checked wide-character native installation, reopened verification and file flush, reported only as Win32 acknowledgment. After any installation ambiguity, writes freeze and evidence remains. Rejected existing slots, conflicting generations or orphan temporaries block automatic writes even when another valid recovery candidate exists. No fallback/reset/migration/promotion silently consumes damaged evidence.

An actual native Release consumer published generations 1 and 2 in a Unicode profile and cold-reloaded each in separate processes. A stale-generation commit preserved both slots. A competing process failed the session lock; killing the holder released it without destructors and cold reload retained generation 2. Truncated and future-version newest slots each exposed the older read-only candidate, rejected commits and preserved both slot byte identities. These observations are local Linux filesystem evidence, not power-loss survival, Windows 11 execution, complete save UI integration or W10 photo fidelity.

Original hash/layout collection used exact pinned GAME code and public architectural/debugger interfaces, not SDK implementations. Final run05/06 verified loaded GAME bytes and unchanged protected identities; no external device/write open succeeded. Earlier failed automatic camera/GPU/input probes are preserved and excluded. A failed software-graphics `udmabuf` allocator probe remains disclosed in the admitted final runs; no blanket no-hardware-attempt history is asserted. No original Flash write/durability or persisted Flash save was measured. GAME-derived images, authored inputs, command/PCM captures and reference executables remain private.

## Deterministic game RNG and supplied-tick projections

`rng.h/.c` extends `isle-native` with caller-owned 32-bit replay state and no global state or allocation. Each seed-based operation advances the modulo-2^32 linear congruential generator once and uses all sixteen high bits, not the C library's fifteen-bit `rand()`. Binary fractions remain exact. Range scaling preserves the original signed product wrap and truncation toward zero, including zero, negative and overflowing ranges. Tick-based operations use only the low eight bits of an explicitly supplied guest `osGetTime`-domain value and do not advance the LCG. Time-range conversion preserves binary32 rounding under the caller's FP mode.

Requirements derive from authoritative GAME `src/sys/misc.c:110–146`, blob `84be12339c2f266dcfb5e2ae7ccd6a73cbe2a46b`, not SDK implementation. Private source-compiled arithmetic comparison used those exact GAME definitions, independent scalar interface declarations and supplied tick values. `-fwrapv` expresses MIPS low-word arithmetic for the private source oracle; the original native service uses defined unsigned arithmetic and in-range signed conversion instead.

| Actual native API consumer | LCG results and resulting seeds matched | Tick-projection results matched | FP modes |
|---|---:|---:|---:|
| GCC Release | 3,670,016 | 14,336 | 4 |
| Clang ASan+UBSan | 3,670,016 | 14,336 | 4 |

Every high16 output and low8 tick value participates; both consumers reported zero differences, with no sanitizer diagnostics. Existing native regressions cover mixed-call advancement, full-bit outputs, signed wrap, division direction and binary32-versus-integer scaling boundaries. All twenty component tests passed under GCC Debug/full LUS, GCC Release and Clang ASan+UBSan. This is native arithmetic and private source-compilation evidence, **not** Original R4300 execution, original time-service mapping, photo replay, gameplay RNG call-order or full W04 qualification. No GAME body or SDK implementation/header is published.

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

All 52 CPU contracts pass GCC Debug, GCC Release and Clang ASan+UBSan. All 23 accepted private trees were complete, stable and absent before their sole initial physical/permission observations: unchanged ROM and zero known attempts/errors. Fatal address/UB checks remain active; ptrace-only leak-scan disable is not leak qualification. Exact per-root counts, executable/archive identities and scopes are in [`actor-collision-inputs.json`](../../../cmake/actor-collision-inputs.json). Compiler/hosted evidence remains tied to its published revision. Complete interaction graphs/native Item/Player services, other species/courses, snapshot reconstruction, product entrypoint, Original equivalence and private Windows11 execution remain open.

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

All 53 CPU contracts pass GCC Debug/Release/Clang ASan+UBSan. All 24 accepted complete trees have sole initial closure/physical-permission observations, stable traces, absent tracer/tasks, unchanged ROM and zero known attempts/errors. Fatal address/UB checks remain enabled; ptrace-only leak-scan disable does not qualify leaks. Exact executable/archive/source scopes and both failing-before/passing-after bugs are recorded in [`actor-interaction-inputs.json`](../../../cmake/actor-interaction-inputs.json). Full C interaction execution, all source callback/services, actual Item/Player simulation, snapshot/reconstruction, other species/courses, product executable, Original parity and private Windows11 execution remain open.

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

At this historical checkpoint, camera flags `0x10`/`0x40` were unsupported and retained-context ProcessImage remained fail-closed. A hidden prefix never authorizes a successful request drop: pinned GAME nonblocking-enqueues the current Detector context regardless of a new capture. The later [native logical-frame qualification](HOST_RENDERING.md#native-logical-frames-and-mid-frame-gtl-tasks) supersedes these implementation limits without changing this historical cohort. Neither was an external SDK blocker.


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
At this historical checkpoint these FIFO and signed drawn-frame paths were
implemented but their expanded renderer/Photo cohort remained unqualified.
The later [native logical-frame qualification](HOST_RENDERING.md#native-logical-frames-and-mid-frame-gtl-tasks)
supersedes that limit and implements real `0x10`/`0x40` task boundaries.

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

