# Prospective native file-image custody

Prospective Linux file-image custody and campaign preparation retain their bounded evidence and open gates.

[Admission index](../ADMISSION.md) · [Project README](../../../README.md)

Contents:

- [Prospective Linux native file-image custody](#prospective-linux-native-file-image-custody)
- [Live public-output and controlled-source campaign preparation](#live-public-output-and-controlled-source-campaign-preparation)

## Prospective Linux native file-image custody

`tools/runtime_custody.py` is private Linux x64 qualification tooling, not a
Runtime package component or a game-state selector. A prospective manifest
identifies the actual application, GNU loader, dependency-free audit module and
each transitive/late provider by whole-file SHA-256, size and exact request
aliases. Its other fields are `schema_version: 1`, absolute `cwd` and explicit
`environment`; image fields are `role`, `path`, `expected_sha256`, `size` and
`aliases`. The application argument must name the declared original path, and
its exact `PT_INTERP` spelling must identify the declared loader.

```sh
python3 -B tools/runtime_custody.py launch \
  --manifest /private/cohort/manifest.json \
  --record /private/cohort/receipt.json --timeout 600 -- \
  /private/native/35mmisle /private/pokemonsnap.z64 \
  --data-directory /private/cohort/data \
  --renderer-resources /private/native/host-renderer-resources
```

Whole images are copied into executable memfds, sealed against writes/growth/
shrinkage/further seal changes, then hashed through retained descriptors.
Exact SONAME and approved absolute requests route to supervisor-owned FD paths;
there is no basename/default-search fallback or symbol interposition. The
freestanding audit helper has no `DT_NEEDED` or undefined dynamic symbols.
Sequenced synchronous admissions reconcile actual dev/inode, load bias,
`PT_LOAD` offsets/protections and independent loader/application/auditor/config
bootstrap witnesses before allowing loader callbacks to return. Absolute
namespace-root `dlmopen` does not bypass mapped-inode admission.

After the blocked activation barrier, structured kernel syscall/event metadata
gates file-backed executable mapping and execute-permission transitions.
Sibling threads are quiescent across FD identity inspection and actual syscall
completion. Native threads may outlive their exited leader. Implicit
`READ_IMPLIES_EXEC`, x32 entries, classic untraced creation and unadmitted
process/exec transitions reject; ordinary native clone3 is not replaced or
artificially made unavailable. No process RAM, private register blobs,
argument-pointer contents or Source state are read, and no syscall/register
result is modified.

The dedicated single-threaded supervisor is a real subreaper and sole child-wait
consumer. Public Python `launch()` isolates it from the caller's children.
Pre-activation failures, escaped sessions and adopted descendants retire before
FD release; complete retirement requires `ECHILD`, empty ledgers and an
accounted root status. Deadline expiry records incomplete retirement while
retaining custody until actual cessation. This is not a malicious-code sandbox
or a guarantee that every kernel-blocked task can terminate within a deadline.

Competing planner/reviewer assessments identified implicit executable
permissions, x32 dispatch, leader-first retirement and escaped-child ownership
gaps before correction. Actual regressions additionally exposed ignored signal
injection at non-delivery ptrace event stops; teardown now queues real SIGKILL
before resuming those stops. The final native fixture run contains 25 scenarios:
24 pass and one explicitly skips unavailable host clone3 creation. Genuine
concurrent pthread mapping/protection, leader-first worker mapping plus late
loading, transient original-inode execution refusal, retained-image execution,
pre-activation escaped-session/double-fork retirement and an unrelated caller
child's intact signal/wait ownership are exercised.

A new prospective actual-application cohort freezes **86 images /
428,366,192 bytes**, including late Mesa/Vulkan providers. Executable
`ba513217d08710e0eac66b33c37f03e994029d03904dcbbe07cb9ea32eb1be59`
renders the Original intro through the actual SDL3/OpenGL and Vulkan
`llvmpipe (LLVM 21.1.8, 256 bits)` devices. The recorded environment selects
X11/EGL, software rendering, the system lvp ICD and dummy Audio; renderer support
resources use the ordinary explicit CLI path. All 85 created native threads and
the root retire normally after an owned-window WM_DELETE, with exit 0,
1,281,384 syscall stops, no rejected task, no supervisor SIGKILL, `ECHILD` and
retained-FD closure only after retirement. All image seals/hashes and six frozen
launcher inputs match. The private final receipt SHA-256 is
`236dacf52fff36bf299359a372c5ecf661099d9fe0574cc11362d202b7f1beba`.
The full product/all 31 existing consumer targets plus application/interface
consumers build; all 61 configured local CPU CTests pass.

That cohort supplies a genuine separately owned private session bus. Public
D-Bus connection/PID lookup identifies the actual application; the daemon
separately exits normally and reaches its own `ECHILD` boundary. It is an
external desktop IPC prerequisite, not a sealed application image or implied
helper closure. The earlier minimal-environment run remains rejected on an
actual post-PREINIT fork; its origin is not inferred from that observation.
The documented [D-Bus autolaunch behavior](https://dbus.freedesktop.org/doc/dbus-launch.1.html)
supports an environment hypothesis, not retrospective caller attribution.

Nonexecuting mapped data and anonymous executable/JIT mappings are explicitly
recorded **without immutable custody**. Actual locale data and anonymous
renderer/RSP work require these already-declared exclusions; no inode-name
exception, ignored unknown file-backed execution or fabricated capability is
added. Kernel vDSO remains independently witnessed. Shader/ICD/data bytes,
private COW/JIT contents, external services, exhaustive compiler closure,
hardware/hearing and Windows 11 execution are not qualified by this result.
Historical campaign/path-custody failures remain unchanged. This is bounded
prospective Linux startup/provider admission, not a replacement full campaign,
whole-port completion or R1/R2 publication approval.


At `18ae6aa726fbca28f171244b44993cb202c76740`,
[run 40032573319](https://github.com/PhilosophiMoonbeam/35mmIsle/actions/runs/40032573319)
passes all ten Linux/MSVC jobs, including both full native products, all
existing consumers, configured CPU contracts, real CPU-Vulkan rendering,
private static-library replacement and runner-local Runtime package checks.
This exact CI result predates the live-output correction below. Hosted Windows
remains build/tooling evidence, not Windows 11 ROM-backed execution; no ROM or
derived artifact is uploaded.

### Live public-output and controlled-source campaign preparation

Readiness observation requires output before producer exit. The custody
launcher's stdout/stderr files now use unbuffered, complete writes through the
existing checked write loop. A real two-stage native consumer exposes the
published buffered behavior: its first output becomes visible only together
with its later output. The corrected launcher exposes the first stage while
the producer is still waiting, then retains exact final bytes, size, hash,
normal exit and `ECHILD` retirement. The permanent native regression uses a
FIFO-controlled continuation, not a fixed sleep or mocked output callback.

A separate frozen current consumer
`db6fcb74e66024e11637eb8c3fea8191b1c502d251ad82998752dbb18a1aea3e`
passes genuine empty-root ingress under sealed image custody: actual Title
pixels, 5,287,546 syscall stops, 33,442 metadata events, normal exit 0 and complete
owned-tree/FD retirement. This is a bounded SDK-consumer ingress, not a fresh
campaign or the shipping application's keyboard/physical-input qualification.

For the next prospective campaign, a fresh detached project checkout binds
all **931 tracked project inputs** to exact Git blobs at
`18ae6aa726fbca28f171244b44993cb202c76740` before configuration. All remain
SHA-256-exact after 628 actual native build steps. The immutable tracked-project
archive is
`f9c79c781714d1f6494411689fdc1e25991a26ae05efecd334cfe9e6292b52bd`;
selected actual cache, command, Ninja, host-pin and composition-input snapshots
are retained. Controlled consumer
`8fd72bbd7c4d0ec903925a230b0c0ed251db833a9f5a01208a3d1894b02fb112`
is frozen separately, never substituted into the earlier ingress receipt.
These are controlled project-source/build facts, not retrospective per-header
build hashes or exhaustive generated/third-party/compiler closure.

The historical completed campaign's runtime-path deviation remains unchanged.
Neither the controlled build nor bounded ingress closes the separate new
empty-root all-seven-course/credits/cold campaign gate.

