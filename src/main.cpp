
#include "X11/keysym.h"
#include "framebuffer.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "imgui/backends/imgui_impl_glut.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/imgui.h"
#include "mat4c2gl.h"
#include "readfile.h"
#include <GL/freeglut.h>
#include <GL/glew.h>
#include <X11/Xlib.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <unistd.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

#define WINDOW_TITLE "Assingment 4"

#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

const char *filename = "cube_text.in";

#define CCW 1 // If one then render as counter-clockwise

#define BUFFER_OFFSET(offset) ((GLvoid *)(offset))
enum { VtxBuffer, NormBuffer, TexBuffer, NumBuffers };
enum Attrib_IDs { vPosition = 0, vNormalVertex = 1, vTexCoord = 2 };
GLint shaderProgram, WindowWidth = 800, WindowHeight = 600;
GLuint VAO;
GLuint Buffers[NumBuffers];
GLuint NumVertices;
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 20.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.5f;
GLuint VBO_color;
bool leftMousePressed = false;
glm::vec3 modelCenter;
std::vector<Triangle> tris;
float Vert[90000];
float Vert_Normal[90000];
float Vert_Tex[90000];
int frameCount = 0;
double fps = 0.0;
auto lastTime = std::chrono::high_resolution_clock::now();
int angle = 0;
int freeCam = 0;
int instruct = 0;
glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
float distcam = 10.0f;
float yaw = -90.0f;
float pitch = 0.0f;
float farplane = 10000.0f;
float nearplane = 0.1f;
float lastX = 400, lastY = 300;
bool firstMouse = true;
float modelColor[3] = {1.0f, 1.0f, 1.0f};
int open = -1;
float hfov = 90.0f;
float vfov = 60.0f;
glm::vec3 lightPos(0.0f, 0.0f, 100.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
glm::vec3 objectColor(1.0f, 1.0f, 1.0f);
int width;
int height;
framebuffer framebuffer(WindowWidth, WindowHeight);
static GLuint close2glTex = 0;
static GLuint quadVAO = 0, quadVBO = 0;
mat4c2gl viewportMatrix;
int renderMode = 1;
int shadingMode = 1;
GLuint texID;
GLuint textureShaderUniform;
int texWidth, texHeight, texChannels;
unsigned char *cpuTexData =
    stbi_load("checker_8x8.jpg", &texWidth, &texHeight, &texChannels, 4);
bool useTexture = true;
static bool useNearest = true;
static bool useBilinear = false;
static bool useMipMapping = false;
static std::vector<std::vector<uint8_t>> mipData;
static std::vector<int> mipWidths;
static std::vector<int> mipHeights;
float mipTransitionWidth = 1.0f;
bool textureEnabled = true;
int textureFilterMode = 0; // 0 = Nearest, 1 = Bilinear, 2 = MipMap
bool modulateShading = true;

void UResizeWindow(int, int);
void URenderGraphics(void);
void URenderGraphicsClose2GL(void);
void UCreateShader(void);
void UCreateBuffers(void);
glm::mat4 RotateCamera(float angle);
void mouseWheel(int button, int dir, int x, int y);
void mouseCallback(int xpos, int ypos);
void mouseButtonCallback(int button, int state, int x, int y);
void setModelColor(float r, float g, float b);
glm::vec3 calculateCenter();
void UpdateFPS();
GLuint compileShader(GLenum type, const GLchar *source);
GLuint createShaderProgram(const GLchar *vertexSource,
                           const GLchar *fragmentSource);
void setCommonUniforms(GLuint shaderProgram, int type);
glm::vec3 ndcToScreen(const glm::vec3 &ndc, int fbWidth, int fbHeight);
inline float edgeFunction(const glm::vec2 &a, const glm::vec2 &b,
                          const glm::vec2 &c) {
  return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}
GLuint LoadTexture(const char *filepath);
void buildMipPyramid(const std::vector<uint8_t> &baseData, int baseW,
                     int baseH);

GLenum err;

int numTris = 0;

const GLchar *vertexShaderSource_GouraudAD = GLSL(
    330,

    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal; layout(location = 2) in vec2 texCoord;

    out vec3 mobileColor; out vec2 TexCoord;

    uniform mat4 model; uniform mat4 view; uniform mat4 projection;
    uniform vec3 lightPos; uniform vec3 lightColor; uniform vec3 objectColor;

    void main() {
      vec3 fragPos = vec3(model * vec4(position, 1.0));
      vec3 norm = normalize(mat3(transpose(inverse(model))) * normal);
      vec3 lightDir = normalize(lightPos - fragPos);
      vec3 ambient = 0.1 * lightColor;
      float diff = max(dot(norm, lightDir), 0.0);
      vec3 diffuse = diff * lightColor;
      mobileColor = (ambient + diffuse) * objectColor;
      TexCoord = texCoord;
      gl_Position = projection * view * vec4(fragPos, 1.0);
    });

const GLchar *vertexShaderSource_GouraudADS = GLSL(
    330,

    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal; layout(location = 2) in vec2 texCoord;

    out vec3 mobileColor; out vec2 TexCoord;

    uniform mat4 model; uniform mat4 view; uniform mat4 projection;
    uniform vec3 lightPos; uniform vec3 viewPos; uniform vec3 lightColor;
    uniform vec3 objectColor;

    void main() {
      vec3 fragPos = vec3(model * vec4(position, 1.0));
      vec3 norm = normalize(mat3(transpose(inverse(model))) * normal);
      vec3 lightDir = normalize(lightPos - fragPos);
      vec3 viewDir = normalize(viewPos - fragPos);
      vec3 reflectDir = reflect(-lightDir, norm);

      vec3 ambient = 0.1 * lightColor;
      float diff = max(dot(norm, lightDir), 0.0);
      vec3 diffuse = diff * lightColor;
      float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
      vec3 specular = 0.5 * spec * lightColor;

      mobileColor = (ambient + diffuse + specular) * objectColor;
      TexCoord = texCoord;
      gl_Position = projection * view * vec4(fragPos, 1.0);
    });

const GLchar *vertexShaderSource_Phong = GLSL(
    330,

    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal; layout(location = 2) in vec2 texCoord;

    out vec3 FragPos; out vec3 Normal; out vec2 TexCoord;

    uniform mat4 model; uniform mat4 view; uniform mat4 projection;

    void main() {
      FragPos = vec3(model * vec4(position, 1.0));
      Normal = normalize(mat3(transpose(inverse(model))) * normal);
      TexCoord = texCoord;
      gl_Position = projection * view * vec4(FragPos, 1.0);
    });

const GLchar *vertexShaderSource_NoShading = GLSL(
    330,

    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal; layout(location = 2) in vec2 texCoord;

    out vec3 mobileColor; out vec2 TexCoord;

    uniform mat4 model; uniform mat4 view; uniform mat4 projection;
    uniform vec3 objectColor;

    void main() {
      gl_Position = projection * view * model * vec4(position, 1.0);
      mobileColor = objectColor;
      TexCoord = texCoord;
    });

const GLchar *fragmentShaderSource_Phong = GLSL(
    330,

    in vec3 FragPos;
    in vec3 Normal;
    in vec2 TexCoord;

    out vec4 gpuColor;

    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform vec3 lightColor;
    uniform vec3 objectColor;
    uniform sampler2D texture1;
    uniform bool useTexture;
    uniform bool modulateShading;

    void main() {
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);

        vec3 ambient = 0.1 * lightColor;
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = 0.5 * spec * lightColor;

        vec3 lighting = ambient + diffuse + specular;
        vec3 texColor = useTexture ? texture(texture1, TexCoord).rgb : objectColor;

        vec3 finalColor = modulateShading ? lighting * texColor : texColor;
        gpuColor = vec4(finalColor, 1.0);
    });

