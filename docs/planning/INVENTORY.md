# Frozen portability inventory

## Scope and method

This is a window-based candidate inventory, not a bug count, blocker count, decompilation completeness measure, or native-port certification. The authoritative snapshot is `ethteck/pokemonsnap` at `df8815e75f3d79618466e01e0827c6165b93d11c`. No unflagged bulk source was hand-read for this review. SDK units are included to expose boundaries, not to create a native SDK import backlog; native equivalents require separate design and rights review.

The frozen unit is one tracked hand-authored C/C++ translation unit, represented by file identity, pinned revision, and numbered source windows around fixed N64/ABI signatures. The deterministic prefilter excludes non-C/C++ translation units, vendor code under `tools/`, files without a fixed signature, and numeric/graphic asset-only units by the `/assets/` or `/textures/` path rule. There are no judgment-based exclusions.

Exact signature:

```regex
\b(?:INCLUDE_ASM|GLOBAL_ASM|os[A-Z][A-Za-z0-9_]*|OS[A-Z][A-Za-z0-9_]*|SPTASK|PHYS_TO_K0|K0_TO_PHYS|K0BASE|GFX_ADDR|SEGMENTED_TO_VIRTUAL|SEGMENT_OFFSET|IO_READ|IO_WRITE)\b|\b(?:u32|s32|uintptr_t|intptr_t)\s*\)\s*(?:&|[A-Za-z_]\w*(?:->|\[))|\b(?:__asm__|asm)\s*\(
```

Window radius: **6 lines**; per-unit evidence cap: **24,000 characters**. Escalate every `mixed`/`other` verdict, error, truncation, or primary top probability **below 0.7**. Also inspect uncertain `assembly` or `asset_payload` boolean probabilities in **[0.3, 0.7]**. Inspect only flagged units, verify claims against exact source, and record confirmation or correction. The single truncated unit (`src/sys/audio.c`) received complete signature-window review; the truncation is fully resolved, not a missing evidence allowance.

### Frozen questions and labels

The primary question asks which evidenced obstacles prevent faithful 64-bit native compilation with libultraship; unseen implementations must not be presumed portable or implemented.

- `n64_platform`: The shown source contains N64 operating-system, device, task, or MIPS assembly dependencies and no explicit pointer-width or address-representation hazard.
- `host_representation`: The shown source contains pointer-width, segmented/physical-address, endian, or serialized-layout dependencies and no N64 operating-system, device, task, or MIPS assembly dependency.
- `mixed`: The shown source contains both N64 platform dependencies and explicit host representation dependencies.
- `gameplay_only`: All matched signatures in the shown source are false positives or portable gameplay constructs without an evidenced N64 platform or host representation dependency.
- `other`: The shown source is insufficient to establish one of the other classifications or evidences a different kind of obstacle.

The two independent boolean questions ask whether shown source explicitly relies on `INCLUDE_ASM`, `GLOBAL_ASM`, inline MIPS assembly, or externally assembled implementation (not prose), and whether it includes/embeds game asset payloads or generated definitions (not handle/name references).

## Counts and denominators

| Stage | Count |
| --- | ---: |
| Tracked paths | 1,398 |
| Deterministic exclusions | 1,067 |
| Non-translation-unit exclusions | 324 |
| No-fixed-signature exclusions | 743 |
| Judged units | 331 |
| Judge errors | 0 |
| Initially truncated units, fully resolved | 1 |
| Flagged units directly reviewed | 38 |
| Confirmed primary verdicts | 13 |
| Corrected primary verdicts | 25 |
| Unflagged units not hand-read | 293 |

The exclusion accounting is 1,398 − (324 + 743) = 331. The raw judge distribution over all 331 units is **313 `n64_platform`, 8 `host_representation`, 8 `mixed`, 2 `gameplay_only` (signature false positives)**. These are candidate classifications, not verified dependency/bug totals. The reviewed subset alone is **2 platform, 9 representation, 24 mixed, 3 false-positive units** after review. Do not substitute this subset distribution for the frozen raw distribution or extrapolate it to the 293 unread units.

## All 38 reviewed units

Rows are grouped by final reviewed kind. Each link pins the source path and evidence span; “confirmed” refers only to the primary label, not blanket source correctness. Evidence summaries preserve the frozen review decisions.

### `n64_platform` (2)

