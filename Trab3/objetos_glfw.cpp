#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <string.h>
#include <iostream>

// estruturas fornecidas
typedef struct _pto3f {
     float x, y, z;
} tipPto3f;

typedef struct _vert {
     int n;     // numero de vértices
     tipPto3f *vets;     // vetor de vértices
} tipVertice;

typedef struct _face {
     int n;     // numero de arestas da face
     int *indAresta;     // indice de aresta
     float normal[3];
} tipFace;

typedef struct _objeto {
     int n;              // numero da faces
     tipFace *face;               // vetor de faces
     tipVertice *vertice;       // vetor de vértices
     int cor;                           // indice da cor do objeto (material index)
     int material;
     float pos[3];
     float rot[3];
     float scale;
     float *vnormals; // per-vertex normals (3 * nverts)
     // GL buffers
     GLuint vao, vbo; // interleaved vertex (pos,norm) buffer
     int vboVertexCount; // number of vertices to draw (triangles * 3)
} tipObjeto;

typedef struct _objetos {
     int m;                         // numero de objetos
     tipObjeto  *vobjs;     // vetor de objetos
} tipObjetos;

// Scene
tipObjetos scene = {0, NULL};

// Simple material definition (ambient, diffuse, specular, shininess)
struct Material { float ambient[3]; float diffuse[3]; float specular[3]; float shininess; };
std::vector<Material> Materials;

// Shader source (simple Phong)
const char* vertexShaderSrc = R"glsl(
#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 vPos;
out vec3 vNormal;

void main() {
    vec4 worldPos = model * vec4(inPos, 1.0);
    vPos = worldPos.xyz;
    vNormal = mat3(transpose(inverse(model))) * inNorm;
    gl_Position = proj * view * worldPos;
}
)glsl";

const char* fragmentShaderSrc = R"glsl(
#version 330 core
struct Light { vec3 pos; vec3 ambient; vec3 diffuse; vec3 specular; };
struct Material { vec3 ambient; vec3 diffuse; vec3 specular; float shininess; };

in vec3 vPos;
in vec3 vNormal;
out vec4 fragColor;

uniform Light light0;
uniform Light light1;
uniform Material mat;
uniform vec3 viewPos;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(viewPos - vPos);

    vec3 result = vec3(0.0);

    // light0
    vec3 L0 = normalize(light0.pos - vPos);
    float diff0 = max(dot(N, L0), 0.0);
    vec3 R0 = reflect(-L0, N);
    float spec0 = pow(max(dot(R0, V), 0.0), mat.shininess);
    result += light0.ambient * mat.ambient + light0.diffuse * (mat.diffuse * diff0) + light0.specular * (mat.specular * spec0);

    // light1
    vec3 L1 = normalize(light1.pos - vPos);
    float diff1 = max(dot(N, L1), 0.0);
    vec3 R1 = reflect(-L1, N);
    float spec1 = pow(max(dot(R1, V), 0.0), mat.shininess);
    result += light1.ambient * mat.ambient + light1.diffuse * (mat.diffuse * diff1) + light1.specular * (mat.specular * spec1);

    fragColor = vec4(result, 1.0);
}
)glsl";

// Utility: compile shader
GLuint CompileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(s, 1024, NULL, log);
        fprintf(stderr, "Shader compile error: %s\n", log);
    }
    return s;
}

// Utility: create program
GLuint CreateProgram(const char* vs, const char* fs) {
    GLuint vsId = CompileShader(GL_VERTEX_SHADER, vs);
    GLuint fsId = CompileShader(GL_FRAGMENT_SHADER, fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vsId);
    glAttachShader(prog, fsId);
    glLinkProgram(prog);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(prog, 1024, NULL, log);
        fprintf(stderr, "Program link error: %s\n", log);
    }
    glDeleteShader(vsId); glDeleteShader(fsId);
    return prog;
}

