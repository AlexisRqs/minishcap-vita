# Contributing

Port Menu v0.3.0 sources are bundled; a regular Git clone is sufficient.
Keep changes focused, preserve third-party notices and explain provenance for
new dependencies. Do not add ROMs, saves, extracted resources, compiled game
objects, VPKs, screenshots containing game assets, SDKs or private diagnostics.

Run `python3 tools/check_publication.py` and the ROM-free checks in the README.
Changes to reconstruction must also pass a fresh external-workspace build from
locally supplied ROMs. Record the exact commit and package SHA-256 tested.
Never equate a host test or simulated ARM boot with physical Vita validation.

Use synthetic files for storage tests. Never overwrite a user's real save or
ask for game data in public issues. Report steps, expected/actual behavior,
version, hardware and narrowly reviewed logs. See docs/HARDWARE-TEST.md.

Source archives include the pinned Port Menu sources and notices.
Physical Vita validation remains pending. No release workflow uploads
build outputs or changes visibility automatically.
