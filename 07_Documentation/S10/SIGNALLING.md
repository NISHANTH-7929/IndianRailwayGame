# S10 automatic block signalling (multiple-aspect colour-light)

Implemented 2026-10-30. Code: `Simulation/RailAutomaticSignals.{h,cpp}` (pure simulation),
lineside heads in `RailTambaramCorridorView::PresentSignals`, driver display in
`RailDrivingHud.cpp`. Tests: `IndianRailway.S10.AutomaticSignals`.

## Rules implemented

| Aspect | Lamps | Indication (G&SR) |
|---|---|---|
| Stop | red | Stop. |
| Caution | one yellow | Proceed, be prepared to stop at the next stop signal. |
| Attention | double yellow | Proceed, be prepared to pass the next signal at restricted speed. |
| Proceed | green | Proceed. |

- A stop signal may show an "off" aspect only when the line is clear to the next stop signal
  **and 120 m beyond it** (GR 9.01(2), double line). Clear = no vehicle of any train in that
  section and no conflicting route reservation.
- Aspect chain: off-signal before a Stop shows Caution; before a Caution, Attention; otherwise
  Proceed. The end of a route reads as Stop unless the next route's station signal is cleared.
- Head layout (Indian MACLS): top to bottom yellow, green, yellow, red, so the two yellows of
  Attention are separated. Signals stand to the left of the line.
- **Station signals** (manual stop signals) stand at every route origin and stay at danger until
  the dispatcher grants the route. **Automatic signals** carry the circular "A" marker and stand at
  a nominal 1 km spacing along each route.
- The existing authority protection (full brake inside `AuthorityStoppingDistance` of the end of
  movement authority) now reads as TPWS: the HUD reports a "TPWS brake intervention" when it
  overrides the driver. Chennai suburban lines are TPWS-fitted.

## Why aspects are derived, not stored

Aspects are a pure function of the authoritative operations snapshot (routes, reservations,
train footprints). They are never saved, cannot drift from movement authority, and needed no
save-format change. AI drivers already stop at the end of authority, which is now always a
signal at danger, so AI and player obey the same signals.

## Not yet modelled (honest limits)

- **Passing an automatic signal at danger** (GR 9.02: stop, wait 1 min by day / 2 min by night,
  whistle, proceed with great caution, ≤ 8 km/h where the line ahead is not clearly visible).
  With today's route-reserved traffic an automatic signal is only at danger behind another
  train, so the player never faces one; implement when following traffic is added.
- Signal positions are representative (1 km spacing from each route origin), not the surveyed
  Southern Railway signal plan. The 120 m overlap is checked within the route's own path.
- No route indicators (junction "feathers"), calling-on or shunt signals yet; no distant /
  "P" marker signals (not used on four-aspect automatic sections).
- Night: lamps are emissive but there is no night lighting scenario yet.

## Sources

- IRFCA, General Rules chapter IX, the automatic block system:
  https://irfca.org/docs/rulebook/gr-automatic-block.html
- IRFCA FAQ, signal aspects and indications (MACLS, 'A' marker):
  https://irfca.org/faq/faq-signal2.html
- railsigns.uk, Indian signalling overview: https://railsigns.uk/overseas/india1.html
