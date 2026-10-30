# S10 air brake and regenerative braking (WAP-7 + LHB)

Implemented 2026-10-30. Code: `Simulation/RailHandling.{h,cpp}` (`AirBrake`,
`RegenerativeEffortN`, `HandlingModel::Step`), enabled per vehicle by
`HandlingDefinition::gradedAirBrake` / `maxRegenN` in `RailTambaramDriving.h`.
Tests: `IndianRailway.S10.AirBrake` (`Simulation/RailAirBrakeTests.cpp`).

## Model

**A9 automatic (train) brake — twin-pipe, graduated release.**

| Handle | Brake pipe (kg/cm²) |
|---|---|
| Release / Running (below 8 % travel) | 5.0 (charged) |
| Minimum reduction | 4.6 |
| Full service | 3.5 (1.5 reduction) |
| Emergency | 0 (vented) |

- Distributor valve: cylinder pressure follows the pipe reduction (2.5 × reduction,
  provisional ratio), so application *and release* are graduated — easing the handle back
  releases part of the brake, as the C3W/KE distributor allows.
- LHB cylinder limited to **3.0 kg/cm²** by the pressure converter (RDSO guidance); full
  service saturates it.
- Fill 0→95 % in 3–5 s; release 3.0→0.4 kg/cm² in 15–20 s (LHB handbook bands; tested).
- Traction interlock: tractive effort is cut while the pipe is below 4.7 kg/cm²; a releasing
  cylinder still drags, so powering away too early is felt.

**Regenerative braking (BE).** The WAP-7 master controller is centre-zero: forward for
tractive effort, back for braking effort. BE up to **182 kN** (provisional: the matching column
of an IR three-phase loco handbook), limited by the 4.5 MW power rating and adhesion, fading out
between ~10 and ~2 km/h — regeneration cannot hold or stop the train, so the final stop is on
the air brake, as on the real loco. BE blends with the air brake and is reported as negative
tractive effort (`tractionN < 0`).

**Instruments.** Cab BP needle follows the brake pipe, BC needle the cylinder (0–3.0 of the 0–10
dial); MR 8.5 and FP 6.0 show a charged supply. The DDU reads `TE|BE kN  BP  BC`. The HUD brake
gate shows REL / MIN / FS at the real handle points with BP and BC readouts; the controller gate
is centre-zero with TE above and BE below.

## Why the brake pipe is algebraic

On a 7-coach LHB rake (~170 m) the pressure wave crosses the train in well under a second and the
loco's own BP gauge falls almost as fast as the A9 vents; the timing a driver feels is the
distributor fill and release, which is modelled. Keeping the pipe algebraic left the saved state
layout unchanged, so existing saves and review fixtures still load (no migration).

## Not modelled yet

- **SA9 independent (loco) brake** and bail-off: needs new saved state; deferred to the next save
  schema version.
- Per-vehicle propagation delay along longer rakes, compressor/MR cycling, brake fade, wheel
  slide and wet-rail adhesion, coupler (in-train) forces.
- Assisted mode and AI still brake with the air brake only (no regenerative blending).

## Sources

- IRIMEE, Air brake system of LHB coaches: https://rskr.irimee.in/sites/default/files/Air%20Brake%20System%20of%20LHB.pdf
- IRIMEE, Name and role of brake system components: https://rskr.irimee.in/sites/default/files/Name%20and%20role%20of%20Brake%20System%20components-v1b.pdf
- RDSO 2021 consolidated guidelines (LHB BC 3.0 kg/cm²): https://rskr.irimee.in/forum/wp-content/uploads/2023/08/2021-1603-RDSO-Consolidated-guidelines-for-addressing-problem-of-wheel-shelling.pdf
- IR three-phase locomotive handbook (braking effort columns): https://st2.indiarailinfo.com/kjfdsuiemjvcya0/0/3/4/7/1572347/16526793/3pha10040366.pdf
- WAP-7 ratings: https://en.wikipedia.org/wiki/Indian_locomotive_class_WAP-7
