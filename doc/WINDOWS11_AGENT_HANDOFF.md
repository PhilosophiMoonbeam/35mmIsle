# Windows 11 x64 agent handoff

35mmIsle is the native Pokémon Snap (1999) port based on libultraship. Repository: <https://github.com/PhilosophiMoonbeam/35mmIsle>.

**Status: deferred; actual Windows 11 runtime acceptance remains BLOCKED.** Prepare this handoff now; execute it only after the Linux orchestrator confirms that Linux and other prerequisite work are complete and the owner explicitly activates Windows work. Runner setup is optional and must be discussed last. Possession of this document is not activation permission.

## 1. Scope and authority

You are a future agent operating on the owner's Windows machine. Complete only the activated stages; report facts and blockers, not inferred passes.

Before making changes, obtain one activation record containing:

- The approved real Windows 11 x64 machine and logged-in desktop user. Windows Server, Wine, a VM substitute and ARM64 x64 emulation do not satisfy this target.
- The **reviewed full application commit**, authorized operations, private workspace, resource ceilings and retention. A syntactically valid SHA is not approval; do not select floating `main` yourself.
- The lawful ROM's private local location, only when private-ROM work is separately activated. Do not search the owner's disks for ROMs or credentials.
- The human responsible for desktop/device observation and eventual hearing/fidelity checks.
- If automation is selected later: the separate **private validation repository**, independently reviewed workflow commit/ref, runner identity and one approved job. Do not create repositories, register runners or retrieve tokens before that approval.

Read the selected checkout's `AGENTS.md`, [BUILDING](../docs/BUILDING.md), [RUNNING](../docs/RUNNING.md), [ADMISSION](../docs/development/ADMISSION.md), [VALIDATION](../docs/planning/VALIDATION.md) and [LICENSING](../docs/planning/LICENSING.md). Use Beads issue `bd_35mmIsle-v4u.15.1` for Windows runtime acceptance. If its authorized database/tooling is unavailable, return evidence to the orchestrator; do not initialize a competing tracker, import JSONL or invent issue completion.

Do not alter Linux sessions, profiles, ownership receipts or deadlines. Do not disable security controls, TLS verification or validation checks. Installer elevation, commits, pushes and remote changes need their own activated authority. A build or host-readiness pass does not close the Windows runtime issue or publication gates R1/R2.

## 2. Host and data boundaries

Use a dedicated standard Windows user with an unlocked, observable desktop. Separate administrator-owned tool installation from non-elevated builds and runner execution. No Windows service/session-0 runner, automatic logon, stored desktop password or service-to-desktop workaround.

Record actual Windows edition/build, OS/process architecture, PowerShell version, session identity, selected compiler/SDK, renderer/device details and command outcomes privately. OS/session checks are necessary, not proof that a window is visible or input/audio works. A remote-desktop lock, disconnect or session switch can invalidate device/presentation evidence. Stop the affected stage rather than substituting headless success.

Keep ROMs, extracted assets, prepared GAME source/objects, private binaries, relink kits, saves, PNG/PCM, crash dumps and raw proof/log bundles **off GitHub**, including private Actions logs, artifacts, job summaries and caches. ROM-free builds can still contain GAME-derived material. MIT applies to original contributions, not every dependency or generated payload.

Before acquisition, register owned directories/processes, canonical paths and cleanup. Choose an owner-approved disk-backed workspace outside the source checkout; restrict its NTFS permissions to the intended user and authorized administrators. Reuse compatible pinned dependencies/build trees. Initial planning ceilings may be 32 GiB for source/build caches, 2 GiB for runner installation and 256 MiB for diagnostics; these are proposed budgets, not established platform minimums. Resolve insufficient space or a ceiling increase with the owner, not by truncating active evidence. Use build concurrency **2**, one validation job and bounded private evidence retention, normally no more than 30 days.

Never recursively clean the user's profile, source checkout, lawful ROM, existing saves, shared caches or unrelated processes. Preserve unique work and failed evidence. Inspect reparse points and ownership before cleanup.

