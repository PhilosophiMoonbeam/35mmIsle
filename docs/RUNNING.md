# Running the native application

Prebuilt preview Runtime archives do not require development SDKs or build tools. Host graphics, audio and input support remain required. Developers should first prepare the dependencies in [Building the native runtime and tools](BUILDING.md). Run build commands from the repository root; run installed commands from the extracted directory containing `bin/`.

[Project README](../README.md) · [Admission index](development/ADMISSION.md)

- [Prebuilt Runtime](#prebuilt-runtime)
- [Native application](#native-application)
- [Opt-in diagnostic labels](#opt-in-diagnostic-labels)
- [Terminal evidence](#terminal-evidence)
- [Native Snap Station](#native-snap-station)

## Application usage

### Prebuilt Runtime

Download the Windows x64 Runtime ZIP or Linux x64 Runtime TAR.GZ from
[GitHub Releases](https://github.com/PhilosophiMoonbeam/35mmIsle/releases).
GitHub's automatic source-code archives are not executables. Extract the
**whole Runtime package**, including `bin/`, libraries, shaders and notices;
do not copy only the executable.

The application reads your own supported **US Pokémon Snap** ROM. It detects
z64/v64/n64 byte order from content and never modifies or bundles the ROM.
Open a terminal in the extracted directory containing `bin/`.

Windows PowerShell:

```powershell
$Data = Join-Path $env:LOCALAPPDATA "35mmIsle\preview8"
New-Item -ItemType Directory -Force -Path $Data | Out-Null
.\bin\35mmisle.exe "C:\path\pokemonsnap.z64" --data-directory $Data --diagnostics
```

Linux:

```sh
DATA="$HOME/.local/share/35mmIsle/preview8"
mkdir -p "$DATA"
./bin/35mmisle "/path/to/pokemonsnap.z64" --data-directory "$DATA" --diagnostics
```

Replace the ROM path. These commands isolate preview data from older profiles;
no configuration or JSON editing is needed. `--diagnostics` is optional and
does not automate play. See [controls](#native-application) and
[diagnostic reporting](#opt-in-diagnostic-labels) below.

The LUS frontend needs working OpenGL support; canonical game rendering needs
a Vulkan loader and compatible device/driver. Linux also requires compatible
glibc/libstdc++ and display/audio/device backend libraries; it is not a
universally static package. Native hosted Windows build/package checks do not
establish Windows 11 gameplay or physical-device qualification.

### Native application

With the host dependencies above and `LLVM_ROOT` pointing to the sealed LLVM root prepared in BUILDING, build the actual application and all required native services:

```sh
cmake -S . -B build/native-game -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DISLE_BUILD_HOST_PROBE=ON -DISLE_BUILD_AUDIO_PROBE=ON \
  -DISLE_BUILD_GAME_ENGINE=ON -DISLE_BUILD_RSP_PROBE=ON -DISLE_BUILD_RDP_PROBE=ON \
  -DCMAKE_PREFIX_PATH="$PREFIX" -DISLE_LLVM_ROOT="$LLVM_ROOT"
cmake --build build/native-game --target 35mmisle --parallel 2
build/native-game/35mmisle /private/pokemonsnap.z64
```

The Windows full-host preset builds `build/windows-host-release/Release/35mmisle.exe`. The application requires compatible Vulkan and SDL3/OpenGL support; it reads the exact supported ROM without modifying or bundling it. `--data-directory` selects profile/configuration storage. `--cpu-renderer` requires an actual Vulkan CPU device; it is not a null backend. `--disable-input` disables controller acquisition only, not Audio or graphics.

Canonical Save slots live in `<data-directory>/profile`; pass the data root,
not that `profile` subdirectory. Diagnostic text reports live in the data root.

`--renderer-resources PATH` selects the existing pinned LUS shader-resource directory explicitly; for the build above, use `build/native-game/host-renderer-resources`. An explicit path must contain `shaders/opengl/default.shader.glsl`; a missing path fails rather than falling back. Omit the option to retain installed-resource discovery and the build-tree default. This selects renderer support data, not ROM assets or an alternative graphics backend.

The product's persistent `game_owner::Owner` constructs and retires State, Runtime, Manager, native continuations, Input, GTL and canonical graphics on a dedicated GAME thread. Main continues actual events and platform requests while Source is inside native waits. Completed pixels remain borrowed until main copies or explicitly declines them; only GAME then releases the real FrameToken.

Native reset preserves the same GAME thread, Input Service and Snap Station. The old Runtime, profile lock, queued acquisitions and native borrowers retire before ordinary boot reopens the profile under the next application epoch. F9/reset decisions are consumed outside returning Source/native callbacks; main never selects a Source scene or advances OM.

Default keyboard controls:

|Keyboard|N64 control|
|---|---|
|`W` / `A` / `S` / `D`|Analog stick: up / left / down / right|
|`X`|A: confirm; shutter while Z is held|
|`C`|B: cancel|
|`Z`|Hold Z: camera viewfinder|
|`Space`|Start: Title menu; course pause|

These bindings are exercised through ordinary SDL keyboard input in the downloaded Linux product. Gamepad/device qualification remains separate.

Current candidate-specific execution evidence is in [Runtime status](STATUS.md#runtime-status) and [ADMISSION](development/ADMISSION.md); earlier campaign, numerical, photo and menu proofs do not transfer automatically to a changed executable. Historical [preview 7](https://github.com/PhilosophiMoonbeam/35mmIsle/releases/tag/v0.1.0-preview.7) remains immutable. Preview8 replaces the production RSP JIT with source-backed LLVM 21.1.8 and the selected MIT parallel-rsp helpers. Ordinary push/PR CI never publishes; an explicit manual dispatch may retain checked Windows ZIP/Linux TGZ candidates for maintainer download, exact-hash inspection and smoke before separate final publication. See [BUILDING](BUILDING.md#preview8-candidate-retention). The maintainer's clean-room Snap distribution basis is recorded in [LICENSING](planning/LICENSING.md#preview8-maintainer-disposition); actual third-party notices still apply. No ROM, extracted assets, private profiles/captures or SDK is bundled. Hosted MSVC `/MT` packaging is not Windows 11 gameplay, hardware, full-campaign or fidelity qualification.

### Opt-in diagnostic labels

Add `--diagnostics` to an ordinary application launch; no configuration or JSON
editing is required:

```sh
build/native-game/35mmisle /private/pokemonsnap.z64 --diagnostics
```

The existing LUS/ImGui presentation draws a noninteractive high-contrast label.
It shows the application version and configured source revision, a stable run ID,
actual application epoch and scene/course entry (including its original raw
argument), GTL update/retrace/submission counts and clock frame, and the latest
offered native image's epoch/entry/frame ordinal. Scene ordinals and image
ordinals are different identities; neither is an invented GUI frame count.
Save status is the actual Boot open result and **boot-loaded** durable generation,
not current RAM, unsaved progress or an exhaustive Save-acknowledgement ledger.

There is no diagnostic gameplay key or interactive window to capture input.
Use the visible run ID together with epoch/entry/frame or GTL update as the report
marker: take a screenshot or note it when something happens, describe what you
observed and what you expected, and close the application normally.
The label updates only through existing native presentation; a stuck image can
therefore retain its last label. It does not independently animate or advance
the game. A real observed failure is marked `FAILED` in red, with its first
error retained even after scene and Host retirement; `none observed` is not a
correctness or sound-fidelity certification.

After normal cleanup/join, the selected application data directory contains
`last-diagnostic.txt`, a bounded private text report (under 4 KiB). Its retained
label text is also printed to standard error. The file is atomically replaced on each diagnostic run;
the run ID inside distinguishes it from an older run. On POSIX the exclusive
report is created with owner-only permissions (`0600`), independently of the
process umask, including replacement of an older report. One exclusively created
`last-diagnostic.txt.tmp` may remain if the process is killed while writing.
A pre-existing temporary file or output failure is refused and reported,
without overwriting that temporary file or the previous report; the process
returns nonzero. Non-diagnostic launches neither show the label nor write this
report. `--data-directory` selects the same storage as ordinary play.

The report includes no ROM/profile payload, inventory, credentials, telemetry
or public upload. Review error text for private filesystem paths before pasting
the relevant text into chat. **Audio fidelity remains unqualified:** playback
and these labels cannot prove or disprove a numerical/sample-rate defect,
including a reported `.35` issue. There is no audible-issue detector. Keep
observations about sound separate from the real error/status context.

### Terminal evidence

`--terminal-report campaign DIRECTORY` or `--terminal-report cold-boot DIRECTORY`
enables passive observation on the actual shipping GAME-owner path. Normal play
leaves it disabled. Select an existing writable directory with none of
`report.json`, `terminal-save.bin` or `boot-load.bin` present; existing files,
FIFO destinations and a symlink output directory are refused.

After worker join and actual native/Host shutdown, the application writes
`report.json` and any available whole immutable canonical Save or completed-Boot
load image. These files may contain private profile data; keep them private.
The purpose selects a terminal evidence predicate, not scenes, inputs, progress
or automatic stopping. Closing before that predicate is complete returns 1
even if ordinary game shutdown succeeds; output failure also returns 1.

Native completion is not whole-run acceptance. Campaign evidence still needs
independent public pixels, including first-Mew acquisition timing. Cold-boot
evidence still needs a genuinely separate executable launch and exact equality
with the accepted full-campaign profile. OS process-tree, wait, pipe, sealed-image
and external-owner closure are separate. See [the ownership contract](development/admission/GAME_OWNER.md#passive-shipping-terminal-evidence).

### Native Snap Station

Add `--snap-station-directory PATH` to the application command, selecting an existing writable output directory. This configures an output-backed virtual accessory; it does not connect automatically. Press `F9` to connect or disconnect. Connect before opening Gallery, select the four photographs through ordinary GAME controls, then choose **Print**.

Original Save and printer-reset commands retire the old application and host, then recreate them through normal Boot using the same saved profile. The native station captures sixteen completed 640×480 GAME-rendered passes into an exclusive job directory containing `sheet.bmp`: a lossless 2560×1920, four-by-four sheet. It does not synthesize substitute photographs or require a physical Snap Station.

The Diagnostic terminal window is blank after printing; output remains in its job directory. `Space`/Start publishes another identical `sheet-copy-N.bmp`; close the window normally to leave. Output errors fail explicitly rather than acknowledging a completed print. [Actual Linux execution and limits](development/admission/SNAP_STATION.md#native-virtual-snap-station-and-original-diagnostic) remain separate from Windows runtime and physical-printer qualification.

