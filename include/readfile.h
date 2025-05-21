#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct Vec3 {
    float x, y, z;
};

struct Triangle {
    Vec3 v0, v1, v2;
    Vec3 norm[3];
    Vec3 face_normal;
    unsigned char Color[3];
    float tex_cords[6];
};


struct MeshData {
    int numTris;
    std::vector<Triangle> tris;
    int has_tex;
};

MeshData readfile(const char* FileName);

#endif