// Compute face normal for triangles
void ComputeFaceNormal(tipFace &f, tipVertice *verts) {
    if (f.n < 3) return;
    int i0 = f.indAresta[0], i1 = f.indAresta[1], i2 = f.indAresta[2];
    tipPto3f p0 = verts->vets[i0];
    tipPto3f p1 = verts->vets[i1];
    tipPto3f p2 = verts->vets[i2];
    float ux = p1.x - p0.x, uy = p1.y - p0.y, uz = p1.z - p0.z;
    float vx = p2.x - p0.x, vy = p2.y - p0.y, vz = p2.z - p0.z;
    float nx = uy*vz - uz*vy;
    float ny = uz*vx - ux*vz;
    float nz = ux*vy - uy*vx;
    float len = sqrt(nx*nx + ny*ny + nz*nz);
    if (len < 1e-8f) { f.normal[0]=f.normal[1]=f.normal[2]=0.0f; return; }
    f.normal[0]=nx/len; f.normal[1]=ny/len; f.normal[2]=nz/len;
}

// Compute vertex normals (average of adjacent faces)
void ComputeVertexNormals(tipObjeto &o) {
    int nv = o.vertice->n;
    o.vnormals = (float*) malloc(sizeof(float)*3*nv);
    for (int i=0;i<3*nv;i++) o.vnormals[i]=0.0f;
    for (int fi=0; fi<o.n; fi++) {
        tipFace &f = o.face[fi];
        for (int k=0;k<f.n;k++) {
            int vid = f.indAresta[k];
            o.vnormals[3*vid+0] += f.normal[0];
            o.vnormals[3*vid+1] += f.normal[1];
            o.vnormals[3*vid+2] += f.normal[2];
        }
    }
    for (int i=0;i<nv;i++) {
        float *vn = &o.vnormals[3*i];
        float len = sqrt(vn[0]*vn[0] + vn[1]*vn[1] + vn[2]*vn[2]);
        if (len < 1e-8f) { vn[0]=0; vn[1]=0; vn[2]=1; }
        else { vn[0]/=len; vn[1]/=len; vn[2]/=len; }
    }
}

// Make triangle face utility
tipFace MakeTri(int a, int b, int c) {
    tipFace f;
    f.n = 3;
    f.indAresta = (int*) malloc(sizeof(int)*3);
    f.indAresta[0]=a; f.indAresta[1]=b; f.indAresta[2]=c;
    f.normal[0]=f.normal[1]=f.normal[2]=0.0f;
    return f;
}

// Free object (including GL buffers)
void FreeObjeto(tipObjeto &o) {
    if (o.face) {
        for (int i=0;i<o.n;i++) if (o.face[i].indAresta) free(o.face[i].indAresta);
        free(o.face); o.face=NULL;
    }
    if (o.vertice) {
        if (o.vertice->vets) free(o.vertice->vets);
        free(o.vertice); o.vertice=NULL;
    }
    if (o.vnormals) { free(o.vnormals); o.vnormals=NULL; }
    if (o.vbo) { glDeleteBuffers(1, &o.vbo); o.vbo = 0; }
    if (o.vao) { glDeleteVertexArrays(1, &o.vao); o.vao = 0; }
}

// Add object to scene (copy struct by value)
void AddObjetoToScene(tipObjeto &o) {
    scene.vobjs = (tipObjeto*) realloc(scene.vobjs, sizeof(tipObjeto)*(scene.m+1));
    scene.vobjs[scene.m] = o;
    scene.m++;
}

