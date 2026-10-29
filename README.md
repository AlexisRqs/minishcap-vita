# Minish Cap Vita

Experimental native PlayStation Vita port of The Legend of Zelda: The Minish Cap,
based on EstebanPdN/zelda-tmc-3ds at
`57fe1a998761d5c1021904cee3cca1067989ba5a`, with Vita **0.06** adaptations.
Maintained by AlexisRqs, developed with AI assistance. This is a curated Vita
source tree; the full upstream multi-platform repository is not included.
**Physical Vita validation pending.**
This source-only snapshot is published for review; no validated binary release is provided.

## Build and install

Follow [REBUILD.md](docs/REBUILD.md). Clone the repository, supply the supported USA
and European ROMs, VitaSDK/libvita2d and a host SDL2 development installation.
The macOS-tested command regenerates engine data and title artwork in a new
external directory. No ROM, extracted data, game screenshot or VPK is tracked.
The runtime needs your European ROM at `ux0:/data/minishcap/baserom_eu.gba`.
See [INSTALL.md](docs/INSTALL.md) for safe updates.

## Features and limits

Native engine and software PPU, Vita controls, synchronized audio, Triangle
settings, cold-start LiveArea Settings, display/input/audio controls, backup
rotation/restoration and frame-time diagnostics. The shared menu is Port Menu
v0.3.0 at `b181eb6a427678b2d374c98432abf114b0330693`.

The maintainer confirmed earlier 0.03 and 0.05 behavior on Vita, including 0.05
LiveArea Settings and audio after sleep. The newly reconstructed 0.06 package
still needs [hardware acceptance](docs/HARDWARE-TEST.md). Host/ARM tests do not
validate physical sleep, GPU/audio timing or the entire campaign. The software
PPU is single-threaded. No guaranteed frame rate or complete playthrough is claimed.

## ROM-free checks

```sh
python3 tools/check_port_menu.py
python3 tools/check_publication.py
python3 tools/test_publication.py
python3 tools/test_rebuild_boundary.py
cmake -S platform/vita/port-menu -B build-menu -DPORT_MENU_BUILD_TESTS=ON
cmake --build build-menu --parallel
ctest --test-dir build-menu --output-on-failure
```

See PORT-MENU.md for deliberate bundled source updates and verification for a
reproducible build. Historical implementation notes are in platform/vita/README-VITA.md.

## Licenses and distribution

Original Vita additions follow the upstream GPL-3.0-or-later notice. Port Menu
is MIT and its PSPSDK font is BSD; other components retain their notices. See
[third-party notices](docs/legal/THIRD-PARTY-LICENSES.md), LICENSE and
[publication checklist](docs/PUBLICATION.md). These licenses do not grant rights
to Nintendo's game intellectual property. This is an unofficial port.

Publish only the audited clean source snapshot. Historical private commits
contain ROM-derived LiveArea images; changing the current repository's visibility
would expose that history. Generated VPKs include game-derived data and are not
public source-release assets. See [DISTRIBUTION.md](docs/DISTRIBUTION.md).