const GLchar *fragmentShaderSource_Basic = GLSL(
    330,

    in vec3 mobileColor;
    in vec2 TexCoord;

    out vec4 gpuColor;

    uniform sampler2D texture1;
    uniform bool useTexture;
    uniform bool modulateShading;

    void main() {
        vec3 texColor = useTexture ? texture(texture1, TexCoord).rgb : mobileColor;
        vec3 finalColor = modulateShading ? texColor * mobileColor : texColor;
        gpuColor = vec4(finalColor, 1.0);
    });
const GLchar *vertexShaderSource_Minimal = GLSL(
    330, layout(location = 0) in vec2 inPos;
    layout(location = 1) in vec2 inTexCoord;

    out vec2 vUV;

    void main() {
      gl_Position = vec4(inPos, 0.0, 1.0);
      vUV = inTexCoord;
    });

const GLchar *fragmentShaderSource_Minimal = GLSL(
    330, uniform sampler2D uTexture;

    in vec2 vUV; out vec4 FragColor;

    void main() { FragColor = texture(uTexture, vUV); });
GLuint shaderProgram_GouraudAD;
GLuint shaderProgram_GouraudADS;
GLuint shaderProgram_Phong;
GLuint shaderProgram_NoShading;
GLuint shaderProgram_Minimal;

int main(int argc, char *argv[]) {

  while (open != 0 && open != 1) {
    std::cout << "Enter 0 for Close2GL or 1 for OpenGL: ";
    std::cin >> open;
  }

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(WindowWidth, WindowHeight);
  glutCreateWindow(WINDOW_TITLE);

  glutReshapeFunc(UResizeWindow);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui::StyleColorsDark();

  ImGui_ImplGLUT_Init();
  ImGui_ImplGLUT_InstallFuncs();
  ImGui_ImplOpenGL3_Init();

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    std::cout << "Failed to initialize GLEW" << std::endl;
    return -1;
  }

  texID = LoadTexture("mandrill_256.jpg");

  shaderProgram_GouraudAD = createShaderProgram(vertexShaderSource_GouraudAD,
                                                fragmentShaderSource_Basic);
  shaderProgram_GouraudADS = createShaderProgram(vertexShaderSource_GouraudADS,
                                                 fragmentShaderSource_Basic);
  shaderProgram_Phong =
      createShaderProgram(vertexShaderSource_Phong, fragmentShaderSource_Phong);
  shaderProgram_NoShading = createShaderProgram(vertexShaderSource_NoShading,
                                                fragmentShaderSource_Basic);
  shaderProgram_Minimal = createShaderProgram(vertexShaderSource_Minimal,
                                              fragmentShaderSource_Minimal);

  shaderProgram = shaderProgram_NoShading;

  MeshData triangles = readfile(filename);

  std::cout << "Loaded " << triangles.numTris << " triangles.\n";

  for (int i = 0; i < triangles.numTris; i++) {
    // vertex coordinates
    Vert[9 * i] = triangles.tris[i].v0.x;
    Vert[9 * i + 1] = triangles.tris[i].v0.y;
    Vert[9 * i + 2] = triangles.tris[i].v0.z;
    Vert[9 * i + 3] = triangles.tris[i].v1.x;
    Vert[9 * i + 4] = triangles.tris[i].v1.y;
    Vert[9 * i + 5] = triangles.tris[i].v1.z;
    Vert[9 * i + 6] = triangles.tris[i].v2.x;
    Vert[9 * i + 7] = triangles.tris[i].v2.y;
    Vert[9 * i + 8] = triangles.tris[i].v2.z;
    // vertex normal coordinates
    Vert_Normal[9 * i] = triangles.tris[i].norm[0].x;
    Vert_Normal[9 * i + 1] = triangles.tris[i].norm[0].y;
    Vert_Normal[9 * i + 2] = triangles.tris[i].norm[0].z;
    Vert_Normal[9 * i + 3] = triangles.tris[i].norm[1].x;
    Vert_Normal[9 * i + 4] = triangles.tris[i].norm[1].y;
    Vert_Normal[9 * i + 5] = triangles.tris[i].norm[1].z;
    Vert_Normal[9 * i + 6] = triangles.tris[i].norm[2].x;
    Vert_Normal[9 * i + 7] = triangles.tris[i].norm[2].y;
    Vert_Normal[9 * i + 8] = triangles.tris[i].norm[2].z;

    if (triangles.has_tex == 1) {
      Vert_Tex[6 * i + 0] = triangles.tris[i].tex_cords[0];
      Vert_Tex[6 * i + 1] = triangles.tris[i].tex_cords[1];
      Vert_Tex[6 * i + 2] = triangles.tris[i].tex_cords[2];
      Vert_Tex[6 * i + 3] = triangles.tris[i].tex_cords[3];
      Vert_Tex[6 * i + 4] = triangles.tris[i].tex_cords[4];
      Vert_Tex[6 * i + 5] = triangles.tris[i].tex_cords[5];
    }
  }

  {
    std::vector<uint8_t> baseVec(cpuTexData,
                                 cpuTexData + texWidth * texHeight * 4);
    buildMipPyramid(baseVec, texWidth, texHeight);
  }

  glUseProgram(shaderProgram);

  tris = triangles.tris;
  numTris = triangles.numTris;

  while ((err = glGetError()) != GL_NO_ERROR) {
    std::cerr << "OpenGL Error: " << err << std::endl;
  }

  glutMouseFunc(ImGui_ImplGLUT_MouseFunc);
  glutMotionFunc(ImGui_ImplGLUT_MotionFunc);
  glutPassiveMotionFunc(ImGui_ImplGLUT_MotionFunc);
  glutMouseWheelFunc(mouseWheel);

  modelCenter = calculateCenter();

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  if (open == 1) {
    glutDisplayFunc(URenderGraphics);
  } else
    glutDisplayFunc(URenderGraphicsClose2GL);

  glutMainLoop();
  glDeleteVertexArrays(1, &VAO);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGLUT_Shutdown();
  ImGui::DestroyContext();

  stbi_image_free(cpuTexData);

  return 0;
}

