// Author: Nadezhda Chernova
// File: Assn-2-RotateLetter.cpp
// Date: 10/08/2024
// Display colorful letter (that consists of triangles) and rotate it about
// x-axis on mouse-drag up/down and about y-axis on mouse-drag left/right

#include <glad.h>     // GL loader
#include <glfw3.h>    // GL toolkit (create window + context for it)
#include "GLXtras.h"  // convenience routines
#include "VecMat.h"   // library for vector/matrix operations

vec2 mouseNow; // current mouse position in pixels

// GPU identifiers
GLuint VAO = 0, VBO = 0, EBO = 0;    // vertex array, vertex buffer, element buffer
GLuint program = 0;                  // shader program ID

// 2D locations of letter's vertices
vec2 points[] = {
        {200, 200}, // v0
        {150, 250}, // v1
        {150, 150}, // v2
        {50,  250}, // v3
        {50,  350}, // v4
        {150, 350}, // v5
        {50,  150}, // v6
        {50,  50},  // v7
        {150, 50},  // v8
        {250, 150}, // v9
        {250, 250}, // v10
        {250, 350}, // v11
        {350, 350}, // v12
        {350, 250}, // v13
        {350, 150}, // v14
        {350, 50},  // v15
        {250, 50}   // v16
};

// RGB colors associated with each vertex
vec3 colors[] = {
        {1.0, 0.5, 0.5},  // Light red (v0)
        {1.0, 0.6, 0.4},  // Light orange (v1)
        {0.9, 0.7, 0.4},  // Orange-green (v2)
        {0.4, 1.0, 0.6},  // Green (v3)
        {0.4, 1.0, 0.8},  // Light green (v4)
        {0.8, 0.6, 0.4},  // Light brown (v5)
        {0.4, 0.8, 1.0},  // Light blue (v6)
        {0.6, 0.6, 1.0},  // Lighter blue (v7)
        {0.8, 0.4, 1.0},  // Light purple (v8)
        {1.0, 0.5, 0.5},  // Red-orange (v9)
        {1.0, 0.6, 0.6},  // Light orange-red (v10)
        {1.0, 0.7, 0.7},  // Light pink (v11)
        {0.9, 0.8, 1.0},  // Softer purple (v12)
        {0.8, 0.8, 1.0},  // Light blue-purple (v13)
        {0.6, 1.0, 0.8},  // Light cyan-green (v14)
        {1.0, 0.9, 0.6},  // Light peach instead (v15)
        {1.0, 0.8, 0.5}   // Light peach-pink (v16)
};

const int nPoints = sizeof(points) / sizeof(vec2); // number of points

// triangle vertex indices to create triangles
int3 triangles[] = {{0,  1,  2},
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
                    {10, 12, 13}
};

// vertex shader: operations before the rasterizer
const char *vertexShader = R"(
	#version 330 core
    uniform mat4 view;   // matrix for rotating letter
	in vec2 point;       // input vertex position
	in vec3 color;       // input vertex color
	out vec3 vColor;     // output color for pixel shader
	void main() {
		gl_Position = view * vec4(point, 0, 1); // apply rotation
		vColor = color; // pass color to pixel shader
	}
)";

// pixel shader: operations after the rasterizer
const char *pixelShader = R"(
	#version 330 core
	in vec3 vColor;
	out vec4 pColor;
	void main() {
		pColor = vec4(vColor, 1); // set final color
	}
)";


void Display() {
    // clear background
    glClearColor(1, 1, 1, 1); // set background to white
    glClear(GL_COLOR_BUFFER_BIT); // clear screen

    // access GPU buffers, activate shader program
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // compute rotation matrix based on mouse position
    mat4 view = RotateY(mouseNow.x) * RotateX(mouseNow.y);
    SetUniform(program, "view", view); // pass matrix to shader

    // associate position input to shader with position array in vertex buffer
    VertexAttribPointer(program, "point", 2, 0, (void *) 0);

    // associate color input to shader with color array in vertex buffer
    VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));

    // draw elements using EBO
    int nVertices = sizeof(triangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, 0);
    // last arg 0 means use EBO

    glFlush(); // ensure all commands are completed
}


void BufferGPU() {
    // make GPU buffers for points and colors, set to active
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // allocate buffer memory to hold vertex locations and colors
    int sPoints = sizeof(points), sColors = sizeof(colors);
    glBufferData(GL_ARRAY_BUFFER, sPoints + sColors, NULL, GL_STATIC_DRAW);

    // copy vertex data to the GPU
    glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);

    // copy color data to the GPU
    glBufferSubData(GL_ARRAY_BUFFER, sPoints, sColors, colors);

    // make EBO to store triangles, set to active
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // copy triangle index data to GPU
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangles) * sizeof(int),
                 triangles, GL_STATIC_DRAW);
}

void StandardizePoints(float s = 1) {
    // scale and offset so points are in range +/-s, centered at origin
    vec2 min, max;
    float range = Bounds(points, nPoints, min, max);
    float scale = 2 * s / range;
    vec2 center = (min + max) / 2;
    for (int i = 0; i < nPoints; i++)
        points[i] = scale * (points[i] - center);
}

// mouse movement callback
void MouseMove(float x, float y, bool leftDown, bool rightDown) {
    if (leftDown)
        mouseNow = vec2(x, y);
}


int main() {
    GLFWwindow *w = InitGLFW(100, 100, 800, 800, "Rotate Letter");
    program = LinkProgramViaCode(&vertexShader, &pixelShader);
    if (!program) {
        printf("can't init shader program\n");
        getchar();
        return 0;
    }
    // register mouse movement callback
    RegisterMouseMove(MouseMove);

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
