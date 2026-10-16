# S5 experiment log

- Initial host suite: 32/32; S2 39, S3 28, S4 39 all pass.
- Initial Unreal editor build succeeded (140.17 seconds). S5 map creation succeeded with zero errors/warnings.
- Expanded host suite: 36/36, including AI traversing the upper curves and completing after S4 rear release, binding guards and transaction rollback.
- Review correction: AI route requests are emitted after the current step's request-processing phase, so their explicit apply tick is current tick + 2. Semantic controls still apply at current tick + 1. This makes recorded scheduling match execution; no permission or physical policy changed.
- Unreal Automation, Android and performance acceptance remain pending until actual evidence is collected.

- Final host36/36 and Unreal4/4 pass after scheduling correction. Android package built in250.78s, exit0. Transfer did not begin because ADB and mDNS both reported no endpoint. User asked to restore wireless debugging; hotspot left unchanged. Existing S4 standalone runner updated to link the added pure adapter dependencies, with its pre-edit copy saved under S5 Baseline.

- APK installed successfully after reconnection; device SHA256 matches. Android startup inventories pass39/28/39/36. Initial touch attempt guarded against launcher foreground before input. The next run passed denial, ownership and actual Actor recreation, then encountered an unexpected user-control pause at tick3303 (valid=true, faulted=false, suspended=true) before the script's scheduled pause. Run interrupted and preserved under ai-controls-unexpected-pause; no acceptance inferred from it. Requested an untouched rerun.

- User confirms no auto-clicker is active. No evidence establishes the cause of the unexpected pause; prior attribution to external input was unproven. Process inspection found no leftover host test process. Wireless ADB disappeared again; direct connection to previously discovered10.148.150.252:39201 returned10061/refused. Requested current address. Added per-touch UTC/coordinate logging to make a future rerun auditable.

- Clean rerun: all17 Android interaction/lifecycle checks pass. A rear release at head700.376230071001m; B autonomously moves to152.829249031068m and stops before165m endpoint with its route retained. Actual Actor recreation before/after movement passes. Pause tick6906 frozen, background tick7009 frozen, samePIDs19113/19173 on resume, Back exits. Unexpected pause did not recur. Its prior cause remains unknown. CSV measurement started on the same verified APK.

- Final normal-run collection had stopped after181s without its final sample/log pull. Its recorded samples were valid; retained as normal-interrupted-181s and excluded from final215s acceptance. A fresh normal-final run was started on the connected phone.

- Final normal run completed 215 seconds with all seven samples valid, no pause/fault and zero overload/fatal/assertion log matches. PSS 461159–467222 KiB; thermal status 0. Final S5 audit: 22/22 PASS. Stop at S5; S6/S7 not implemented.