bool key_is_pressed(KeySym ks) {
  Display *dpy = XOpenDisplay(":0");
  char keys_return[32];
  XQueryKeymap(dpy, keys_return);
  KeyCode kc2 = XKeysymToKeycode(dpy, ks);
  bool isPressed = !!(keys_return[kc2 >> 3] & (1 << (kc2 & 7)));
  XCloseDisplay(dpy);
  return isPressed;
}

void UResizeWindow(int w, int h) {
  WindowWidth = w;
  WindowHeight = h;
  framebuffer.resize(WindowWidth, WindowHeight);
  glViewport(0, 0, WindowWidth, WindowHeight);
  cout << WindowHeight << "\n";
}

void URenderGraphics(void) {

  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);

  glViewport(0, 0, w, h);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGLUT_NewFrame();
  ImGui::NewFrame();

  glGenVertexArrays(1, &VAO);

  if (textureEnabled) {
    glBindTexture(GL_TEXTURE_2D,
                  texID);

    if (textureFilterMode == 0) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else if (textureFilterMode == 1) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else if (textureFilterMode == 2) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
  }

  glBindVertexArray(VAO);
  //
  // Create buffers for vertices and normals
  //
  glCreateBuffers(NumBuffers, Buffers);
  //
  // Bind the vextex buffer, allocate storage for it, and load the vertex
  // coordinates. Then, bind the normal buffer, allocate storage and load normal
  // coordinates
  //
  glBindBuffer(GL_ARRAY_BUFFER, Buffers[VtxBuffer]);
  glBufferStorage(
      GL_ARRAY_BUFFER, numTris * 9 * sizeof(GL_FLOAT), Vert,
      GL_DYNAMIC_STORAGE_BIT); // Vert is the array with coordinates of
  // the vertices created after reading the file
  glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
  glEnableVertexAttribArray(vPosition);
  //
  glBindBuffer(GL_ARRAY_BUFFER, Buffers[NormBuffer]);
  glBufferStorage(GL_ARRAY_BUFFER, numTris * 9 * sizeof(GL_FLOAT), Vert_Normal,
                  GL_DYNAMIC_STORAGE_BIT); // Vert_Normal is the
  // array with vertex normals created after reading the file
  glVertexAttribPointer(vNormalVertex, 3, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(0));
  glEnableVertexAttribArray(vNormalVertex);
  //
  // set the number of vertices, which will be used in the openglDisplay
  // function for calling glDrawArrays

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);

  NumVertices = numTris * 3;

  glBindVertexArray(0);

  glutMouseWheelFunc(mouseWheel);

  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBindVertexArray(VAO);

  glm::mat4 model(1.0f);
  model = glm::translate(model, -modelCenter);

  glm::mat4 view(1.0f);

  // Locked camera
  if (freeCam == 0) {

    if (key_is_pressed(XK_a)) {
      yaw -= 1;
    }
    if (key_is_pressed(XK_d)) {
      yaw += 1;
    }
    if (key_is_pressed(XK_w)) {
      pitch += 1;
    }
    if (key_is_pressed(XK_s)) {
      pitch -= 1;
    }

    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 direction;
    direction.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    direction.y = sin(glm::radians(pitch));
    direction.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));

    cameraPos = target - glm::normalize(direction) * distcam;

    cameraFront = glm::normalize(target - cameraPos);
    view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), cameraUp);
  }

  // Free camera
  if (freeCam == 1) {
    glutMouseFunc(mouseButtonCallback);
    glutMotionFunc(mouseCallback);
    if (key_is_pressed(XK_a)) {
      cameraPos -=
          glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
    if (key_is_pressed(XK_d)) {
      cameraPos +=
          glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
    if (key_is_pressed(XK_w)) {
      cameraPos += cameraSpeed * cameraFront;
    }
    if (key_is_pressed(XK_s)) {
      cameraPos -= cameraSpeed * cameraFront;
    }
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
  }

  glm::mat4 projection;
  projection = glm::perspective(
      45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, nearplane, farplane);

  GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
  GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
  GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

  GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
  GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
  GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
  GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");

  glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
  glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
  glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

  glm::vec3 objectColor(modelColor[0], modelColor[1], modelColor[2]);
  glUniform3fv(objectColorLoc, 1, glm::value_ptr(objectColor));

  glUniform1i(glGetUniformLocation(shaderProgram, "textureEnabled"),
              textureEnabled);
  glUniform1i(glGetUniformLocation(shaderProgram, "modulateShading"),
              modulateShading);

  glutPostRedisplay();
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, NumVertices);

  ImGui::Begin("Viewer Controls");
  ImGui::Text("FPS: %.2f", fps);
  ImGui::Text("Camera mode: %s", freeCam ? "FreeCam" : "LockedCam");
  ImGui::SliderFloat("Near Plane", &nearplane, 0.1f, farplane - 0.1f);
  ImGui::SliderFloat("Far Plane", &farplane, nearplane + 0.1f, 1000.0f);
  ImGui::ColorEdit3("Model Color", modelColor);
  ImGui::SliderFloat3("Light Position", &lightPos[0], -200.0f, 200.0f);
  ImGui::Checkbox("Free Camera", (bool *)&freeCam);
  ImGui::Text("Shading Mode:");
  if (ImGui::Button("Gouraud AD")) {
    shaderProgram = shaderProgram_GouraudAD;
    setCommonUniforms(shaderProgram, 0);
  }
  if (ImGui::Button("Gouraud ADS")) {
    shaderProgram = shaderProgram_GouraudADS;
    setCommonUniforms(shaderProgram, 0);
  }
  if (ImGui::Button("Phong")) {
    shaderProgram = shaderProgram_Phong;
    setCommonUniforms(shaderProgram, 0);
  }
  if (ImGui::Button("No Shading")) {
    shaderProgram = shaderProgram_NoShading;
    setCommonUniforms(shaderProgram, 1);
  }
  if (ImGui::Button("Reset Camera")) {
    cameraPos = glm::vec3(0.0f, 0.0f, 20.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    target = glm::vec3(0.0f, 0.0f, 0.0f);
    distcam = 10.0f;
    yaw = -90.0f;
    pitch = 0.0f;
  }
  ImGui::Separator();
  ImGui::Text("Texture Options:");
  ImGui::Checkbox("Texture ON/OFF", &textureEnabled);
  ImGui::RadioButton("Nearest-Neighbor", &textureFilterMode, 0);
  ImGui::RadioButton("Bilinear", &textureFilterMode, 1);
  ImGui::RadioButton("Mip Mapping", &textureFilterMode, 2);
  ImGui::End();

  glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), textureEnabled);
  glUniform1i(glGetUniformLocation(shaderProgram, "modulateShading"), modulateShading);

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glutSwapBuffers();
  glBindVertexArray(0);
  UpdateFPS();
}

