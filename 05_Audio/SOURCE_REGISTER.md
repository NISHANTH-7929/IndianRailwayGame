# Audio source register

## User-approved source

On 2026-10-09, the user authorized using suitable sounds for the game from
[Pixabay's Indian train sound search](https://pixabay.com/sound-effects/search/indian%20train/).
Use this collection when sourcing required railway audio.

The project architecture calls for authentic stock sounds, interior/exterior
audio, station ambience, and a source/permission register. Source approval does
not establish the authenticity or technical suitability of individual clips.

## Initial candidates

These listings were inspected on 2026-10-09. Coach and station recordings below
have now been downloaded, processed, and imported into Unreal; the rural recording
remains a candidate. Auditory approval is pending; uses are provisional.

| Clip | Creator | Potential use | Review before integration |
| --- | --- | --- | --- |
| [Indian Train Sound from Vestibule – Realistic Interior Noise](https://pixabay.com/sound-effects/city-indian-train-sound-from-vestibule-realistic-interior-noise-314562/) | arunangshubanerjee | Coach/vestibule movement ambience | Listen for loop seams and background events; do not assume locomotive cab suitability. |
| [Indian Railway Station Ambience – Crowd Chatter and Train Arrival](https://pixabay.com/sound-effects/city-indian-railway-station-ambience-crowd-chatter-and-train-arrival-331012/) | arunangshubanerjee | Platform ambience | Check speech, location suitability, and embedded train events before looping. |
| [Rural Indian Train Ambience – Horn, Wind, and Light Coach Noise](https://pixabay.com/sound-effects/film-special-effects-rural-indian-train-ambience-horn-wind-and-light-coach-noise-328069/) | arunangshubanerjee | Rural journey ambience/reference | Mixed recording; not yet validated as an isolated controllable horn or stock-specific bank. |

## Provenance for acquired clips

The first integrated bank is documented in `Pixabay_20261009/manifest.json`,
including the original and prepared hashes, creator, trim, format, and asset path.
It adds [Train Horn by TanwerAman](https://pixabay.com/sound-effects/city-train-horn-337875/)
and [Depressurization of the brakes of a train by Oscar de Ávila (Freesound)](https://pixabay.com/sound-effects/city-depressurization-of-the-brakes-of-a-train-106983/).
All four download pages displayed "Free for use under the Pixabay Content License."
The full terms were inspected on 2026-10-09 (page states last updated November 18, 2024).
These are general recordings; no WAP-7/WAG-9-specific recording identity is claimed.

Retain the source URL, creator, download date, original filename, license evidence,
file hash, edits, and eventual Unreal asset path for every acquired clip.
Review the applicable [Pixabay Content License](https://pixabay.com/service/license-summary/)
and [full terms](https://pixabay.com/service/terms/) when acquiring files.
The inspected summary permits free use and adaptation without required attribution,
subject to its restrictions; it prohibits standalone redistribution and notes
that additional third-party rights may apply.

Keep original recordings and provenance under `05_Audio/`; runtime imports belong
in the Unreal project's scoped audio folders, as specified by
`02_TechnicalArchitecture/ARCHITECTURE_REVIEW_AND_BLUEPRINT.md`.

## S10 re-cut (2026-10-30)

`S10/RealRecordingsV1/` re-cuts the same authorized originals; no new sources were added
(`prepare.py`, `manifest.json` with source and prepared hashes, trims and filters):

- `SW_Wap7HornLoop`: sustain 5.60-10.40 s of the TanwerAman horn, high-passed at 300 Hz to remove
  the recording's idling-engine rumble, equal-power loop; the held cab horn.
- `SW_TrafficHorn`: the separate blast at 14.15-20.25 s of the same recording; other trains.
- `SW_StationChatter`: 0.20-17.60 s of the arunangshubanerjee station ambience, crowd chatter only.
  The original's horn (18 s) and train arrival (25-33 s) are excluded so no phantom train sounds.

The vestibule loop and the synthetic `S9/JourneyAudioV3` textures are no longer used: train sound
is synthesised from the simulation (`07_Documentation/S10/AUDIO.md`). Recommended next downloads
are listed there.
