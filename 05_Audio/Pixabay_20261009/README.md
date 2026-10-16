# First Pixabay railway audio bank

Downloaded and integrated 2026-10-09 at the user's request.

| Asset | Runtime behavior |
| --- | --- |
| SW_Rail | Crossfaded rolling loop; volume follows speed, smoothed pitch limited to 0.85–1.20. |
| SW_Station | Crossfaded platform ambience; fades within 120 m of platform centre, reduced during horn playback. |
| SW_Horn | Four-second excerpt, triggered by the existing horn control; overlapping taps are ignored. |
| SW_Brake | Pneumatic event when applied braking crosses engage/release thresholds; hysteresis avoids chatter. |

The original S7 operational cue is retained. All five voices stop on pause,
streaming block, application background, or service completion. Audio reads
simulation state without changing train physics. Cab view reduces ambience levels.
The new assets live in `/Game/IndianRailway/S8/Audio`, which is included in cook settings.
Original S7 placeholder sound assets remain available for recovery.

`Audition.wav` previews horn, rail, brake, then eight seconds of station ambience,
with short gaps. It is a sequence of source excerpts, not a recording of gameplay.
The four prepared WAVs total approximately 4.4 MB before engine compression.

`Originals/` preserves the downloaded MP3s. `Prepared/` contains 48 kHz, mono,
16-bit WAVs. `manifest.json` records source URLs, creators, SHA-256 hashes,
transformations and intended asset paths. This is a private project source bank,
not a standalone stock-audio redistribution package.

Regenerate with `python 05_Audio/prepare_pixabay.py`. Import through Unreal's
PythonScript commandlet with `05_Audio/import_pixabay.py`. The import checks
channels, duration and loop configuration. Existing assets are not reimported
automatically, so later source changes require an intentional reimport.

Verification evidence: `import-result.json`, `automation.log`, `Automation/`,
`runtime.log` and `runtime-result.json`. The driving smoke uses a separate
`Saved/S8/AudioReview` checkpoint folder and verifies normal checkpoint hashes.
Run it with `pwsh -File 05_Audio/test_audio.ps1` after building MyProjectEditor.

The first full automated run passed 15 tests. The 45-second driving smoke passed
all six checks with the Windows WASAPI mixer and Realtek speaker device active:
five assets loaded, horn and brake events triggered, rail and station voices
playing, and normal checkpoint hashes unchanged. Additional lifecycle regression
coverage passed both AudioBank and AudioLifecycle tests with zero failures or
warnings. It checks paused horn rejection, pause/block/background stopping, and
reuse of the same five sounds on resume. Results are recorded separately in
`LifecycleAutomation/`.

Limitations: numerical checks and Unreal playback state do not establish acoustic
quality. Human listening is still needed for horn-excerpt quality, repeating
background voices/events, seam perceptibility, phone-speaker balance and exact
locomotive authenticity. Station distance currently follows the player's route
position, not an independently movable listener. The bank is shared by stock
classes; it is not a verified WAP-7 or WAG-9 signature. No Android APK has been
rebuilt as part of this audio change.
