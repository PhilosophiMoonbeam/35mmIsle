# Baselines and source authority

## Scope and evidence rules

This register freezes the source identities used for the planning package. It is not a dependency installation manifest, build result, rights clearance or claim about later upstream branches. [PLAN](PLAN.md) owns architectural decisions; the domain studies own exact source-span evidence; [LICENSING](LICENSING.md) owns provenance and import/release restrictions; [VERIFICATION](VERIFICATION.md) records exercised checks.

**Observed** means captured repository metadata or inspected pinned source. **Proposed** means a future design or proof obligation. An inference is not a runtime observation. Repository/assembly path totals below describe tracked metadata only: no fresh matched-function, matched-code, remaining-function-assembly or decompilation-completeness percentage was established.

Current candidate-scoped implementation admissions are recorded in [ADMISSION](../development/ADMISSION.md): original runtime/services and synthetic fixtures, pinned MIT LUS host integration, and user-authorized lawful private ROM extraction/reference/capture development. Restricted SDK implementation is excluded and replaced; private derived outputs remain private. [cmake/dependencies.json](../../cmake/dependencies.json) records selected Linux/Windows LUS declarations and historical remote resolutions, not a complete enforced lock or release-composition approval. The integration owner observed the lawful private ROM at 16,777,216 bytes with the US identity below. Domain-research observations remain historical; no current-wave build/test/runtime result or full-game proof is asserted here.

## Seven immutable references

The branch column records the observed upstream default branch, not a floating dependency selection. Each link addresses the exact full commit. The present-day Zelda decomp pins are independent comparison baselines; their historical ancestry relative to the native ports was not established.

