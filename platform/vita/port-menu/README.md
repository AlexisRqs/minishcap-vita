# Port Menu 0.3 — shared port module

C11, API `PM_API_VERSION=2`, with no dependency on VitaSDK, SDL or game data.
File operations require POSIX libc (`fsync`, `access`, `rename`). Sources use
MIT; the PSPSDK bitmap font retains its BSD license. The Minish Cap profile is
in `../vita_menu.c`; Pokémon and Minish Cap use their own adapters.

## Integration

Compile the required sources, or use `add_subdirectory(port-menu)` followed by
`target_link_libraries(my_game PRIVATE port_menu)`. The menu requires
`port_menu.c` and `port_model.c`. `port_backup.c` and `port_runtime.c` are
optional, independent components.

```c
static PM_Option options[] = {
 {.id="volume", .label="Volume (%)", .type=PM_RANGE,
  .value=100, .minimum=0, .maximum=100, .step=10,
  .category="Audio", .help="Game volume."},
 {.id="show_fps", .label="Show FPS", .type=PM_BOOL,
  .value=0, .minimum=0, .maximum=1, .step=1,
  .category="Display", .help="Frames presented per second."}
};
static PM_Menu menu;
if (!PM_Init(&menu, "My game", "menu.ini", options, 2, context)) {
    /* Invalid profile: report an integration error. */
}
PM_Load(&menu);
```

Keep the arrays, strings and context alive for the lifetime of the menu. Use
unique, stable ASCII identifiers; at most 32 options are supported. Types are
`PM_BOOL`, `PM_RANGE`, `PM_CHOICE` and `PM_ACTION`, with optional `enabled`,
`action` and confirmation callbacks. `PM_Reset` restores the values supplied
to `PM_Init` without touching game saves. Actions manage their own effects.

Either all options must have a nonempty category or none may have one. Mixed
profiles are rejected. Without categories, navigation remains flat and supports
older profiles after recompilation. Categories follow their first occurrence.
Pages contain seven rows. Help and confirmation text support up to eight lines
of 28 characters: use short ASCII text. The ASCII font is 8x8; labels support
16 characters and values 8. Rendering targets a 240x160 RGBA8 buffer with
stride >=240; colors in `menu.theme` are ABGR words.

At a safe engine boundary, pause game logic and mixing, call
`PM_Open(menu, held_buttons)`, then call `PM_Update`/`PM_Draw` in the modal loop.
Map physical controls to `PM_*` bits independently of custom game bindings.
Cross confirms, Circle goes back, Triangle/Start closes, and Square opens help.
Help and confirmation dialogs consume the back input first. After closing,
apply settings and wait for controls to be released before resuming game input.

The `port_menu=1` settings format remains compatible with 0.1. Unknown IDs and
out-of-range values are ignored. Saving writes `.tmp`, synchronizes, rotates
`.bak`, then replaces the main file. A missing or invalid main header triggers
recovery from `.bak`. On write failure, the menu remains open; Start explicitly
exits without saving.

## Save backups

`PB_Store` supplies a save path, a **game- and region-specific prefix**, a fixed
size and an optional validator. A validator for the actual save format is
recommended. Call `PB_Recover` at startup before loading the game save, but
after initializing any data needed by the validator. On failure, preserve the
files and stop loading.

Call `PB_Capture` only after a complete, synchronized save, outside a transaction.
Three slots `${prefix}.0.pmb` through `.2.pmb` rotate by generation and deduplicate
against the newest backup. The 32-byte header contains PMBACK1 magic, size,
data CRC32, generation, timestamp and source-path CRC32. This detects accidental
corruption; it is not authentication. Date and generation metadata are not
authenticated. `PB_InfoAt` checks size and integrity; the game-specific validator
runs during capture and restore.

`PB_Restore` preserves the current file as `${path}.before-restore`, validates
the temporary replacement, then replaces the save with rollback protection.
An existing current file is required. The game must then **exit or completely
reload its state** so stale in-memory progress cannot overwrite the restored
save. Backups cover the entire file, not one in-game save slot. Minish Cap exits
after confirmation, and all three slots are restored together. These are not
memory save states.

Replacement uses `.tmp`, `.old` and `.restore.rollback`. `PB_Recover` recovers
interrupted rotations. If both a main file and rollback exist but the main file
fails validation, both are preserved and recovery fails. Concurrent access is
unsupported: the adapter must serialize reading, capture and restore. Paths
are limited to 280 characters; save images to 16 MiB.

## Controls, pause and profiling

`PR_MapButtons` maps a physical-button mask through an index table to masks
chosen by the port. Several buttons may share an action. `PR_Stick` uses GBA
directional bits (16 right, 32 left, 64 up, 128 down); translate them in adapters
using another convention. Positive Y points down; dead zones apply per axis.

