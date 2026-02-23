/*
shader comp commands
shadercRelease.exe -f vs_mesh.sc -o vs_mesh.bin --type vertex --platform windows --profile 120 -i .
shadercRelease.exe -f fs_mesh.sc -o fs_mesh.bin --type fragment --platform windows --profile 120 -i .
*/

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

#include <SDL.h>
#include <SDL_syswm.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tinygltf-2.9.7/tiny_gltf.h>

#include <vector>
#include <iostream>
#include <fstream>

// ------------------------------------------------------------
// Helper: load compiled bgfx shader
// ------------------------------------------------------------
bgfx::ShaderHandle loadShader(const char* filePath, std::vector<char>& bufferStorage)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader: " << filePath << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    bufferStorage.resize(size);
    file.read(bufferStorage.data(), size);
    if (!file) {
        std::cerr << "Failed to read shader: " << filePath << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    return bgfx::createShader(bgfx::makeRef(bufferStorage.data(), bufferStorage.size()));
}
// ------------------------------------------------------------

struct Vertex
{
    float x, y, z;
    float nx, ny, nz;
};

struct MeshBuffers
{
    bgfx::VertexBufferHandle vbh;
    bgfx::IndexBufferHandle ibh;
    uint32_t indexCount;
    bool use32;
};

struct MeshInstance
{
    MeshBuffers buffers;
    int nodeIndex; // reference to gltf node
};

// ------------------------------------------------------------
// Helper: get node transform
// ------------------------------------------------------------
void getNodeMatrix(const tinygltf::Node& node, float out[16])
{
    if (!node.matrix.empty())
    {
        for (int i = 0; i < 16; ++i) out[i] = static_cast<float>(node.matrix[i]);
    }
    else
    {
        bx::mtxIdentity(out);

        if (!node.translation.empty())
        {
            bx::mtxTranslate(out,
                static_cast<float>(node.translation[0]),
                static_cast<float>(node.translation[1]),
                static_cast<float>(node.translation[2])
            );
        }

        // Optional: handle rotation & scale if needed
    }
}

