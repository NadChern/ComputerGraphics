// Author: Nadezhda Chernova
// File: Assn-7-Hierarchy.cpp
// Date: 11/12/2024
// Hierarchy of meshes (dog, bird, hat)

#include "glad.h"
#include "glfw3.h"
#include <stdio.h>
#include <vector>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "IO.h"
#include <cmath>

// preset matrices
mat4 cameraM(-0.63f, 1.68f, 0.05f, -0.44f,
             -0.64f, -0.29f, 1.65f, -0.02f,
             1.55f, 0.56f, 0.70f, -5.00f,
             0.00f, 0.00f, 0.00f, 1.00f); // modelview

// hard-wired transformation matrices for each mesh
mat4 dogM(1.00f, 0.00f, 0.00f, -1.10f,
          0.00f, 1.00f, 0.00f, 0.00f,
          0.00f, 0.00f, 1.00f, -0.19f,
          0.00f, 0.00f, 0.00f, 1.00f);

mat4 birdM(-0.07f, -0.22f, -0.02f, -1.07f,
           0.22f, -0.07f, 0.02f, 0.36f,
           -0.03f, -0.01f, 0.23f, 0.22f,
           0.00f, 0.00f, 0.00f, 1.00f);

mat4 hatM(0.04f, 0.01f, 0.02f, -1.02f,
          0.02f, -0.00f, -0.04f, 0.22f,
          -0.01f, 0.05f, -0.00f, 0.37f,
          0.00f, 0.00f, 0.00f, 1.00f);

// GPU program, window, camera, colors
GLuint program = 0;
int winWidth = 1000, winHeight = 800;
Camera camera(0, 0, winWidth, winHeight, cameraM);
vec3 wht(1, 1, 1), red(1, 0, 0);

// lights
vec3 lights[] = {{1, -.2f,    .4f},
                 {-.7f, .8f,  1},
                 {-.5f, -.2f, 1}};
int nLights = sizeof(lights) / sizeof(vec3);

// meshes in hierarchy
class HMesh {
public:
    vector<vec3> points, normals;     // vertices
    vector<vec2> uvs;                 // texture
    vector<int3> triangles;           // facets
    GLuint VAO = 0, VBO = 0, EBO = 0; // vertex array object, vertex buffer, element buffer
    GLuint textureName = 0;           // texture map
    mat4 toWorld;                     // transformation to world space
    HMesh *child;                     // pointer to child mesh
    void Init(const char *dir, const char *objName, const char *texName,
              HMesh *parent) {
        // assign this mesh as a child to parent mesh
        if (parent)
            parent->child = this;
        // load mesh data from obj file
        string objFilename(string(dir) + string(objName));
        if (!ReadAsciiObj(objFilename.c_str(), points, triangles, &normals,
                          &uvs))
            printf("can't read %s\n", objFilename.c_str());
        else {
            // same initial size for all objects
            Standardize(points.data(), points.size());
            // create vertex array object and buffer object
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            // allocate memory for vertex points, colors, and normals
            size_t sPts = points.size() * sizeof(vec3), sNrms =
                    normals.size() * sizeof(vec3), sUvs =
                    uvs.size() * sizeof(vec2);
            glBufferData(GL_ARRAY_BUFFER, sPts + sNrms + sUvs, NULL,
                         GL_STATIC_DRAW);
            // load data
            glBufferSubData(GL_ARRAY_BUFFER, 0, sPts, points.data());
            glBufferSubData(GL_ARRAY_BUFFER, sPts, sNrms, normals.data());
            glBufferSubData(GL_ARRAY_BUFFER, sPts + sNrms, sUvs, uvs.data());
            // copy triangle data to GPU
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         triangles.size() * sizeof(int3), triangles.data(),
                         GL_STATIC_DRAW);
        }
        // load texture data
        string texFilename(string(dir) + string(texName));
        ReadTexture(texFilename.c_str(), &textureName);
    }

    // render mesh
    void Render(mat4 modelview) {
        size_t sPts = points.size() * sizeof(vec3), sNrms =
                normals.size() * sizeof(vec3);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        VertexAttribPointer(program, "point", 3, 0, (void *) 0);
        VertexAttribPointer(program, "normal", 3, 0, (void *) sPts);
        VertexAttribPointer(program, "uv", 2, 0, (void *) (sPts + sNrms));
        SetUniform(program, "modelview", modelview * toWorld);
        SetUniform(program, "textureName", textureName);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureName);
        glDrawElements(GL_TRIANGLES, 3 * triangles.size(), GL_UNSIGNED_INT, 0);
    }

    // apply transformation to mesh and its children(if any)
    void ApplyTransform(mat4 m) {
        toWorld = m * toWorld;
        if (child)
            child->ApplyTransform(m);
    }

    // get world space origin of mesh
    // return vec3(toWorld * vec4(0, 0, 0, 1)) works as well
    vec3 Origin() {
        // return vec3(0,0,0) transformed by toWorld
        vec4 v = toWorld * vec4(0, 0, 0, 1);
        return vec3(v.x, v.y, v.z);
    }

    // constructor, initializes transformation matrix
    HMesh(mat4 m = mat4(1)) : toWorld(m) {}
};

