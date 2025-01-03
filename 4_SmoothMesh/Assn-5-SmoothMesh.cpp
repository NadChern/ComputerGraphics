// Author: Nadezhda Chernova
// File: Assn-5-SmoothMesh.cpp
// Date: 10/29/2024
// Smooth-shade and texture-map a mesh (dog)

#include <glad.h>     // GL loader
#include <glfw3.h>    // GL toolkit (create window + rendering context for it)
#include "GLXtras.h"  // convenience routines
#include "VecMat.h"   // library for vector/matrix operations
#include "Camera.h"   // camera class
#include "Draw.h"     // Screen drawing, Star
#include "IO.h"       // ReadTexture
#include "Widgets.h"  // Mover
#include <vector>     // Dynamic arrays for mesh

// GPU identifiers
GLuint VAO = 0, VBO = 0, EBO = 0;    // vertex array, vertex buffer, element buffer
GLuint program = 0;                  // shader program ID

// display
int winWidth = 800, winHeight = 800;
Camera camera(0, 0, winWidth, winHeight, vec3(15, -30, 0), vec3(0, 0, -5), 30);

// dynamic arrays to read OBJ mesh file
vector<vec3> points;    // vertex locations
vector<vec3> normals;   // surface normals
vector<vec2> uvs;       // texture coordinates
vector<int3> triangles; // triplets of vertex indices

// OBJ filename
const char *objFilename = "/Users/nadin/Documents/Graphics/Apps/Assets/pear.obj";

// texture image
const char *textureFilename =
        "/Users/nadin/Documents/Graphics/Apps/Assets/pear_color.jpg";
GLuint textureName = 0; // id for texture image, set by ReadTexture
int textureUnit = 0; // id for GPU image buffer, may be freely set

// movable lights
vec3 lights[] = { {.5, 0, 1}, {1, 1, 0} };
const int nLights = sizeof(lights)/sizeof(vec3);

// interaction
void *picked = NULL;	// if non-null: light or camera
Mover mover;

// Shaders
const char *vertexShader = R"(
	#version 330
	in vec3 point;
    in vec3 normal;
	in vec2 uv;

	out vec3 vPoint;
	out vec2 vUv;
    out vec3 vNormal;
	uniform mat4 modelview, persp;

	void main() {
		vPoint = (modelview * vec4(point, 1)).xyz;
        vNormal = (modelview * vec4(normal,0)).xyz;
		gl_Position = persp * vec4(vPoint, 1);
		vUv = uv;
	}
)";

const char *pixelShader = R"(
	#version 330
	in vec3 vPoint;
	in vec2 vUv;
    in vec3 vNormal;
	out vec4 pColor;
	uniform sampler2D textureImage;
	uniform int nLights = 0;
	uniform vec3 lights[20];
	uniform float amb = .1, dif = .8, spc =.7;					// ambient, diffuse, specular
	void main() {
		float d = 0, s = 0;
		vec3 N = normalize(vNormal);						    // unit-length normal
		vec3 E = normalize(vPoint);								// eye vector
		for (int i = 0; i < nLights; i++) {
			vec3 L = normalize(lights[i]-vPoint);				// light vector
			vec3 R = reflect(L, N);					      	    // highlight vector
			d += max(0, dot(N, L));								// one-sided diffuse
			float h = max(0, dot(R, E));						// highlight term
			s += pow(h, 100);									// specular term
		}
		float ads = clamp(amb+dif*d+spc*s, 0, 1);
		pColor = vec4(ads*texture(textureImage, vUv).rgb, 1);
	}
)";

// Display
void Display(GLFWwindow *w) {
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

    // set pointers
    VertexAttribPointer(program, "point", 3, 0, (void*)0);
    VertexAttribPointer(program, "uv", 2, 0, (void*)(points.size() * sizeof(vec3)));
    VertexAttribPointer(program, "normal", 3, 0, (void*)((points.size() * sizeof(vec3)) + (uvs.size() * sizeof(vec2))));

    // update matrices and light
    SetUniform(program, "modelview", camera.modelview);
    SetUniform(program, "persp", camera.persp);
    SetUniform(program, "nLights", nLights);
    SetUniform3v(program, "lights", nLights, (float *) lights, camera.modelview);

    // bind 2D texture, activate appropriate texture unit (enable GPU buffer)
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, textureName);
    SetUniform(program, "textureImage", textureUnit);

    // render for MAC
     glDrawElements(GL_TRIANGLES, triangles.size() *3, GL_UNSIGNED_INT, 0);

    glDisable(GL_DEPTH_TEST);
    UseDrawShader(camera.fullview);
    for (int i = 0; i < nLights; i++)
        Star(lights[i], 8, vec3(1, .8f, 0), vec3(0, 0, 1));
    if (picked == &camera && !Shift())
        camera.arcball.Draw(Control());
    glFlush();
}

// Mouse Callbacks
void MouseButton(float x, float y, bool left, bool down) {
    picked = NULL;
    if (left && down) {
        // light picked?
        for (int i = 0; i < nLights; i++)
            if (MouseOver(x, y, lights[i], camera.fullview)) {
                picked = &mover;
                mover.Down(&lights[i], (int) x, (int) y, camera.modelview, camera.persp);
            }
        if (picked == NULL) {
            picked = &camera;
            camera.Down(x, y, Shift(), Control());
        }
    }
    else camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
    if (leftDown) {
        if (picked == &mover)
            mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
        if (picked == &camera)
            camera.Drag(x, y);
    }
}

void MouseWheel(float spin) {
    camera.Wheel(spin, Shift());
}

// Initialization
void BufferVertices() {
    // make GPU buffers for points and colors, set to active
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // allocate/load memory for points, uvs, normals
    size_t sPoints = points.size()*sizeof(vec3);
    size_t sUvs = uvs.size()*sizeof(vec2);
    size_t sNormals = normals.size()*sizeof(vec3);
    size_t sTotal = sPoints + sUvs + sNormals;
    glBufferData(GL_ARRAY_BUFFER, sTotal, NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sPoints, sUvs, uvs.data());
    glBufferSubData(GL_ARRAY_BUFFER, sPoints+sUvs, sNormals, normals.data());

    // make EBO to store triangles, set to active
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // copy triangle index data to GPU
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(int3),
                 triangles.data(), GL_STATIC_DRAW);
}

void Resize(int width, int height) {
    camera.Resize(width, height);
    glViewport(0, 0, width, height);
}

// Application
int main(int ac, char **av) {
    // read OBJ file
    if (!ReadAsciiObj(objFilename, points, triangles, &normals, &uvs))
        printf("can’t read %s\n", objFilename);

    // if normals are missing, generate them
    if (normals.empty()) {
        SetVertexNormals(points, triangles, normals);
    }

    // enable anti-alias, init app window and GL context
    GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "Smooth Mesh");

    // init shader program, set GPU buffer, read texture image
    program = LinkProgramViaCode(&vertexShader, &pixelShader);

    // fit modal into the window
    Standardize(points.data(), points.size(), .8f);

    // allocate vertex memory in the GPU
    BufferVertices();

    // read texture
    ReadTexture(textureFilename, &textureName);

    // callbacks
    RegisterMouseMove(MouseMove);
    RegisterMouseButton(MouseButton);
    RegisterMouseWheel(MouseWheel);
    RegisterResize(Resize);

    // event loop
    while (!glfwWindowShouldClose(w)) {
        glfwPollEvents();
        Display(w);
        glfwSwapBuffers(w);
    }

    // unbind vertex buffer, free GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &VBO);
    glfwDestroyWindow(w);
    glfwTerminate();
}



