# S3 inspection and extension plan

Authoritative S2 report and current pure kernel/subsystem/view/vehicle assets inspected before edits. Master brief, Game_Vision.md, architecture dynamics/command boundary and roadmap S3/T05/T06 inspected. Owner's narrower S3 scope takes precedence over roadmap suggestions for stations/weather; neither will be implemented.

S2 integration point: Simulation::Update runs its30Hz/four-step accumulator and calls AdvanceDistance. Add an UpdateSteps callback variant sharing that exact accumulator, leaving existing S2 Update behavior and39 tests intact. No graph/path/placement replacement.

Add one pure HandlingModel, owned by the existing world subsystem alongside its S2 Simulation. Commands carry train/source/sequence/tick and validated bounded values. One assigned command source; no AI implementation. Model derives mass, traction, brake and resistance capabilities from immutable per-definition records. Grade is mass-weighted at every vehicle centre along retained history.

Use bounded force/power/adhesion traction, explicit actuator response, resistance and signed gravity. Integrate constant force analytically within each fixed step, split a zero-speed crossing, and solve holding/rollback explicitly. Retain S2 overload policy. Add data assets with clearly provisional parameters, a small UMG touch adapter and read-only telemetry. Preserve the S2 map/mode. Host tests, Unreal Automation, Android controls/lifecycle, S2 regression and measured performance are required before PASS.

The original source/config/descriptor and hashes are under Baseline/Evidence. No S4 occupancy, authority, points, signals or AI are authorized.
