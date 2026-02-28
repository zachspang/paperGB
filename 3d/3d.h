#pragma once

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

#include <SDL.h>
#include <SDL_syswm.h>

#include <tinygltf-2.9.7/tiny_gltf.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <limits>
#include <cmath>

#include "3d.h"
#include "../src/TextureBuffer.h"

struct Vertex
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

struct LineVertex
{
    float x, y, z;
    uint32_t abgr;
};

struct CpuMesh
{
    std::vector<float>    positions;
    std::vector<uint32_t> indices;
};

struct MeshBuffers
{
    bgfx::VertexBufferHandle vbh;
    bgfx::IndexBufferHandle  ibh;
    uint32_t                 indexCount;
    bool                     use32;
    bgfx::TextureHandle      texture;
};

struct MeshInstance
{
    MeshBuffers buffers;
    int         nodeIndex;
    std::string meshName;
    CpuMesh     cpuMesh;
};

bgfx::ShaderHandle loadShader(const char* filePath, std::vector<char>& bufferStorage);

void getNodeMatrix(const tinygltf::Node& node, float out[16]);

void getGlobalNodeMatrix(const tinygltf::Model& model,
    const std::vector<int>& nodeParents,
    int                      nodeIndex,
    float                    out[16]);

bgfx::TextureHandle uploadGltfImage(const tinygltf::Model& model, int imageIndex);

bool rayTriangleIntersect(const float orig[3], const float dir[3],
    const float v0[3], const float v1[3], const float v2[3],
    float& t);

void transformPoint(const float m[16], const float in[3], float out[3]);
int run3d(TextureBuffer* emuScreenTexBuffer);