// Author: Nadezhda Chernova
// File: Assn-4-Texture3dLetter.cpp
// Date: 10/22/2024
// Texture-map 3D letter, display it using multiple lights and rotate it by
// mouse-drag, move it with SHIFT key, zoom with mouse wheel using camera

#include <glad.h>     // GL loader
#include <glfw3.h>    // GL toolkit (create window + rendering context for it)
#include "GLXtras.h"  // convenience routines
#include "VecMat.h"   // library for vector/matrix operations
#include "Camera.h"   // camera class
#include "Draw.h"     // Screen drawing, Star
#include "IO.h"       // ReadTexture
#include "Widgets.h"  // Mover

// GPU identifiers
GLuint VAO = 0, VBO = 0, EBO = 0;    // vertex array, vertex buffer, element buffer
GLuint program = 0;                  // shader program ID

// window size
int winWidth = 800;
int winHeight = 800;

// Camera setup
Camera camera(0, 0, winWidth, winHeight, vec3(15, -30, 0), vec3(0, 0, -5), 30);

// 3D locations of letter's vertices
vec3 points[] = {
        // front (z = 0)
        {200, 200, 0}, // v0
        {150, 250, 0}, // v1
        {150, 150, 0}, // v2
        {50,  250, 0}, // v3
        {50,  350, 0}, // v4
        {150, 350, 0}, // v5
        {50,  150, 0}, // v6
        {50,  50,  0}, // v7
        {150, 50,  0}, // v8
        {250, 150, 0}, // v9
        {250, 250, 0}, // v10
        {250, 350, 0}, // v11
        {350, 350, 0}, // v12
        {350, 250, 0}, // v13
        {350, 150, 0}, // v14
        {350, 50,  0}, // v15
        {250, 50,  0}, // v16

        // back (z = -50)
        {200, 200, -50}, // v17
        {150, 250, -50}, // v18
        {150, 150, -50}, // v19
        {50,  250, -50}, // v20
        {50,  350, -50}, // v21
        {150, 350, -50}, // v22
        {50,  150, -50}, // v23
        {50,  50,  -50},  // v24
        {150, 50,  -50},  // v25
        {250, 150, -50}, // v26
        {250, 250, -50}, // v27
        {250, 350, -50}, // v28
        {350, 350, -50}, // v29
        {350, 250, -50}, // v30
        {350, 150, -50}, // v31
        {350, 50,  -50},  // v32
        {250, 50,  -50}   // v33
};

const int nPoints = sizeof(points) / sizeof(vec3); // number of points
vec2 uvs[nPoints]; // array of texture coordinates with size of nPoints

// Uvs setup (mapping of (x,y) of points to uv-range [0..1]
void SetUvs() {
    vec3 min, max;
    Bounds(points, nPoints, min, max); // find min/max bounds of points
    vec3 dif(max -
             min); // difference between max and min (range along each axis)
    for (int i = 0; i < nPoints; i++)
        // normalize (x,y) to [0..1]
        uvs[i] = vec2((points[i].x - min.x) / dif.x,
                      (points[i].y - min.y) / dif.y);
}

