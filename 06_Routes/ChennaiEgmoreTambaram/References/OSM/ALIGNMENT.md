# Authored corridor alignment, version 3

`generate_tambaram_alignment.py` consumes the retained OSM reference and topology
audit, verifies their hash relationship, and selects connected endpoint candidate
0 for its **corridor shape**. This selection does not establish the direction of
travel, track category or an operational WAP-7 platform allocation. In particular,
the connected candidate must not be advertised as a verified down main line.

The node polyline is simplified with 2 m Douglas–Peucker tolerance. Circular
fillets fit inside 44% of the neighboring line lengths, giving continuous
positions and tangents without overlapping corners. Runtime graph edges are
analytic lines and circular arcs; no spline projection or train teleport is used.
`alignment-audit.json` records the measured authored length, smallest radius,
maximum source-node deviation, source ways and hash. There is no source evidence
for gradient or cant, so both remain zero. This is a representative horizontal
alignment, not a surveyed reproduction.

300 m tangent extensions at both ends provide station/authority envelopes. They
are explicitly authored and are not counted as OSM fit. Station POIs are projected
onto the authored line **offline** to establish labels and station envelopes.
Runtime uses only distance along the graph. Therefore the authored total length
includes envelopes and must not be confused with the historical Southern Railway
24.820 km sectional chainage or rounded 25 km timetable distance.

The player track follows the generated line. Three additional tracks use exact
parallel-arc geometry, with radii and lengths adjusted for their lateral offset.
Their offsets preserve the established Egmore art cross-section (5, 0, 16.35,
21.35 m). They are not independently surveyed tracks. Actual yard expansion,
platform numbers and main/suburban separation require further source-backed
authoring. Version 3 is incompatible with earlier placeholder route snapshots;
the original Egmore content and vehicle definitions are unchanged.

Attribution: © OpenStreetMap contributors, licensed under ODbL 1.0.
https://www.openstreetmap.org/copyright
