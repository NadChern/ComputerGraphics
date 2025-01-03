// Author: Nadezhda Chernova
// File: Assn-10-Animation.cpp
// Date: 12/03/2024
// Animation of airplane along the flight path, as its propellers spins

#include "glad.h"
#include "glfw3.h"
#include "GLXtras.h"
#include "Camera.h"
#include "Draw.h"
#include "IO.h"
#include <stdio.h>
#include <vector>
#include <time.h>
#include <cmath>


// GPU program, window, camera
GLuint program = 0;
int winWidth = 800, winHeight = 800;
Camera camera(0, 0, winWidth, winHeight, vec3(15, -15, 0), vec3(0, 0, -5), 30);

// lights
vec3 lights[] = {{1,    -.2f, .4f},
                 {-.7f, .8f,  1},
                 {-.5f, -.2f, 1}};
int nLights = sizeof(lights) / sizeof(vec3);


// meshes
class HMesh {
public:
    vector<vec3> points, normals;     // vertices, normals
    vector<int3> triangles;           // facets
    GLuint VAO = 0, VBO = 0, EBO = 0; // vertex array object, vertex buffer, element buffer
    mat4 toWorld;                     // transformation to world space

    // read obj file, initialize GPU for rendering
    void Read(const char *dir, const char *objName) {
        // load mesh data from obj file
        string objFilename(string(dir) + string(objName));
        if (!ReadAsciiObj(objFilename.c_str(), points, triangles, &normals))
            printf("can't read %s\n", objFilename.c_str());
        else {
            // create vertex array object and buffer object
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);

            // allocate memory for vertex points and normals
            size_t sPts = points.size() * sizeof(vec3);
            size_t sNrms = normals.size() * sizeof(vec3);
            glBufferData(GL_ARRAY_BUFFER, sPts + sNrms, NULL,
                         GL_STATIC_DRAW);
            // load data
            glBufferSubData(GL_ARRAY_BUFFER, 0, sPts, points.data());
            glBufferSubData(GL_ARRAY_BUFFER, sPts, sNrms, normals.data());

            // copy triangle data to GPU
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         triangles.size() * sizeof(int3), triangles.data(),
                         GL_STATIC_DRAW);
        }
    }

    // render mesh
    void Render(const vec3 &color) {
        size_t sPts = points.size() * sizeof(vec3);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        VertexAttribPointer(program, "point", 3, 0, (void *) 0);
        VertexAttribPointer(program, "normal", 3, 0, (void *) sPts);
        SetUniform(program, "color", color);
        SetUniform(program, "modelview", camera.modelview * toWorld);
        SetUniform(program, "persp", camera.persp);
        glDrawElements(GL_TRIANGLES, 3 * triangles.size(), GL_UNSIGNED_INT, 0);
    }

    // constructor, initializes transformation matrix
    HMesh(mat4 m = mat4(1)) : toWorld(m) {}
};


// Bezier curve class
class Bezier {
public:
    vec3 *pts = NULL; // pointer to 4 control points
    // constructor
    Bezier(vec3 *pts) : pts(pts) {}

    // compute point on cubic Bezier curve for t in range [0,1]
    vec3 Position(float t) {
        float t2 = t * t, t3 = t * t2;
        return (-t3 + 3 * t2 - 3 * t + 1) * pts[0] +
               (3 * t3 - 6 * t2 + 3 * t) * pts[1] + (3 * t2 - 3 * t3) * pts[2] +
               t3 * pts[3];
    }

    // draw path, control points and control mesh
    void Draw(int res = 50, float curveWidth = 3.5f, float meshWidth = 2.5f) {
        vec3 lineColor(.7f, .2f, .5f), meshColor(0, 0, 1), pointColor(0, .7f,
                                                                      0);
        // draw curve as res number of straight segments
        for (int i = 0; i < res; i++)
            Line(Position((float) i / res), Position((float) (i + 1) / res),
                 curveWidth, lineColor);
        // draw control mesh
        for (int i = 0; i < 3; i++)
            LineDash(pts[i], pts[i + 1], meshWidth, meshColor, meshColor);
        // draw control points
        for (int i = 0; i < 4; i++)
            Disk(pts[i], 5 * curveWidth, pointColor);
    }

    vec3 Velocity(float t) {
        //derivative (tangent) of curve
        float t2 = t * t;
        return (-3 * t2 + 6 * t - 3) * pts[0] +
               (9 * t2 - 12 * t + 3) * pts[1] +
               (6 * t - 9 * t2) * pts[2] + 3 * t2 * pts[3];
    }

    mat4 Frame(float t) {
        vec3 v = normalize(Velocity(t)); //tangent to curve
        vec3 n = normalize(
                cross(v, vec3(0, 1, 0))); //normal (“right” wrt tangent)
        vec3 b = normalize(cross(n, v)); // binormal (“up” wrt tangent)
        vec3 p = Position(t);
        mat4 frame{
                {n[0], b[0], -v[0], p[0]},
                {n[1], b[1], -v[1], p[1]},
                {n[2], b[2], -v[2], p[2]},
                {0,    0,    0,     1}
        };
        return frame;
    }
};


