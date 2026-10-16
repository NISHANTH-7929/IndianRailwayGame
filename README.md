# Indian Railway Game

An Unreal Engine railway game project with locomotive and station assets, route data, audio, development tools, and implementation documentation.

## Open the project

1. Install Git LFS, then clone this repository and download its binary assets:

   ```powershell
   git lfs install
   git clone https://github.com/NISHANTH-7929/IndianRailwayGame.git
   cd IndianRailwayGame
   git lfs pull
   ```

2. Use the matching Unreal Engine installation. Workspace documentation records UE 5.8.2; the project descriptor currently references a local engine installation ID, which may need reassociation on another machine.
3. Open `03_UnrealProject/MyProject/MyProject.uproject`. Generate project files and compile the `MyProject` module as needed. The descriptor enables the editor plugins `ModelContextProtocol`, `AllToolsets`, and `ModelingToolsEditorMode`; install the corresponding plugins for your engine setup.
4. See `07_Documentation/S9/DRIVING_THE_JOURNEY.md`, `07_Documentation/S9/CURRENT_CONTINUATION.md`, and the reproduction notes under `07_Documentation/` for controls, current development state, validation, and platform setup. Some tools reference paths specific to the original workstation.

## Repository layout

- `01_GameDesign/`: game design documents.
- `02_Art/` and `02_TechnicalArchitecture/`: art work and technical plans.
- `03_UnrealProject/`: Unreal source, configuration, and content.
- `04_Assets/`, `05_Audio/`, and `assets/`: source assets, preparation tools, and provenance.
- `06_Routes/`: route data.
- `07_Documentation/`: milestone reports and development tools.

Binary content is stored through Git LFS. Generated Unreal folders, validation captures and milestone baseline backups, packaged builds and downloaded SDKs under `08_Builds/`, local editor settings, caches, and credentials are excluded by `.gitignore`. Local excluded files are retained on disk.

## Asset terms

Consult each asset's bundled attribution, manifest, and license records. Third-party assets retain their respective terms; public repository visibility does not grant additional redistribution or reuse rights. No project-wide open-source license is declared.
