# S10 train audio: simulation-driven WAP-7 sound

Implemented 2026-10-30. Code: `RailLocoSound.h` (pure C++ synth), `RailLocoSoundComponent.{h,cpp}`
(Unreal `USynthComponent` host), `RailServiceAudio.cpp` (mix and per-train voices),
`RailServiceAudioModel.h` (air flows, camera perspective, passing envelope),
`Simulation/RailTrackJoints.{h,cpp}` (where wheels strike rail joints).
Tests: `IndianRailway.S10.LocoSound`, `IndianRailway.S10.TrackJoints`, `IndianRailway.S8.Audio*`.
Offline render: `07_Documentation/S10/Tools/LocoSoundPreview.cpp` (cab and trackside WAVs of a
scripted run). Listening renders: `Evidence-Audio-20261030/wap7-run-{cab,outside}.mp3`; in-game
mixer capture of the cab motion review (`capture_tambaram_desktop.py` with TAMBARAM_MOTION_REVIEW,
TAMBARAM_RECORD_AUDIO, TAMBARAM_AUDIO_CAB): `Evidence-Audio-20261030/game-cab-motion.mp3`
(115 s, peak 0.71, no clipped samples, 100 Hz hum throughout; raw run under the ignored
`Evidence/Audio-20261030-game-cab-motion`). All 32 `IndianRailway` automation suites pass.

## What was wrong

Every train used the same three files: a 5.6 s *coach vestibule* loop for rolling (player cab,
traffic and opposing trains alike), one synthetic 60 Hz motor hum, and one 4 s horn clip. The
station loop contained a recorded horn and a train arrival, so a phantom train arrived every 33 s.
Regenerative braking (negative effort) was silent.

## What it is now

Each sound is tied to a simulated quantity, so it cannot disagree with the train:

| Layer | Driven by | Model |
|---|---|---|
| Traction converter / motor magnetic tones | stator frequency from wheel speed, realised TE **or BE** | 6-pole motors, gear 72:20, half-worn 1054 mm wheels: 71 km/h = 1283 rpm (CLW/CAMTECH figure, test-checked). Asynchronous PWM (400 Hz carrier) when starting, 9- and 5-pulse synchronous modes, square wave above base speed. Silent at zero effort (pulses blocked). |
| Gear mesh | motor speed x 20-tooth pinion | louder under torque |
| Main transformer hum | VCB closed, load | 100 Hz (2 x 50 Hz OHE) and harmonics |
| Traction-motor and oil-cooler blowers | VCB closed | broadband + blade-pass tones, slow drift |
| Rolling | speed | structure-borne rumble, wheel/rail roar (~30 log v), aerodynamic (~60 log v) with slow track-quality variation, never a repeating loop |
| Wheel strikes | each axle crossing a real joint | insulated rail joints at every signal (track-circuit boundary), switch and crossing nose at turnouts (1 in 12 lead 28 m). WAP-7 Co-Co gives the three-axle "da-da-dum", the rake's 28 axles follow quieter and further back. |
| Flange squeal | curvature, speed | only on sharp curves at low and moderate speed |
| Brake friction / stopping squeal | brake cylinder, speed | grind while braking, squeal in the last metres |
| A9 service exhaust | rate of brake-cylinder fill | hiss under the desk while the brake applies |
| Distributor release exhaust | rate of cylinder fall x pressure | recorded exhaust burst when a charged cylinder (> 0.6 kg/cm2) releases, scaled by its pressure; synthesised flow for the full graduated release (small corrections are flow only) |
| Emergency vent | emergency command | full-bore vent decaying as the pipe empties (~2.5 s) |
| Horn | horn switch | recorded twin-tone blast, **sounds while held** (minimum 0.35 s), fades as the valve closes |
| Platform | station proximity | crowd chatter only (re-cut; no embedded horn or arrival) |

**Perspective.** Inside the cab the balance shifts to rumble, transformer, blowers and desk air;
outside, to rolling roar, converter whine and wheel ring. Outside recordings are low-passed by the
closed cab; the train's own voice is not (its cab balance is built in).

