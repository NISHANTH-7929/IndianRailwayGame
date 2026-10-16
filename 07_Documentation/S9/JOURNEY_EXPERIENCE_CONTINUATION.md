# Journey experience continuation - 16 October 2026

User requested realistic journey experience and implementation of full packaged journey/performance checks. Retain WAP-7/LHB and Egmore-Tambaram; no Android/paid/publishing. Real railway authenticity remains limited by representative route, meshes and generic licensed audio.

Implemented RailJourneyPresentation.h: derived station/progress and smooth station ambience. HUD shows actual station, passing/terminal/departure role, route limit and approach advice, elapsed journey time and applied controls. Station audio fades at all15 station locations. HUD toggle replaces DEBUG in corridor dock; Shipping cannot enable debug. Compact mode retains restrictions/emergency/service notices; text shadow improves contrast. No simulation/save schema change.

JourneyExperience-20261016-02:62/62 PASS. JourneyExperience-Shipping-20261016-01 and02 built/cooked successfully. Final02 includes corrected origin label/compact notices/shadow. Windows archive contains Start Railway Journey.cmd (normal profile), JOURNEY-README.txt; package-manifest.json binds all files.

ACTIVE packaged process21188 is final Shipping02, paused after restoring the isolated trip. UI helper activation failed twice after target refresh; asked user to restore MyProject from taskbar. Do not send more inputs until target state is readable. Window525792 returned by sky.list_windows; reacquire before input. One GPU game process only.

UserDir:07_Documentation/S9/Evidence/JourneyExperience-Packaged-20261016-01/UserData. Initial Shipping01 trip started through UI selection/confirm/start/release/power100. Protected hold completed naturally; train resumed. Saved at tick9366/head1279.786m. Shipping02 cold restore matches tick9366/logical fingerprint, generation37 paused. before-update/after-update telemetry and update-recovery.json retained. No full trip or settlement pass yet. Initial sampler session3382 exited with old process34312; start new sampler with Label journey-observations-resumed when resumed.

PresentMon1.8.0 targeted old PID34312 for120s:17281 presents,mean6.9429ms,p957.5347ms,p997.9558ms,max42.9882ms,13>33.33ms,none>50ms,dropped0. Application present intervals only, departure/junction window, not full route/GPU execution/floor qualification. CSV and summary retained. Memory/logical observations in journey-observations.jsonl. All118 baseline checkpoints unchanged.

Next: user restores window; inspect/click Resume and continue real-time assisted trip. Observe no overload/fault, sample moving/streaming performance. At terminal inspect result, settle once, save/cold-reload verify reward. Final02 source changes are compiled but compact warnings need final UI inspection. Then update report/manifests. Human art/audio/realism acceptance remains open. Do not call everything realistic or full MVP complete.