## 3. Prepare the reviewed local checkout

Required tools: PowerShell 7; Git; Python 3.9+ available as `python`; Visual Studio 2022 with C++ x64 tools and a Windows SDK; CMake 3.24+, or **3.27+** for the private runtime relink kit. Install only approved missing components from their official distributions. The preparation script selects the actual VS2022 developer environment; do not invent SDK/library paths or substitute a different generator/toolset.

Choose the workspace and approved commit through the activation record. In the following PowerShell session, set `$Workspace` and `$SourceCommit` to those approved values, then run:

```powershell
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSVersion.Major -lt 7) { throw 'PowerShell 7 is required' }
if ($SourceCommit -cnotmatch '^[0-9a-f]{40}$') { throw 'Expected an approved full commit' }
$SourceRoot = Join-Path $Workspace 'source'

if (-not (Test-Path -LiteralPath $SourceRoot)) {
    git -c core.autocrlf=false clone --no-checkout https://github.com/PhilosophiMoonbeam/35mmIsle.git $SourceRoot
    if ($LASTEXITCODE -ne 0) { throw 'Source clone failed' }
}
$Origin = git -C $SourceRoot remote get-url origin
if ($LASTEXITCODE -ne 0 -or $Origin -ne 'https://github.com/PhilosophiMoonbeam/35mmIsle.git') {
    throw 'Existing checkout is not the approved source repository'
}
$Changes = @(git -C $SourceRoot status --porcelain)
if ($LASTEXITCODE -ne 0 -or $Changes.Count -ne 0) { throw 'Preserve existing work; checkout is not clean' }
git -C $SourceRoot fetch --no-tags origin $SourceCommit
if ($LASTEXITCODE -ne 0) { throw 'Approved source fetch failed' }
git -c core.autocrlf=false -c core.eol=lf -C $SourceRoot checkout --detach $SourceCommit
if ($LASTEXITCODE -ne 0) { throw 'Approved checkout failed' }
$ActualCommit = git -C $SourceRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $ActualCommit -ne $SourceCommit) { throw 'Source commit differs from approval' }
Set-Location -LiteralPath $SourceRoot
```

Stop on dirty/unowned/wrong-pin inputs; never use `reset --hard`, `clean`, force replacement or newline-normalized alternate hashes. Current commands at the selected commit are authoritative; reconcile incompatible documentation with the orchestrator before weakening a check.

## 4. ROM-free compiler readiness

