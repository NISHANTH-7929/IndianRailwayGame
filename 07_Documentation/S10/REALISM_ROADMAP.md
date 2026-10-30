# S10 realism roadmap (started 2026-10-30)

Goal: the most authentic Indian Railways loco-pilot experience on Android —
"Monoposto for Indian trains". Realism means the whole railway behaves
consistently: physics, cab, signalling, traffic, stations, environment, sound.

Ordered by player-perceived impact ÷ effort. Each slice ships with rendered or
tested evidence and is committed separately.

## Done in S10 slice 1 — visual baseline overhaul

- Physically based Chennai daylight: SkyAtmosphere + atmosphere sun (~09:30),
  real-time captured sky ambient, humid coastal haze, warm unbound grade.
- Cab interior no longer flooded by open-sky ambient (mobile has no sky
  occlusion): sky fill is dimmed only while the camera is in the cab.
- Corridor density: front-row plots packed like Chennai suburbs, a third
  housing row, and a continuous far skyline (houses, black rooftop tanks,
  coconut palms above canopy) so the horizon is never an empty plain.
- Pastel emulsion palette plus `M_ChennaiFacade`: monsoon rain streaks under
  slab lines, faded paint and storey-base dust (original procedural mask).

## Done in S10 slice 2 — cab that matches the locomotive, loco-style controls

- Cab V2 rebuilt from ray-cast measurements of the exterior: single 2.30 x 0.55 m
  windscreen at 2.83-3.38 m on the 16 deg nose, sliding side window and narrow door
  where the exterior has them, cab length 2.1 m (the S8 cab ran 0.4 m into the
  machine room and had 0.85 m tall windows 0.25 m too high, plus grilles the
  exterior does not have).
- Seated driver eye 1.3 m behind the screen; desk and hooded binnacle below the
  sight line; MR/BP/FP/BC gauges, speed indicator, TE meter, DDU with live speed;
  A9/SA9 brake pedestal on the left, TE/BE master controller, reverser, VCD/horn
  and emergency button on the desk; wipers park on the sill.
- HUD redesigned: A9 brake gate (REL/MIN/FS, BC kg/cm2), TE master-controller gate
  (notches, live TE kN), DDU strip (speed, MPS, direction, messages) at the top,
  icon keys; the centre of the windscreen stays clear.

## Done in S10 slice 3 — real surfaces and Indian automatic signalling

- CC0 Poly Haven ground (laterite + grass), concrete, asphalt and plaster materials.
- Four-aspect automatic block signalling (see `SIGNALLING.md`): station signals at route
  origins, 'A'-marker automatic signals every ~1 km, aspects derived from occupancy and
  reservations with the GR 9.01 120 m overlap, lineside heads, DDU signal indicator, TPWS notice.

## Next slices

1. **IR automatic block signalling (gameplay).** Four-aspect colour-light
   signals at each block boundary on the suburban lines (Egmore–Tambaram is
   automatic territory): Red / Yellow (caution: be prepared to stop at next
   signal) / Double Yellow (attention: pass next signal at restricted speed) /
   Green. Aspects derived in the pure simulation from block occupancy and
   reservations; AI obeys them; HUD shows next aspect + distance; passing Red
   is a SPAD (emergency, penalty). Automatic signals carry the "A" marker:
   after stopping at Red, a train may proceed after the prescribed wait at
   restricted speed (G&SR automatic-signal rule) — verify exact figures against
   the current G&SR / Southern Railway SR before implementing.
2. **Brake realism.** Separate A9 (automatic/train) and SA9 (independent/loco)
   brakes; brake-pipe pressure (5.0 kg/cm² charged) with propagation delay
   along the rake; brake-cylinder pressure; graduated release; regenerative
   braking (BE) on the WAP-7 with its own speed-dependent limit. Cab gauges
   (BP, BC, MR, FP) driven by these pressures.
3. **Cab procedures.** Pantograph raise, VCB close, reverser, TE/BE master
   controller notches, vigilance control device (periodic acknowledgement
   pedal/button), DDU screen showing demanded vs realised TE/BE, OHE kV and
   primary current, speed with class limit.
4. **Environment assets.** Procedural primitives have a ceiling. For photoreal
   results bring in real assets: Fab/Megascans (free with UE, needs the
   developer's Epic account), CC0 Poly Haven textures/HDRIs, and a
   commissioned or purchased WAP-7 + LHB set. Prosopis juliflora scrub along
   the formation, coconut/neem/rain trees, compound walls, hoardings, power
   poles with sagging wires, people at stations and level crossings.
5. **Ground and verge.** `M_CorridorOuterGround` is referenced in code but the
   asset was never created (falls back to `M_DryGround`). Author a proper
   laterite + weed ground material with texture detail.
6. **In-train forces, curve resistance, wheel slip on wet rail** after brakes.
7. **Audio from real recordings** (WAP-7 traction motors, compressor, horn,
   LHB bogie rhythm), licensed or field-recorded.

## Sources consulted

- WAP-7 ratings (6120 hp continuous, 322.4 kN starting TE, 228 kN at 71 km/h,
  air + regenerative loco brake): https://en.wikipedia.org/wiki/Indian_locomotive_class_WAP-7
- DDU screen contents (TE/BE demanded vs realised, OHE voltage, class speed
  limit): https://manualedistruzioni.it/arc/monitor/ddu-v50/user-manual/tu9n
- IR colour-light signalling overview (2/3/4-aspect, double yellow):
  https://railsigns.uk/overseas/india1.html — secondary; confirm against the
  official G&SR before implementing rules.
- WAP-7 cab photographs: https://irfca.org/gallery/Locos/Electric/electric_cab_photos
