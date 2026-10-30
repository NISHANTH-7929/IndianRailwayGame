# Native driving controls redesign

Status recorded on 2026-10-26. This change implements the Egmore–Tambaram
driving interface inside Unreal, over the existing 3D cab and route. The earlier
web control study is not the deliverable. Evidence for this pass belongs in
`Evidence/DrivingControls-20261024-01`.

## Implemented interface

`RailDrivingHud.cpp` provides a native Slate widget, mounted by
`RailServiceWidget.cpp` for Tambaram content. During driving it replaces the
legacy button rows and text HUD. Briefing, pause, results and sound settings
continue through the service menu. Other route variants retain their existing
controls.

The landscape layout uses a 720-unit height, scales uniformly with viewport
height, and anchors the right controls to the available width. Its intended
hierarchy is the 3D windscreen first, driving instruments second:

- Brake on the left and power on the right, with graphite grips, light edge
  highlights, marked travel, and separate requested/applied values. The lever
  bodies are 104 by 316 design units; draggable lanes are 104 by 216. Amber
  identifies braking and mint identifies traction.
- A compact speed dial at the lower left includes actual speed, current
  whole-train limit, a limit marker and an overspeed indication. The centre of
  the windscreen remains available for the track and signals.
- Route, next station, distance and progress occupy the upper left. Mode,
  elapsed service time, direction, CAB and pause occupy the upper right.
- Horn, view and emergency actions stay near the bottom edges. A short status
  strip presents route holds, traction holds, upcoming restrictions, station
  approach, dwell, emergency and scenery-loading information from runtime
  state. DEPART and emergency reset appear only when their conditions allow.
- CAB opens a modal panel containing headlights, pantograph, wipers, cab light,
  instrument view and direction. Pause and emergency remain available while
  that panel is open. Save and Exit is in the pause menu.

This is an original screen-control design, not a claim that the two on-screen
levers reproduce the physical WAP-7 control stand.

## Interaction behavior

`Simulation/RailDrivingControlModel.h` makes dragging relative to the selected
value when a contact begins. Touching a different point in the lane does not
instantly jump to full power or full brake. Upward motion increases the value;
177 design units covers the full range. Values clamp to 0–100% with 1% precision.
The plus/minus buttons and keyboard arrows change values by five percentage
points. Handles retain their values after release.

The release position is committed even if no move event arrives between press
and release. This fixes fast gestures coalesced within a single game frame;
the original issue was reproduced in the actual game before the correction.

The widget tracks pointer identities and rejects a second contact on a lever
already held. Keyboard Up/Down changes power; Right/Left changes brake. Escape
closes CAB or pauses. Horn and Space currently trigger the existing horn action
once per press; continuous hold-to-sound horn control is not implemented here.

Assisted mode labels power as POWER LIMIT and shows applied power/brake
separately from the player's request. It retains the existing assisted-driving
semantics. Emergency cuts requested power before applying the emergency action;
normal lever commands are blocked while emergency is active. Reset is offered
only when stopped with power at zero. Backend command validation remains
authoritative for direction and other train operations.

Route holds use the actual runtime hold reason, including dispatch watchdog
notices. Details wrap to two lines. Rejected control actions use a separate
six-second feedback strip so they cannot replace an overspeed warning.

`Simulation/RailHudInputRegions.h` and `RailLocomotiveView.cpp` arbitrate cab
input separately from the visible widget. The top 18%, bottom 20%, and outer
18% on each side below 24% height reserve space for the HUD. A pointer admitted
to camera input stays its owner until release. A contact beginning in the HUD
cannot become a camera drag by entering the centre; a second finger cannot
steal the first camera gesture. Mouse input is suppressed when touch input is
present. Pause, modal changes, camera changes and application deactivation
cancel cab gestures and require release before re-entry.

## Reference study

These references informed visual hierarchy and interaction choices; their art
and layouts were not imported into the project.

