# Beads contract mapping

## Authority and use

Implementation epic: **`bd_35mmIsle-v4u`**. Planning epic: **`bd_35mmIsle-b7h`**. These are actual local Beads IDs, not proposed names. [ROADMAP.md](ROADMAP.md) defines the implementation contracts; this document maps those contracts to the live tracker. Published contracts and IDs remain useful even when the live database is local.

**The local Dolt-backed Beads database is the sole source of current task status, claims, blockers, and completion.** This is not a parallel status tracker. The original planning handoff recorded all 16 implementation units open and unclaimed; that historical observation does not describe their current status. No Dolt remote synchronization was authorized or claimed at that handoff. Current Git publication and Dolt synchronization follow active user authorization and `bd prime`; publishing this Markdown alone does not synchronize the database.

Use the repository's Beads environment:

```sh
bd prime
bd ready
bd show bd_35mmIsle-v4u
bd show bd_35mmIsle-v4u.1
```

`bd ready` discovers currently unblocked work; `bd show <id>` reads the actual contract, dependencies, and status. Follow `bd prime` for claiming and updating work. Git publication and Dolt remote synchronization are distinct authorization decisions.

## Autonomous execution and local blockers

Pursue every reachable, independently authorized implementation or qualification task before requesting intervention. Inspect unfinished `in_progress` and blocked records as well as `bd ready`; an empty ready queue is not proof that useful work is exhausted. Distinguish implemented functionality, proven defects, validation mechanisms and release acceptance rather than repeating work from stale titles.

Attach missing data, host permission, physical-device/human observations, rights decisions and deferred Windows prerequisites only to the proofs they actually block. Keep true prerequisites and acceptance criteria intact. Parent-completion dependencies do not automatically prohibit independent lower-level work: create narrowly scoped runnable tasks with their actual inputs, retain the parent gate, and record bounded evidence without claiming parent completion.

Use independent lanes for product defects, native components/runtime, packaging/install and fidelity. Claim and verify the next reachable lane when another is blocked. Before escalation, record the remaining obligations and why each requires an inaccessible input; do not ask for intervention merely because the current preferred path stopped. Existing ownership, isolation, publication and platform-deferral constraints remain binding.

## Work-unit and dependency IDs

Dependencies below are prerequisites of the row, not tasks blocked by the row. IDs and dependency lists reproduce the handoff mapping.

| Unit | Contract | Actual Beads ID | Prerequisite units | Prerequisite Beads IDs |
| --- | --- | --- | --- | --- |
| W00 | Source, legal and ROM authority / R | `bd_35mmIsle-v4u.1` | None | None |
| W01 | Baseline reconciliation / R | `bd_35mmIsle-v4u.2` | W00 | `bd_35mmIsle-v4u.1` |
| W02 | Original-region fixture process / R + original tooling | `bd_35mmIsle-v4u.3` | W00, W01 | `bd_35mmIsle-v4u.1`, `bd_35mmIsle-v4u.2` |
| W03 | Build and ABI foundation / I | `bd_35mmIsle-v4u.4` | W00, W01 | `bd_35mmIsle-v4u.1`, `bd_35mmIsle-v4u.2` |
| W04 | Scheduling, time and RNG semantics / R | `bd_35mmIsle-v4u.5` | W02, W03 | `bd_35mmIsle-v4u.3`, `bd_35mmIsle-v4u.4` |
| W05 | Validated asset decode and relocation / I | `bd_35mmIsle-v4u.6` | W01, W02, W03 | `bd_35mmIsle-v4u.2`, `bd_35mmIsle-v4u.3`, `bd_35mmIsle-v4u.4` |
| W06 | Custom ucode and render/photo feasibility / R + limited original experiment | `bd_35mmIsle-v4u.7` | W02, W03 | `bd_35mmIsle-v4u.3`, `bd_35mmIsle-v4u.4` |
| W07 | Real host scheduling integration / I | `bd_35mmIsle-v4u.8` | W04, W05, W06 | `bd_35mmIsle-v4u.5`, `bd_35mmIsle-v4u.6`, `bd_35mmIsle-v4u.7` |
| W08 | Initial original-scene rendering / I | `bd_35mmIsle-v4u.10` | W05, W06, W07, W12 | `bd_35mmIsle-v4u.6`, `bd_35mmIsle-v4u.7`, `bd_35mmIsle-v4u.8`, `bd_35mmIsle-v4u.9` |
| W09 | Input and fixed-step course gameplay / I | `bd_35mmIsle-v4u.11` | W04, W08 | `bd_35mmIsle-v4u.5`, `bd_35mmIsle-v4u.10` |
| W10 | Photo capture, reconstruction and Oak evaluation / I | `bd_35mmIsle-v4u.12` | W06, W09 | `bd_35mmIsle-v4u.7`, `bd_35mmIsle-v4u.11` |
| W11 | Native save, album and report / I | `bd_35mmIsle-v4u.13` | W10 | `bd_35mmIsle-v4u.12` |
| W12 | Native original audio / I | `bd_35mmIsle-v4u.9` | W02, W04, W05, W07 | `bd_35mmIsle-v4u.3`, `bd_35mmIsle-v4u.5`, `bd_35mmIsle-v4u.6`, `bd_35mmIsle-v4u.8` |
| W13 | Complete original game progression / I | `bd_35mmIsle-v4u.14` | W10, W11, W12 | `bd_35mmIsle-v4u.12`, `bd_35mmIsle-v4u.13`, `bd_35mmIsle-v4u.9` |
| W14 | Desktop/backend fidelity qualification / I + evidence | `bd_35mmIsle-v4u.15` | W13 | `bd_35mmIsle-v4u.14` |
| W15 | Native packaging and release gate / I | `bd_35mmIsle-v4u.16` | W00, W14 | `bd_35mmIsle-v4u.1`, `bd_35mmIsle-v4u.15` |

The frozen parent-completion graph contains 16 work units and 34 edges and is acyclic. W12 remains prerequisite to full W08 qualification/closure; numeric unit IDs do not imply execution order. The explicitly amended [H9 implementation contract](ROADMAP.md#source-closed-startup-implementation-h9), tracked by `bd_35mmIsle-v4u.10.6`, permits source-closed real startup implementation before whole-corpus qualification without deleting an edge or closing either parent. Roadmap acceptance criteria remain future stop/go contracts, not completed tests.