void URenderGraphicsClose2GL(void) {

  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);

  if (w != framebuffer.getWidth() || h != framebuffer.getHeight()) {
    framebuffer.resize(w, h);
    glViewport(0, 0, w, h);
    viewportMatrix.setViewportMatrix(0, 0, w, h);

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);

    glDeleteTextures(1, &close2glTex);
    close2glTex = 0;
  }

  if (quadVAO == 0) {
    float quadVertices[] = {
        -1.f, -1.f, 0.f, 0.f, 1.f, -1.f, 1.f, 0.f, 1.f,  1.f, 1.f, 1.f,
        -1.f, -1.f, 0.f, 0.f, 1.f, 1.f,  1.f, 1.f, -1.f, 1.f, 0.f, 1.f,
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(2 * sizeof(float)));

    glBindVertexArray(0);
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGLUT_NewFrame();
  ImGui::NewFrame();

  mat4c2gl modelcgl(1.0f);
  modelcgl[0][0] = 2.0f;
  modelcgl[1][1] = 2.0f;
  modelcgl[2][2] = 2.0f;
  modelcgl[3][3] = 1.0f;
  modelcgl[0][3] = 2.0f * -modelCenter.x;
  modelcgl[1][3] = 2.0f * -modelCenter.y;
  modelcgl[2][3] = 2.0f * -modelCenter.z;

  mat4c2gl viewcgl;

  if (freeCam == 0) {
    if (key_is_pressed(XK_a))
      yaw -= 1;
    if (key_is_pressed(XK_d))
      yaw += 1;
    if (key_is_pressed(XK_w))
      pitch += 1;
    if (key_is_pressed(XK_s))
      pitch -= 1;

    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 direction;
    direction.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    direction.y = sin(glm::radians(pitch));
    direction.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));

    cameraPos = target - glm::normalize(direction) * distcam;
    cameraFront = glm::normalize(target - cameraPos);

    std::array<double, 3> eye = {cameraPos.x, cameraPos.y, cameraPos.z};
    std::array<double, 3> center = {0.0, 0.0, 0.0};
    std::array<double, 3> up = {cameraUp.x, cameraUp.y, cameraUp.z};

    viewcgl.setViewMatrix(eye, center, up);
  } else {
    if (key_is_pressed(XK_a))
      cameraPos -=
          glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (key_is_pressed(XK_d))
      cameraPos +=
          glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (key_is_pressed(XK_w))
      cameraPos += cameraSpeed * cameraFront;
    if (key_is_pressed(XK_s))
      cameraPos -= cameraSpeed * cameraFront;

    ImGuiIO &io = ImGui::GetIO();
    if (io.MouseDown[0] && !io.WantCaptureMouse) {
      float sensitivity = 0.1f;
      yaw += io.MouseDelta.x * sensitivity;
      pitch -= io.MouseDelta.y * sensitivity;
      pitch = glm::clamp(pitch, -89.0f, 89.0f);
      glm::vec3 front;
      front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
      front.y = sin(glm::radians(pitch));
      front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
      cameraFront = glm::normalize(front);
    }

    std::array<double, 3> eyecgl = {cameraPos.x, cameraPos.y, cameraPos.z};
    std::array<double, 3> centercgl = {cameraPos.x + cameraFront.x,
                                       cameraPos.y + cameraFront.y,
                                       cameraPos.z + cameraFront.z};
    std::array<double, 3> upcgl = {cameraUp.x, cameraUp.y, cameraUp.z};

    viewcgl.setViewMatrix(eyecgl, centercgl, upcgl);
  }

  mat4c2gl projectioncgl;
  projectioncgl.setProjectionMatrixHV(vfov, hfov, nearplane, farplane);

  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glCreateBuffers(NumBuffers, Buffers);

  std::vector<glm::vec3> culledVertices;
  std::vector<GLfloat> culledNormals;
  std::vector<glm::vec2> culledUVs;
  std::vector<glm::vec3> culledUVW;

  mat4c2gl mvpcgl = projectioncgl * viewcgl * modelcgl;
  mat4c2gl mvcgl = viewcgl * modelcgl;

  culledVertices.clear();
  culledNormals.clear();
  culledUVs.clear();
  culledUVW.clear();

  bool cullClockwise = false;

  for (int i = 0; i < numTris; ++i) {
    glm::vec3 v0 = glm::make_vec3(&Vert[i * 9 + 0]);
    glm::vec3 v1 = glm::make_vec3(&Vert[i * 9 + 3]);
    glm::vec3 v2 = glm::make_vec3(&Vert[i * 9 + 6]);

    glm::vec4 clip0 = mvpcgl * glm::vec4(v0, 1.0f);
    glm::vec4 clip1 = mvpcgl * glm::vec4(v1, 1.0f);
    glm::vec4 clip2 = mvpcgl * glm::vec4(v2, 1.0f);

    glm::vec3 ndc0 = glm::vec3(clip0) / clip0.w;
    glm::vec3 ndc1 = glm::vec3(clip1) / clip1.w;
    glm::vec3 ndc2 = glm::vec3(clip2) / clip2.w;

    float invw0 = 1.0f / clip0.w;
    float invw1 = 1.0f / clip1.w;
    float invw2 = 1.0f / clip2.w;

    auto inFrustum = [](const glm::vec3 &ndc) {
      return (ndc.x >= -1 && ndc.x <= 1 && ndc.y >= -1 && ndc.y <= 1 &&
              ndc.z >= -1 && ndc.z <= 1);
    };

    if (!(inFrustum(ndc0) || inFrustum(ndc1) || inFrustum(ndc2)))
      continue;

    glm::vec3 mv_v0 = glm::vec3(mvcgl * glm::vec4(v0, 1.0f));
    glm::vec3 mv_v1 = glm::vec3(mvcgl * glm::vec4(v1, 1.0f));
    glm::vec3 mv_v2 = glm::vec3(mvcgl * glm::vec4(v2, 1.0f));

    glm::vec3 v0v1 = mv_v1 - mv_v0;
    glm::vec3 v0v2 = mv_v2 - mv_v0;
    glm::vec3 normal = glm::normalize(glm::cross(v0v1, v0v2));

    glm::vec3 viewDir;
    if (CCW == 1)
      viewDir = glm::normalize(-mv_v0);
    else
      viewDir = glm::normalize(mv_v0);

    float facing = glm::dot(normal, viewDir);

    if ((cullClockwise && facing >= 0) || (!cullClockwise && facing <= 0))
      continue;

    culledVertices.push_back(ndc0);
    culledVertices.push_back(ndc1);
    culledVertices.push_back(ndc2);

    for (int j = 0; j < 9; ++j)
      culledNormals.push_back(Vert_Normal[i * 9 + j]);

    glm::vec2 uv0(Vert_Tex[i * 6 + 0], Vert_Tex[i * 6 + 1]);
    glm::vec2 uv1(Vert_Tex[i * 6 + 2], Vert_Tex[i * 6 + 3]);
    glm::vec2 uv2(Vert_Tex[i * 6 + 4], Vert_Tex[i * 6 + 5]);
    culledUVs.push_back(uv0);
    culledUVs.push_back(uv1);
    culledUVs.push_back(uv2);

    culledUVW.push_back(glm::vec3(uv0.x * invw0, uv0.y * invw0, invw0));
    culledUVW.push_back(glm::vec3(uv1.x * invw1, uv1.y * invw1, invw1));
    culledUVW.push_back(glm::vec3(uv2.x * invw2, uv2.y * invw2, invw2));
  }

  std::vector<glm::vec3> screenVertices;
  screenVertices.reserve(culledVertices.size());

  for (size_t i = 0, end = culledVertices.size(); i < end; ++i) {
    glm::vec3 ndc = culledVertices[i];
    glm::vec3 sc = ndcToScreen(ndc, w, h);
    screenVertices.push_back(sc);
  }

  framebuffer.clearDepthBuffer(1.0f);
  framebuffer.clearColorBuffer({0, 0, 0, 255});

  if (renderMode == 0)
    for (size_t i = 0; i + 2 < screenVertices.size(); i += 3) {
      auto v0 = screenVertices[i];
      auto v1 = screenVertices[i + 1];
      auto v2 = screenVertices[i + 2];

      glm::vec2 a(v0.x, v0.y), b(v1.x, v1.y), c(v2.x, v2.y);

      float area = edgeFunction(a, b, c);
      if (std::fabs(area) < 1e-6f)
        continue;

      glm::vec3 n0 = glm::make_vec3(&culledNormals[(i + 0) * 3]);
      glm::vec3 n1 = glm::make_vec3(&culledNormals[(i + 1) * 3]);
      glm::vec3 n2 = glm::make_vec3(&culledNormals[(i + 2) * 3]);

      glm::vec3 p0 = culledVertices[i + 0];
      glm::vec3 p1 = culledVertices[i + 1];
      glm::vec3 p2 = culledVertices[i + 2];

      glm::vec3 modelColorV3 =
          glm::vec3(modelColor[0], modelColor[1], modelColor[2]);

      auto phongLighting = [&](glm::vec3 normal, glm::vec3 fragPos) {
        glm::vec3 lightDir = glm::normalize(glm::vec3(lightPos) - fragPos);
        glm::vec3 viewDir = glm::normalize(cameraPos - fragPos);
        glm::vec3 reflectDir = glm::reflect(-lightDir, normal);

        float diff = glm::max(glm::dot(normal, lightDir), 0.0f);
        float spec = pow(glm::max(glm::dot(viewDir, reflectDir), 0.0f), 32.0f);

        glm::vec3 ambient = 0.1f * modelColorV3;
        glm::vec3 diffuse = 0.6f * diff * modelColorV3;
        glm::vec3 specular = 0.3f * spec * glm::vec3(1.0f);

        return glm::clamp(ambient + diffuse + specular, 0.0f, 1.0f);
      };

      glm::vec3 c0 = phongLighting(glm::normalize(n0), p0);
      glm::vec3 c1 = phongLighting(glm::normalize(n1), p1);
      glm::vec3 c2 = phongLighting(glm::normalize(n2), p2);

      int x0 = std::max(0, (int)std::floor(std::min({a.x, b.x, c.x})));
      int y0 = std::max(0, (int)std::floor(std::min({a.y, b.y, c.y})));
      int x1 = std::min(framebuffer.getWidth() - 1,
                        (int)std::ceil(std::max({a.x, b.x, c.x})));
      int y1 = std::min(framebuffer.getHeight() - 1,
                        (int)std::ceil(std::max({a.y, b.y, c.y})));

      for (int y = y0; y <= y1; ++y) {
        glm::vec2 prevUV(0.0f);
        bool firstPixelInScan = true;
        for (int x = x0; x <= x1; ++x) {
          glm::vec2 p(x + 0.5f, y + 0.5f);

          float w0 = edgeFunction(b, c, p);
          float w1 = edgeFunction(c, a, p);
          float w2 = edgeFunction(a, b, p);

          if (w0 * area < 0 || w1 * area < 0 || w2 * area < 0)
            continue;

          w0 /= area;
          w1 /= area;
          w2 /= area;

          float z = w0 * v0.z + w1 * v1.z + w2 * v2.z;

          auto sampleLevel = [&](int L, const glm::vec2 &uv) {
            int w = mipWidths[L], h = mipHeights[L];
            const auto &data = mipData[L];
            float x = uv.x * w - 0.5f;
            float y = uv.y * h - 0.5f;
            int x0 = glm::clamp(int(floor(x)), 0, w - 1);
            int y0 = glm::clamp(int(floor(y)), 0, h - 1);
            int x1 = glm::clamp(x0 + 1, 0, w - 1);
            int y1 = glm::clamp(y0 + 1, 0, h - 1);
            float fx = x - x0;
            float fy = y - y0;

            auto fetch = [&](int sx, int sy) {
              int idx = (sy * w + sx) * 4;
              return glm::vec3(data[idx + 0] / 255.0f, data[idx + 1] / 255.0f,
                               data[idx + 2] / 255.0f);
            };

            glm::vec3 c00 = fetch(x0, y0);
            glm::vec3 c10 = fetch(x1, y0);
            glm::vec3 c01 = fetch(x0, y1);
            glm::vec3 c11 = fetch(x1, y1);

            glm::vec3 cx0 = glm::mix(c00, c10, fx);
            glm::vec3 cx1 = glm::mix(c01, c11, fx);
            return glm::mix(cx0, cx1, fy);
          };

          float &curDepth = framebuffer.getDepth(x, y);
          if (z < curDepth) {
            auto uvw0 = culledUVW[i + 0];
            auto uvw1 = culledUVW[i + 1];
            auto uvw2 = culledUVW[i + 2];

            float u_over_w = w0 * uvw0.x + w1 * uvw1.x + w2 * uvw2.x;
            float v_over_w = w0 * uvw0.y + w1 * uvw1.y + w2 * uvw2.y;
            float inv_w = w0 * uvw0.z + w1 * uvw1.z + w2 * uvw2.z;

            glm::vec2 uv = glm::vec2(u_over_w, v_over_w) / inv_w;
            uv = glm::fract(uv);

            glm::vec3 texColor;
            if (useTexture && useMipMapping) {

              float ds = 0.0f, dt = 0.0f;
              if (!firstPixelInScan) {
                ds = fabs(uv.x - prevUV.x);
                dt = fabs(uv.y - prevUV.y);
              }
              prevUV = uv;

              firstPixelInScan = false;

              float L = 0.0f;
              if (ds > 0 || dt > 0) {
                float rho =
                    std::max(ds * float(texWidth), dt * float(texHeight));
                L = log2f(rho);
              }

              int maxL = int(mipData.size()) - 1;
              int L0 = std::max(0, int(std::floor(L)));
              int L1 = std::min(maxL, L0 + 1);

              float frac = L - float(L0);

              float w = mipTransitionWidth;
              float e0 = 0.5f - 0.5f * w;
              float e1 = 0.5f + 0.5f * w;
              float t = glm::smoothstep(e0, e1, frac);

              glm::vec3 c0 = sampleLevel(L0, uv);
              glm::vec3 c1 = sampleLevel(L1, uv);

              static float maxLSeen = 0.0f;
              if (L > maxLSeen) {
                maxLSeen = L;
                std::cout << "New max mip level this frame: " << maxLSeen
                          << std::endl;
              }

              texColor = glm::mix(c0, c1, t);
            } else if (useTexture) {
              int tx, ty;
              if (useBilinear) {

                float u_img = uv.x * texWidth - 0.5f;
                float v_img = uv.y * texHeight - 0.5f;
                int i0 = std::floor(u_img);
                int j0 = std::floor(v_img);
                float fu = u_img - i0;
                float fv = v_img - j0;

                auto clampX = [&](int x) {
                  return std::clamp(x, 0, texWidth - 1);
                };
                auto clampY = [&](int y) {
                  return std::clamp(y, 0, texHeight - 1);
                };

                int x0 = clampX(i0), x1 = clampX(i0 + 1);
                int y0 = clampY(j0), y1 = clampY(j0 + 1);

                glm::vec3 c00, c10, c01, c11;
                int idx00 = (y0 * texWidth + x0) * 4;
                int idx10 = (y0 * texWidth + x1) * 4;
                int idx01 = (y1 * texWidth + x0) * 4;
                int idx11 = (y1 * texWidth + x1) * 4;
                c00 = glm::vec3(cpuTexData[idx00 + 0], cpuTexData[idx00 + 1],
                                cpuTexData[idx00 + 2]) /
                      255.0f;
                c10 = glm::vec3(cpuTexData[idx10 + 0], cpuTexData[idx10 + 1],
                                cpuTexData[idx10 + 2]) /
                      255.0f;
                c01 = glm::vec3(cpuTexData[idx01 + 0], cpuTexData[idx01 + 1],
                                cpuTexData[idx01 + 2]) /
                      255.0f;
                c11 = glm::vec3(cpuTexData[idx11 + 0], cpuTexData[idx11 + 1],
                                cpuTexData[idx11 + 2]) /
                      255.0f;

                // bilinear interpolate
                texColor = glm::mix(glm::mix(c00, c10, fu),
                                    glm::mix(c01, c11, fu), fv);
              } else if (useNearest) {
                tx = std::clamp(int(uv.x * texWidth + 0.5f), 0, texWidth - 1);
                ty = std::clamp(int(uv.y * texHeight + 0.5f), 0, texHeight - 1);
                int idx = (ty * texWidth + tx) * 4;
                texColor = glm::vec3(cpuTexData[idx + 0] / 255.0f,
                                     cpuTexData[idx + 1] / 255.0f,
                                     cpuTexData[idx + 2] / 255.0f);
              } else {
                tx = std::clamp(int(uv.x * texWidth), 0, texWidth - 1);
                ty = std::clamp(int(uv.y * texHeight), 0, texHeight - 1);
                int idx = (ty * texWidth + tx) * 4;
                texColor = glm::vec3(cpuTexData[idx + 0] / 255.0f,
                                     cpuTexData[idx + 1] / 255.0f,
                                     cpuTexData[idx + 2] / 255.0f);
              }
            } else {
              texColor = glm::vec3(1.0f);
            }

            // combine with lighting
            glm::vec3 litColor = w0 * c0 + w1 * c1 + w2 * c2;
            glm::vec3 finalColor = texColor * litColor;
            std::array<uint8_t, 4> pixelColor = {
                static_cast<uint8_t>(finalColor.r * 255.0f),
                static_cast<uint8_t>(finalColor.g * 255.0f),
                static_cast<uint8_t>(finalColor.b * 255.0f), 255};
            framebuffer.setPixel(x, y, pixelColor);
          }
        }
      }
    }

  if (renderMode == 1)
    for (size_t i = 0; i + 2 < screenVertices.size(); i += 3) {
      glm::vec3 v[3] = {screenVertices[i], screenVertices[i + 1],
                        screenVertices[i + 2]};

      for (int e = 0; e < 3; ++e) {
        glm::vec3 p0 = v[e];
        glm::vec3 p1 = v[(e + 1) % 3];

        float dx = p1.x - p0.x;
        float dy = p1.y - p0.y;
        float dz = p1.z - p0.z;

        int steps = (int)std::max(std::fabs(dx), std::fabs(dy));
        float sx = dx / (float)steps;
        float sy = dy / (float)steps;
        float sz = dz / (float)steps;

        float x = p0.x;
        float y = p0.y;
        float z = p0.z;

        for (int s = 0; s <= steps; ++s) {
          int xi = (int)std::round(x);
          int yi = (int)std::round(y);
          if (xi >= 0 && xi < framebuffer.getWidth() && yi >= 0 &&
              yi < framebuffer.getHeight()) {

            float &d = framebuffer.getDepth(xi, yi);
            if (z < d) {
              d = z;
              std::array<uint8_t, 4> col = {
                  (uint8_t)std::clamp(modelColor[0] * 255.0f, 0.0f, 255.0f),
                  (uint8_t)std::clamp(modelColor[1] * 255.0f, 0.0f, 255.0f),
                  (uint8_t)std::clamp(modelColor[2] * 255.0f, 0.0f, 255.0f),
                  255};
              framebuffer.setPixel(xi, yi, col);
            }
          }
          x += sx;
          y += sy;
          z += sz;
        }
      }
    }

  if (close2glTex == 0) {
    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &close2glTex);
    glBindTexture(GL_TEXTURE_2D, close2glTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, framebuffer.getWidth(),
                   framebuffer.getHeight());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, close2glTex);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, framebuffer.getWidth(),
                  framebuffer.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE,
                  framebuffer.colorBuffer.data());

  glDisable(GL_DEPTH_TEST);

  glUseProgram(shaderProgram_Minimal);
  glUniform1i(glGetUniformLocation(shaderProgram_Minimal, "uTexture"), 1);
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
  glUseProgram(0);

  ImGui::Begin("Viewer Controls");
  ImGui::Text("FPS: %.2f", fps);
  ImGui::Text("Camera mode: %s", freeCam ? "FreeCam" : "LockedCam");
  ImGui::SliderFloat("Near Plane", &nearplane, 0.1f, farplane - 0.1f);
  ImGui::SliderFloat("Far Plane", &farplane, nearplane + 0.1f, 1000.0f);
  ImGui::SliderFloat("Hfov", &hfov, 0.0f, 200.0f);
  ImGui::SliderFloat("Vfov", &vfov, 00.0f, 200.0f);
  ImGui::ColorEdit3("Model Color", modelColor);
  ImGui::SliderFloat3("Light Position", &lightPos[0], -200.0f, 200.0f);
  ImGui::Checkbox("Free Camera", (bool *)&freeCam);
  ImGui::Checkbox("Wireframe Mode", (bool *)&renderMode);
  ImGui::Checkbox("Texture On/Off", (bool *)&useTexture);
  ImGui::Checkbox("Nearest Sampling", (bool *)&useNearest);
  if (useNearest) {
    useBilinear = false;
    useMipMapping = false;
  }
  ImGui::Checkbox("Bilinear Sampling", (bool *)&useBilinear);
  if (useBilinear) {
    useNearest = false;
    useMipMapping = false;
  }
  ImGui::Checkbox("Mip Mapping", &useMipMapping);
  if (useMipMapping) {
    useNearest = false;
    useBilinear = false;
  }
  if (ImGui::Button("Reset Camera")) {
    cameraPos = glm::vec3(0.0f, 0.0f, 20.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    target = glm::vec3(0.0f, 0.0f, 0.0f);
    distcam = 10.0f;
    yaw = -90.0f;
    pitch = 0.0f;
  }
  ImGui::End();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glutSwapBuffers();
  UpdateFPS();

  glutPostRedisplay();
}