Run preparation, configuration and build in the **same PowerShell 7 session**. Do not put preparation in a child `pwsh -File` process and assume its environment returns. These are the current [Windows host recipe](../.github/workflows/native.yml) and [dependency contract](../docs/BUILDING.md#windows-static-runtime-host-dependencies):

```powershell
./tools/prepare_windows_host.ps1 -Root (Join-Path $Workspace 'dependencies/vcpkg')
if (-not $?) { throw 'Frozen Windows dependency preparation failed' }
cmake --preset windows-host-release -DISLE_BUILD_RUNTIME_RELINK_KIT=ON
if ($LASTEXITCODE -ne 0) { throw 'Host configure failed' }
cmake --build --preset windows-host-release --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Host build failed' }
ctest --preset windows-host-release
if ($LASTEXITCODE -ne 0) { throw 'Host CPU tests failed' }

cmake --build --preset windows-host-release --target `
    35mmisle-application-runtime-rom-smoke post-course-interfaces-rom-smoke post-course-component-rom-smoke --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Application/interface consumer build failed' }
foreach ($Name in @('rom', 'host-probe', 'audio-probe', 'rsp-probe', 'rdp-probe')) {
    & "./build/windows-host-release/Release/35mmisle-$Name.exe" --help
    if ($LASTEXITCODE -ne 0) { throw "$Name help failed" }
}
& ./build/windows-host-release/Release/35mmisle.exe --help
if ($LASTEXITCODE -ne 0) { throw 'Product help failed' }
```

Also execute the complete explicit consumer-target build in the selected workflow's `windows-host` job; currently it contains 31 additional targets. They are opt-in, not implied by the default build. Names containing `rom-smoke` do not mean a ROM was executed. Record actual test counts, selected versions and exit codes; do not copy historical counts.

Preserve the frozen vcpkg source/registry/tool, executable size/SHA-256 checks, static CRT, canonical LF inputs, pinned LUS export and notice/patch checks. No floating bootstrap, overlays, automatic upstream vcpkg, binary-cache substitution, scripting/TCC, MPQ or DLL-loading shortcut. Existing wrong-pin dependencies must reject, not be silently replaced.

The relink-kit option requires CMake 3.27+. If its production is outside the activation scope, omit that option. If approved, build `runtime-relink-kit` and follow its generated `RELINK-INSTRUCTIONS.txt`; do not improvise a replacement-library proof. Runtime archives, prepared sources and kits remain private. Existing hosted Server 2022 CI is compiler/package evidence, not Windows 11 execution.

## 5. Separate desktop and private-ROM qualification

After compiler readiness, an approved ROM-free `35mmisle-host-probe.exe --frames 240` can exercise procedural LUS rendering. Observe its actual owned window and later colored pixels; a submission marker, gray startup frame or exit zero alone is not pixel proof. Record normal process/owner retirement. Do not copy Linux X11/EGL environment settings to Windows.

Actual application execution additionally requires compatible **Vulkan and SDL3/OpenGL**; the canonical renderer requires Vulkan 1.1 and enabled 8-/16-bit storage-buffer access. Record the backend actually selected. `--cpu-renderer` needs a real Vulkan CPU device, not a null renderer; `--disable-input` does not disable graphics/audio and cannot satisfy input qualification.

**Stop before private-ROM launch unless the orchestrator supplies a Windows-specific ownership, deadline, capture and acceptance plan.** Do not transplant Linux `timeout`, Xvfb, `/proc`, memfd or X11 retirement recipes. An Actions job timeout is not that plan. An agent/runner/window disappearance is not an actual child wait or whole-tree closure.

Once separately approved, the documented product syntax is:

```powershell
& ./build/windows-host-release/Release/35mmisle-rom.exe check $PrivateRom
if ($LASTEXITCODE -ne 0) { throw 'Supported ROM admission failed' }
& ./build/windows-host-release/Release/35mmisle.exe `
    $PrivateRom --data-directory $FreshPrivateDataDirectory
if ($LASTEXITCODE -ne 0) { throw 'Native application failed' }
```

`$PrivateRom` and a genuinely new `$FreshPrivateDataDirectory` come from the approved private plan, not guesses or disk searches. The application takes a **positional ROM**, unlike probes using `--rom`. Use the project checker, not a filename/header-only test or a byte-order-insensitive assumption. Read the ROM without modifying it. Do not reuse partial/failed Linux profiles or synthesize progression.

Use ordinary visible input: WASD stick, X confirm/shutter with Z held, C cancel, Z viewfinder, Space Start. Observe only approved public surfaces for gameplay control; do not read private progress/RNG/counters or manipulate SDK state to reach a scene. Actual startup is not the full course/photo/Oak/Album/save/quit/cold-reload route. Compiler, rendering, audio stream completion, physical hearing and calibrated fidelity are distinct claims. Missing devices, failed ownership, fallback, timeout or nonzero exit must remain a failure/blocker, not be suppressed. Current validation and licensing gates still apply.

## 6. Optional runner automation — discuss and activate last

Local Windows work does not require Actions. If the owner later chooses automation, use the following boundary, after the preceding activation decisions. Never change public `native.yml` from hosted runners to self-hosted: it has public `push` and `pull_request` triggers.

### Trust and credentials

- Use a separate owner-approved **private validation repository**, preferably repository-scoped registration. No public runner pool or broadly shared organization group. Labels route jobs; they do not isolate data.
- Review and record **both** the private workflow commit and the public application commit. Manual dispatch can select another ref: verify the resolved queued workflow revision before connecting the runner. Restrict repository writers/admins and accept no free-form command/repository inputs, PR workflows, `pull_request_target`, `workflow_run`, public reusable workflows or external build artifacts.
- The owner supplies a one-hour registration token through a separate trusted administrative context. No owner browser/CLI login, PAT, SSH agent, cloud credentials or unrelated shares in the runner account. Never place registration/removal tokens in chat, workflow inputs, files, transcripts, environment dumps or reports. Do not use `config.cmd --check --pat` as a default diagnostic.
- Jobs transmit stdout/stderr to GitHub even without artifact actions. Initial readiness is checkout-free, build-free and ROM-free. No game output, private paths, caches, artifacts or raw diagnostic uploads. Do not stage the ROM until the runner has stopped and deregistered. Ephemeral registration is not filesystem sanitization or a sandbox.

### Download and one-job registration

On the private repository's **Settings → Actions → Runners → New self-hosted runner**, select **Windows / x64**. Use that repository's currently offered version and matching official `actions/runner` release checksum; progressive rollout means “latest release” need not be the selected version.

Before extraction or execution, compare `Get-FileHash -Algorithm SHA256` of the downloaded ZIP with its approved published SHA-256. Record version, URL and digest privately. Never invent a digest, use a permanently floating download URL, ignore a mismatch or execute an unchecked archive. Reuse a compatible verified installation only if it is unregistered and demonstrably inactive; otherwise preserve it and use an explicitly owned namespace.

In its owned installation directory, verify the selected package's `config.cmd --help` supports the flags below. Supply the approved private repository and unique non-personal runner name as `$ValidationRepository` and `$RunnerName`. Enter the short-lived token directly through the private console/secret-input channel, not the LLM transcript:

```powershell
Set-Location -LiteralPath $RunnerRoot
.\config.cmd --url "https://github.com/$ValidationRepository" `
    --name $RunnerName --labels "35mmisle-win11-desktop" --work "_work" --ephemeral
if ($LASTEXITCODE -ne 0) { throw 'Private one-job registration failed' }
.\run.cmd
if ($LASTEXITCODE -ne 0) { throw 'Foreground runner exited unsuccessfully' }
```

Decline Windows-service installation. Run unelevated in the intended desktop terminal; do not configure autologon or unattended re-registration. `run.cmd` remains foreground and its real process ownership must survive the agent's command wrapper. A normal listener exit, job result and remote deregistration are separate facts. No completed job means ephemeral registration can remain idle indefinitely: the owner must bound that wait and remove an unused registration safely.

### Initial readiness workflow

Install this **only in the approved private validation repository**, then review its resolved commit before dispatch/listener connection. It has one job, no source checkout or arbitrary inputs, no external actions and no data transfer steps. Its five-minute timeout applies only to these host checks, **never gameplay**. `cache-mode: none` is an explicit current GitHub control; if unavailable at activation, stop and resolve compatibility rather than silently removing it.

```yaml
name: Windows 11 desktop runner readiness
on:
  workflow_dispatch:
permissions: {}
cache-mode: none
concurrency:
  group: win11-desktop-readiness
  cancel-in-progress: false
jobs:
  readiness:
    if: github.event.repository.private == true
    runs-on: [self-hosted, Windows, X64, 35mmisle-win11-desktop]
    timeout-minutes: 5
    steps:
      - name: Check limited host prerequisites without private content
        shell: pwsh
        run: |
          $ErrorActionPreference = 'Stop'
          if (-not $IsWindows -or $PSVersionTable.PSVersion.Major -lt 7) {
            throw 'Native Windows and PowerShell 7 are required'
          }
          $os = Get-CimInstance -ClassName Win32_OperatingSystem
          if ($os.ProductType -ne 1 -or [int]$os.BuildNumber -lt 22000 -or $os.Caption -notmatch 'Windows\s*11') {
            throw 'This job requires Windows 11 client Windows'
          }
          if ([Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString() -ne 'X64' -or -not [Environment]::Is64BitProcess) {
            throw 'Native x64 OS and process are required'
          }
          if (-not [Environment]::UserInteractive -or (Get-Process -Id $PID).SessionId -eq 0) {
            throw 'An interactive desktop session is required'
          }
          $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
          try {
            $principal = [Security.Principal.WindowsPrincipal]::new($identity)
            if ($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
              throw 'Do not run this runner elevated'
            }
          } finally {
            $identity.Dispose()
          }
          Write-Output 'WINDOWS11_RUNNER_PREFLIGHT_OK; desktop visibility and application acceptance not established'
```

The owner verifies exactly one successful job, actual foreground listener exit and remote removal of the registration. A skipped/cancelled job, an Idle status or “Listening for Jobs” is not a pass. After this job, stop: another job needs fresh authorization and registration. Any later compiler workflow needs its own reviewed source pin, bounded scope and log/privacy review; do not paste the public workflow into this repository unreviewed.

## 7. Cleanup and return contract

On success, failure, cancellation or interruption, retain actual process/runner ownership until retirement is proved. Stop only owned instances, not `taskkill /IM` or wildcard services. Cancel/remove only the authorized run/registration; do not disturb another runner. Quarantine uncertain or unexpectedly modified hosts for owner-controlled recovery, not automatic reuse.

Preserve necessary diagnostics in approved restricted storage, with purpose/size/expiry; review secrets before any transfer. Remove session-owned credentials, work directories and disposable downloads only after actual inactivity and canonical-path/ownership checks. Keep compatible required caches and unique work. Report retained private paths to the owner privately. Do not treat deregistration as credential erasure or wipe the user's ROM/saves to make cleanup appear successful.

Return an allowlisted summary to the Linux orchestrator: approved source/workflow revisions; Windows edition/build and OS/process architecture; selected tool/compiler/SDK/renderer versions; exact commands and exit codes; actual CTest counts; candidate SHA-256; exercised stage; actual wait/normal-exit and runner-deregistration facts; bounded failures; private evidence identifiers; retained-resource purpose/expiry; untested limits. Exclude tokens, usernames, machine names, absolute private paths, serials and raw logs/payloads. Report physical desktop/audio/hearing/fidelity only when actually observed and authorized. Host readiness alone leaves `bd_35mmIsle-v4u.15.1` blocked.

## Authoring-validation limit

Local `actionlint` v1.7.12 rejects GitHub's documented `cache-mode` key; declaring the custom runner label leaves that one parser-compatibility error. This is **not a clean actionlint pass**. Keep `cache-mode: none`, use the official syntax reference, and validate against the selected GitHub service/runner at activation rather than deleting a security control to silence a validator. Windows PowerShell execution is not available on the authoring Linux host.

## References

- Project [build contract](../docs/BUILDING.md), [runtime commands](../docs/RUNNING.md), [hosted-only workflow](../.github/workflows/native.yml), [runtime composition/privacy](../docs/development/admission/RUNTIME_PACKAGING.md), [validation](../docs/planning/VALIDATION.md) and [licensing](../docs/planning/LICENSING.md).
- GitHub [self-hosted runner security](https://docs.github.com/en/actions/reference/security/secure-use#hardening-for-self-hosted-runners) and [adding runners](https://docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/add-runners).
- GitHub [supported hosts, ephemeral lifecycle and networking](https://docs.github.com/en/actions/reference/runners/self-hosted-runners), [Windows service distinction](https://docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/configure-the-application) and [monitoring/removal](https://docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/monitor-and-troubleshoot).
- GitHub [manual workflow execution](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow), [cache-mode controls](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching#controlling-cache-access-with-cache-mode) and [official runner releases/checksums](https://github.com/actions/runner/releases).

This handoff specifies future Windows operations. Authoring it on Linux does not mean its Windows commands were executed, a runner was registered, a Windows host was provisioned or any Windows runtime gate passed.
