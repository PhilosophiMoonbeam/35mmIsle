# 35mmIsle

A planning-only project for a faithful **native desktop port of Pokémon Snap (1999)**, informed by the authoritative decompilation and existing libultraship-based ports. The proposed targets are Linux, Windows and macOS; none is qualified yet.

**There is no playable build or native game implementation in this package.** No ROM, extracted game assets, Nintendo SDK, microcode or reference checkout is supplied. Source research, a planning-document check or an isolated ABI experiment is not evidence that the game builds, runs, or is complete.

## Read the plan

Start with [PLAN](docs/planning/PLAN.md) for scope, architecture and decisions, then use the following evidence and implementation-contract index.

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
| [VERIFICATION](docs/planning/VERIFICATION.md) | What was actually checked for this planning package, and what remains unproved |
| [BEADS](docs/planning/BEADS.md) | Planning/implementation issue mapping; Beads remains the live status authority |

The [upstream `ethteck/pokemonsnap`](https://github.com/ethteck/pokemonsnap/tree/df8815e75f3d79618466e01e0827c6165b93d11c) pin is game-source authority, not the modified local fork. Initial fidelity work is limited to the exact US ROM identity recorded in BASELINES. Upstream is a work in progress; file counts are not decompilation-completeness measurements.

## Intended result, not current functionality

The completion contract is the original loop: all seven courses, original interactions and unlocks, camera/focus/shutter, photo selection and Oak evaluation, report/album/gallery, durable save and cold reload, original audio, ending and credits. Faithful photography requires both per-object live focus-color history and reconstructed-photo depth/scoring semantics; attractive rendering alone cannot pass. Real native audio synthesis precedes full original-scene boot. ROADMAP and VALIDATION define the evidence needed before any of these are claimed.

No ROM acquisition instructions or download are provided. Future setup would validate a user-supplied, rights-appropriate ROM locally and generate private resources; identity validation and archive separation do not establish permission to use or distribute material.

## License and contributions

[LICENSE](LICENSE) is MIT for **original 35mmIsle contributions**, copyright 2026 35mmIsle contributors. It does not relicense Pokémon Snap, Nintendo/SGI material, decompilations, reference ports, dependencies or extracted payloads. Their rights and notices remain with their respective holders; missing or restricted grants block the affected import/release. Read LICENSING before proposing any imported source or generated content.

Work is tracked in Beads, not a second checklist in these documents. The documents distinguish observed repository/source evidence from proposed implementation and unproved runtime behavior. No implementation or game-content import is authorized merely by its appearance in the plan.