void mouseWheel(int button, int dir, int x, int y) {
  if (dir > 0) {
    distcam -= 5.5f;
  } else {
    distcam += 5.5f;
  }
  glutPostRedisplay();
}

void mouseCallback(int xpos, int ypos) {

  if (!leftMousePressed)
    return;

  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos;

  lastX = xpos;
  lastY = ypos;

  float sensitivity = 0.1f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  yaw += xoffset;
  pitch += yoffset;

  if (pitch > 89.0f)
    pitch = 89.0f;
  if (pitch < -89.0f)
    pitch = -89.0f;

  glm::vec3 front;
  front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  front.y = sin(glm::radians(pitch));
  front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  cameraFront = glm::normalize(front);
}

void mouseButtonCallback(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      leftMousePressed = true;
      firstMouse = true;
    } else if (state == GLUT_UP) {
      leftMousePressed = false;
    }
  }
}

void setModelColor(float r, float g, float b) {
  float newColors[9 * numTris];

  for (int i = 0; i < numTris; i++) {
    for (int j = 0; j < 3; j++) {
      newColors[i * 9 + j * 3 + 0] = r;
      newColors[i * 9 + j * 3 + 1] = g;
      newColors[i * 9 + j * 3 + 2] = b;
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, VBO_color);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newColors), newColors);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

