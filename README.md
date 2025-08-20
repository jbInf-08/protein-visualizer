# Protein Visualization Tool

A desktop application for visualizing protein structures in 3D using C++ and OpenGL.

## Features
- Load and parse PDB files
- Visualize proteins in multiple modes (Ball-and-Stick, Space Filling, etc.)
- 3D camera navigation
- Interactive UI with ImGui

## Build Instructions

### Prerequisites
- CMake >= 3.10
- C++17 compiler
- [GLFW](https://www.glfw.org/), [GLAD](https://glad.dav1d.de/), [GLM](https://github.com/g-truc/glm), [ImGui](https://github.com/ocornut/imgui)

### Build Steps
1. Clone this repository and its submodules (for external dependencies):
   ```sh
   git clone --recurse-submodules <repo-url>
   cd protein-visualizer
   ```
2. Create a build directory and run CMake:
   ```sh
   mkdir build && cd build
   cmake ..
   make
   ```
3. Run the application:
   ```sh
   ./ProteinVisualizer
   ```

## Usage
- Use the UI to load PDB files and switch visualization modes.
- Navigate the 3D scene with mouse and keyboard.

## Project Structure
```
protein-visualizer/
├── src/
│   ├── core/
│   ├── renderer/
│   ├── model/
│   └── utils/
├── shaders/
├── resources/
├── CMakeLists.txt
├── README.md
└── docs/
```

## License
MIT 