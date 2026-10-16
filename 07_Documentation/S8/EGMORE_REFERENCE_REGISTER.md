# Chennai Egmore reference and production register

The user selected Chennai Egmore on 2026-10-03. The nine supplied photographs
are examples of desired realism, not a request to recreate Babina or Renigunta.
Egmore-specific architecture takes precedence over those sample landmarks.
The date/era choice is pending; the preserved heritage facade can proceed.

## Photographic references

Reference-only files under 04_Assets/S8/References/Egmore:

| File | Creator/date | Source and photo license |
|---|---|---|
| facade-panorama.jpg | PlaneMad, 2009-01-01 | https://commons.wikimedia.org/wiki/File:Chennai_Egmore_facade_panorama.jpg — CC BY-SA 3.0 |
| heritage-front.jpg | RealinRear, 2022-09-29 | https://commons.wikimedia.org/wiki/File:Chennai_Egmore_Railway_Station_Front_Heritage_01.jpg — CC BY-SA 4.0 |
| east.jpg | Pratyeka, 2011-05-22 | https://commons.wikimedia.org/wiki/File:Chennai_Egmore_Railway_Station_(view_from_the_east).jpeg — CC BY-SA 3.0 |
| platform.jpg | Nileshantony92, 2013-06-26 | https://commons.wikimedia.org/wiki/File:Vaigai_Exp_in_MS.jpg — CC BY-SA 3.0 |

License links: https://creativecommons.org/licenses/by-sa/3.0/ and
https://creativecommons.org/licenses/by-sa/4.0/. Files downloaded unchanged.
They are not baked onto model surfaces, imported into Unreal, or shipped in APKs.
The checked-in generator reads no photographs. User samples retain their
watermarks and are separately recorded in REAL_STATION_REFERENCE_NOTES.md.

## Evidence limits

The facade views establish red masonry/stucco fields, cream arch surrounds,
repeated columns and cornices, projecting balconies, white domes and parapets.
The 2011 track view establishes blue roofs, dense pale OHE gantries and lush
vegetation. The 2013 image establishes small red platform tiles but is mostly
obscured by a locomotive. None is a dimensional survey or present-day layout.

Recent reporting describes continuing redevelopment, so a 2011 platform layout
must not be labelled an exact 2026 reconstruction:
https://www.newindianexpress.com/cities/chennai/2026/Sep/21/egmore-station-revamp-limits-access-for-disabled

## Independent source

`build_egmore_kit.py` starts a separate `Station.Egmore.Heritage.01`, version 1,
under `04_Assets/S8/EgmoreKit`. It does not overwrite the measured Station.Kit.01.
Initial modules: six-metre arcade, four-metre tower, dome, nine-metre projecting
entrance pavilion and canopy bay. Shared trim cross-sections reduced aggregate
source triangles from 95,982 to 45,230 while retaining the sampled arch profiles.
Dimensions and colors are explicitly estimated. The initial assembled study is
for proportions/material review, not an exact full facade or a playable Egmore
route. Ornament, central entrance, complete building layout, signage and platform
layout require subsequent refinement. No completion or Android claim applies to
these new Egmore assets until import, integration and measurement have run.
The assembled study deliberately does not claim a surveyed building layout;
the exact facade bay sequence, full central pediment relief, multilingual
lettering, side returns and actual station depth remain to be resolved.

Existing Shantipur/S7/LHB content IDs and saves remain unchanged. A real Egmore
service must have a distinct content identity and validated route/platform
geometry; renaming the 50 m test platform is not a station reconstruction.

## Historical nameboard reference (2026-10-05)
- Superfast1111, Chennai Egmore - Stationboard, photographed 2011-11-18.
- https://commons.wikimedia.org/wiki/File:Chennai_Egmore.jpg
- CC BY-SA 3.0: https://creativecommons.org/licenses/by-sa/3.0/
- Reference thumbnail retained at References/Egmore/stationboard.jpg (960 px).
  Original download was rate-limited; the available Commons thumbnail succeeded.
- Used to inspect Tamil / English / Hindi language order and yellow board framing.
  New text was independently typeset; no photograph pixels enter game textures.
- Tamil: சென்னை எழும்பூர்; English: CHENNAI EGMORE; Hindi: चेन्नै एषुंबूर.
  Historical naming/transliteration retained, not represented as a present-day survey.
