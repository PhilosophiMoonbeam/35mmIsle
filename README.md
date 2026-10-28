<p align="center">
  <img src="assets/35mmisle-logo.svg" alt="A professional island photo-safari emblem featuring sculpted gold and ivory lettering, an optical camera-lens horizon framing a volcanic caldera and waterfalls, a winding 35mm film strip with exposed frames, parted jungle grass under a viewfinder, a half-buried spiral fossil stone, and a coastal cliff profile watching the bay." width="1000">
</p>

# 35mmIsle

**35mmIsle is a native port of Pokémon Snap (1999) to modern systems, based on [libultraship (LUS)](https://github.com/Kenix3/libultraship).** Its target platforms are Linux x64 and Windows 11 x64.

The authoritative game source is the upstream [Pokémon Snap decompilation](https://github.com/ethteck/pokemonsnap). [BASELINES](docs/planning/BASELINES.md) records the authoritative source pin and the initial exact US ROM baseline. ROMs and extracted game assets are not distributed with this repository.

## Project scope

The port's scope covers all seven courses, original interactions and unlocks, camera/focus/shutter, photo selection and Oak evaluation, Report/Album/gallery, durable saves and cold reload, original audio, and the ending and credits. Faithful photography requires both per-object live focus-color history and reconstructed-photo depth/scoring semantics. [ROADMAP](docs/planning/ROADMAP.md) defines implementation dependencies; [VALIDATION](docs/planning/VALIDATION.md) defines fidelity requirements and acceptance gates. This scope is not a statement of current qualification.

## Getting started

The default build provides standalone native tools without LUS or game/reference checkouts. The native application is opt-in and requires a user-supplied supported ROM and additional private build inputs. See [BUILDING](docs/BUILDING.md) for prerequisites and build options, then [RUNNING](docs/RUNNING.md) for application setup and use.

The project is in development. [STATUS](docs/STATUS.md) distinguishes current candidate-specific evidence from historical results and open qualification gates. Those results do not establish a complete current-build pixel-only campaign, Windows 11 gameplay, physical-device qualification or release approval. Beads remains the live authority for work tracking; the documents below provide design, contracts and evidence rather than a live task ledger.

## Documentation

| Guide | Purpose |
|---|---|
| [BUILDING](docs/BUILDING.md) | Prerequisites, standalone tools, opt-in native application and build configurations |
| [RUNNING](docs/RUNNING.md) | Native application setup, launch options, input and runtime requirements |
| [STATUS](docs/STATUS.md) | Runtime evidence, qualification boundaries and release restrictions |
| [COMPONENTS](docs/development/COMPONENTS.md) | Historical component checkpoints and native service contracts |
| [PROBES](docs/development/PROBES.md) | Private development probes, commands and bounded evidence |
| [ADMISSION](docs/development/ADMISSION.md) | Introduction and index to detailed admission and change records |

## Planning and reference

Start with [PLAN](docs/planning/PLAN.md) for architecture and design decisions.

| Document | Purpose |
|---|---|
| [PLAN](docs/planning/PLAN.md) | Architecture, faithful core loop, decisions and stop/go questions |
| [ASSESSMENT](docs/planning/ASSESSMENT.md) | Independent review, amendments, tradeoffs and falsifying experiments |
| [BASELINES](docs/planning/BASELINES.md) | Immutable source pins, game-source authority and exact ROM baseline |
| [DECOMP](docs/planning/DECOMP.md) | Authoritative game source, scheduler, assets, graphics/photo, save and audio boundaries |
| [LUS / Ghostship](docs/planning/LUS_GHOSTSHIP.md) | Host APIs, template mismatch, resources and rendering/readback contracts |
| [2Ship / MM](docs/planning/2SHIP_MM.md) | Paired native-port/decomp comparison and limits on transferable patterns |
| [Shipwright / OOT](docs/planning/SHIPWRIGHT_OOT.md) | Paired lifecycle, resource, ABI, audio, save and packaging comparison |
| [ROADMAP](docs/planning/ROADMAP.md) | Dependency-gated work units, ownership and integration contracts |
| [VALIDATION](docs/planning/VALIDATION.md) | Original-ROM fixtures, behavioral acceptance and platform/release gates |
| [LICENSING](docs/planning/LICENSING.md) | Original-work MIT scope, third-party restrictions and import/publication gates |
| [INVENTORY](docs/planning/INVENTORY.md) | Portability screening, direct-review evidence and coverage limits |
| [VERIFICATION](docs/planning/VERIFICATION.md) | Recorded development-documentation checks and their evidence limits |
| [BEADS](docs/planning/BEADS.md) | Planning/implementation issue mapping; Beads remains the live status authority |

## License and contributions

[LICENSE](LICENSE) is MIT for **original 35mmIsle contributions**, copyright 2026 35mmIsle contributors. It does not relicense Pokémon Snap, Nintendo/SGI material, decompilations, reference ports, dependencies or extracted payloads. Their rights and notices remain with their respective holders; missing or restricted grants block the affected import or release. Read [LICENSING](docs/planning/LICENSING.md) before proposing imported source or generated content.