| Reference | Full commit | Observed default branch | Role and authority limit |
|---|---|---|---|
| [ethteck/pokemonsnap](https://github.com/ethteck/pokemonsnap/tree/df8815e75f3d79618466e01e0827c6165b93d11c) | `df8815e75f3d79618466e01e0827c6165b93d11c` | `main` | Authoritative game-source/ROM-layout baseline; upstream WIP, not a native port or blanket license grant. [DECOMP](DECOMP.md) |
| [Kenix3/libultraship](https://github.com/Kenix3/libultraship/tree/759b13401cd6903daa67a00a753066f791622e78) | `759b13401cd6903daa67a00a753066f791622e78` | `main` | Primary host-API authority and admitted integration pin; Snap compatibility and runtime qualification remain unproved. [LUS / Ghostship](LUS_GHOSTSHIP.md) |
| [HarbourMasters/Ghostship](https://github.com/HarbourMasters/Ghostship/tree/40eef13823a954a466574dc78e73187a17a2dd42) | `40eef13823a954a466574dc78e73187a17a2dd42` | `develop` | SM64 integration/extraction example, not a game-neutral template or API-compatible substitute for the standalone LUS pin. [LUS / Ghostship](LUS_GHOSTSHIP.md) |
| [2ship2harkinian/2ship2harkinian](https://github.com/2ship2harkinian/2ship2harkinian/tree/f1dda4c28cb9ed7cc80d8c78337ee31a8d2c7d0e) | `f1dda4c28cb9ed7cc80d8c78337ee31a8d2c7d0e` | `develop` | Mature MM native-port pattern reference; not Snap behavior or a source-import authorization. [2Ship / MM](2SHIP_MM.md) |
| [zeldaret/mm](https://github.com/zeldaret/mm/tree/bf6bd9d0d3a90fe2f8960e034fbfd328e8bdef22) | `bf6bd9d0d3a90fe2f8960e034fbfd328e8bdef22` | `main` | Independent MM decomp side of the paired comparison; no assertion that this is 2Ship's exact ancestor. [2Ship / MM](2SHIP_MM.md) |
| [HarbourMasters/Shipwright](https://github.com/HarbourMasters/Shipwright/tree/6b4edce43f949fea554bc029daed42dd9b04569a) | `6b4edce43f949fea554bc029daed42dd9b04569a` | `develop` | Mature OOT native-port pattern reference; its platform, audio and resource assumptions are not adopted wholesale. [Shipwright / OOT](SHIPWRIGHT_OOT.md) |
| [zeldaret/oot](https://github.com/zeldaret/oot/tree/8a3634ca774a9b58d822cbc2cc09309f31eac6a4) | `8a3634ca774a9b58d822cbc2cc09309f31eac6a4` | `main` | Independent OOT decomp side of the paired comparison; no historical ancestry claim. [Shipwright / OOT](SHIPWRIGHT_OOT.md) |

Ghostship's observed gitlinks are LUS `93a1a41741225fd7f91265035e3288c1e86b4059` and Torch `106621f0f0f9731b8739bec95227c2c5887492df`. They are dependency metadata, **not two additional audited source roots**: nested checkout contents were unavailable. In particular Ghostship's LUS is not the standalone audited LUS. Exact API/build-option mismatches and dependency-freeze requirements are in [the host study](LUS_GHOSTSHIP.md#dependency-pin-decisions-and-exact-mismatch-risks). No complete transitive dependency lock is claimed.

## Preserved local state

These paths identify where evidence was collected on the original workstation; they are not repository-relative dependencies or instructions to reproduce that workstation layout.

| Local reference | Observed state | Preservation / reconciliation rule |
|---|---|---|
| `/home/bbferko/repos/pokemonsnap` | HEAD `b35d6de06edd43aa842fdd3d03175967dfecfbea`; branch `hermes/final-stretch`; **102 tracked dirty entries**; origin `PhilosophiMoonbeam/pokemonsnap`, upstream `ethteck/pokemonsnap` | Preserve checkout, branch, staged/unstaged work and other local material unchanged. Dirty/generated/untracked material is not authoritative upstream evidence. W01 compares every intended delta against the authoritative pin and records retain/rederive/reject plus provenance and behavioral rationale before any approved import. The dirty-entry count is not a complete inventory of local/untracked content. |
| `/home/bbferko/repos/libultraship` | HEAD `759b13401cd6903daa67a00a753066f791622e78`; branch `main`; no tracked changes observed | Local HEAD equals the captured upstream LUS commit; used read-only. Equality is an identity observation, not a build/compatibility result. |
| `/home/bbferko/repos/Ghostship` | HEAD `40eef13823a954a466574dc78e73187a17a2dd42`; branch `develop`; no tracked changes observed | Local HEAD equals the captured upstream Ghostship commit; used read-only. No nested-submodule audit implied. |

The authoritative Snap, 2Ship, MM, Shipwright and OOT snapshots were obtained under `/tmp/35mmisle-planning-xaf_qkz0/` in children `snap`, `2ship`, `mm`, `shipwright` and `oot`, respectively, at the commits above. They are temporary read-only **audit caches**, not future build paths, vendored dependencies, distributable source or permission to import their contents. Reproduction must fetch the named authorized repositories/commits into a chosen local cache; no future build may rely on these temporary paths. The planning work did not modify the reference checkouts.

## Snap input and recovery boundary

The initial supported-input proposal is the **US / North America** `pokemonsnap.z64` with normalized z64 SHA-1 **`edc7c49cc568c045fe48be0d18011c30f393cbaf`**, as declared by the [pinned upstream README](https://github.com/ethteck/pokemonsnap/blob/df8815e75f3d79618466e01e0827c6165b93d11c/README.md#L9-L21). The exact hash is revision authority, not the filename or an inferred revision label. No other region/revision is accepted by this plan. Future local tooling must recognize permitted byte orders explicitly, validate the normalized identity before extraction, and reject unsupported/corrupt inputs rather than patch them into a recognized image. No ROM was supplied, extracted, rebuilt or played as part of the domain research.

Upstream describes itself as **WIP**. Its reconstruction toolchain and generated ROM/linker/data outputs are reference machinery, not the proposed native toolchain. Source exists across surveyed game systems, but machine material and unresolved semantics remain, including SDK/entry assembly and extracted RSP programs. A matching rebuild would prove reconstruction for that input/toolchain, not complete semantic recovery, 64-bit portability, runtime parity or rights clearance. [DECOMP](DECOMP.md#buildsetup-findings) distinguishes these boundaries.

Observed tracked metadata at the authoritative Snap pin:

| Metadata | Count | Limit |
|---|---:|---|
| All tracked paths | 1,398 | Includes non-code/generated-input/tooling paths; not functions or completion |
| Tracked C/C++ paths | 1,074 | Path classification only; does not establish recovered semantics or import permission |
| Tracked assembly paths under `src` | 5 | Metadata, not residual function count |
| Tracked assembly paths under `ultralib` | 54 | Metadata, not a claim that this SDK material may be copied |
| Combined tracked assembly paths | **59** | Does not count all generated/extracted assembly, RSP binaries or unresolved behavior |

The separate [INVENTORY](INVENTORY.md) is a deterministic, windowed portability screen with explicit selection and direct-review limits, not a census of every native-port change. Its findings do not convert these path totals into completeness claims.

## Rights and future pin changes

[LICENSING](LICENSING.md) records actual notices and their limits rather than relying on repository metadata. Original 35mmIsle work is MIT only where contributors own the rights; referenced game, SDK, port, dependency and extracted material retains its own rights. Exclude restricted SDK implementations and replace their needed services with original code or permissive dependencies. A missing project-wide grant or SGI proprietary notice is a candidate-specific import/release concern, not a blanket stop on original engineering, pinned MIT LUS integration or the admitted lawful private ROM research. Private-use authorization is not a third-party redistribution grant.

The earlier whole-roadmap external-permission block is historical and superseded; `bd_35mmIsle-v4u.1.3` is no longer a global W00 dependency. W00/W01 still require provenance for actual imports, reconciliation of intended fork deltas and a coherent dependency/API/build-option baseline for the corresponding cutover. This wave's original process continuations, bounded queues and nonplayable real-LUS host probe are admitted within [ADMISSION](../development/ADMISSION.md), not proof that W00–W15 are complete. Rights decisions for future game-derived imports and release composition remain specific to the selected material/use. A changed pin or affected source/API/ABI/option/input/linkage assumption invalidates the relevant admission/proof; update its evidence and consumers. [ROADMAP](ROADMAP.md) defines gate dependencies; [BEADS](BEADS.md) maps them to live issues without duplicating task status here.