// ------------------------------------------------------------
int main(int argc, char* argv[])
{
    // ------------------------------------------------------------
    // SDL INIT
    // ------------------------------------------------------------
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        "GLB Viewer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    // ------------------------------------------------------------
    // BGFX INIT
    // ------------------------------------------------------------
    bgfx::Init init;
    init.type = bgfx::RendererType::OpenGL;
    init.resolution.width = 1280;
    init.resolution.height = 720;
    init.resolution.reset = BGFX_RESET_VSYNC;

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    SDL_GetWindowWMInfo(window, &wmi);

#if defined(_WIN32)
    init.platformData.nwh = wmi.info.win.window;
#elif defined(__linux__)
    init.platformData.nwh = (void*)(uintptr_t)wmi.info.x11.window;
#elif defined(__APPLE__)
    init.platformData.nwh = wmi.info.cocoa.window;
#endif

    bgfx::init(init);
    bgfx::setViewClear(0,
        BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
        0x303030ff,
        1.0f,
        0);

    bgfx::setViewRect(0, 0, 0, 1280, 720);

    // ------------------------------------------------------------
    // LOAD GLB
    // ------------------------------------------------------------
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool loaded = loader.LoadBinaryFromFile(
        &gltfModel,
        &err,
        &warn,
        "C:\\Users\\spang\\Desktop\\Projects\\paperGB\\3d\\gb.glb"
    );

    if (!loaded)
    {
        std::cout << "Failed to load .glb\n";
        return 1;
    }

    // ------------------------------------------------------------
    // LOAD SHADERS
    // ------------------------------------------------------------
    std::vector<char> vsBuffer;
    bgfx::ShaderHandle vsh = loadShader("C:/Users/spang/Desktop/Projects/paperGB/3d/vs_mesh.bin", vsBuffer);
    std::vector<char> fsBuffer;
    bgfx::ShaderHandle fsh = loadShader("C:/Users/spang/Desktop/Projects/paperGB/3d/fs_mesh.bin", fsBuffer);
    bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh, true);

    // ------------------------------------------------------------
    // PREPARE BUFFERS FOR ALL MESHES
    // ------------------------------------------------------------
    std::vector<MeshInstance> allMeshes;

    for (size_t nodeIdx = 0; nodeIdx < gltfModel.nodes.size(); ++nodeIdx)
    {
        const auto& node = gltfModel.nodes[nodeIdx];
        if (node.mesh < 0) continue;

        const auto& mesh = gltfModel.meshes[node.mesh];
        for (const auto& primitive : mesh.primitives)
        {
            const auto& posAccessor =
                gltfModel.accessors.at(primitive.attributes.find("POSITION")->second);
            const auto& posView = gltfModel.bufferViews[posAccessor.bufferView];
            const auto& posBuffer = gltfModel.buffers[posView.buffer];

            const uint8_t* posBase = posBuffer.data.data() + posView.byteOffset + posAccessor.byteOffset;
            size_t posStride = posView.byteStride;
            if (posStride == 0) posStride = sizeof(float) * 3;

            const auto& normAccessor =
                gltfModel.accessors.at(primitive.attributes.find("NORMAL")->second);
            const auto& normView = gltfModel.bufferViews[normAccessor.bufferView];
            const auto& normBuffer = gltfModel.buffers[normView.buffer];

            const uint8_t* normBase = normBuffer.data.data() + normView.byteOffset + normAccessor.byteOffset;
            size_t normStride = normView.byteStride;
            if (normStride == 0) normStride = sizeof(float) * 3;

            uint32_t vertexCount = posAccessor.count;

            const auto& indexAccessor = gltfModel.accessors[primitive.indices];
            const auto& indexView = gltfModel.bufferViews[indexAccessor.bufferView];
            const auto& indexBuffer = gltfModel.buffers[indexView.buffer];

            std::vector<uint16_t> indices16;
            std::vector<uint32_t> indices32;
            bool use32 = false;

            if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                const uint16_t* ptr = reinterpret_cast<const uint16_t*>(
                    &indexBuffer.data[indexAccessor.byteOffset + indexView.byteOffset]);
                indices16.assign(ptr, ptr + indexAccessor.count);
            }
            else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                const uint32_t* ptr = reinterpret_cast<const uint32_t*>(
                    &indexBuffer.data[indexAccessor.byteOffset + indexView.byteOffset]);
                indices32.assign(ptr, ptr + indexAccessor.count);
                use32 = true;
            }
            else
            {
                std::cerr << "Unsupported index component type!" << std::endl;
                continue;
            }

            std::vector<Vertex> vertices(vertexCount);
            for (uint32_t i = 0; i < vertexCount; ++i)
            {
                const float* pos = reinterpret_cast<const float*>(posBase + posStride * i);
                vertices[i].x = pos[0];
                vertices[i].y = pos[1];
                vertices[i].z = -pos[2];

                const float* n = reinterpret_cast<const float*>(normBase + normStride * i);
                vertices[i].nx = n[0];
                vertices[i].ny = n[1];
                vertices[i].nz = -n[2];
            }

            bgfx::VertexLayout layout;
            layout.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
                .end();

            bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
                bgfx::copy(vertices.data(), sizeof(Vertex) * vertexCount),
                layout
            );

            bgfx::IndexBufferHandle ibh;
            if (use32)
            {
                ibh = bgfx::createIndexBuffer(
                    bgfx::copy(indices32.data(), indices32.size() * sizeof(uint32_t)),
                    BGFX_BUFFER_INDEX32
                );
            }
            else
            {
                ibh = bgfx::createIndexBuffer(
                    bgfx::copy(indices16.data(), indices16.size() * sizeof(uint16_t))
                );
            }

            MeshBuffers buf = { vbh, ibh, (uint32_t)indexAccessor.count, use32 };
            allMeshes.push_back({ buf, static_cast<int>(nodeIdx) });
        }
    }

    // ------------------------------------------------------------
    // MAIN LOOP
    // ------------------------------------------------------------
    bool running = true;
    float angle = 0.0f;
    float cameraDistance = 150.0f;

    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;
        }

        angle += 0.01f;

        float view[16];
        float proj[16];

        bx::mtxLookAt(view, { 0.0f, 0.0f, cameraDistance }, { 0.0f, 0.0f, 0.0f });
        bx::mtxProj(proj, 60.0f, 1280.0f / 720.0f, 0.1f, 1000.0f, bgfx::getCaps()->homogeneousDepth);

        bgfx::setViewTransform(0, view, proj);
        bgfx::touch(0);

        for (const auto& meshInst : allMeshes)
        {
            float nodeMtx[16];
            getNodeMatrix(gltfModel.nodes[meshInst.nodeIndex], nodeMtx);

            float rotation[16];
            bx::mtxRotateY(rotation, angle);

            float finalModel[16];
            bx::mtxMul(finalModel, nodeMtx, rotation);

            bgfx::setTransform(finalModel);
            bgfx::setVertexBuffer(0, meshInst.buffers.vbh);
            bgfx::setIndexBuffer(meshInst.buffers.ibh);

            bgfx::setState(
                BGFX_STATE_WRITE_RGB |
                BGFX_STATE_WRITE_Z |
                BGFX_STATE_DEPTH_TEST_LESS |
                BGFX_STATE_CULL_CW
            );

            bgfx::submit(0, program);
        }

        bgfx::frame();
    }

    for (const auto& meshInst : allMeshes)
    {
        bgfx::destroy(meshInst.buffers.vbh);
        bgfx::destroy(meshInst.buffers.ibh);
    }

    bgfx::destroy(program);
    bgfx::shutdown();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}