| Official source | Observed design lesson |
| --- | --- |
| [Train Sim World 7 controls introduction](https://live.dovetailgames.com/news/play-tsw7-now-early-access), [published HUD image](https://media-cdn.dovetailgames.com/2026/092026/09/TSW7-SimpleControlsUI-WM-01.jpg) | Actual 3D cab fills the screen; transparent instruments and narrow power/brake columns sit at an edge. The pictured desktop cluster is not a phone touch-size specification. |
| [Trainz Simulator 3](https://info.trainzsimulator.com/ts3), [published cab image](https://static.tildacdn.com/tild6665-3264-4166-a637-646263383062/controls---Copy.jpg) | Rubber/metal grips, marked travel and legible machinery labels provide a tactile visual reference. That page describes direct 3D interaction as a planned feature, so it does not establish its release status. |
| [Indian Train Simulator official Google Play listing](https://play.google.com/store/apps/details?gl=IN&hl=en-GB&id=com.HighbrowInteractive.IndianTrainSim) | Store screenshots preserve a full-width 3D scene with an edge throttle and compact route/speed information. This redesign reduces the dense utility row by putting occasional actions in CAB. |

## Verification and remaining work

The desktop Editor target compiled successfully; see `editor-build-final-03.log`.
All three focused Unreal tests passed with zero failures or warnings in
`automation-release/index.json`: `IndianRailway.S9.DrivingControlPrecision`,
`IndianRailway.S9.DrivingHudInputOwnership` and
`IndianRailway.S9.DrivingHudRapidDrag`. The last test drives the native Slate
press/release handlers with no move event, and also checks focus cancellation
and emergency lockout.

Actual 1280×720 gameplay was inspected on Windows (captured at 125% desktop
scaling). The following checks passed in `interactive-release.log`:

| Interaction | Observed result |
| --- | --- |
| Rapid upward brake drag | Requested brake changed from 0% to 50%; applied brake followed. |
| Rapid downward power drag | Requested power changed from 100% to 50%. |
| Lever gestures | Cab orientation stayed fixed; no accidental camera pan. |
| CAB | Drawer opened and retained reachable pause/emergency controls. |
| Emergency while CAB open | Drawer closed, requested power became 0%, emergency latched and applied brake reached 100%. |
| Power + while emergency active | Requested power remained 0%. |
| Reset while stopped | Emergency released; normal view control returned. |
| Pause and resume | Service paused, resumed and restored the driving HUD. |
| VIEW | Switched to the exterior camera with the HUD retained. |

Captures are `native-driving-controls.jpg`, `native-cab-menu.jpg` and
`native-emergency.jpg` in the evidence directory. The 122 pre-existing checkpoint
files were unchanged in `save-preservation.json`; interactive checks used an
isolated profile.

Android ASTC packaging succeeded in `Evidence/Controls-Android-20261026-04`.
The signed ARM64 Development APK is
`08_Builds/S9_Controls-Android-20261026-04/Android_ASTC/MyProject-arm64.apk`
relative to the repository root (332,876,626 bytes). APK signature verification
passed; its embedded command line selects the live Egmore–Tambaram cab service.
It retains the existing test application identity and version.

The packaging helper disables MCP server auto-start only inside the cooker
process. Editor plugin classes stay loaded and interactive editor settings are
unchanged. This resolves the local port conflict that interrupted earlier runs.
Two small type fixes in `RailTambaramDesktopReview.cpp` were also required to
compile the existing track-asset audit: explicit static-mesh pointer conversion
and serializing an already-shared JSON reference directly.

No phone is connected, so no installation or device test is claimed.

The passing checks do not establish native Slate multitouch behavior on physical
Android hardware, comfortable thumb reach, readable physical sizes, or touch
feedback. Simultaneous lever contacts, small utility-button target sizes, device
aspect ratios, cutouts, dark cab views and sustained driving performance still
need phone testing. A mirrored layout, haptics and a new practice tutorial are
not implemented in this pass. One initial editor launch during shader loading
triggered the existing frame-overload pause; the later warmed interaction pass
completed. This is not a full-route performance qualification.

This record is not production acceptance, full-route performance qualification,
or Play Store release signoff.