// Create VBO/VAO from object's faces/vertices (interleaved pos+norm)
void CreateGLBuffers(tipObjeto &o) {
    // build float array of size (triangles*3) * (3+3)
    int triCount = o.n;
    int totalVerts = triCount * 3;
    std::vector<float> data; data.reserve(totalVerts * 6);
    for (int fi=0; fi<o.n; fi++) {
        tipFace &f = o.face[fi];
        for (int k=0;k<3;k++) {
            int vid = f.indAresta[k];
            tipPto3f p = o.vertice->vets[vid];
            float nx = o.vnormals[3*vid+0];
            float ny = o.vnormals[3*vid+1];
            float nz = o.vnormals[3*vid+2];
            data.push_back(p.x); data.push_back(p.y); data.push_back(p.z);
            data.push_back(nx); data.push_back(ny); data.push_back(nz);
        }
    }
    // create VAO/VBO
    glGenVertexArrays(1, &o.vao);
    glGenBuffers(1, &o.vbo);
    glBindVertexArray(o.vao);
    glBindBuffer(GL_ARRAY_BUFFER, o.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*data.size(), data.data(), GL_STATIC_DRAW);
    // pos location 0, norm location 1
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, (void*)(sizeof(float)*3));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    o.vboVertexCount = totalVerts;
}

// Create basic geometries (using given data structures)
tipObjeto CreateCube(float size, int materialIndex, float px, float py, float pz) {
    tipObjeto o; memset(&o, 0, sizeof(o));
    float s = size*0.5f;
    int nv = 8;
    o.vertice = (tipVertice*) malloc(sizeof(tipVertice));
    o.vertice->n = nv;
    o.vertice->vets = (tipPto3f*) malloc(sizeof(tipPto3f)*nv);
    tipPto3f v[8] = {{-s,-s,-s},{s,-s,-s},{s,s,-s},{-s,s,-s},{-s,-s,s},{s,-s,s},{s,s,s},{-s,s,s}};
    for (int i=0;i<nv;i++) o.vertice->vets[i]=v[i];
    o.n = 12;
    o.face = (tipFace*) malloc(sizeof(tipFace)*o.n);
    int tri[12][3] = {{0,1,2},{0,2,3},{4,7,6},{4,6,5},{0,4,5},{0,5,1},{1,5,6},{1,6,2},{2,6,7},{2,7,3},{3,7,4},{3,4,0}};
    for (int i=0;i<o.n;i++) o.face[i] = MakeTri(tri[i][0], tri[i][1], tri[i][2]);
    o.cor = materialIndex; o.material = materialIndex;
    o.pos[0]=px; o.pos[1]=py; o.pos[2]=pz;
    o.rot[0]=o.rot[1]=o.rot[2]=0.0f; o.scale=1.0f;
    for (int i=0;i<o.n;i++) ComputeFaceNormal(o.face[i], o.vertice);
    ComputeVertexNormals(o);
    CreateGLBuffers(o);
    return o;
}

tipObjeto CreatePyramid(float size, int materialIndex, float px, float py, float pz) {
    tipObjeto o; memset(&o, 0, sizeof(o));
    float s = size*0.5f;
    int nv = 5;
    o.vertice = (tipVertice*) malloc(sizeof(tipVertice));
    o.vertice->n = nv;
    o.vertice->vets = (tipPto3f*) malloc(sizeof(tipPto3f)*nv);
    tipPto3f v[5] = {{-s,0,-s},{s,0,-s},{s,0,s},{-s,0,s},{0,size,0}};
    for (int i=0;i<nv;i++) o.vertice->vets[i]=v[i];
    o.n = 6;
    o.face = (tipFace*) malloc(sizeof(tipFace)*o.n);
    int tri[6][3] = {{0,1,2},{0,2,3},{0,4,1},{1,4,2},{2,4,3},{3,4,0}};
    for (int i=0;i<o.n;i++) o.face[i]=MakeTri(tri[i][0], tri[i][1], tri[i][2]);
    o.cor = materialIndex; o.material = materialIndex;
    o.pos[0]=px; o.pos[1]=py; o.pos[2]=pz;
    o.rot[0]=o.rot[1]=o.rot[2]=0.0f; o.scale=1.0f;
    for (int i=0;i<o.n;i++) ComputeFaceNormal(o.face[i], o.vertice);
    ComputeVertexNormals(o);
    CreateGLBuffers(o);
    return o;
}