// triangle vertex indices to create triangles
int3 triangles[] = {
        // front triangles
        {0,  1,  2},
        {0,  2,  9},
        {0,  9,  10},
        {0,  1,  10},
        {1,  4,  5},
        {1,  3,  4},
        {1,  2,  3},
        {2,  3,  6},
        {2,  6,  7},
        {2,  7,  8},
        {9,  15, 16},
        {9,  14, 15},
        {9,  10, 13},
        {9,  13, 14},
        {10, 11, 12},
        {10, 12, 13},

        // back triangles
        // offset by number of origin vertices (17), reverse order
        {17, 19, 18},
        {17, 26, 19},
        {17, 27, 26},
        {17, 27, 18},
        {18, 22, 21},
        {18, 21, 20},
        {18, 20, 19},
        {19, 23, 20},
        {19, 24, 23},
        {19, 25, 24},
        {26, 33, 32},
        {26, 32, 31},
        {26, 30, 27},
        {26, 31, 30},
        {27, 29, 28},
        {27, 30, 29},

        // side triangles
        {1,  10, 18},
        {18, 10, 27},   // Side between v1-v10 and v18-v27
        {1,  2,  19},
        {1,  19, 18},     // Side between v1-v2 and v18-v19
        {1,  5,  22},
        {1,  22, 18},     // Side between v1-v5 and v18-v22
        {2,  8,  19},
        {19, 8,  25},     // Side between v2-v8 and v19-v25
        {2,  9,  19},
        {19, 9,  26},     // Side between v2-v9 and v19-v26
        {3,  4,  21},
        {21, 3,  20},     // Side between v3-v4 and v20-v21
        {3,  6,  20},
        {20, 6,  23},     // Side between v3-v6 and V20-v23
        {4,  5,  21},
        {21, 5,  22},     // Side between v4-v5 and v21-v22
        {6,  7,  23},
        {23, 7,  24},     // Side between v6-v7 and v23-v24
        {7,  8,  25},
        {25, 7,  24},     // Side between v7-v8 and v24-v25
        {9,  10, 27},
        {27, 9,  26},     // Side between v9-v10 and v26-v27
        {10, 11, 28},
        {28, 10, 27},   // Side between v10-v11 and v27-v28
        {11, 12, 28},
        {28, 12, 29},   // Side between v11-v12 and v28-v29
        {12, 13, 29},
        {29, 13, 30},   // Side between V12-v13 and v29-v30
        {13, 14, 30},
        {30, 14, 31},   // Side between v13-v14 and v30-v31
        {14, 15, 31},
        {31, 15, 32},  // Side between v14-v15 and v31-v32
        {15, 16, 33},
        {33, 15, 32},  // Side between v15-v16 and v32-v33
        {16, 9,  26},
        {26, 16, 33}    // Side between v16-v9 and v26-v33
};

// texture image
const char *textureFilename =
        "/Users/nadin/Documents/Graphics/Apps/Textures/christmas-tree-574742_1920.jpg";
GLuint textureName = 0; // id for texture image, set by ReadTexture
int textureUnit = 0; // id for GPU image buffer, may be freely set

// Moving lights
Mover mover; // to move light
void *picked = NULL; // user selection (&mover or &camera)

// two lights
vec3 lights[] = {{.5, 0, 1},
                 {1,  1, 0}};
const int nLights = sizeof(lights) / sizeof(vec3);

// vertex shader: operations before the rasterizer
const char *vertexShader = R"(
	#version 330 core
    uniform mat4 modelview;   // modelview matrix
    uniform mat4 persp;       // perspective matrix

    in vec3 point;            // input vertex position
	in vec2 uv;               // input uv texture coordinates of each point
    out vec3 vPoint;          // output point for pixel shader
	out vec2 vUv;             // output texture coordinates for pixel shader

    void main() {
        vPoint = (modelview * vec4(point, 1)).xyz; // transformed to world space
        gl_Position = persp * vec4(vPoint, 1); // transformed to perspective space
        vUv = uv;
	}
)";

// pixel shader: operations after the rasterizer
// apply lighting, texture mapping, color computation
const char *pixelShader = R"(
	#version 330 core

    uniform sampler2D textureImage;       // access to 2D texture image
    uniform int nLights = 0;              // number of lights in scene
    uniform vec3 lights[20];              // array of light (max = 20)

    uniform float amb = 0.3;             // ambient term
    uniform float dif = 0.8;             // diffuse weight
    uniform float spc = 0.7;             // specular weight

    in vec3 vPoint; // transformed point from vertex shader
    in vec2 vUv;    // texture coordinates from vertex shader
    out vec4 pColor; // final color

	void main() {
        vec3 dx = dFdx(vPoint), dy = dFdy(vPoint); // vPoint change, horizontally/vertically
        vec3 N = normalize(cross(dx, dy)); // unit-length surface normal
        vec3 E = normalize(vPoint);         // eye direction
        float diffuseTotal = 0;
        float specularTotal = 0;

        for (int i = 0; i < nLights; i++) {
            vec3 L = normalize(lights[i] - vPoint); // unit-length light vector
            vec3 R = reflect(-L, N);            // reflection vector
            float d = abs(dot(N, L)); // diffuse term
            diffuseTotal += d;
            float h = max(0.0, dot(R, E));      // highlight term
            float s = pow(h, 100.0);            // specular term
            specularTotal += s;
        }

        float intensity = min(1, amb + dif * diffuseTotal) + spc * specularTotal; // weighted sum
        vec3 col = texture(textureImage, vUv).rgb; // vUv is parametric texture map location
		pColor = vec4(intensity * col, 1); // opaque
	}
)";