`PR_SetPause` combines MENU and SYSTEM reasons. The game resumes only when all
reasons are cleared. `PR_ReleaseInputs` blocks through a neutral input sample,
then allows the next one. The module does not pause the engine or audio: the
adapter does that. Minish Cap registers a Vita power callback, processes events
at a safe boundary, resets the pacing clock and reopens audio output on resume.

`PR_ProfileFrame` receives a monotonic microsecond clock, rendering and
presentation costs, and a scene identifier. It aggregates roughly one-second
windows into a 60-window ring buffer: FPS, averages, worst duration and frames
above 20 ms. `PR_ProfileResetClock` excludes pauses without clearing history.
`PR_ProfileExport` writes CSV on request. Presentation time includes copying
and waiting; it is not an isolated GPU measurement. Host results do not establish
Vita performance.

## Tests

```sh
cc -std=c11 -D_POSIX_C_SOURCE=200809L -I. port_menu.c port_model.c tests/menu_test.c -o menu-test
cc -std=c11 -D_POSIX_C_SOURCE=200809L -I. port_backup.c port_runtime.c port_model.c tests/runtime_test.c -o runtime-test
mkdir -p test-output/storage
./menu-test test-output
./runtime-test test-output/storage
```

Use a fresh directory for each storage-test run. ASan/UBSan are supported.
Coverage includes categories, help, paging, confirmations, defaults, persistence,
write failures, rotation, corruption, restore, recovery, mappings, dead zones,
nested pauses and profiling history. Minish Cap mixer and integration tests
are in `../tests`.

The font originates from `sample/debugScreenFont.c` in the Mystery Dungeon
port and ultimately PSPSDK. Author and license notices are retained in
`font8x8.inc` and `FONT-LICENSE.txt`. Original license:
https://github.com/pspdev/pspsdk/blob/master/LICENSE.

## Version 0.3 and Pokémon integration

`port_model.c/.h` separates option and navigation rules from rendering.
Minish Cap uses `PM_AdjustOption` through the 240x160 menu. Pokémon uses the
same function and `PM_Nav` with native Vita rendering, tabs, localization and
touch controls. Hidden choices are filtered during navigation and touch input;
tab recall stores identifiers rather than row indices. Game-specific options
and values remain in the adapter. Menu API 2 is unchanged, with added model
functions and no new settings format. Pokémon retains its binary settings
file and existing migrations.

`PM_SortNewest` sorts valid backups by descending generation even if the console
clock changes. `PM_FormatDate` includes seconds using the console locale. Both
games use these functions. Save formats and game-specific validation remain
separate; existing backups need no conversion or rewrite.

Minish Cap exposes `PB_List` with information and sorted physical slots: restore
the returned slot, never the displayed row index. Menu actions occupy a full
row to keep dates readable. `PB_PreviousInfo` validates the raw `.before-restore`
copy compatible with 0.04/0.05. `PB_RestorePrevious` first captures the current
save in the rotation, restores the raw copy and leaves `.before-restore` intact.
If capture fails, the current save is not replaced. Exit after success, as for
`PB_Restore`. The timestamp records when the file was set aside, which may not
be the date of the in-game progress.

`tests/model_test.c`, compiled with `port_model.c`, covers navigation, hidden
options, empty groups, bounds and wraparound choices. Pokémon adapters add
differential tests against 0.45, rendering and touch tests. Storage tests also
cover returning to the pre-restore save and recovering an interrupted replacement
of `.before-restore`.

## Bundled source and pinned revisions

The shared development repository is https://github.com/AlexisRqs/port-menu
(private). The first extracted version is `v0.3.0`, based on the 0.3 C code and
font, with CMake tests and Git integration added. These public ports bundle the
source directly; **no private-repository access or submodule initialization is
required**. Game-specific adapters, options and save formats remain in the ports.

This README is translated and updated for the bundled distribution. The runtime
sources, font and licenses are unchanged. The parent repository's
`tools/port-menu-source.json` records the upstream revision and current file
hashes, including documentation-only changes. Verify from the parent root:

```sh
python3 tools/check_port_menu.py
```

Develop shared code in the standalone repository, then review, test and update
each bundled snapshot and its manifest deliberately. Do not replace a pinned
snapshot with an unreviewed moving branch. From this module directory:

```sh
cmake -S . -B build -DPORT_MENU_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Tests use separate temporary directories and can be rerun. Game builds using
`add_subdirectory` leave tests disabled by default. Source archives include the
bundled files. `sync_port_menu.py` remains for legacy non-Git copies: it excludes
Git metadata and refuses to overwrite a Git checkout with `--write`.