| File and pinned evidence | Judge → review | Decision | Evidence |
| --- | --- | --- | --- |
| [`src/app_level/items.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_level/items.c#L223-L223) | `n64_platform` → `n64_platform` | Confirmed | OSTime SDK type only; keep as type seam, not proof of a thread/device requirement. |
| [`src/window/element.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/window/element.c#L905-L907) | `mixed` → `n64_platform` | Corrected | Cache call is a platform seam; line 480 converts a numeric byte count (window.h:112), not a pointer. |

### `host_representation` (9)

| File and pinned evidence | Judge → review | Decision | Evidence |
| --- | --- | --- | --- |
| [`src/app_render/47380.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_render/47380.c#L971-L1029) | `host_representation` → `host_representation` | Confirmed | Course effect ROM-boundary and destination symbols are narrowed to 32-bit integers. |
| [`src/app_render/568D0.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_render/568D0.c#L76-L80) | `gameplay_only` → `host_representation` | Corrected | userData is converted to s32 as an array index; encoded integer payload requires explicit host representation. |
| [`src/app_render/effect.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/app_render/effect.c#L1302-L1340) | `host_representation` → `host_representation` | Confirmed | Pointer-width cursor uses sizeof safely, but object-parent traversal depends on pointer sentinel 1; no direct truncation claim. |
| [`src/main_menu/A08E30.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/main_menu/A08E30.c#L1311-L1313) | `n64_platform` → `host_representation` | Corrected | VPK_VRAM heap sizing subtracts an s32-narrowed host address. |
| [`src/photo_check/98C330.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/photo_check/98C330.c#L395-L397) | `n64_platform` → `host_representation` | Corrected | Photo-scene sizing subtracts addresses narrowed to s32. |
| [`src/sys/ml.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/ml.c#L11-L17) | `host_representation` → `host_representation` | Confirmed | The u32 complement mask erases the upper 32 pointer bits; compiled synthetic and actual malloc smoke proves the hazard. |
| [`src/sys/render.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/render.c#L840-L843) | `host_representation` → `host_representation` | Confirmed | Parent pointer sentinel 1 participates in matrix-stack decisions; preserve the representation intentionally. |
| [`ultralib/src/gu/us2dex_emu.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/gu/us2dex_emu.c#L386-L389) | `host_representation` → `host_representation` | Confirmed | S2DEX emulation narrows image pointer to u32; numeric sizing matches are otherwise false positives. |
| [`ultralib/src/os/physicaltovirtual.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/os/physicaltovirtual.c#L4-L5) | `n64_platform` → `host_representation` | Corrected | Pure physical-to-K0 address conversion; function name alone is not a second device/thread dependency. |

### `mixed` (24)

| File and pinned evidence | Judge → review | Decision | Evidence |
| --- | --- | --- | --- |
| [`src/more_funcs/5BF20.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/more_funcs/5BF20.c#L646-L652) | `n64_platform` → `mixed` | Corrected | Save-relative address fields are derived from native pointer differences alongside osGetTime; fixed serialized format must be separated. |
| [`src/sys/audio.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/audio.c#L473-L511) | `n64_platform` → `mixed` | Corrected | Complete signature windows reviewed after truncation; DMA/queues plus physical audio buffer addresses need separate native synthesis/address contracts. |
| [`src/sys/error.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/error.c#L462-L531) | `n64_platform` → `mixed` | Corrected | OS thread-context/framebuffer handling plus emulated stack/address interpretation; glyph payload confirmed separately at lines 80–88. |
| [`src/sys/gtl.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/gtl.c#L178-L203) | `n64_platform` → `mixed` | Corrected | OS task/queue lifecycle plus address alignment checked through s32; uintptr_t casts elsewhere are not blanket portability proof. |
| [`src/sys/sched.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/sys/sched.c#L598-L606) | `n64_platform` → `mixed` | Corrected | OS RSP/VI task scheduler embeds framebuffer pointers into task output/address fields. |
| [`ultralib/src/audio/env.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/audio/env.c#L399-L404) | `n64_platform` → `mixed` | Corrected | SDK audio ABI uses physical state addresses plus profiling clock calls. |
| [`ultralib/src/audio/load.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/audio/load.c#L66-L104) | `mixed` → `mixed` | Confirmed | ADPCM ABI physical book addresses and pointer-to-s32 wave-base arithmetic. |
| [`ultralib/src/audio/parse_abi.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/audio/parse_abi.c#L12-L17) | `mixed` → `mixed` | Confirmed | Physical command-list address is serialized to an N64 host-debug transport. |
| [`ultralib/src/audio/resample.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/audio/resample.c#L93-L96) | `n64_platform` → `mixed` | Corrected | RSP resample ABI embeds physical state address; profiling clock is a platform call. |
| [`ultralib/src/audio/reverb.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/audio/reverb.c#L297-L345) | `n64_platform` → `mixed` | Corrected | RSP resample/load ABI embeds physical ring-buffer/state addresses. |
| [`ultralib/src/flash/flashchange.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/flash/flashchange.c#L4-L6) | `n64_platform` → `mixed` | Corrected | Flash-device handle uses N64 physical-to-K1 mapping. |
| [`ultralib/src/flash/flashreinit.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/flash/flashreinit.c#L4-L6) | `n64_platform` → `mixed` | Corrected | Flash-device reinitialization maps start address to K1 memory. |
| [`ultralib/src/gt/dumpturbo.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/gt/dumpturbo.c#L49-L62) | `mixed` → `mixed` | Confirmed | OSTask debugger dumps a native pointer via int32 arithmetic; SDK diagnostics are not host gameplay code. |
| [`ultralib/src/gu/dump_gbi.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/gu/dump_gbi.c#L208-L220) | `mixed` → `mixed` | Confirmed | SDK display-list debugger narrows task pointers; address-space parsing is not reusable as-is. |
| [`ultralib/src/gu/loadtextureblockmipmap.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/gu/loadtextureblockmipmap.c#L616-L626) | `n64_platform` → `mixed` | Corrected | Texture GBI emits osVirtualToPhysical buffer addresses. |
| [`ultralib/src/gu/parse_gbi.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/gu/parse_gbi.c#L48-L53) | `mixed` → `mixed` | Confirmed | Display-list debug transport combines physical addressing and host device writes; additional segment translation at lines 84–86. |
| [`ultralib/src/gu/parse_rdp.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/gu/parse_rdp.c#L27-L32) | `mixed` → `mixed` | Confirmed | RDP list physical address sent through N64 debug transport. |
| [`ultralib/src/gu/parse_string.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/gu/parse_string.c#L26-L31) | `mixed` → `mixed` | Confirmed | String physical address serialized through N64 debug transport. |
| [`ultralib/src/host/host_ptn64.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/host/host_ptn64.c#L13-L23) | `n64_platform` → `mixed` | Corrected | Hardware PI mapped pointer plus OS wait queue/timer; not a desktop host implementation. |
| [`ultralib/src/os/getmemsize.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/os/getmemsize.c#L11-L24) | `n64_platform` → `mixed` | Corrected | SDK memory sizing probes fixed K1 RAM addresses through volatile pointers. |
| [`ultralib/src/os/initialize_kmc.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/os/initialize_kmc.c#L16-L26) | `n64_platform` → `mixed` | Corrected | KMC initialization uses fixed MMIO pointers/physical mapping plus PI IO; exception-vector writes at lines 99–111. |
| [`ultralib/src/os/initrdb.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/os/initrdb.c#L11-L28) | `n64_platform` → `mixed` | Corrected | SDK debug buffer narrows pointers to u32 and masks interrupts; excluded debug functionality must not enter native gameplay. |
| [`ultralib/src/os/virtualtophysical.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/os/virtualtophysical.c#L5-L11) | `n64_platform` → `mixed` | Corrected | K0/K1 physical translation falls back to __osProbeTLB, a platform MMU dependency. |
| [`ultralib/src/rmon/rmonmem.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/rmon/rmonmem.c#L85-L90) | `n64_platform` → `mixed` | Corrected | RMON address validation calls physical translation on 32-bit protocol addresses. |

### `gameplay_only` (3)

| File and pinned evidence | Judge → review | Decision | Evidence |
| --- | --- | --- | --- |
| [`src/26D80.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/26D80.c#L597-L598) | `n64_platform` → `gameplay_only` | Corrected | Sprite float-to-integer coordinates, not pointer conversion; overturn platform false positive. |
| [`src/window/8456E0.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/src/window/8456E0.c#L524-L525) | `n64_platform` → `gameplay_only` | Corrected | Sprite coordinate numeric casts, not address narrowing; overturn platform false positive. |
| [`ultralib/src/os/ackramromread.c`](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/ultralib/src/os/ackramromread.c#L15-L16) | `n64_platform` → `gameplay_only` | Corrected | Empty SDK function body is a signature false positive, not an evidenced porting dependency. |

