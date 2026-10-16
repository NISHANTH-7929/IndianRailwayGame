# Excluded overlapping run

Although the runner's three recovery cases and final checks passed, it overlapped
oct10-cab-audio-smoke2 due to owner sequencing error. Its baseline commandline/wake
was captured during that smoke and therefore is not the original idle baseline.
Exclude this run from final regression acceptance. The original idle baseline
was subsequently restored and verified in smoke2/restoration-recovery.json.
Repeat serially as oct10-combined-legacy-recovery2.
