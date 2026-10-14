# Original implementation admission

## Decision and evidence boundary

This record admits independently authored ROM identity tooling, checked native primitives, build declarations and synthetic fixtures for Linux x64 and Windows 11 x64. It is a material/use decision, not game qualification, a completed dependency audit, or publication authority. No Snap/SDK/Ghostship/Zelda source import is selected; no game-derived release is approved; no native gameplay exists in this admission. The objective remains the full native Pokémon Snap port through libultraship, not an identity tool substituted for the game. All 34 parent dependency edges remain; these children do not close W00/W01/W03.

Inputs are original implementation requirements; public byte-order and SHA-1 algorithm facts (including [RFC 3174](https://www.rfc-editor.org/rfc/rfc3174), not its implementation); the pinned [Snap identity declaration](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/README.md#L9-L21); and read-only source-span research in [BASELINES](../planning/BASELINES.md) and [LUS / Ghostship](../planning/LUS_GHOSTSHIP.md). Reference checkouts are not build inputs. Authored portions fall under the root MIT grant only insofar as contributors own the necessary rights. Copied, adapted or translated implementations would invalidate this original-only classification.

| Admitted surface / implementation owner | Inputs and allowed use | Ownership and containment | Falsifier |
|---|---|---|---|
| `include/35mmisle/rom.hpp`, `src/rom.cpp`, `src/rom_main.cpp`, `tests/rom_tests.cpp` — ROM identity owner | Original C++20 streaming normalization, SHA-1 identity implementation and CLI; z64/v64/n64 content magic; full normalized size/hash check | Caller owns input stream; bounded working memory (at most 128 KiB); read-only private file access; metadata-only output; no extraction, normalization output, repair, cache or game-byte dump | Wrong identity accepted; filename/header-only acceptance; missing byte-order normalization; unchecked read failure or partial tail; game bytes written/logged; source translation |
| `include/35mmisle/native.h`, `src/native.c`, `tests/native_tests.c` — native primitives owner | Original C11 checked size/alignment arithmetic, caller-backed arena and bounded span resolution; synthetic allocations only | Caller retains backing allocation; rejected operations leave outputs/cursor unchanged; real pointers use native widths; no SDK structures, copied allocator, game heap capacities or fake pointer sentinels | Overflow before checking; pointer narrowing; out-of-bounds pointer formation; state changes on rejection; claimed high-address proof without real allocation |
| `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/native.yml`, `.gitignore` — build integration owner | Original explicit-source C11/C++20, 64-bit tooling build and synthetic CI declarations | No LUS/game linking; tooling-only CLI/package naming; no ROM-fed public jobs, private caches or uploads; immutable action revisions require evidence | Hidden source/dependency import; fake game/host executable; payload packaging; unsupported platform success claim |
| `cmake/dependencies.json` and this record — admission/baseline owner | Original evidence inventory for the selected future standalone LUS baseline | No fetched/vendored implementation; manifest is descriptive, not an enforced complete lock | Observed tag resolution presented as enforced build pin; unresolved package/font/transitive composition called locked or approved |

Synthetic fixtures must be independently constructed, not extracted or reformatted game material. They may establish only their tested mechanisms. Source candidate identities are the exact reviewed contribution revisions/content, not a hash frozen here while parallel implementation changes. No final source hashes, build results, test results, high-address outcome or platform runtime results were observed by this record's author; those require the integration owner's subsequent evidence. Passing mechanism tests cannot establish game boot, fidelity, original audio, photography, completion or release readiness.

## Private development input

The user confirmed a lawful private development ROM. The integration owner observed normalized US SHA-1 `edc7c49cc568c045fe48be0d18011c30f393cbaf` and 16,777,216 bytes. This is supplied evidence, not a new ROM read by this author. Availability is no longer a missing prerequisite for the **read-only development identity check** admitted here. Matching the identifier proves revision identity only, not acquisition rights. No ROM path or payload is needed in public records or synthetic fixtures.

This admission does not grant source, ROM, asset, microcode, archive or compiled game-payload redistribution rights. Extraction, reference rebuilds, captures and game-fed caches are not admitted by the identity-tool permission; each needs its actual inputs/use reviewed under [LICENSING](../planning/LICENSING.md). Private input and derived outputs remain private; no upload or acquisition service is requested.

## Exact unresolved import and release decisions

The external rights-decision prerequisite is tracked as `bd_35mmIsle-v4u.1.3` directly under the implementation epic; W00 depends on it, without changing the original 34 WID-to-WID edges. The integration owner's exact candidate examples are immutable Git blobs: `src/sys/om.c` → `4ca82a764345116a8c924fb3449e4638e0720857`; `src/sys/audio.c` → `a617f2ee04ad0851e0db46d31a6cf034e1eb0355`; `ultralib/include/PR/os.h` → `5de240174869c0b672716ba5c5bbea4ad2e4ee91`. They identify material needing decisions, not selected imports. The owner directly read `os.h:5-23` and found its explicit SGI proprietary/no-copy notice unchanged from the authoritative baseline. Repository API license metadata was null and root LICENSE/COPYING searches found none: these are evidence of an unresolved grant, not a sweeping legal conclusion about every file or intended use. The 102 tracked dirty entries remain preserved.

Before any Snap game routine, local-fork derivative, SDK header/implementation, reconstructed table, extracted RSP program or game asset enters a destination, **I1** must name the exact repository/full commit/path/span and content hash, modifications, authors/history, copied versus original portions, governing notices, generated inputs, destination and intended source/binary/linking distribution. No candidate is currently chosen. The Snap authority remains `ethteck/pokemonsnap@df8815e75f3d79618466e01e0827c6165b93d11c`, not the local fork.

**I2** requires an affirmative rights basis for that exact material and use. In particular Snap `ultralib/include/PR/os.h` carries SGI/Nintendo proprietary restrictions; missing game-wide grants and inherited port/SDK ancestry cannot be cleared by a root MIT/CC0 badge. Missing, incompatible, proprietary or ambiguous rights require an applicable rights-holder grant or qualified legal review, with recorded scope/conditions or rejection—not another generic user “proceed.” Independent replacement is a distinct provenance candidate, not permission obtained by renaming/translating.

Any derived source or asset-free game executable release separately needs **R1/R2** composition review: exact source/dependency/artifact identities, all embedded game/SDK/font/vendor content, linkage, source offers and notice delivery, plus payload/secret exclusion evidence. The original-tool admission neither approves that release nor automatically approves even a tooling package for publication. LUS remains a conditional future dependency; its static-library behavior at the selected pin does not imply every dependency is static or every embedded component is cleared.

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

## Future baseline and invalidation

[Selected dependency evidence](../../cmake/dependencies.json) records standalone LUS `759b13401cd6903daa67a00a753066f791622e78`, not Ghostship's unaudited nested gitlink. Only selected common fetched tags were resolved with `git ls-remote`; no fetch/build/link was performed. Linux/Windows package identities, embedded font/header rights, transitive notices and final composition remain unresolved. The current original tools do not link LUS.

The future reduced option set is a selection proposal, not Snap compatibility proof. `GBI_UCODE` is unvalidated and PRIVATE at this pin: game consumers must explicitly agree with library/exporter definitions, vertex representation and runtime decoder. Static source inspection identifies missing `G_MW_MATRIX` handling in the F3DEX2 moveword handler, no implemented Snap `G_SPECIAL_1` operation, and no L3DEX decoder entry; this does not claim a complete opcode audit or that Snap needs no special operations. Ghostship's SM64 flags, audio parameters and graphics-call assumptions are not Snap selections.

Any source, fixture, public algorithm input, API, pin, patch, package, option, ABI, target, embedded-data, input-use or linkage change invalidates the affected admission/proof and requires renewed provenance, compatibility, notice and behavioral review. Discovery of restricted ancestry, payload leakage or a violated containment rule suspends that material's admission. Historical research observations remain historical; unproved assumptions stay unproved.

## Subsequent orchestrator verification

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

Private trace data and throwaway consumer code/binaries were removed. Public CI receives no ROM, extracted resources or captures and uploads no artifacts. None of these checks runs the native game, an original reference/oracle, renderer/photo scoring, synthesis/device output, persistence, progression or clean-machine game packages. W00–W15 parent gates remain unclosed; the external I1/I2 decision and all downstream implementation/qualification obligations remain.