glm::vec3 calculateCenter() {
  glm::vec3 center(0.0f);
  for (const auto &tri : tris) {
    center += glm::vec3(tri.v0.x, tri.v0.y, tri.v0.z);
    center += glm::vec3(tri.v1.x, tri.v1.y, tri.v1.z);
    center += glm::vec3(tri.v2.x, tri.v2.y, tri.v2.z);
  }

  center /= static_cast<float>(tris.size() * 3);
  return center;
}

void UpdateFPS() {
  frameCount++;
  auto currentTime = std::chrono::high_resolution_clock::now();
  double elapsed =
      std::chrono::duration<double>(currentTime - lastTime).count();

  if (elapsed >= 1.0) { // a cada 1 segundo
    fps = frameCount / elapsed;
    frameCount = 0;
    lastTime = currentTime;
  }
}

GLuint compileShader(GLenum type, const GLchar *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLchar infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    std::cerr << "Shader Compilation Error:\n" << infoLog << std::endl;
  }

  return shader;
}

GLuint createShaderProgram(const GLchar *vertexSource,
                           const GLchar *fragmentSource) {
  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
  GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    GLchar infoLog[512];
    glGetProgramInfoLog(program, 512, NULL, infoLog);
    std::cerr << "Shader Linking Error:\n" << infoLog << std::endl;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return program;
}