// define meshes with their transform matrices
HMesh dog(dogM), bird(birdM), hat(hatM), *meshes[] = {&dog, &bird,
                                                      &hat}, *pickedMesh = NULL;

// shaders
const char *vertexShader = R"(
	#version 410 core
	in vec3 point, normal;
	in vec2 uv;
	out vec3 vPoint, vNormal;
	out vec2 vUv;
	uniform mat4 modelview, persp;
	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
		vNormal = (modelview*vec4(normal, 0)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vUv = uv;
	}
)";
const char *pixelShader = R"(
	#version 410 core
	in vec3 vPoint, vNormal;
	in vec2 vUv;
	uniform int nLights = 0;
	uniform vec3 lights[20];
	uniform sampler2D textureName;
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
		vec3 c = texture(textureName, vUv).rgb;
		pColor = vec4(ads*c, 1);
	}
)";

// Display
void Display() {
    // background, z-buffer
    glClearColor(.4f, .4f, .8f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(program);
    // lights
    SetUniform(program, "nLights", nLights);
    SetUniform3v(program, "lights", nLights, (float *) lights,
                 camera.modelview);
    // scene
    SetUniform(program, "persp", camera.persp);
    for (HMesh *m: meshes)
        m->Render(camera.modelview);
    // markings
    glDisable(GL_DEPTH_TEST);
    UseDrawShader(camera.fullview);
    if (pickedMesh)
        Frame(pickedMesh->toWorld, camera.modelview, camera.persp, .2f, wht);
    for (HMesh *m: meshes)
        Disk(m->Origin(), 10, m == pickedMesh ? red : wht);
    if (camera.down)
        camera.Draw();
    glFlush();
}

// Mouse
void MouseButton(float x, float y, bool left, bool down) {
    if (left && down)
        camera.Down(x, y, Shift(), Control());
    if (left && !down)
        camera.Up();
    if (!left && down) {
        pickedMesh = NULL;
        for (HMesh *m: meshes)
            if (MouseOver(x, y, m->Origin(), camera.fullview))
                pickedMesh = m;
    }
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
    if (leftDown)
        camera.Drag(x, y);
}

void MouseWheel(float spin) {
    camera.Wheel(spin, Shift());
}

// Keyboard
// hold X, Y, or Z: LEFT/RIGHT arrows: move, UP/DOWN arrows: rotate
// s: scale larger, S: smaller
void TestKey() {
    //  call KeyDown() to test keys per usage message
    //  compute transformation and apply to picked mesh
    if (!pickedMesh) return;
    mat4 mT = mat4(1);

    // determine state of keys
    bool l = KeyDown(GLFW_KEY_LEFT), r = KeyDown(GLFW_KEY_RIGHT);
    bool u = KeyDown(GLFW_KEY_UP), d = KeyDown(GLFW_KEY_DOWN);
    bool x = KeyDown('X'), y = KeyDown('Y'), z = KeyDown('Z');
    bool s = KeyDown('S');
    bool shift = Shift();

    float rot = 60 * (M_PI / 180.0f);
    float translationStep = 0.01f;
    float scaleFactor = shift ? 0.9f : 1.1f;

    vec3 origin = pickedMesh->Origin();
    // translation and rotation based on X, Y, or Z key being held
    if (x) {
        if (l)
            mT = Translate(-translationStep, 0, 0) * mT;  // move left along X
        if (r)
            mT = Translate(translationStep, 0, 0) * mT;   // move right along X
        if (u)
            mT = Translate(origin) * RotateX(rot) * Translate(-origin) *
                 mT; // rotate up around X
        if (d)
            mT = Translate(origin) * RotateX(-rot) * Translate(-origin) *
                 mT; // rotate down around X
    } else if (y) {
        if (l)
            mT = Translate(0, -translationStep, 0) * mT;  // move left along Y
        if (r)
            mT = Translate(0, translationStep, 0) * mT;   // move right along Y
        if (u)
            mT = Translate(origin) * RotateY(rot) * Translate(-origin) *
                 mT; // rotate up around Y
        if (d)
            mT = Translate(origin) * RotateY(-rot) * Translate(-origin) *
                 mT; // rotate down around Y
    } else if (z) {
        if (l)
            mT = Translate(0, 0, -translationStep) * mT;  // move left along Z
        if (r)
            mT = Translate(0, 0, translationStep) * mT;   // move right along Z
        if (u)
            mT = Translate(origin) * RotateZ(rot) * Translate(-origin) *
                 mT; // rotate up around Z
        if (d)
            mT = Translate(origin) * RotateZ(-rot) * Translate(-origin) *
                 mT; // rotate down around Z
    }
    // scaling
    if (s) {
        mT = Scale(scaleFactor) * mT;
    }
    // apply transformation matrix to picked mesh
    pickedMesh->ApplyTransform(mT);
}

void MWrite(mat4 m, const char *name) {
    printf("mat4 %s(", name);
    for (vec4 &r: m.row)
        printf("%3.2f.f, %3.2f.f, %3.2f.f, %3.2f.f%s", r[0], r[1], r[2], r[3],
               r == m.row[3] ? ");\n" : ", ");
}

void Keyboard(int k, bool press, bool shift, bool control) {
    if (press) {
        if (k == 'R')
            dog.toWorld = bird.toWorld = hat.toWorld = mat4(1);
        if (k == 'P') {
            MWrite(dog.toWorld, "dog");
            MWrite(bird.toWorld, "bird");
            MWrite(hat.toWorld, "hat");
            MWrite(camera.modelview, "modelview");
        }
    }
}

// Application
void Resize(int width, int height) {
    camera.Resize(width, height);
    glViewport(0, 0, width, height);
}

const char *usage = R"(
    Left-mouse: camera
    Right-mouse: select mesh
    For selected mesh
        hold X, Y, or Z: LEFT/RIGHT arrows: move
                         UP/DOWN arrows: rotate
        s/S: scale
    R: set matrices to identity
    P: print matrices
)";

int main(int ac, char **av) {
    // init app, GPU program
    GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "Hierarchy");
    program = LinkProgramViaCode(&vertexShader, &pixelShader);
    // read models, textures, set hierarchy
    dog.Init("/Users/nadin/Documents/Graphics/Apps/Assets/", "Dog1.obj",
             "Dog1.jpg", NULL);
    bird.Init("/Users/nadin/Documents/Graphics/Apps/Assets/", "Bird.obj",
              "Bird.jpg", &dog);
    hat.Init("/Users/nadin/Documents/Graphics/Apps/Assets/", "Hat.obj",
             "Hat.png", &bird);
    // callbacks
    RegisterMouseMove(MouseMove);
    RegisterMouseButton(MouseButton);
    RegisterMouseWheel(MouseWheel);
    RegisterResize(Resize);
    RegisterKeyboard(Keyboard);
    printf("Usage:%s", usage);
    // event loop
    while (!glfwWindowShouldClose(w)) {
        TestKey();
        Display();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }
}