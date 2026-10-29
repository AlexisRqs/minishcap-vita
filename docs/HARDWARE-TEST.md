# Hardware acceptance — Minish Cap 0.06

Status: PENDING. No console access or personal save access in this preparation.
Record source commit, VPK SHA-256, Vita model, firmware, plugins and clock settings.
Earlier version acceptance does not certify this reconstructed package.

1. Fully close the game. Make and verify an independent backup of its entire
   data directory on another device. Preserve the previous VPK.
2. Keep original data isolated under a unique backup directory name; populate
   the normal data path with a verified copy for testing. Do not merge folders.
   Pokémon uses ux0:/data/PMDFR0001/; Minish Cap uses ux0:/data/minishcap/.
   No existing folder should be overwritten. See INSTALL.md.
3. Install over the existing bubble. A filename suffix does not create a separate
   application: the title ID/data paths are unchanged. Do not uninstall.
4. Test cold launch, load, input, movement, sound, Triangle settings, return to
   game, full exit/restart, and the cold-start LiveArea Settings entry.
5. Test physical sleep/resume from gameplay and from the menu: audio resumes,
   input releases correctly, timing recovers and the game remains responsive.
6. Only on disposable data copies: make a manual backup, advance/save, restore
   with confirmation, relaunch and check expected progress and before-restore copy.
   Verify canceling restoration does not change the save.
7. Play for at least 20 minutes; record crashes, audio glitches, visual corruption
   and observations rather than extrapolating to a complete campaign.

Game-specific checks:
- Check title/forest LiveArea artwork and the Settings label/launch target.
- Load a test copy, travel between rooms, fight, save and reload after full exit.

After testing, fully close the game. Preserve test data under a new unique name
and restore the original directory name without merging. Reinstall the previous
VPK if rolling back the executable. Never upload personal saves, ROMs or VPKs to
issues. Report pass/fail plus exact reproduction steps and reviewed log excerpts.
