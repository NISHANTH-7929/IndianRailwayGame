# WAP-7 cab presentation — 24 October 2026

This pass improves the approved WAP-7 cab on the existing Egmore–Tambaram journey.
It retains the traditional desk layout and the source assembly's reference limits.

## Delivered changes

- Nineteen separate S9 material assets distinguish enamel, charcoal panels, rubber,
  fabric, steel, brass and instrument bezels. Fifteen are used by the current cab.
- Fine surface grain uses the correct detail UVs: channel 1 on the merged contact
  shaded surfaces, with the contact atlas kept on channel 0. Texture connections
  are checked during import; lettering and dial markings remain separate.
- Window and instrument glass use restrained surface lighting and grazing-angle
  opacity, keeping central sightlines clear.
- Visor mounts, recorder support/conduit and recessed fastener details add 468
  triangles to the existing core mesh. No extra component or material slot is added.
  The actual loaded cab is 49,347 triangles at LOD0, below its 50,000 budget.
  The source export is 49,587; Unreal removes the same 240 degenerate triangles
  from the original and replacement. Outer bounds and original UVs are unchanged.
- The existing lamp has a warm, soft source, calibrated to the corridor's fixed
  exposure. Final intensity is 30 lumens in the engine, not a measured prototype
  lamp specification. The initial 650-lumen trial washed out the panel and was
  rejected during visual review.
- Instrument zoom keeps 55-degree FOV and raises the gaze from the old -12-degree
  offset to -3 degrees, so the pressure and electrical instrument faces fit.
  Leaving zoom restores the normal driving view.
- Runtime telemetry reports the actual core mesh, finish count and cab triangles,
  so packaging can be checked for silent fallback to older assets.

## Verification

The editor build and `IndianRailway.S8.CabStabilization` pass. That integration test
checks the actual candidate core and eight key finish paths, the unchanged runtime
component budget, control/gauge poses, emergency indication, lamp visibility,
instrument zoom and the four journey cameras.

`Evidence/CabVisual-20261024-02` contains before/after 1600×900 captures of the wide
cab and close instruments with the lamp off and on. Both reviews pass; the copied
checkpoint remains unchanged at tick 9035, paused and unblocked. The final four
candidate images were visually checked for panel contrast, gauge framing and
window visibility. Thirty warmup frames avoid capturing unsettled temporal text.
These are static presentation checks, not a new full-journey or frame-time result.

Shipping package `08_Builds/S9_CabRealism-Desktop-20261024-01/Windows` built
successfully. Launch it with `Start Live Railway Journey.cmd`. All 19 new material
shader entries are present in both the SM5 and SM6 cooked libraries. The isolated
packaged recovery check in `Evidence/CabShipping-20261024-01` passes: the candidate
core and 15 finish materials load, all 21 locomotive components are present,
the corridor and station are ready, and the saved state remains unchanged at
tick 9035 / fingerprint `75ae0214f9bb165f`, paused and unblocked.
The tested Shipping executable SHA256 is
`688F70738B79DDA26F9C23CF52FA348EC75EE8D406ADB18C24D9D62560BE6E2B`.

Asset import receipts and build/test logs are in `Evidence/CabRealism-20261023-01`.
The first material commandlet failed at a texture-input name before saving assets;
the corrected import succeeded. The hardware import saved and validated the asset,
then its editor process failed during early teardown. The wrapper now delays exit;
the saved mesh subsequently reopened, passed integration tests and rendered
successfully. Earlier rejected trials remain in the evidence directories.

## Remaining limits

Mobile is disconnected and this cab revision has not been accepted on-device.
Pressure and electrical gauges retain their parked presentation because those
physical channels are not simulated. The cab is not a measured or fleet-certified
replica. This pass does not change route logic, driving physics or player saves.

## Reproduce

- `Tools/build_cab_hardware_v1.py`: build a derivative from the finalized S8 blend.
- `Tools/import_cab_hardware_editor.py`: full-editor import with safe delayed exit.
- `Tools/refine_cab_realism_v1.py`: create fresh derivative materials.
- `Tools/Review-Cab.ps1 -RunId <fresh-id>`: isolated before/after visual checks.
- `Tools/Verify-PackagedCab.ps1 -Archive <Windows-folder> -RunId <fresh-id>`:
  launch the Shipping build against copied saves and verify the loaded candidate.

Asset-producing scripts require fresh targets and preserve the original S8 assets.
