# Source publication status

**Tests Vita en attente / Physical Vita validation pending.**

The maintainer authorized public source publication on 2026-10-29. This repository
starts from the audited source-only snapshot. Earlier development history, pull
requests and logs remain in a separate private archive. Port Menu v0.3.0 sources
are bundled with their notices and checked hashes; no private submodule is needed.

Local reconstruction and automated checks passed before publication. They do not
validate physical GPU rendering, audio, sleep/resume, performance or real save
handling on Vita. Follow HARDWARE-TEST.md on isolated copies before claiming
hardware acceptance of this candidate.

Game files, generated resources, saves and VPKs remain private and are not release
assets. See DISTRIBUTION.md and license/provenance notices for the source scope.
Run `python3 tools/check_publication.py --history` on the complete Git history.