// Mesh objects and solid colors for plane and prop
HMesh body, prop;
vec3 bodyColor(0, 1, 0);
vec3 propColor(1, 0, 0);

bool isFlightPathDisplayed = true;

vec3 path[] = {
        {2 / 3.f,  0,   2 / 3.f},
        {1,        0,   1 / 3.f},
        {1,        .1f, -1 / 3.f},        //curve1: path[0]-path[3]
        {2 / 3.f,  .1f, -2 / 3.f},
        {1 / 3.f,  .1f, -1},
        {-1 / 3.f, .4f, -1},              //curve2: path[3]-path[6]
        {-2 / 3.f, .4f, -2 / 3.f},
        {-1,       .4f, -1 / 3.f},
        {-1,       0,   1 / 3.f},        //curve3: path[6]-path[9]
        {-2 / 3.f, 0,   2 / 3.f},
        {-1 / 3.f, 0,   1},
        {1 / 3.f,  0,   1},
        {2 / 3.f,  0,   2 / 3.f}        //curve4: path[9]-path[12]
};

Bezier bezier[] = {Bezier(&path[0]), Bezier(&path[3]), Bezier(&path[6]),
                   Bezier(&path[9])};
const int nBezier = sizeof(bezier) / sizeof(Bezier);

time_t startTime = clock();
float duration = 3;
Mover mover;

void Animate() {
    float elapsed = (float) (clock() - startTime) / CLOCKS_PER_SEC;
    float alpha = nBezier * elapsed / duration;
    float beta = fmod(alpha, (float) nBezier);
    float t = beta - floor(beta);
    int i = (int) floor(beta);
    mat4 f = bezier[i].Frame(t);
    body.toWorld = f * Scale(.35f) * RotateY(-90);
    prop.toWorld =
            body.toWorld * Translate(-.6f, 0, 0) * RotateY(-90) * Scale(.25f) *
            RotateZ(1500 * elapsed);
}


// shaders
const char *vertexShader = R"(
	#version 410 core
	in vec3 point, normal;
	out vec3 vPoint, vNormal;
	uniform mat4 modelview, persp;
	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
		vNormal = (modelview*vec4(normal, 0)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
	}
)";

const char *pixelShader = R"(
	#version 410 core
	in vec3 vPoint, vNormal;
	uniform int nLights = 0;
	uniform vec3 lights[20];
	uniform vec3 color;
	out vec4 pColor;
	void main() {
		float d = 0, s = 0;							// diffuse, specular terms
		vec3 N = normalize(vNormal);
		vec3 E = normalize(vPoint);					// eye vector
		for (int i = 0; i < nLights; i++) {
			vec3 L = normalize(lights[i]-vPoint);	// light vector
			vec3 R = reflect(L, N);					// highlight vector
			d += max(0, dot(N, L));					// one-sided diffuse
			float h = max(0, dot(R, E));			// highlight term
			s += pow(h, 100);						// specular term
		}
		float ads = clamp(.1+.7*d+.7*s, 0, 1);
		pColor = vec4(ads*color, 1);
	}
)";

// Display
void Display() {
    // background, z-buffer
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(program);

    // lights
    SetUniform(program, "nLights", nLights);
    SetUniform3v(program, "lights", nLights, (float *) lights,
                 camera.modelview);

    // render plane and prop
    body.Render(bodyColor);
    prop.Render(propColor);

    if (isFlightPathDisplayed) {
        UseDrawShader(camera.fullview);
        for (auto &curve: bezier) {
            curve.Draw();
        }
    }

    glFlush();
}

// Mouse
void MouseButton(float x, float y, bool left, bool down) {
    if (left && down)
        camera.Down(x, y, Shift(), Control());
    if (left && !down)
        camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
    if (leftDown)
        camera.Drag(x, y);
}

void MouseWheel(float spin) {
    camera.Wheel(spin, Shift());
}

void Keyboard(int k, bool press, bool shift, bool control) {
    if (press) {
        if (k == 'P')
            isFlightPathDisplayed = !isFlightPathDisplayed;
    }
}

void Resize(int width, int height) {
    camera.Resize(width, height);
    glViewport(0, 0, width, height);
}

// Application
int main(int ac, char **av) {

    // init app, GPU program
    GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "Aerial Animation");
    program = LinkProgramViaCode(&vertexShader, &pixelShader);

    // read models
    body.Read("/Users/nadin/Documents/Graphics/Apps/Assets/",
              "Airplane-Body.obj");
    prop.Read("/Users/nadin/Documents/Graphics/Apps/Assets/",
              "Airplane-Propeller.obj");

    // callbacks
    RegisterMouseMove(MouseMove);
    RegisterMouseButton(MouseButton);
    RegisterMouseWheel(MouseWheel);
    RegisterResize(Resize);
    RegisterKeyboard(Keyboard);

    // event loop
    while (!glfwWindowShouldClose(w)) {
        Animate();
        Display();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }

    // unbind vertex buffer, free GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &body.VBO);
    glfwDestroyWindow(w);
    glfwTerminate();
}