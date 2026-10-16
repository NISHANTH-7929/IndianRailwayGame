# S2 initial inspection and decisions

The owner explicitly accepted S1 in the S2 request. Historical S1 offline limitations remain in its evidence; they do not revoke that acceptance. S2 is now authorized, S3 is not.

Existing project: `03_UnrealProject/MyProject/MyProject.uproject`, UE 5.8.2 CL56702186. Blueprint-only at S2 entry, no source/module, five tiny S1 map/material assets. Mobile Vulkan/ASTC ARM64 Development harness and its scripts remain the regression baseline. Descriptor/config/assets were preserved in `Baseline/` before S2 changes, with hashes in `Evidence/baseline-hashes.json`.

Only VS2019/MSVC14.29 is registered. Local UE Windows_SDK.json requires MSVC14.38 minimum and prefers supported14.44 or14.50 builds; VS2022 build tools are therefore required for the new editor module. Android SDK36/build-tools36.1.0/NDKr27c already passed S1. The phone hotspot must not be disabled; offline testing will preserve that constraint and report unavailable evidence honestly.

Implementation contract: one ordinary C++ kernel without UObject/Actor dependencies, owned by one world subsystem. SI doubles (metres, seconds, kilograms), stable typed numeric IDs with zero invalid, right-handed mathematical cross-product operations in an Unreal-compatible X/Y/Z world with Z up; only presentation converts metres to centimetres. Geometry is analytic line/circular helix sampled by true3D arc length. Directed edge position uses canonical start-to-end distance and an explicit +1/-1 orientation.

One ordered path tape records directed edge occurrences and legal transitions, including repeated edges as distinct occurrences. A stable formation end is the path anchor; negative commanded movement reverses motion without reordering identities. Both tape ends can extend through explicit transitions. Consumed junction choices remain immutable for the retained tape, so reversing retraces the same path. No point locks, occupancy, authority, dynamics or route planner is introduced.

Fixed step candidate30Hz, four-step catch-up maximum; overload pauses explicitly with its accumulated debt, never silently skips distance. A thin view consumes interpolated placement snapshots and can be destroyed/recreated without owning or modifying the train. T03 here validates path history only; the roadmap's point-locking portion is explicitly deferred to S4 by the user's S2 exclusions.
