# WAP-7 dimensional review — open fidelity issue

The current versioned LHB formation uses the acquired locomotive's measured
21.33043212890625 m envelope and authored bogie pivots at +/-5.85 m. Those values
remain frozen under existing definition 3000/version 1 and content IR-S8-LHB-3A-1.
They are not a prototype dimensional certification.

Eastern Railway ELTC Asansol's February 2018 three-phase locomotive study material,
printed page 5, lists WAP-7 length over buffers 20,562 mm, overall width 3,100 mm,
new wheel diameter 1,092 mm, and height with pantograph locked 4,255 mm. Source:
https://er.indianrailways.gov.in/cris/uploads/files/1593155664454-study%20material%20three%20phase%20loco.pdf
(consulted 7 October 2026; parsed technical-data table, not a dimensional drawing).
The acquired presentation currently declares wheel radius 58.35094 cm. These
differences require a geometry audit, including named measurement endpoints.

Do not simply replace the logical length with the published buffer dimension:
the authored envelope, buffer planes, coupled CBC planes, bogie centres and wheel
surface extrema must be distinguished. Inspect the actual source components and
confirm the drawing/variant before correcting proportions. Cab/window/eye-point
alignment must be checked with any shell edit. Do not uniformly scale wheels,
bogie wheelbase and body together to conceal inconsistent source proportions.

Any corrected physical formation must receive a separate supported version/identity
and matching presentation binding. Old definition 3000/version 1, old LHB identity,
original 22 m fixture and all existing saves must continue to resolve to their
original geometry contract. No dimensions, handling calibration, formation logic,
save identities or algorithms were modified during this review.
