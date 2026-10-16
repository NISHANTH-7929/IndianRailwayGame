# WAG-9 S8 visual asset

Original procedural mesh for a classic green/yellow WAG-9 presentation. Generator uses metre dimensions, +X forward, +Y right, +Z up, rail top Z=0. Editable .blend and full FBX are accompanied by grouped Parts/*.fbx and assembly.json.

Reference discovery:
- IRFCA WAG-9 gallery, controls photograph of 31132 at GZB: https://irfca.org/gallery/Locos/Electric/wag9x/Bunty_indiatrip_2007-08%2B081.jpg.html
- IRFCA control panel reference: https://irfca.org/gallery/Locos/Electric/wag9x/?g2_page=5
- WAG9 manufacturer technical manual (public mirror; fetch returned 403): https://st2.indiarailinfo.com/kjfdsuiemjvcya0/0/1/9/0/1643190/0/g9vol117628761.pdf
- IRFCA classic green/yellow livery record: https://irfca.org/apps/locos/9766

Reference images are not embedded or redistributed as textures. Gauge/control layout remains a representative adaptation, not a certified match for a specific numbered engine. Number 31082 is a visual identifier, not an assertion of matching that engine's equipment.

Modeling includes six wheelsets, bogie frames and suspension, buffers/coupler/hoses, louvres, roof ventilators and high-voltage equipment, two single-arm pantographs, cab windshield/wipers/windows, seats, pressure gauges, display, controls and basic cab lamp.

Limits: no baked textures or weathering; gauges are simplified; pantograph runtime retraction is approximate; independent brake has no existing simulation command and is visual-only. Asset costs in mesh_report.json and assembly.json are desktop geometry estimates, not Android acceptance. Runtime audio, camera and material cost must be profiled together on device. Moving roles in assembly.json describe pivots; animation and interaction require runtime binding.

Run Blender --background --python generate_wag9.py, then Blender --background --python export_parts.py. Renders show model review only, not Unreal/Android output.