**Other trains.** Traffic and opposing trains each have their own voices: a locomotive voice at the
loco (converter, gear, transformer, blowers, brake air) and a rolling voice at the nearest bogie of
the rake, each with its own random seed, Doppler and distance envelope, plus a separate horn blast.
A stationary energised loco nearby now idles audibly (blowers and hum), as a real one does.

**Mobile budget.** One player voice and four traffic voices, each ~20 table-sine partials and
~14 biquads at the device rate; traffic voices stop when inaudible. No streaming, no large loops;
the resident recording set fell from ten files to five.

## Provisional values

- Converter pulse pattern (async carrier 400 Hz, mode thresholds 27/48/64 Hz): GTO practice, not a
  published WAP-7 schedule. Later IGBT-converter WAP-7s switch faster and sound smoother.
- Slip (1.6 Hz at full effort), blower blade-pass tones (539 / 220 Hz), axle pitch 1.9 m, turnout
  lead 28 m, all mix levels.
- The horn and station recordings are Indian field recordings of unverified locomotive class.

## Sources

- WAP-7 drive train: CLW spec 4TMS 096 081 (wheel 1092/1054/1016 mm, 6FRA 6068 motors):
  https://clw.indianrailways.gov.in/uploads/4TMS_096_081%20Rev-1%20Alt-4.pdf
- 6FRA 6068 (six-pole, 850 kW, 1283/2484 rpm): CAMTECH pamphlet
  https://st2.indiarailinfo.com/kjfdsuiemjvcya0/0/3/3/9/1418339/0/pamphleton3phasetractionmotor6fra6068wag9wap717773.pdf
- Gear ratio 72:20: https://en.wikipedia.org/wiki/Indian_locomotive_class_WAP-7
- GTO switching limited to a few hundred hertz; SPWM at low frequency, selective-harmonic patterns
  above: https://koreascience.kr/article/JAKO199911919585103.pub
- Recordings: `05_Audio/SOURCE_REGISTER.md`, `05_Audio/S10/RealRecordingsV1/manifest.json`.

## Next (needs recordings the developer downloads)

Claude cannot download from Pixabay's CDN (blocked here); the developer clicks **Download** on each
page and the files land in `Downloads`, as for the first bank. Candidates (Pixabay Content License):

1. Indian express passing a level crossing (exterior pass-by, horn):
   https://pixabay.com/sound-effects/film-special-effects-indian-express-train-passing-through-level-crossing-331010/
2. Loose railway track, trains passing, loopable (joint clatter reference):
   https://pixabay.com/sound-effects/film-special-effects-loose-railway-track-sound-trains-passing-loopable-332669/
3. Indian Railway train arriving announcement (platform PA):
   https://pixabay.com/sound-effects/film-special-effects-indian-railway-train-arriving-announcement-333043/
4. India train ride from Howrah: voices, clacks, horns, announcements:
   https://pixabay.com/sound-effects/city-india-train-ride-from-howrah-station-voices-clacks-horns-announcements-10-24991/
5. Inside express train, Indian SFX library (coach interior, for rolling-spectrum calibration):
   https://pixabay.com/sound-effects/city-inside-express-train-sound-effect-indian-sfx-library-266409/
6. Engine air compressor (MR compressor cycling, once MR is simulated):
   https://pixabay.com/sound-effects/technology-engine-air-compressor-090720-35551/

The definitive step is WAP-7 field recordings (cab and lineside, GTO and IGBT variants) to replace
the provisional converter pattern and calibrate every level.

## Not modelled yet

- Compressor / MR cycling (MR is not simulated; the gauge shows a charged 8.5).
- Station announcements (needs recording 3 above) and other trains' wheel strikes.
- Pantograph/VCB start-up clunks (no cab start-up sequence yet), vigilance (VCD) buzzer.
- Wet-rail and wheel-slip sounds (no adhesion model).
