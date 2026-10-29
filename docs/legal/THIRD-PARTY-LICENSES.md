# Vita source snapshot: third-party notices

This snapshot is derived from EstebanPdN/zelda-tmc-3ds at
`57fe1a998761d5c1021904cee3cca1067989ba5a`. Its lineage and original copyright
notice are preserved in COPYRIGHT.md; the complete upstream notice is retained
in UPSTREAM-THIRD-PARTY-LICENSES.md for attribution, including components from
upstream targets not built by this curated Vita tree.

| Component | License / retained notice | Vita role |
| --- | --- | --- |
| Original Picori/3DS/Vita additions | GPL-3.0-or-later, root LICENSE and COPYRIGHT.md | Native platform and port work |
| Software PPU / VirtuaPPU | GPL-3.0-or-later, src/port/ppu/README.md | Software rendering |
| agbplay | LGPL-3.0, src/third_party/agbplay_core/LICENSE and COPYING.LESSER | Linked into the Vita target |
| Port Menu v0.3.0 | MIT, platform/vita/port-menu/LICENSE | Shared settings/storage |
| PSPSDK bitmap font | BSD, platform/vita/port-menu/FONT-LICENSE.txt | Menu and generated Settings image |
| libvita2d | MIT, platform/vita/VITA2D-LICENSE.txt | External linked graphics library |
| VitaSDK | External compiler/system interfaces; not bundled | Build dependency |

The complete agbplay source used by this target is retained, and the local
build recipe supports rebuilding with modified components. No generated
binary/relink package is approved for public redistribution by this preparation.

The zeldaret/tmc decompilation and Nintendo game intellectual property are not
relicensed by these notices. Assets, dialogue and data are generated from a
user-supplied ROM outside Git. The public license applies only to work its
contributors can license; it does not grant rights to the original game.

Vita adaptation copyright (c) 2026 Alexis RQS, licensed GPL-3.0-or-later to the
extent of independently authored contributions; upstream notices remain intact.
See platform/vita/PROVENANCE.txt and docs/DISTRIBUTION.md.
