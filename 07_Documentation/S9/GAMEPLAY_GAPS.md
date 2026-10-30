# Gameplay findings and remaining scope — 17 October 2026

User preference: both relaxed assisted driving and manual loco-pilot driving,
selected by the player. Retain WAP-7/LHB and Egmore–Tambaram.

The retained original S8 service has three trains total: player, one Egmore crossing express,
and one reverse-directed northbound adjacent-track service. The player route
runs straight through the Egmore crossover. Thus an AI crossover does not mean
that the player experiences track switching. Most of the route has no new
traffic conflict, and Assisted automatically regulates cruise and braking.

Implemented presentation follow-up: actual-position opposing-train notices;
junction messages based on traffic clearance; briefing explains both modes
and the real number/type of encounters. No decorative train spawns or invented
route grants. These cues improve awareness but do not fill the gameplay gap.

New S9 live service implemented: protected player crossover and three moving
opposing services (five trains total), separate save identity, both driving modes,
and nearest-opposing sound presentation. Host tests verify actual crossover
traversal under both point locks, cold recovery, three moving encounters and
terminal dwell. Shipping03 builds/cooks successfully; packaged route review is
in progress. The original checkpoints are retained.

Remaining operations scope: more varied conflicts beyond the Egmore hold,
additional player routing choices and dated operating data. The three opposing
services occupy separate protected lines; they add passing traffic, not three
new player delays. Do not imply manual dispatcher control or a full timetable.

Realism also remains limited by representative scenery, no fully modelled coach
interiors, and no verified stock-specific field-recording bank. Cosmetic tuning
and HUD notices alone cannot establish realistic railway gameplay.
