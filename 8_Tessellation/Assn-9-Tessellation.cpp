// Author: Nadezhda Chernova
// File: Assn-9-Tessellation.cpp
// Date: 11/26/2024
// Use tessellation shader to display a texture-mapped cone and torus,
// which interpolate over time between each other

#include "glad.h"     // GL loader
#include "glfw3.h"    // GL toolkit (create window + rendering context for it)
#include "GLXtras.h"  // Convenience routines
#include "VecMat.h"   // Library for vector/matrix operations
#include "Camera.h"   // Camera control
#include "Draw.h"     // Screen drawing, Star
#include "IO.h"       // Input/output handling
#include "Widgets.h"  // mover, arcball
#include <time.h>     // for ability to animate over time

// GPU identifiers
GLuint VBO = 0, EBO = 0; // vertex and element buffers
GLuint program = 0;      // shader program ID

// display parameters
int winWidth = 800, winHeight = 600;
Camera camera(0, 0, winWidth, winHeight, vec3(0, 0, 0), vec3(0, 0, -6));
vec3 red(1, 0, 0), blu(0, 0, 1);

// texture parameters
GLuint textureName = 0; // texture ID
int textureUnit = 0;
const char *textureFilename = "/Users/nadin/Documents/Graphics/Apps/Assets/Chessboard.tga";

// interaction parameters
vec3 light(-1.4f, 1.f, 1.f);  // light source position
void *picked = NULL;          // if non-null: light or camera
Mover mover;

// time of animation
time_t startTime = clock();
float PI = 3.141592;
float duration = 4.0; // duration of interpolation

// vertex shader (no operations)
const char *vShader = R"(
	#version 410 core
	void main() { }
)";

// tessellation evaluation shader for cone and torus
const char *teShader = R"(
	#version 410 core
	layout (quads, equal_spacing, ccw) in; // use quads, counter-clockwise
	uniform mat4 modelview, persp;
    uniform float innerRadius = 1, outerRadius = 1;
    uniform float alpha; // interpolation factor
	out vec3 point, normal;
	out vec2 uv;
	float PI = 3.141592;

    // Rotate 2d point around y-axis
	vec3 RotateAboutY(vec2 p, float radians) {
		return vec3(cos(radians)*p.x, p.y, sin(radians)*p.x);
	}

    // Compute cone
    void Slant(float v, out vec2 p, out vec2 n) {
        p = vec2((1-v)*innerRadius, 2*v-1);
        n = normalize(vec2(2, -innerRadius));
    }

    // Compute torus
    void Circle(float v, out vec2 p, out vec2 n) {
        float angle = 2 *PI*v -PI, c = cos(angle), s = sin(angle);
        p = innerRadius*vec2(c, s);
        n = vec2(c, s);
    }

	void main() {
		uv = gl_TessCoord.st; // unique TessCoord for each invocation
			// u (0 to 1) longitude 0 to 2PI
			// v (0 to 1) latitude -PI/2 (S pole) to PI/2 (N pole)

        // Cone
        vec2 xp1, xn1;
        Slant(uv.y, xp1, xn1);
        vec3 p1 = RotateAboutY(xp1, uv.x*2*PI);
		vec3 n1 = RotateAboutY(xn1, uv.x*2*PI);

        // Torus
        vec2 xp2, xn2;
        Circle(uv.y, xp2, xn2);
        xp2.x += outerRadius;
        vec3 p2 = RotateAboutY(xp2, uv.x*2*PI);
        vec3 n2 = RotateAboutY(xn2, uv.x*2*PI);

        // Interpolate between cone and torus based on alpha
        vec3 p3 = mix(p1, p2, alpha);
        vec3 n3 = normalize(mix(n1, n2, alpha));

        point = (modelview*vec4(p3, 1)).xyz;		// transform point to ws
		normal = (modelview*vec4(n3, 0)).xyz;		// transform normal to ws

        gl_Position = persp*vec4(point, 1);        // transform to clip space
	}
)";

