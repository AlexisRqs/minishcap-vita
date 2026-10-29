# Local installation — candidate 0.06

Build from both supported ROMs. Title ID: MINISH001. The generated
`build/minishcap-vita-0.06.vpk` is private build output, not a public release.
A homebrew-capable Vita with VitaShell is required; record exact firmware and
plugins when testing rather than assuming every configuration is supported.

Close the game and independently back up the entire `ux0:/data/minishcap/`
directory. Install over the existing bubble in VitaShell; do not uninstall it.
Copy your verified European ROM to `ux0:/data/minishcap/baserom_eu.gba` if absent.
The ROM prepared by the build is under `install/ux0/data/minishcap/` outside Git.
Do not replace existing saves/settings with build or synthetic test files.

Keep the previous VPK for rollback. Same-version installation may require
VitaShell confirmation; if refused, stop rather than deleting data.
Follow HARDWARE-TEST.md with a copy of the data before normal use.
