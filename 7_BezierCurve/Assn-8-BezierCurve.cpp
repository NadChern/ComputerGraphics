// Author: Nadezhda Chernova
// File: Assn-8-BezierCurve.cpp
// Date: 11/19/2024
// Cubic parametric 3D Bezier curve

#include <glad.h>     // GL loader
#include <glfw3.h>    // GL toolkit (create window + rendering context for it)
#include "GLXtras.h"  // convenience routines
#include "VecMat.h"   // library for vector/matrix operations
#include "Camera.h"   // camera class
#include "Draw.h"     // Screen drawing, Star
#include "Widgets.h"  // Mover
#include <vector>     // Dynamic arrays for mesh
#include <time.h>     // for ability to animate over time

// GPU identifiers
GLuint VAO = 0, VBO = 0, EBO = 0;    // vertex array, vertex buffer, element buffer
GLuint program = 0;                  // shader program ID

// display
int winWidth = 800, winHeight = 800;
Camera camera(0, 0, winWidth, winHeight, vec3(15, -15, 0), vec3(0, 0, -5), 30);

time_t startTime = clock();
float duration = 4; // seconds for animated dot back and forth

// Bezier curve class
class Bezier {
public:
    vector<vec3> ctrlPoints; // stores 4 control points
    const size_t SEGMENT_COUNT = 100; // segments for drawing the curve
    const int NUM_POINTS = 4; // number of points for cubic Bezier curve
    const float WIDTH = 1.0f; // width of curve
    const float OPACITY = 1.0f; // opacity of curve
    const vec3 COLOR_CURVE = vec3(0.75f, 0.0f, 0.0f); // color of curve
    const vec3 COLOR_POINT = vec3(0.0f, 1.0f, 0.0f); // color for control points
    const float DIAM_POINT = 25.0f; // diameter of control point
    const vec3 COLOR_DASHLINE = vec3(0.0f, 0.0f, 1.0f); // color for dash line
    const vec3 COLOR_DOT = vec3(1.0f, 0.0f, 0.0f);

    // constructor
    Bezier(const vector<vec3> &points) {
        if (points.size() != NUM_POINTS) {
            throw invalid_argument("Cubic Bezier curve requires 4 control "
                                   "points");
        }
        ctrlPoints = points;
    }

    // Compute point on cubic Bezier curve for t in range [0,1]
    vec3 ComputePointOnCurve(float t) {
        float s = 1 - t;
        float tt = t * t;
        float ss = s * s;
        float ttt = tt * t;
        float sss = ss * s;
        return (sss * ctrlPoints[0]) + (3 * ss * t * ctrlPoints[1]) +
               (3 * s * tt * ctrlPoints[2]) + ttt * ctrlPoints[3];
    }

    // draw cubic Bezier curve
    void DrawCurve() {
        // 100 segments (within a loop, each time calling Line
        vec3 p0 = ComputePointOnCurve(0);
        for (size_t i = 1; i <= SEGMENT_COUNT; i++) {
            float t = (float) (i) / (float) SEGMENT_COUNT;
            vec3 p1 = ComputePointOnCurve(t);
            Line(p0, p1, WIDTH, COLOR_CURVE, OPACITY);
            p0 = p1;
        }
    }

    // draw control points
    void DrawControlPoints() {
        for (const auto &point: ctrlPoints) {
            Disk(point, DIAM_POINT, COLOR_POINT, OPACITY);
        }
    }

    // draw control polygon
    void DrawControlPolygon() {
        const float DASH_LEN = 20.0f;
        const float PERCENT_DASH = 0.5f;
        for (size_t i = 0; i < NUM_POINTS - 1; i++) {
            LineDash(ctrlPoints[i], ctrlPoints[i + 1], WIDTH, COLOR_DASHLINE,
                     COLOR_DASHLINE,
                     OPACITY, DASH_LEN, PERCENT_DASH);
        }
    }

    // draw moving dot along the curve
    void DrawMovingDot() {
        float elapsedTime = (float) (clock() - startTime) / CLOCKS_PER_SEC;
        float alpha =
                (float) (sin(2 * 3.1415f * elapsedTime / duration) + 1) / 2;
        Disk(ComputePointOnCurve(alpha), DIAM_POINT, COLOR_DOT);
    }
};

// control points
vector<vec3> points = {{-1.0f, 1.0f,  0.0f},
                       {-1.0f, -1.0f, 0.0f},
                       {1.0,   1.0f,  0.0f},
                       {1.0f,  -1.0f, 0.0f}};
const int nPoints = points.size();

// interaction
void *picked = NULL;    // if non-null: light or camera
Mover mover;


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

    // draw Bezier curve, polygon, control points and moving dot along curve
    Bezier curve(points);
    UseDrawShader(camera.fullview);
    curve.DrawControlPolygon();
    curve.DrawControlPoints();
    curve.DrawCurve();
    curve.DrawMovingDot();
    glFlush();
}

// Mouse Callbacks
void MouseButton(float x, float y, bool left, bool down) {
    picked = NULL;
    if (left && down) {
        // light picked?
        for (int i = 0; i < nPoints; i++)
            if (MouseOver(x, y, points[i], camera.fullview)) {
                picked = &mover;
                mover.Down(&points[i], (int) x, (int) y, camera.modelview,
                           camera.persp);
            }
        if (picked == NULL) {
            picked = &camera;
            camera.Down(x, y, Shift(), Control());
        }
    } else camera.Up();
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

void Resize(int width, int height) {
    camera.Resize(width, height);
    glViewport(0, 0, width, height);
}

// Application
int main(int ac, char **av) {

    // enable anti-alias, init app window and GL context
    GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight,
                             "Cubic Bezier curve");

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