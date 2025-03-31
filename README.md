# Computer Graphics Projects

## Course Information
- **Course:** Computer Graphics
- **Professor:** Jules Bloomenthal (MSCS, Seattle University)
- **Textbook:** _"Computer Graphics: Implementation and Explanation"_ by J.Bloomenthal

## Technical Stack
- **Graphics API:** OpenGL 4.0+
- **Shading Language:** GLSL 400+
- **Programming Language:** C++17

## Setup Instructions

### Prerequisites
- OpenGL 4.0 or higher
- C++17 compatible compiler
- CMake 3.10 or higher

### Building the Projects
1. Clone the repository
2. Navigate to the specific assignment directory
3. Run CMake to generate build files
4. Build using your preferred compiler

## Project Structure
- `Assets/` - Contains textures, models, and output GIFs
- `1_Rotate2dLetter/` - 2D letter rotation implementation
- `2_Shade3dLetter/` - 3D letter with shading
- `3_Texture3dLetter/` - Texture mapping on 3D letter
- `4_SmoothMesh/` - Mesh smoothing and texture mapping
- `5_BumpyMesh/` - Bump mapping implementation
- `6_Hierarchy/` - Hierarchical modeling
- `7_BezierCurve/` - 3D Bezier curve implementation
- `8_Tessellation/` - Shape interpolation using tessellation
- `9_Animation/` - Flight path animation

## Assignments

### Common Controls
- **Camera Movement:** WASD keys
- **Rotation:** Arrow keys
- **Zoom:** Mouse scroll wheel


### 1. 2D Letter Rotation
**Objective:** Create and rotate a 2D letter using vertex coordinates and colors
- Implementation of basic vertex manipulation
- RGB color interpolation
- Basic transformation matrices
- **Controls:** Use arrow keys to rotate

<img src="./Assets/Assn-2.gif" width="200" height="200"/>

</br>

### 2. 3D Letter with Camera
**Objective:** Transform 2D letter into 3D with camera controls
- Implementation of perspective projection
- Basic Phong shading
- Camera movement system
- **Controls:** WASD for movement, arrow keys for rotation, scroll for zoom
</br>

<img src="./Assets/Assn-3.gif" width="200" height="200"/>

</br>

### 3. Textured 3D Letter
**Objective:** Add texture mapping and advanced lighting
- UV coordinate mapping
- Multiple light source implementation
- Texture sampling in GLSL
- **Controls:** Standard camera controls, L key to toggle lights

<img src="./Assets/Assn-4.gif" width="200" height="200"/>

</br>

### 4. Smooth-Shaded Mesh
**Objective:** Implement mesh loading and smooth shading
- OBJ file loading and parsing
- Normal calculation and smoothing
- Texture coordinate mapping
- **Controls:** Standard camera controls

<img src="./Assets/Assn-5.gif" width="200" height="200"/>

</br>

### 5. Bump Mapping
**Objective:** Implement bump mapping on a 3D mesh
- Normal map generation and usage
- Tangent space calculations
- Advanced lighting interactions
- **Controls:** Standard camera controls, B key to toggle bump mapping

<img src="./Assets/Assn-6.gif" width="200" height="200"/>
</br>

### 6. Hierarchical Modeling
**Objective:** Create hierarchical relationships between objects
- Scene graph implementation
- Parent-child transformations
- Joint manipulation
- **Controls:** Number keys (1-9) to select different parts, arrow keys to manipulate

<img src="./Assets/Assn-7.png" width="200" height="150"/>

</br>

### 7. Bezier Curves
**Objective:** Implement 3D Bezier curve system
- Cubic Bezier curve mathematics
- Control point manipulation
- Curve interpolation
- **Controls:** Click and drag control points, spacebar to add points

<img src="./Assets/Assn-8.gif" width="200" height="150"/>

</br>

### 8. Tessellation Shading
**Objective:** Implement shape morphing using tessellation
- Tessellation shader programming
- Shape interpolation
- Dynamic level of detail
- **Controls:** Use T/G to increase/decrease tessellation level

<img src="./Assets/Assn-9.gif" width="200" height="150"/>

</br>

### 9. Flight Animation
**Objective:** Create complex animation with multiple moving parts
- Path following algorithm
- Multiple object animation
- Time-based movement
- **Controls:** Space to play/pause, R to reset animation

<img src="./Assets/Assn-10.gif" width="200" height="150"/>

</br>
