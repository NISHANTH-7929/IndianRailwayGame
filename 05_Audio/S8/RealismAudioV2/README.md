# Supplemental railway audio V2

Five original deterministic synthesized waveforms supplement the existing licensed field recordings. They are generic sound design, not authentic recordings of a WAP-7 or a particular LHB brake installation. `manifest.json` records provenance, CC0 dedication, hashes, PCM format, peaks and loop seams. Existing field recordings retain their original licences.

`RailServiceAudioModel.h` derives rolling volume/pitch from absolute speed, motor volume from actual applied traction and throttle, coasting texture from speed and load, and friction from curved track/applied braking. Gain and pitch are smoothed. Brake application, release and emergency are separate events. Held emergency state restored from a save does not replay its one-shot. Pause, background, streaming block and teardown stop owned voices; resume reuses the ten-component bank.

This is presentation over the current aggregate train handling model. It does not invent brake-pipe pressure, compressor cycling, wheel slip or regenerative braking telemetry. There is no claim of studio mixing, spatial Doppler, railway acoustic certification or subjective perfection.

Generate with `python 05_Audio/S8/RealismAudioV2/generate_audio.py`; import through Unreal Python with `import_audio.py`. Import changes only `/Game/IndianRailway/S8/Audio/RealismV2`. Check the import receipt and the `IndianRailway.S8.AudioBank`, `AudioDynamics` and `AudioLifecycle` automation tests.

The optional `TAMBARAM_RECORD_AUDIO=1` motion review captures Unreal's master mixer as `native/motion-audio.wav` while a disposable railway accelerates, brakes, reverses and emergency-stops. The ordinary simulation owner and normal save slots are preserved. This records the real audio components and mixer, not an offline reconstruction of the gain curves.