tipObjeto CreateUVSphere(float radius, int slices, int stacks, int materialIndex, float px, float py, float pz) {
    tipObjeto o; memset(&o, 0, sizeof(o));
    int nv = (stacks+1)*(slices+1);
    o.vertice = (tipVertice*) malloc(sizeof(tipVertice));
    o.vertice->n = nv;
    o.vertice->vets = (tipPto3f*) malloc(sizeof(tipPto3f)*nv);
    int idx=0;
    for (int i=0;i<=stacks;i++) {
        float v = (float)i / stacks;
        float phi = v * M_PI;
        for (int j=0;j<=slices;j++) {
            float u = (float)j / slices;
            float theta = u * (M_PI*2.0f);
            float x = radius * sinf(phi) * cosf(theta);
            float y = radius * cosf(phi);
            float z = radius * sinf(phi) * sinf(theta);
            o.vertice->vets[idx++] = {x,y,z};
        }
    }
    int nf = stacks * slices * 2;
    o.n = nf;
    o.face = (tipFace*) malloc(sizeof(tipFace)*o.n);
    int fidx = 0;
    for (int i=0;i<stacks;i++) {
        for (int j=0;j<slices;j++) {
            int a = i*(slices+1) + j;
            int b = a + slices + 1;
            o.face[fidx++] = MakeTri(a, b, a+1);
            o.face[fidx++] = MakeTri(a+1, b, b+1);
        }
    }
    o.cor = materialIndex; o.material = materialIndex;
    o.pos[0]=px; o.pos[1]=py; o.pos[2]=pz;
    o.rot[0]=o.rot[1]=o.rot[2]=0.0f; o.scale=1.0f;
    for (int i=0;i<o.n;i++) ComputeFaceNormal(o.face[i], o.vertice);
    ComputeVertexNormals(o);
    CreateGLBuffers(o);
    return o;
}

tipObjeto CreateCylinder(float radius, float height, int slices, int materialIndex, float px, float py, float pz) {
    tipObjeto o; memset(&o, 0, sizeof(o));
    int ring = slices+1;
    int nv = ring*2 + 2;
    o.vertice = (tipVertice*) malloc(sizeof(tipVertice));
    o.vertice->n = nv;
    o.vertice->vets = (tipPto3f*) malloc(sizeof(tipPto3f)*nv);
    int idx=0;
    float half = height*0.5f;
    for (int j=0;j<=slices;j++) {
        float theta = (float)j / slices * 2.0f * M_PI;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);
        o.vertice->vets[idx++] = {x, -half, z};
    }
    for (int j=0;j<=slices;j++) {
        float theta = (float)j / slices * 2.0f * M_PI;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);
        o.vertice->vets[idx++] = {x, half, z};
    }
    o.vertice->vets[idx++] = {0.0f, -half, 0.0f};
    o.vertice->vets[idx++] = {0.0f, half, 0.0f};
    o.n = slices*2 + slices*2;
    o.face = (tipFace*) malloc(sizeof(tipFace)*o.n);
    int fidx=0;
    for (int j=0;j<slices;j++) {
        int b0 = j;
        int b1 = (j+1);
        int t0 = ring + j;
        int t1 = ring + (j+1);
        o.face[fidx++] = MakeTri(b0, t0, b1);
        o.face[fidx++] = MakeTri(b1, t0, t1);
    }
    int centerBottom = nv-2;
    int centerTop = nv-1;
    for (int j=0;j<slices;j++) {
        int b0 = j;
        int b1 = (j+1);
        o.face[fidx++] = MakeTri(centerBottom, b1, b0);
    }
    for (int j=0;j<slices;j++) {
        int t0 = ring + j;
        int t1 = ring + (j+1);
        o.face[fidx++] = MakeTri(centerTop, t0, t1);
    }
    o.cor = materialIndex; o.material = materialIndex;
    o.pos[0]=px; o.pos[1]=py; o.pos[2]=pz;
    o.rot[0]=o.rot[1]=o.rot[2]=0.0f; o.scale=1.0f;
    for (int i=0;i<o.n;i++) ComputeFaceNormal(o.face[i], o.vertice);
    ComputeVertexNormals(o);
    CreateGLBuffers(o);
    return o;
}

