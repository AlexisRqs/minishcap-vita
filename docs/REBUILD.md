# Fresh local ROM build — Minish Cap Vita 0.06

Prerequisites: Git, Python 3, CMake, a host C compiler for tests, and VitaSDK
with GCC 15.2 and libvita2d, plus SDL2 development files for local title rendering.
The LiveArea composer uses Python standard library and the bundled BSD bitmap
font; Pillow and proprietary system fonts are not required. Tested on macOS. The shared menu sources are bundled; no GitHub authentication is required.

```sh
git clone https://github.com/AlexisRqs/minishcap-vita.git
cd minishcap-vita
python3 tools/rebuild_from_rom.py \
  --rom-us /path/to/minish-cap-usa.gba \
  --rom-eu /path/to/minish-cap-europe.gba \
  --sdk /path/to/vitasdk \
  --workspace "$HOME/BipBoop-builds/minishcap-006-001"
```

Required USA ROM SHA-1: `b4bd50e4131b027c334547b4524e2dbbd4227130`.
Required European ROM SHA-1: `cff199b36ff173fb6faf152653d1bccf87c26fb7`.
The USA revision regenerates the tables used by the native engine. The European
ROM is used for local title rendering and at runtime by the French installation; it is
copied only to `install/ux0/data/minishcap/baserom_eu.gba` in the private workspace.
Both ROM arguments are required; the USA ROM regenerates engine tables, while
the European ROM supplies the runtime and title captures.
No ROM is downloaded, modified or uploaded.

The workspace must be new and outside every Git repository. The source ROMs
must also be outside Git (symlink destinations are checked). Failed builds and
old builds are never overwritten. Read `build-report.json` and `build.log`.
Success produces `build/minishcap-vita-0.06.vpk` and sets `ok: true`.

The four formerly missing generated inputs are now rebuilt from:

- offset/length/hash records and C layout templates (`rom-layout.json`, `*.c.in`);
- sound layout metadata, containing names and offsets but no sound samples.

No raw ROM arrays are present in these templates. All 411 extracted byte arrays
are regenerated locally and their hashes checked. The regenerated tables match the previously validated local build. Changing
the clone path can change assertion filenames embedded by the compiler, so
executables built in different directories need not be byte-identical.

The low-level `platform/vita/build.py` also requires `--assets-dir` pointing to
locally generated shell files. Use the ROM-based entry point for fresh clones.
It first compiles a host renderer and captures frame 2000 twice (title/forest)
in isolated directories with dummy SDL video/audio drivers. Game art stays in
the external workspace. The shell layout uses the original title pixels and a
BSD-font Settings label; visual placement still requires Vita acceptance.
Use `python3 tools/test_rebuild_boundary.py` to check the filesystem/ROM guardrails.
Compilation and host tests do not establish performance or correctness on Vita.