void Display() {
    // z-buffer to avoid self-obscure of 3d object
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // clear background
    glClearColor(1, 1, 1, 1); // set background to white
    glClear(GL_COLOR_BUFFER_BIT); // clear screen

    // access GPU buffers, activate shader program
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // set modelview and perspective matrices in vertex shader
    SetUniform(program, "modelview", camera.modelview);
    SetUniform(program, "persp", camera.persp);

    // associate vertex positions with the 'point' attribute in vertex shader
    VertexAttribPointer(program, "point", 3, 0, (void *) 0);

    // associate uv coordinates with the 'uv' attribute in vertex shader
    VertexAttribPointer(program, "uv", 2, 0, (void *) sizeof(points));

    // inform pixel shader which image buffer to read from
    SetUniform(program, "textureImage", textureUnit);

    // bind 2D texture, activate appropriate texture unit (enable GPU buffer)
    glBindTexture(GL_TEXTURE_2D, textureName);
    glActiveTexture(GL_TEXTURE0 + textureUnit);

    // draw elements using EBO
    int nVertices = sizeof(triangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, 0);
    // last arg 0 means use EBO

    // draw arcball to control camera rotation
    glDisable(GL_DEPTH_TEST); // disable depth testing before drawing
    if (!Shift() && camera.down) {
        glDisable(GL_DEPTH_TEST);
        camera.arcball.Draw(Control());
    }

    // update/transform lights
    SetUniform(program, "nLights", nLights);
    SetUniform3v(program, "lights", nLights, (float *) lights,
                 camera.modelview);

    // draw each light as a golden asterisk
    UseDrawShader(camera.fullview); // use draw shader for rendering
    for (int i = 0; i < nLights; i++)
        Star(lights[i], 8, vec3(1, .8f, 0), vec3(0, 0, 1));
    // draw star at light position with 8 points

    glFlush(); // ensure all commands are completed
}

void BufferGPU() {
    // make GPU buffers for points and colors, set to active
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // allocate buffer memory to hold vertex locations and uv's
    int sPoints = sizeof(points);
    int sUvs = nPoints * sizeof(vec2);
    glBufferData(GL_ARRAY_BUFFER, sPoints + sUvs, NULL, GL_STATIC_DRAW);

    // copy vertex data to the GPU
    glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);

    // copy uv data to the GPU
    glBufferSubData(GL_ARRAY_BUFFER, sPoints, sUvs, uvs);

    // make EBO to store triangles, set to active
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // copy triangle index data to GPU
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangles) * sizeof(int),
                 triangles, GL_STATIC_DRAW);
}

void StandardizePoints(float s = 1) {
    // scale and offset so points are in range +/-s, centered at origin
    vec3 min, max;
    float range = Bounds(points, nPoints, min, max);
    float scale = 2 * s / range;
    vec3 center = (min + max) / 2;
    for (int i = 0; i < nPoints; i++)
        points[i] = scale * (points[i] - center);
}

void MouseButton(float x, float y, bool left, bool down) {
    picked = NULL;
    for (int i = 0; i < nLights; i++) {
        if (MouseOver(x, y, lights[i], camera.fullview)) {
            picked = &mover;
            mover.Down(&lights[i], (int) x, (int) y, camera.modelview,
                       camera.persp);
        }
    }
    if (picked == NULL) {
        picked = &camera;
        camera.Down(x, y, Shift(), Control());
    }
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
    if (leftDown) {
        if (picked == &mover) {
            mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
        } else if (picked == &camera) {
            camera.Drag(x, y);
        }
    }
}

void MouseWheel(float spin) {
    camera.Wheel(spin, Shift());
}

// update GL + camera when resize app
void Resize(int width, int height) {
    glViewport(0, 0, width, height);
    camera.Resize(width, height);
}

int main() {
    GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight,
                             "Texture 3d Letter");
    program = LinkProgramViaCode(&vertexShader, &pixelShader);
    if (!program) {
        printf("can't init shader program\n");
        getchar();
        return 0;
    }

    // register callbacks
    RegisterMouseMove(MouseMove);
    RegisterMouseButton(MouseButton);
    RegisterMouseWheel(MouseWheel);
    RegisterResize(Resize);

    // read texture image from file
    ReadTexture(textureFilename, &textureName);

    // initialize uv coordinates
    SetUvs();

    // fit letter into the window
    StandardizePoints(0.8f);

    // allocate vertex memory in the GPU
    BufferGPU();

    // event loop
    while (!glfwWindowShouldClose(w)) {
        Display();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }

    // unbind vertex buffer, free GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &VBO);
    glfwDestroyWindow(w);
    glfwTerminate();
}
