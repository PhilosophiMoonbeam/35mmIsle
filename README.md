<p align="center">
  <img src="assets/35mmisle-logo.svg" alt="35mmIsle — a film camera with a palm island and coral sun inside its lens" width="600">
</p>

# 35mmIsle

**35mmIsle is the native port of Pokémon Snap (1999) to modern systems, based on [libultraship (LUS)](https://github.com/Kenix3/libultraship).** The project targets Linux, Windows and macOS.

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

## License and contributions

[LICENSE](LICENSE) is MIT for **original 35mmIsle contributions**, copyright 2026 35mmIsle contributors. It does not relicense Pokémon Snap, Nintendo/SGI material, decompilations, reference ports, dependencies or extracted payloads. Their rights and notices remain with their respective holders; missing or restricted grants block the affected import/release. Read LICENSING before proposing any imported source or generated content.

Development work is tracked in Beads. Architecture, porting decisions and validation requirements are documented under `docs/planning/`.