// Setup materials
void initMaterials() {
    Materials.clear();
    Materials.push_back({{0.329412f,0.223529f,0.027451f},{0.780392f,0.568627f,0.113725f},{0.992157f,0.941176f,0.807843f},27.0f}); // brass
    Materials.push_back({{0.2f,0.0f,0.0f},{0.8f,0.0f,0.0f},{0.9f,0.6f,0.6f},64.0f}); // red
    Materials.push_back({{0.0f,0.05f,0.0f},{0.4f,0.5f,0.4f},{0.04f,0.7f,0.04f},10.0f}); // green
    Materials.push_back({{0.0f,0.0f,0.05f},{0.1f,0.3f,0.8f},{0.6f,0.6f,0.7f},50.0f}); // blue
    Materials.push_back({{0.25f,0.25f,0.25f},{0.4f,0.4f,0.4f},{0.774597f,0.774597f,0.774597f},76.8f}); // chrome
}

// Create scene (several objects)
void SetupScene() {
    // free old
    for (int i=0;i<scene.m;i++) FreeObjeto(scene.vobjs[i]);
    free(scene.vobjs); scene.vobjs=NULL; scene.m=0;
    initMaterials();
    tipObjeto cube = CreateCube(1.2f, 1, -2.5f, 0.0f, 0.0f);
    AddObjetoToScene(cube);
    tipObjeto pyr = CreatePyramid(1.4f, 2, 0.0f, -0.5f, 0.0f);
    AddObjetoToScene(pyr);
    tipObjeto sph = CreateUVSphere(0.9f, 32, 16, 3, 2.5f, 0.0f, 0.0f);
    AddObjetoToScene(sph);
    tipObjeto cyl = CreateCylinder(0.6f, 2.0f, 32, 4, 0.0f, 0.0f, -2.0f);
    AddObjetoToScene(cyl);
}

// Simple math helpers (matrices)
void Mat4Identity(float *m) { memset(m,0,16*sizeof(float)); m[0]=m[5]=m[10]=m[15]=1.0f; }
void Mat4Translate(float *m, float x, float y, float z) { Mat4Identity(m); m[12]=x; m[13]=y; m[14]=z; }
void Mat4Scale(float *m, float s) { Mat4Identity(m); m[0]=m[5]=m[10]=s; }
void Mat4RotateY(float *m, float angleDeg) {
    float a = angleDeg * (M_PI/180.0f);
    Mat4Identity(m);
    m[0]= cos(a); m[2]= sin(a);
    m[8]= -sin(a); m[10]= cos(a);
}
void Mat4Multiply(float *res, const float *a, const float *b) {
    float tmp[16];
    for (int r=0;r<4;r++) for (int c=0;c<4;c++) {
        tmp[c+r*4] = a[r*4+0]*b[0*4+c] + a[r*4+1]*b[1*4+c] + a[r*4+2]*b[2*4+c] + a[r*4+3]*b[3*4+c];
    }
    memcpy(res, tmp, 16*sizeof(float));
}

// Camera
float camDist = 8.0f;
float camYaw = 20.0f, camPitch = -20.0f;
double lastX=0, lastY=0;
bool dragging=false;

// Globals
GLuint programId = 0;
int winW=1280, winH=720;
float light0_pos[3] = {5.0f, 8.0f, 10.0f};
float light1_pos[3] = {-6.0f, -4.0f, 5.0f};