void setCommonUniforms(GLuint shaderProgram, int type) {
  glUseProgram(shaderProgram);

  GLint objColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
  glUniform3f(objColorLoc, 1.0f, 0.0f, 0.0f);

  if (type != 1) {
    GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);

    GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    glUniform3f(lightPosLoc, 2.0f, 4.0f, 1.0f);

    GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    glUniform3f(viewPosLoc, 0.0f, 0.0f, 5.0f);
  }
}

glm::vec3 ndcToScreen(const glm::vec3 &ndc, int fbWidth, int fbHeight) {
  float xN = ndc.x * 0.5f + 0.5f;
  float yN = ndc.y * 0.5f + 0.5f;

  float xPix = xN * static_cast<float>(fbWidth);
  float yPix = yN * static_cast<float>(fbHeight);
  float zPix = ndc.z * 0.5f + 0.5f;

  return {xPix, yPix, zPix};
}

GLuint LoadTexture(const char *filepath) {
  GLuint textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int width, height, nrChannels;
  // stbi_set_flip_vertically_on_load(true);

  unsigned char *data = stbi_load(filepath, &width, &height, &nrChannels, 0);
  if (data) {
    GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cerr << "Failed to load texture: " << filepath << std::endl;
  }
  stbi_image_free(data);
  return textureID;
}

