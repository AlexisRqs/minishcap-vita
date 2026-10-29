# Source-only distribution

The publication unit is the curated source tree, its notices, build recipes,
ROM-free tests and pinned Port Menu source. The public repository must never
include the external build workspace. A user supplies the supported original
ROMs separately and generates private resources and the VPK locally.

Neither the current development VPKs nor their relink ZIPs are public release
assets: they contain generated game data. No binary release is prepared here.
Buildable sources and separate component licenses do not grant rights to game
code, graphics, music, dialogue or trademarks.

Before opening access, audit all branches/tags and reachable history, not just
the latest tree. An orphan branch does not sanitize other branches in the same
repository. Keep the private development history private. Use the separately
prepared clean source snapshot for an initial public repository, or explicitly
plan and verify history replacement before changing existing visibility.

Port Menu sources and notices are bundled at the recorded revision.
Check an unauthenticated clone after publication; no private dependency is required.
