# S5 inspection and implementation contract

Inspected the accepted S2, S3 and S4 reports and actual RailKernel, RailHandling, RailOperations, Unreal subsystem, views, widget, Automation wrappers and build/device tools before source edits. S4 acceptance is 27/27; regression inventories are S2 39, S3 28 and S4 39. Pre-edit source/config copies and SHA256 hashes are under Evidence/Baseline.

S5 adds a pure value-observation driver at 10 Hz within the existing 30 Hz transactional world. AI state commits with railway state. Commands enter HandlingModel::Submit; requests enter the same S4 request queue and grant validation. Public player controls cannot overwrite the AI-owned train. No AI code receives a mutable world or spatial simulation reference.

Reuse all S4 routes and resources. The Player A route request is queued normally during scenario setup. AI B retries R2 every 90 simulation ticks. A rear clearance releases R1, after which a later B request can succeed. B's endpoint cannot accommodate full rear clearance; B must retain its occupied route and stop. No topology expansion, fake release or automatic reverse is permitted.

Signal Stop after passing a route entry applies behind the train. The driver checks the governing signal before entry and continuously checks matching authority and resource ownership. Semantic service braking approaches the endpoint conservatively. S4 remains the final safety guard even if driver intent is wrong.

Actual train-view and infrastructure Actor recreation must preserve full AI/railway JSON and reconstruct presentation. Host copy tests alone do not satisfy that requirement. Android and Unreal results remain pending until actually executed. No S6/S7 work is authorized.