void buildMipPyramid(const std::vector<uint8_t> &baseData, int baseW,
                     int baseH) {
  mipData.clear();
  mipWidths.clear();
  mipHeights.clear();

  mipData.push_back(baseData);
  mipWidths.push_back(baseW);
  mipHeights.push_back(baseH);

  int level = 0;
  while (mipWidths[level] > 1 && mipHeights[level] > 1) {
    int w0 = mipWidths[level], h0 = mipHeights[level];
    int w1 = std::max(1, w0 / 2), h1 = std::max(1, h0 / 2);
    std::vector<uint8_t> next(w1 * h1 * 4);

    auto &src = mipData[level];
    for (int y = 0; y < h1; ++y) {
      for (int x = 0; x < w1; ++x) {
        int sx = x * 2, sy = y * 2;
        glm::vec4 sum(0.0f);
        for (int oy = 0; oy < 2; ++oy)
          for (int ox = 0; ox < 2; ++ox) {
            int ix = std::min(sx + ox, w0 - 1), iy = std::min(sy + oy, h0 - 1);
            int idx = (iy * w0 + ix) * 4;
            sum += glm::vec4(src[idx + 0], src[idx + 1], src[idx + 2],
                             src[idx + 3]) /
                   255.0f;
          }
        sum *= 0.25f;
        int dstIdx = (y * w1 + x) * 4;
        next[dstIdx + 0] = uint8_t(sum.r * 255);
        next[dstIdx + 1] = uint8_t(sum.g * 255);
        next[dstIdx + 2] = uint8_t(sum.b * 255);
        next[dstIdx + 3] = uint8_t(sum.a * 255);
      }
    }

    mipData.push_back(std::move(next));
    mipWidths.push_back(w1);
    mipHeights.push_back(h1);
    ++level;
  }
}