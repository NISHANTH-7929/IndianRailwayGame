# S7 usability observation guide

Status: final Android build validated and installed on the iQOO. Awaiting real
participants. No observations have been collected or inferred from automated tests.

Recruit three casual players (C1–C3) and three railway enthusiasts (E1–E3).
The developer does not count. Record anonymous IDs only. Keep the phone hotspot on.
Each person should begin at service selection, with no coaching beyond this task:

> Complete the Shantipur service, review your result and save the reward.

Let the player choose Assisted or Standard. Record the actual mode. The intended
journey is approximately four minutes, including a real wait for the express.
Do not explain the control sequence unless the participant is stuck; record every
hint, the screen, and the elapsed time before giving it. Safety and comfort take
priority over completing a trial.

Observe selection, formation confirmation, initial movement, understanding of the
traffic hold, response when the route clears, station berth, dwell, departure,
finish stop, interpretation of result and reward settlement. During the traffic
hold, ask the participant to pause and resume without explaining how. After saving
the reward, allow the participant to finish or replay without requesting replay;
record whether replay was voluntary. Ask afterward:

1. Why did you have to wait? How did you know you could move again?
2. How did you know where to stop and when to depart?
3. Why did you receive that grade? Was the traffic delay treated fairly?
4. What changed after saving the reward? Could you find replay?
5. Were text, controls and sound cues clear? What was most confusing?

Use `usability-observations.csv`, one row per person. Preserve detailed notes in a
separate Markdown file keyed by anonymous ID. Record failures and assistance as
observed; do not mark a coached run as an independent success. Investigate repeated
confusion, fix it, and record a new trial rather than overwriting the first.

Acceptance gate 33 stays pending until all six real observations are collected
and their findings are reported. Automated ADB runs do not satisfy this gate.
