# a-raylib-game-3d

A 3D FPS game built with raylib and OpenGL.

## Setup

1. Install raylib
2. Build with `make`
3. Run with `./main`

## 3D Development Notes

### Models
- Create models using Blender/3DS Max/etc and export as `.obj` or `.gltf`
- Place models in `resources/models/` directory
- Load with `LoadModel()` function
- Supported formats: OBJ, GLTF, VOX, IQM