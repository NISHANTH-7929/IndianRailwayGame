# Excluded manual action timeout

Read-only preflight initially refused unavailable Wi-Fi endpoints. After reconnect,
the runner launched, captured step0, then expired waiting for the manual action.
action0 arrived after exit and was not executed. All restoration/preservation
checks pass. No audio-interaction acceptance from this run.

The bounded audio-sequence runner removes the manual action-file deadline, using
coordinates grounded in inspected captures with foreground, 2800x1260 size, cab,
logical-state and pause-state guards. It does not remove readiness requirements.
