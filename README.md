# Protein Visualization Tool

[![CI](https://github.com/jbInf-08/protein-visualizer/actions/workflows/ci.yml/badge.svg)](https://github.com/jbInf-08/protein-visualizer/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## Golden path

```sh
cmake -S . -B build
cmake --build build --config Release
```

Desktop app for viewing protein structures (PDB) in 3D with **OpenGL 4.5 Core**, **GLFW**, **GLAD**, **GLM**, and **Dear ImGui**.

## Features

- Load **ATOM** / **HETATM** records from PDB files
- **Instanced UV-sphere** rendering per atom (CPK-style colors, approximate van der Waals radii)
- Visualization modes: **ball-and-stick** (scaled spheres), **space filling**, plus **ribbon/cartoon** placeholders (same atom spheres until ribbons exist)
- **Orbit camera**: left-drag to rotate, scroll to zoom (when ImGui is not capturing the mouse)
- **ImGui** panel: PDB path, **Load**, **Visualization** combo
- **Drag and drop** a `.pdb` file onto the window to load it

## Dependencies

| Component | How it is provided |
|-----------|-------------------|
| **GLFW**, **ImGui**, **GLM** | Downloaded automatically on first **CMake configure** via `FetchContent` (requires network once) |
| **GLAD** (OpenGL 4.5 core) | **Vendored** under `external/glad/` (generated code committed intentionally for reproducible/offline builds) |
| **OpenGL** | System / driver (`find_package(OpenGL)`) |

## Prerequisites

- **CMake** 3.16 or newer
- **C++17** compiler
- Internet access for the **first** CMake configure (fetches archives for GLFW, ImGui, GLM)

## Build

```sh
cmake -S . -B build
cmake --build build --config Release
```

Run the executable from the **build** directory so relative paths resolve (`shaders/`, `resources/` are copied next to the binary by CMake):

```sh
cd build
./ProteinVisualizer        # Linux / macOS
ProteinVisualizer.exe      # Windows (Release)
```

### Regenerate GLAD

If you change the OpenGL version or profile, regenerate the loader with the Python **glad** CLI:

```sh
pip install glad
glad --api gl=4.5 --profile core --generator c --out-path external/glad
```

### Shader note

`shaders/cylinder_instanced.frag` and `shaders/sphere_instanced.frag` intentionally share the same fragment logic today so instanced primitives render consistently while geometry stages differ.

## Usage

1. Start the app from `build/`; it tries to load **`resources/1CRN.pdb`** (crambin from the PDB, ~327 atoms). A tiny **`resources/sample.pdb`** is also included for smoke tests.
2. Edit the **PDB path** field and press **Load**, or drop a `.pdb` onto the window.

### Fetching other structures

Download any entry from [RCSB PDB](https://www.rcsb.org/) (e.g. `https://files.rcsb.org/download/4HHB.pdb`) and point the path at your file, or save it under `resources/` and re-run **CMake configure** so it is copied into `build/resources/`.
3. Change **Visualization** in the panel; use **left mouse drag** on the 3D view to orbit and **scroll** to zoom.

## Project layout

```
protein-visualizer/
├── external/glad/     # Generated GLAD loader (committed)
├── src/
│   ├── core/          # Application loop, window + GLAD init
│   ├── renderer/      # Camera, instanced sphere renderer
│   ├── model/         # Atom / Protein
│   └── utils/         # PDB parser
├── shaders/
├── resources/
└── CMakeLists.txt
```

## License

MIT
