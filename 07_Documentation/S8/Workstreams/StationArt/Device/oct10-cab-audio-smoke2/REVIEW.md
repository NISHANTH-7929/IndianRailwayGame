# Interrupted capture: excluded from completed smoke acceptance

The owner mistakenly started legacy recovery before this capture runner returned.
This lost foreground and caused a wake-restoration verification mismatch. Original
result.json remains unchanged; do not treat this as a completed audio smoke.
After both runners finished, restore_egmore_interrupted was invoked with the
correct AudioReview telemetry path. restoration-recovery.json PASSES: original
commandline/telemetry/wake restored, normal saves unchanged, exact APK retained,
game stopped. No checkpoint restoration/overwrite was performed.

screen-005.png is unobscured and was inspected before the overlap: gauges and
major controls visible; small central panel lettering remains dark/indistinct.
That visual observation does not establish final cab readability acceptance.
The project has no game-dev comparison adapter, so no pixel regression claim.
