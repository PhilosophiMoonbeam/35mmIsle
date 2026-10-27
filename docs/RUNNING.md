# Running the native application

Install the prerequisite host dependencies described in [Building the native runtime and tools](BUILDING.md) before building and launching the application. This page covers launch options, controls and Snap Station use. Run all commands from the repository root.

[Project README](../README.md) · [Admission index](development/ADMISSION.md)

- [Native application](#native-application)
- [Native Snap Station](#native-snap-station)

## Application usage

### Native application

With the host dependencies above, build the actual application and all required native services:

```sh
cmake -S . -B build/native-game -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DISLE_BUILD_HOST_PROBE=ON -DISLE_BUILD_AUDIO_PROBE=ON \
  -DISLE_BUILD_GAME_ENGINE=ON -DISLE_BUILD_RSP_PROBE=ON -DISLE_BUILD_RDP_PROBE=ON \
  -DCMAKE_PREFIX_PATH="$PREFIX"
cmake --build build/native-game --target 35mmisle --parallel 2
build/native-game/35mmisle /private/pokemonsnap.z64
```

The Windows full-host preset builds `build/windows-host-release/Release/35mmisle.exe`. The application requires compatible Vulkan and SDL3/OpenGL support; it reads the exact supported ROM without modifying or bundling it. `--data-directory` selects profile/configuration storage. `--cpu-renderer` requires an actual Vulkan CPU device; it is not a null backend. `--disable-input` disables controller acquisition only, not Audio or graphics.

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

Current candidate-specific execution evidence is in [Runtime status](STATUS.md#runtime-status) and [ADMISSION](development/ADMISSION.md); earlier campaign, numerical, photo and menu proofs do not transfer automatically to a changed executable. Historical [preview 7](https://github.com/PhilosophiMoonbeam/35mmIsle/releases/tag/v0.1.0-preview.7) remains immutable. CI builds and checks new Runtime archives without uploading them while redistribution/compliance gates remain open. Windows ROM gameplay and Windows 11 hardware remain unqualified. [LICENSING](planning/LICENSING.md#import-and-release-review-gates) governs embedded terms, linkage, source/relink delivery and publication; successful compilation or an asset-free archive is not a redistribution grant.

### Native Snap Station

Add `--snap-station-directory PATH` to the application command, selecting an existing writable output directory. This configures an output-backed virtual accessory; it does not connect automatically. Press `F9` to connect or disconnect. Connect before opening Gallery, select the four photographs through ordinary GAME controls, then choose **Print**.

Original Save and printer-reset commands retire the old application and host, then recreate them through normal Boot using the same saved profile. The native station captures sixteen completed 640×480 GAME-rendered passes into an exclusive job directory containing `sheet.bmp`: a lossless 2560×1920, four-by-four sheet. It does not synthesize substitute photographs or require a physical Snap Station.

The Diagnostic terminal window is blank after printing; output remains in its job directory. `Space`/Start publishes another identical `sheet-copy-N.bmp`; close the window normally to leave. Output errors fail explicitly rather than acknowledging a completed print. [Actual Linux execution and limits](development/admission/SNAP_STATION.md#native-virtual-snap-station-and-original-diagnostic) remain separate from Windows runtime and physical-printer qualification.

