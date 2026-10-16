# Isolated Egmore cold-recovery runtime checks

PASS: a temporary fresh service checkpoint restored at generation 1 / tick 300
with identical logical fingerprint and service state. Two fresh samples remained
paused, with both locomotive presentations and the Egmore scene ready.

Missing and corrupt checkpoint cases failed closed, remained at tick zero in
Selection, and wrote no checkpoints. These cases were tested only in the
AssetReview directory. All eight normal checkpoint paths stayed unchanged.
The complete pre-existing isolated directory was backed up, restored and
hash-verified. Temporary diagnostic contents were removed; original contents
remain recoverable in isolated-original.

All 14 desktop Automation suites pass in sibling oct09-egmore-isolated-resume1.
The earlier runtime1 attempt is excluded: its harness encountered a file-read
issue and its result serialization used integer dictionary keys. Both harness
issues were fixed before this run. No Android, explicit save-button rejection,
reward settlement or final S8 acceptance is claimed here.
