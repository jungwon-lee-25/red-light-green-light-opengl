#ifndef MODEL_H
#define MODEL_H

#include <sb7.h>
#include <vmath.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

struct Vertex {
    vmath::vec3 Position;
    vmath::vec2 TexCoords;
    vmath::vec3 Normal;
};

class Model {
public:
    GLuint VAO, VBO;
    int vertexCount;
    vmath::vec3 minB, maxB;

    Model(const char* path) : vertexCount(0) {
        minB = vmath::vec3(1e9f); maxB = vmath::vec3(-1e9f);
        loadOBJ(path);
    }

    vmath::mat4 getCorrectionMatrix() {
        vmath::vec3 center = (minB + maxB) * 0.5f;
        vmath::vec3 size = maxB - minB;
        float maxDim = std::max({ size[0], size[1], size[2] });
        if (maxDim < 0.001f) maxDim = 1.0f;
        return vmath::scale(1.0f / maxDim) * vmath::translate(-center[0], -minB[1], -center[2]);
    }

    void draw() {
        if (vertexCount == 0) return;
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }

private:
    void loadOBJ(const char* path) {
        std::vector<vmath::vec3> temp_vertices;
        std::vector<vmath::vec2> temp_uvs;
        std::vector<vmath::vec3> temp_normals;
        std::vector<Vertex> out_vertices;

        std::ifstream file(path);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string prefix;
            ss >> prefix;

            if (prefix == "v") {
                vmath::vec3 v; ss >> v[0] >> v[1] >> v[2];
                temp_vertices.push_back(v);
                for (int i = 0; i < 3; i++) { minB[i] = std::min(minB[i], v[i]); maxB[i] = std::max(maxB[i], v[i]); }
            }
            else if (prefix == "vt") {
                vmath::vec2 vt; ss >> vt[0] >> vt[1];
                temp_uvs.push_back(vt);
            }
            else if (prefix == "vn") {
                vmath::vec3 vn; ss >> vn[0] >> vn[1] >> vn[2];
                temp_normals.push_back(vn);
            }
            else if (prefix == "f") {
                for (int i = 0; i < 3; i++) {
                    std::string vData; ss >> vData;
                    std::replace(vData.begin(), vData.end(), '/', ' ');
                    std::stringstream vss(vData);
                    int vi = 0, ti = 0, ni = 0;
                    vss >> vi >> ti >> ni;
                    Vertex v;
                    if (vi > 0) v.Position = temp_vertices[vi - 1];
                    if (ti > 0) v.TexCoords = temp_uvs[ti - 1]; else v.TexCoords = vmath::vec2(0, 0);
                    if (ni > 0) v.Normal = temp_normals[ni - 1]; else v.Normal = vmath::vec3(0, 1, 0);
                    out_vertices.push_back(v);
                }
            }
        }
        vertexCount = (int)out_vertices.size();
        if (vertexCount == 0) return;

        glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, out_vertices.size() * sizeof(Vertex), &out_vertices[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords)); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal)); glEnableVertexAttribArray(2);
    }
};

#endif