> Historical implementation notes. For the current candidate, build and validation
> status see the root README and docs/HARDWARE-TEST.md. No historical statement
> below certifies the current reconstructed package on hardware.

# Minish Cap Vita 0.06

Native Vita port based on EstebanPdN/zelda-tmc-3ds, commit
`57fe1a998761d5c1021904cee3cca1067989ba5a`, descended from Project Picori and
zeldaret/tmc. The game's C code runs on the Vita CPU. Inherited `Platform3DS`
names refer to the platform interface.

## Installation and controls

Use the current [installation guide](../../docs/INSTALL.md) and back up your
existing data before installing a locally built `minishcap-vita-0.06.vpk` with
VitaShell over a previous version. The European ROM remains at
`ux0:data/minishcap/baserom_eu.gba`
(SHA-1 `cff199b36ff173fb6faf152653d1bccf87c26fb7`, 16 MiB). The VPK contains no
ROM, but contains locally generated game resources and must remain private.
French can be selected in the game.

Cross = GBA A, Circle = GBA B, D-pad/left stick = movement, and L/R, Start and
Select map to the corresponding GBA buttons. **Triangle opens the port settings**,
pausing game logic and audio mixing. Up/down selects; Cross opens a category
or confirms; left/right changes values. Circle goes back, Triangle/Start closes,
and Square opens help. Inputs remain blocked until buttons are released and
sticks return to their dead zones.

Supported options include FPS, left stick, volume 0–100%, image size
3x / Max / 2x, sharp/smooth filtering and GBA color correction. Max preserves
aspect ratio (816 × 544); 3x is centered at 720 × 480; 2x is 480 × 320. The menu
stays at 3x for readability. Settings apply on closing and persist in
`ux0:data/minishcap/vita-menu.ini` with a `.bak` copy. On write failure, the menu
stays open; Start resumes with in-memory settings without claiming they were saved.

On first use, the profile imports existing volume/color/FPS values from
`minishcap.ini`; `vita-menu.ini` then takes precedence for those options.
Other inherited settings remain in `minishcap.ini`. The game save is
`tmc_eu.sav` (8 KiB EEPROM). Ordinary menu settings do not modify it; explicit
restore actions described below do. Logs are `vita.log`, `stdout.log` and
`diagnostic.txt` in the same directory.

## Shared architecture

`port-menu/` began as a standalone C11 module at version 0.1.0/API 1. The current
bundle is v0.3.0/API 2: options by identifier, paging, actions and confirmations,
callbacks, theming and persistence. `vita_menu.c` declares Minish Cap options
and applies them. `vita_platform.c` handles the modal loop, display and Vita/SDL
input. `vita_audio.c` synchronizes pause with the worker, outputs silence without
advancing the mixer, then resumes without resetting it.

See `port-menu/README.md` to integrate another game. Game-specific features such
as turbo and save restoration require an adapter and are not offered without
an implementation.

## Version 0.06 changes

Backups are sorted by generation, newest first, with full dates including seconds.
Actions occupy a full row; help and confirmation identify the selected backup.
The pre-restore action restores the `.before-restore` copy compatible with
0.04/0.05 after capturing the current save in the rotation. It exits the game
on success. All three save slots are affected and unsaved progress is lost.
The pre-restore file remains intact; its displayed date records when it was
set aside.

Shared module 0.3 provides the model and backup ordering also used by Pokémon
0.46. Pokémon's adapter retains its native Vita rendering, localization and
storage format. Historically, identical shared sources were checked with
`sync_port_menu.py`; the public bundles now use `tools/check_port_menu.py` from
the repository root. ASan/UBSan tests and host-menu restoration passed.
The new VPK still requires Vita testing. The user previously confirmed LiveArea,
Settings and audio after sleep worked in 0.05; that does not validate this build.

## Version 0.05 changes: LiveArea

The shell includes an icon, forest background, thumbnail with the original logo
and a Settings button. Assets use indexed 8-bit PNGs and dimensions are checked
before packaging. Historical development used `assets-source` and
`build_livearea.py` with Pillow and a macOS font. The current public reconstruction
instead generates images from the locally supplied ROM with the bundled BSD font;
see [REBUILD.md](../../docs/REBUILD.md). The host option
`TMC_TITLE_BACKGROUND=1` renders the background without logo layers.

