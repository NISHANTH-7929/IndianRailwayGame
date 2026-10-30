# Poly Haven texture set (S10)

Source: https://polyhaven.com — all assets **CC0 1.0** (public domain dedication,
https://polyhaven.com/license). No attribution required; authors are still recorded
in `provenance.json` together with each file's URL and SHA-256.

| Asset | Used for | Unreal material |
|---|---|---|
| `red_laterite_soil_stones` | Trackside and backland soil | `M_ChennaiGround` |
| `sparse_grass` | Weedy grass blended over the soil | `M_ChennaiGround` |
| `painted_plaster_wall` | Plaster detail on painted houses | `M_ChennaiFacade` |
| `concrete_wall_008` | Slabs, boundary walls, posts, drains | `M_ChennaiConcrete` |
| `asphalt_02` | Service roads | `M_ChennaiAsphalt` |

1K JPG maps only (colour, DirectX normal, ARM) to keep the Android texture budget small.
Re-fetch: `python -I 07_Documentation/S10/Tools/fetch_polyhaven_v1.py`; re-author materials:
`UnrealEditor-Cmd <uproject> -run=pythonscript -script=<abs>/07_Documentation/S10/Tools/import_polyhaven_materials_v1.py`.
