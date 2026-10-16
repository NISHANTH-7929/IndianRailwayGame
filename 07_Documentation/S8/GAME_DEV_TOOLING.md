# Game Development Studio CLI — 2026-10-04

Installed CLI 1.1.0 from the publisher's GitHub release using its Windows guide:
https://github.com/theisegoria/game-development-studio/blob/v1.1.0/docs/windows-install.md

The release tarball was matched against SHA256SUMS.txt before npm installation
with `--ignore-scripts`. Both are retained in Evidence/Tooling/GameDev110.
Installed prefix: C:/Users/nisha/AppData/Local/GameDevelopmentStudio/cli.
The prefix was appended to user PATH without replacing existing entries.
Existing shells can use Tools/Invoke-GameDev.ps1, which sets BLENDER_PATH to the
existing Blender 5.2 executable. Node 24.12.0 meets the CLI requirement.

Capabilities and initial/configured doctor receipts are preserved. Configured
doctor finds Blender. No paid provider calls were made; provider credentials
are absent. Windows and standalone skill-directory discovery warnings are
reported by doctor; plugin skills remain at their plugin paths.

## Egmore portable validation

Eleven original modules were exported from the saved source .blend to
04_Assets/S8/EgmorePortable/v1-fittings. Initial inspection found missing
tangents on polygonal meshes. The portable exporter now triangulates in memory
and explicitly exports tangents without saving changes to the source .blend
or replacing runtime FBX. Before/after evidence is retained.

All eleven pass the explicit portable policy and CLI package verification.
Packages and catalog are under 04_Assets/S8/EgmorePortable/.game-dev; build and
verification receipts are under Evidence/Egmore/Packages. The manifest binds
each GLB digest to the source .blend digest. Package metadata describes original
project authorship and pending visual acceptance; it grants no public license.

The policy permits the current 512px repeating material maps. The inspector's
close-range texture warning remains relevant; passing this technical policy
does not certify sufficient visual detail. CLI triangle/material defaults are
technical screens, not a measured Android performance budget. Unreal LOD,
scale, rendering and device measurements remain separate evidence.

No runtime game code, content identity, save, or installed Android APK was
changed by this portable packaging pass. The outstanding realism/integration
work remains recorded in STATION_VISUAL_REVIEW.md and STATION_WORK_LOG.md.