As in Pokémon, `psla:-settings`, read through `sceAppMgrGetAppParam`, opens the
shared menu before `AgbMain`. Closing it continues startup. Save actions remain
unavailable until the engine has loaded the game. LiveArea refreshes from `app0:`
on the first run of a new revision; failure is nonfatal and retried next launch.
The button appears after installation or refresh.

To test Settings, fully close the game before using the button. During gameplay,
Triangle still opens settings. Cold launch was validated on the host, with matching
image/audio/state after closing the menu. Dispatch for an already-open bubble and
actual placement require Vita testing. The 0.04 engine changes remain unchanged.

## Version 0.04 changes

Six categories: Display, Controls, Audio, General, Performance and Saves.
Cross/Circle/Square/L/R/Start/Select can be remapped; Triangle and internal menu
controls remain fixed. Stick dead zone ranges from 10% to 60%. Resetting settings
requires confirmation and does not delete saves. Optional fast boot skips to file
selection on the next launch.

Three automatic backups are captured after a complete game save:
`tmc-backup.0.pmb` through `.2.pmb`. They contain all three in-game save slots.
Capture-now copies the latest on-disk save; it is not a memory save state.
Restoration requires confirmation, preserves the current file as
`tmc_eu.sav.before-restore`, then exits. Unsaved in-memory progress is lost.
Backups with invalid size or CRC cannot be restored. Startup recovers interrupted
replacements without overwriting ambiguous cases.

Sleep/resume notifications combine with menu pause, reset the clock, release
controls and reopen audio output. Profiling covers about 60 seconds: FPS,
software rendering, presentation, worst duration and frames above 20 ms. Export
from the menu writes `ux0:data/minishcap/performance.csv`. Presentation includes
waiting and copying, not an isolated GPU counter. These are diagnostic metrics;
no speed improvement is claimed here.

Historical 0.04 tests: ASan/UBSan for menu, storage and helpers; nested audio
pauses; a 14,510-frame host comparison with simulated pause/menu/sleep and matching
state/audio/save/image; restoration through the menu; help/defaults; normal/fast
startup and CSV export. The ARM stairs walkthrough covered 18,000 frames.
Physical sleep, Vita audio recovery and performance still require hardware tests.

## Building

Use the current [ROM reconstruction guide](../../docs/REBUILD.md). Direct Vita
packaging requires both `--generated-source-dir` and `--assets-dir`, pointing
to private generated inputs outside Git. Historical commands without these
arguments are unsupported. The host CMake build also needs
`-DTMC_GENERATED_DIR=/path/to/generated` and SDL2.

Run the SDL2 host version from an isolated test directory with a local test save.
T opens the menu, X confirms, Z goes back and C opens help. Test controls include
`TMC_FRAMES`, `TMC_UNPACED`, `TMC_INPUT_SCRIPT`, `TMC_DUMP_AT`, `TMC_TRACE` and
`TMC_AUTOLOAD=0`. `TMC_AUDIO_CAPTURE=audio.raw` produces reproducible synchronous
mixing. `TMC_MENU_AT` opens the menu at a frame boundary. `TMC_MENU_SCRIPT` uses
start/end-exclusive intervals and hexadecimal PM_* masks on a separate modal
clock. `TMC_SUSPEND_AT` simulates a system pause; `TMC_MENU_DUMP` writes a PPM
image. These modal scripting controls exist only in host builds.

## Validation and limitations

Version 0.02 corrected seven entity-deletion checks to compare exactly with -1,
preserving Vita pointers whose high bit is set. The ARM walkthrough reached the
village, Deku Scrub, castle ceremony, sword and two Octoroks. Rendering was sampled
and system services were simulated.

Version 0.03 tests covered ASan/UBSan, settings, failures and recovery, three SDL
mixer pause cycles, and reloading six options. Host runs with and without opening
the menu produced matching states, final image, synchronous audio and test save.
Gameplay advanced after closing the menu. The ARM 0.03 binary also passed the
bedroom/entrance/bedroom route (18,000 frame boundaries, Unicorn).

The user confirmed 0.03 worked on their Vita. Version 0.04 passed local tests but
still required hardware validation at that point. Host and ARM tests do not
validate Vita audio output, actual performance, sleep/resume or the entire game.
The software PPU remains single-threaded. CPU: 444 MHz; audio: 48 kHz stereo;
target cadence: approximately 59.73 Hz.

The port uses GPL-3.0-or-later under the upstream notices. The standalone module
uses MIT; its font retains BSD terms. Other components retain their licenses;
see `PROVENANCE.txt` and the license notices.