// Render one object
void RenderObjeto(const tipObjeto &o, GLuint prog, const float *viewMat, const float *projMat, const float *viewPos) {
    glUseProgram(prog);
    // model matrix = translate * rotateY * scale (simple)
    float T[16], R[16], S[16], M1[16], M[16];
    Mat4Translate(T, o.pos[0], o.pos[1], o.pos[2]);
    Mat4RotateY(R, o.rot[1]);
    Mat4Scale(S, o.scale);
    Mat4Multiply(M1, T, R);
    Mat4Multiply(M, M1, S);

    GLint locModel = glGetUniformLocation(prog, "model");
    GLint locView = glGetUniformLocation(prog, "view");
    GLint locProj = glGetUniformLocation(prog, "proj");
    glUniformMatrix4fv(locModel, 1, GL_FALSE, M);
    glUniformMatrix4fv(locView, 1, GL_FALSE, viewMat);
    glUniformMatrix4fv(locProj, 1, GL_FALSE, projMat);

    // set lights
    GLint l0pos = glGetUniformLocation(prog, "light0.pos");
    GLint l0amb = glGetUniformLocation(prog, "light0.ambient");
    GLint l0dif = glGetUniformLocation(prog, "light0.diffuse");
    GLint l0spec = glGetUniformLocation(prog, "light0.specular");
    GLint l1pos = glGetUniformLocation(prog, "light1.pos");
    GLint l1amb = glGetUniformLocation(prog, "light1.ambient");
    GLint l1dif = glGetUniformLocation(prog, "light1.diffuse");
    GLint l1spec = glGetUniformLocation(prog, "light1.specular");

    float la0[3] = {0.1f, 0.1f, 0.1f};
    float ld0[3] = {0.8f, 0.8f, 0.8f};
    float ls0[3] = {1.0f, 1.0f, 1.0f};
    float la1[3] = {0.05f, 0.05f, 0.08f};
    float ld1[3] = {0.4f, 0.4f, 0.6f};
    float ls1[3] = {0.6f, 0.6f, 0.8f};

    glUniform3fv(l0pos, 1, light0_pos);
    glUniform3fv(l0amb, 1, la0);
    glUniform3fv(l0dif, 1, ld0);
    glUniform3fv(l0spec, 1, ls0);

    glUniform3fv(l1pos, 1, light1_pos);
    glUniform3fv(l1amb, 1, la1);
    glUniform3fv(l1dif, 1, ld1);
    glUniform3fv(l1spec, 1, ls1);

    // material
    Material mat = Materials[o.material % Materials.size()];
    GLint mAmb = glGetUniformLocation(prog, "mat.ambient");
    GLint mDif = glGetUniformLocation(prog, "mat.diffuse");
    GLint mSpec = glGetUniformLocation(prog, "mat.specular");
    GLint mSh = glGetUniformLocation(prog, "mat.shininess");
    glUniform3fv(mAmb, 1, mat.ambient);
    glUniform3fv(mDif, 1, mat.diffuse);
    glUniform3fv(mSpec, 1, mat.specular);
    glUniform1f(mSh, mat.shininess);

    // view pos
    GLint vpos = glGetUniformLocation(prog, "viewPos");
    glUniform3fv(vpos, 1, viewPos);

    // draw
    glBindVertexArray(o.vao);
    glDrawArrays(GL_TRIANGLES, 0, o.vboVertexCount);
    glBindVertexArray(0);
    glUseProgram(0);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    winW = width; winH = (height==0?1:height);
    glViewport(0,0,winW,winH);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button==GLFW_MOUSE_BUTTON_LEFT) {
        if (action==GLFW_PRESS) { dragging=true; glfwGetCursorPos(window, &lastX, &lastY); }
        else dragging=false;
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!dragging) return;
    double dx = xpos - lastX;
    double dy = ypos - lastY;
    camYaw += dx * 0.2f;
    camPitch += dy * 0.2f;
    if (camPitch > 89) camPitch = 89;
    if (camPitch < -89) camPitch = -89;
    lastX = xpos; lastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camDist -= yoffset * 0.5f;
    if (camDist < 1.0f) camDist = 1.0f;
    if (camDist > 50.0f) camDist = 50.0f;
}