// pixel shader
const char *pShader = R"(
	#version 410 core
	in vec3 point, normal;
	in vec2 uv;
	out vec4 pColor;
	uniform sampler2D textureMap;
	uniform vec3 light;
	void main() {
		vec3 N = normalize(normal);					// surface normal
		vec3 L = normalize(light-point);			// light vector
		vec3 E = normalize(point);					// eye vertex
		vec3 R = reflect(L, N);						// highlight vector
		float dif = max(0, dot(N, L));				// one-sided diffuse
		float spec = pow(max(0, dot(E, R)), 50);    // specular
		float ad = clamp(.8+dif, 0, 1);            // amb + diffuse
		vec3 texColor = texture(textureMap, uv).rgb;
		pColor = vec4(ad*texColor+vec3(spec), 1);   // combine light + texture
	}
)";

// display

void Display() {
    float elapsedTime = (float)(clock() - startTime) / CLOCKS_PER_SEC;
    float alpha = (float)(sin(2 * PI * elapsedTime / duration) + 1) / 2;

    // background, zbuffer, anti-alias lines
    glClearColor(.6f, .6f, .6f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // send alpha and matrices to tessellation shader
    SetUniform(program, "alpha", alpha);
    SetUniform(program, "modelview", camera.modelview);
    SetUniform(program, "persp", camera.persp);

    // send transformed light to pixel shader
    SetUniform3v(program, "light", 1, (float *) &light, camera.modelview);

    // set texture
    glActiveTexture(GL_TEXTURE0 + textureUnit); // active texture corresponds with textureUnit
    glBindTexture(GL_TEXTURE_2D, textureName); // bind active texture to textureName
    SetUniform(program, "textureMap", textureUnit);

    // draw 4-sided, tessellated patch
    glPatchParameteri(GL_PATCH_VERTICES, 4);
    float res = 64, outerLevels[] = {res, res, res, res}, innerLevels[] = {res,
                                                                           res};
    glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outerLevels);
    glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, innerLevels);
    glDrawArrays(GL_PATCHES, 0, 4);

    // draw arcball, light
    glDisable(GL_DEPTH_TEST);
    if (picked == &camera && !camera.shift)
        camera.arcball.Draw(camera.control);
    UseDrawShader(camera.fullview);
    Star(light, 9, red, blu);
    glFlush();
}

// mouse callbacks
void MouseButton(float x, float y, bool left, bool down) {
    picked = NULL;
    if (left && !down)
        camera.Up();
    if (left && down) {
        if (MouseOver(x, y, light, camera.fullview)) {
            mover.Down(&light, (int) x, (int) y, camera.modelview,
                       camera.persp);
            picked = &mover;
        }
        if (!picked) {
            camera.Down(x, y, Shift());
            picked = &camera;
        }
    }
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
    if (leftDown && picked == &mover)
        mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
    if (leftDown && picked == &camera)
        camera.Drag((int) x, (int) y);
}

void MouseWheel(float spin) {
    camera.Wheel(spin, Shift());
}


void Resize(int width, int height) {
    camera.Resize(winWidth = width, winHeight = height);
    glViewport(0, 0, width, height);
}

// application
int main(int ac, char **av) {
    // init app window, OpenGL, shader program, texture
    GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight,
                             "Tessellate Cone and Torus");
    program = LinkProgramViaCode(&vShader, NULL, &teShader, NULL, &pShader);
    ReadTexture(textureFilename, &textureName);

    // callbacks
    RegisterMouseMove(MouseMove);
    RegisterMouseButton(MouseButton);
    RegisterMouseWheel(MouseWheel);
    RegisterResize(Resize);

    // event loop
    while (!glfwWindowShouldClose(w)) {
        Display();
        glfwPollEvents();
        glfwSwapBuffers(w);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &VBO);
    glfwDestroyWindow(w);
    glfwTerminate();
}