int main(int argc, char** argv) {
    if (!glfwInit()) { fprintf(stderr, "Failed to init GLFW\n"); return -1; }
    // Request core profile 3.3 for portability
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(winW, winH, "Objetos - GLFW + GLEW (Wayland compatible)", NULL, NULL);
    if (!window) { fprintf(stderr, "Failed to create GLFW window\n"); glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    // Init GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { fprintf(stderr, "Failed to init GLEW\n"); glfwDestroyWindow(window); glfwTerminate(); return -1; }

    // callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glEnable(GL_DEPTH_TEST);

    // create shader program
    programId = CreateProgram(vertexShaderSrc, fragmentShaderSrc);

    // build scene
    SetupScene();

    // simple ground as VAO (no lighting, just dark quad in shader by making a big object could be more work)
    // main loop
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        double dt = now - lastTime; lastTime = now;

        // rotate objects slowly
        for (int i=0;i<scene.m;i++) scene.vobjs[i].rot[1] += 20.0f * dt * (i+1);

        glClearColor(0.12f,0.12f,0.12f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // camera matrices (simple lookAt and perspective)
        float aspect = (float)winW / (float)winH;
        float proj[16]; Mat4Identity(proj);
        float fovy = 45.0f * (M_PI/180.0f);
        float f = 1.0f / tanf(fovy*0.5f);
        proj[0] = f / aspect; proj[5] = f; proj[10] = - (100.0f + 0.1f) / (100.0f - 0.1f);
        proj[11] = -1.0f; proj[14] = - (2.0f * 100.0f * 0.1f) / (100.0f - 0.1f); proj[15]=0.0f;

        // compute camera position from spherical coords
        float eyex = camDist * cosf(camPitch*(M_PI/180.0f)) * sinf(camYaw*(M_PI/180.0f));
        float eyey = camDist * sinf(camPitch*(M_PI/180.0f));
        float eyez = camDist * cosf(camPitch*(M_PI/180.0f)) * cosf(camYaw*(M_PI/180.0f));

        // view matrix (lookAt simple)
        float view[16]; Mat4Identity(view);
        // compute forward and right vectors
        float cx = 0.0f, cy = 0.0f, cz = 0.0f;
        float fx = cx - eyex, fy = cy - eyey, fz = cz - eyez;
        float flen = sqrtf(fx*fx + fy*fy + fz*fz); if (flen==0) flen=1;
        fx/=flen; fy/=flen; fz/=flen;
        float upx=0, upy=1, upz=0;
        // right = cross(forward, up)
        float rx = fy*upz - fz*upy;
        float ry = fz*upx - fx*upz;
        float rz = fx*upy - fy*upx;
        float rlen = sqrtf(rx*rx + ry*ry + rz*rz); if (rlen==0) rlen=1;
        rx/=rlen; ry/=rlen; rz/=rlen;
        // recompute up = cross(right, forward)
        float ux = ry*fz - rz*fy;
        float uy = rz*fx - rx*fz;
        float uz = rx*fy - ry*fx;
        // view matrix fill (column-major)
        view[0]=rx; view[4]=ux; view[8]=-fx; view[12]=0;
        view[1]=ry; view[5]=uy; view[9]=-fy; view[13]=0;
        view[2]=rz; view[6]=uz; view[10]=-fz; view[14]=0;
        view[3]=0;  view[7]=0;  view[11]=0;  view[15]=1;
        // translate camera
        float trans[16]; Mat4Translate(trans, -eyex, -eyey, -eyez);
        Mat4Multiply(view, view, trans);

        float viewPos[3] = {eyex, eyey, eyez};

        // draw each object
        for (int i=0;i<scene.m;i++) {
            RenderObjeto(scene.vobjs[i], programId, view, proj, viewPos);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    for (int i=0;i<scene.m;i++) FreeObjeto(scene.vobjs[i]);
    free(scene.vobjs); scene.vobjs = NULL; scene.m = 0;
    glDeleteProgram(programId);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
