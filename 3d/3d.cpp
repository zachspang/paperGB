/*
shader comp commands
shadercRelease.exe -f vs_mesh.sc -o vs_mesh.bin --type vertex --platform windows --profile 120 -i .
shadercRelease.exe -f fs_mesh.sc -o fs_mesh.bin --type fragment --platform windows --profile 120 -i .
shadercRelease.exe -f vs_line.sc -o vs_line.bin --type vertex   --platform windows --profile 120 -i .
shadercRelease.exe -f fs_line.sc -o fs_line.bin --type fragment --platform windows --profile 120 -i .
*/

#include "3d.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tinygltf-2.9.7/tiny_gltf.h>
#include <commdlg.h>

// ------------------------------------------------------------
// Constants
// ------------------------------------------------------------

static constexpr int   WINDOW_WIDTH = 1280;
static constexpr int   WINDOW_HEIGHT = 720;
static constexpr float CAMERA_DISTANCE = 3.0f;
static constexpr float CAMERA_HEIGHT = 0.0f;
static constexpr float CAMERA_ORBIT_SPEED = 0.01f;
static constexpr float CAMERA_FOV_DEG = 60.0f;
static constexpr float CAMERA_NEAR = 0.1f;
static constexpr float CAMERA_FAR = 10.0f;

static const char* GLB_PATH = "C:/Users/spang/Desktop/Projects/paperGB/3d/gb8.glb";
static const char* VS_BIN_PATH = "C:/Users/spang/Desktop/Projects/paperGB/3d/vs_mesh.bin";
static const char* FS_BIN_PATH = "C:/Users/spang/Desktop/Projects/paperGB/3d/fs_mesh.bin";
static const char* VS_LINE_BIN_PATH = "C:/Users/spang/Desktop/Projects/paperGB/3d/vs_line.bin";
static const char* FS_LINE_BIN_PATH = "C:/Users/spang/Desktop/Projects/paperGB/3d/fs_line.bin";

// Names of meshes that respond to click-drag rotation
static const std::unordered_set<std::string> DRAGGABLE_MESH_NAMES = {
    "base_low_Main_0",
    "battery_low_External_0",
    "backvent_low_External_0",
    "Cylinder010_External_0",
    "dviBox_low_External_0",
    "dviMetal_low_External_0",
    "dviPart_low_External_0",
    "dviSmall_low_External_0",
    "emuScreen",
    "lock_low_External_0",
    "lockOpen_low_External_0",
    "Plane009_Black Screen_0",
    "screen_low_External_0",
    "screenHolder_low_External_0",
    "volumeBox_low_External_0"
};

// ------------------------------------------------------------
// Animation structures
// ------------------------------------------------------------

enum class AnimTargetPath { Translation, Rotation, Scale };
enum class AnimInterpolation { Linear, Step };

struct AnimSampler
{
    std::vector<float> times;
    std::vector<float> values;
    AnimInterpolation  interp = AnimInterpolation::Linear;
    int                components = 3;
};

struct AnimChannel
{
    int            nodeIndex = -1;
    AnimTargetPath path = AnimTargetPath::Translation;
    AnimSampler    sampler;
};

struct Animation
{
    std::string              name;
    std::vector<AnimChannel> channels;
    float                    duration = 0.0f;
};

struct AnimationState
{
    int   animIndex = -1;
    float time = 0.0f;
    bool  playing = false;
};

struct NodeTRS
{
    float t[3] = { 0, 0, 0 };
    float r[4] = { 0, 0, 0, 1 };
    float s[3] = { 1, 1, 1 };
    bool  hasT = false;
    bool  hasR = false;
    bool  hasS = false;
};

// ------------------------------------------------------------
static std::vector<float> readFloatAccessor(const tinygltf::Model& model, int accessorIdx)
{
    const auto& acc = model.accessors[accessorIdx];
    const auto& view = model.bufferViews[acc.bufferView];
    const auto& buf = model.buffers[view.buffer];

    int components = 1;
    switch (acc.type)
    {
    case TINYGLTF_TYPE_SCALAR: components = 1;  break;
    case TINYGLTF_TYPE_VEC2:   components = 2;  break;
    case TINYGLTF_TYPE_VEC3:   components = 3;  break;
    case TINYGLTF_TYPE_VEC4:   components = 4;  break;
    case TINYGLTF_TYPE_MAT4:   components = 16; break;
    default: break;
    }

    size_t stride = view.byteStride ? view.byteStride : (components * sizeof(float));
    const uint8_t* base = buf.data.data() + view.byteOffset + acc.byteOffset;

    std::vector<float> out;
    out.reserve(acc.count * components);
    for (size_t i = 0; i < acc.count; ++i)
    {
        const float* fp = reinterpret_cast<const float*>(base + stride * i);
        for (int c = 0; c < components; ++c)
            out.push_back(fp[c]);
    }
    return out;
}

// ------------------------------------------------------------
static std::vector<Animation> parseAnimations(const tinygltf::Model& model)
{
    std::vector<Animation> result;
    for (const auto& anim : model.animations)
    {
        Animation a;
        a.name = anim.name;

        std::vector<AnimSampler> samplers;
        for (const auto& s : anim.samplers)
        {
            AnimSampler as;
            as.times = readFloatAccessor(model, s.input);
            as.components = (model.accessors[s.output].type == TINYGLTF_TYPE_VEC4) ? 4 : 3;
            as.values = readFloatAccessor(model, s.output);
            as.interp = (s.interpolation == "STEP") ? AnimInterpolation::Step : AnimInterpolation::Linear;
            samplers.push_back(std::move(as));
        }

        for (const auto& ch : anim.channels)
        {
            if (ch.sampler < 0 || ch.sampler >= (int)samplers.size()) continue;
            if (ch.target_node < 0) continue;

            AnimChannel ac;
            ac.nodeIndex = ch.target_node;
            ac.sampler = samplers[ch.sampler];

            if (ch.target_path == "translation") ac.path = AnimTargetPath::Translation;
            else if (ch.target_path == "rotation")    ac.path = AnimTargetPath::Rotation;
            else if (ch.target_path == "scale")       ac.path = AnimTargetPath::Scale;
            else continue;

            if (!ac.sampler.times.empty())
                a.duration = std::max(a.duration, ac.sampler.times.back());

            a.channels.push_back(std::move(ac));
        }

        result.push_back(std::move(a));
    }
    return result;
}

// ------------------------------------------------------------
static void sampleSampler(const AnimSampler& s, float t, float* out)
{
    const int N = (int)s.times.size();
    if (N == 0) return;

    if (t >= s.times[N - 1])
    {
        for (int i = 0; i < s.components; ++i)
            out[i] = s.values[(N - 1) * s.components + i];
        return;
    }

    if (t <= s.times[0])
    {
        for (int i = 0; i < s.components; ++i)
            out[i] = s.values[i];
        return;
    }

    int hi = 1;
    while (hi < N - 1 && s.times[hi] < t) ++hi;
    int lo = hi - 1;

    if (s.interp == AnimInterpolation::Step || s.times[hi] == s.times[lo])
    {
        for (int i = 0; i < s.components; ++i)
            out[i] = s.values[lo * s.components + i];
        return;
    }

    float alpha = fmaxf(0.0f, fminf(1.0f, (t - s.times[lo]) / (s.times[hi] - s.times[lo])));
    for (int i = 0; i < s.components; ++i)
        out[i] = s.values[lo * s.components + i] * (1.0f - alpha)
        + s.values[hi * s.components + i] * alpha;

    if (s.components == 4)
    {
        float len = sqrtf(out[0] * out[0] + out[1] * out[1] + out[2] * out[2] + out[3] * out[3]);
        if (len > 1e-9f) { out[0] /= len; out[1] /= len; out[2] /= len; out[3] /= len; }
    }
}

// ------------------------------------------------------------
static void applyAnimation(const Animation& anim, float time, std::vector<NodeTRS>& animTRS)
{
    for (const auto& ch : anim.channels)
    {
        if (ch.nodeIndex < 0 || ch.nodeIndex >= (int)animTRS.size()) continue;
        NodeTRS& trs = animTRS[ch.nodeIndex];
        switch (ch.path)
        {
        case AnimTargetPath::Translation: sampleSampler(ch.sampler, time, trs.t); trs.hasT = true; break;
        case AnimTargetPath::Rotation:    sampleSampler(ch.sampler, time, trs.r); trs.hasR = true; break;
        case AnimTargetPath::Scale:       sampleSampler(ch.sampler, time, trs.s); trs.hasS = true; break;
        }
    }
}

// ------------------------------------------------------------
static void getGlobalNodeMatrixAnimated(const tinygltf::Model& model,
    const std::vector<int>& nodeParents,
    int nodeIndex,
    const std::vector<NodeTRS>& animTRS,
    float out[16])
{
    const auto& node = model.nodes[nodeIndex];
    const NodeTRS& trs = animTRS[nodeIndex];

    float local[16];
    if (!node.matrix.empty())
    {
        for (int i = 0; i < 16; ++i) local[i] = static_cast<float>(node.matrix[i]);
    }
    else
    {
        float tx = trs.hasT ? trs.t[0] : (node.translation.size() >= 3 ? (float)node.translation[0] : 0.0f);
        float ty = trs.hasT ? trs.t[1] : (node.translation.size() >= 3 ? (float)node.translation[1] : 0.0f);
        float tz = trs.hasT ? trs.t[2] : (node.translation.size() >= 3 ? (float)node.translation[2] : 0.0f);
        float qx = trs.hasR ? -trs.r[0] : (node.rotation.size() >= 4 ? (float)node.rotation[0] : 0.0f);
        float qy = trs.hasR ? -trs.r[1] : (node.rotation.size() >= 4 ? (float)node.rotation[1] : 0.0f);
        float qz = trs.hasR ? trs.r[2] : (node.rotation.size() >= 4 ? (float)node.rotation[2] : 0.0f);
        float qw = trs.hasR ? trs.r[3] : (node.rotation.size() >= 4 ? (float)node.rotation[3] : 1.0f);
        float sx = trs.hasS ? trs.s[0] : (node.scale.size() >= 3 ? (float)node.scale[0] : 1.0f);
        float sy = trs.hasS ? trs.s[1] : (node.scale.size() >= 3 ? (float)node.scale[1] : 1.0f);
        float sz = trs.hasS ? trs.s[2] : (node.scale.size() >= 3 ? (float)node.scale[2] : 1.0f);

        float T[16], R[16], S[16], tmp[16];
        bx::mtxTranslate(T, tx, ty, tz);
        bx::mtxFromQuaternion(R, bx::Quaternion(qx, qy, qz, qw));
        bx::mtxScale(S, sx, sy, sz);
        bx::mtxMul(tmp, S, R);
        bx::mtxMul(local, tmp, T);
    }

    int parentIdx = nodeParents[nodeIndex];
    if (parentIdx >= 0)
    {
        float parentMtx[16];
        getGlobalNodeMatrixAnimated(model, nodeParents, parentIdx, animTRS, parentMtx);
        bx::mtxMul(out, local, parentMtx);
    }
    else
    {
        memcpy(out, local, sizeof(local));
    }
}

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

// ------------------------------------------------------------
// Helper: get node local transform
// ------------------------------------------------------------
void getNodeMatrix(const tinygltf::Node& node, float out[16])
{
    bx::mtxIdentity(out);

    // Translation
    if (!node.translation.empty())
    {
        bx::mtxTranslate(out,
            static_cast<float>(node.translation[0]),
            static_cast<float>(node.translation[1]),
            static_cast<float>(node.translation[2])
        );
    }

    // Rotation (quaternion)
    if (!node.rotation.empty())
    {
        float qx = static_cast<float>(node.rotation[0]);
        float qy = static_cast<float>(node.rotation[1]);
        float qz = static_cast<float>(node.rotation[2]);
        float qw = static_cast<float>(node.rotation[3]);

        bx::Quaternion quat(qx, qy, qz, qw);

        float rot[16];
        bx::mtxFromQuaternion(rot, quat);

        float tmp[16];
        bx::mtxMul(tmp, out, rot);
        memcpy(out, tmp, sizeof(tmp));
    }

    // Scale
    if (!node.scale.empty())
    {
        float scale[16];
        bx::mtxScale(scale,
            static_cast<float>(node.scale[0]),
            static_cast<float>(node.scale[1]),
            static_cast<float>(node.scale[2])
        );
        float tmp[16];
        bx::mtxMul(tmp, out, scale);
        memcpy(out, tmp, sizeof(tmp));
    }

    // Override if node has a 4x4 matrix
    if (!node.matrix.empty())
    {
        for (int i = 0; i < 16; ++i) out[i] = static_cast<float>(node.matrix[i]);
    }
}

// ------------------------------------------------------------
// Helper: get global node transform (includes parent hierarchy)
// ------------------------------------------------------------
void getGlobalNodeMatrixStatic(const tinygltf::Model& model,
    const std::vector<int>& nodeParents,
    int nodeIndex,
    float out[16])
{
    const auto& node = model.nodes[nodeIndex];
    float local[16];
    getNodeMatrix(node, local);
    int parentIdx = nodeParents[nodeIndex];

    if (parentIdx >= 0)
    {
        float parentMtx[16];
        getGlobalNodeMatrixStatic(model, nodeParents, parentIdx, parentMtx);
        bx::mtxMul(out, local, parentMtx);
    }
    else
    {
        memcpy(out, local, sizeof(local));
    }
}

// ------------------------------------------------------------
// Helper: upload a glTF image to bgfx
// ------------------------------------------------------------
bgfx::TextureHandle uploadGltfImage(const tinygltf::Model& model, int imageIndex)
{
    if (imageIndex < 0 || imageIndex >= (int)model.images.size())
        return BGFX_INVALID_HANDLE;

    const tinygltf::Image& img = model.images[imageIndex];

    if (img.image.empty())
    {
        std::cerr << "Image " << imageIndex << " has no pixel data." << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    bgfx::TextureFormat::Enum fmt = (img.component == 3)
        ? bgfx::TextureFormat::RGB8
        : bgfx::TextureFormat::RGBA8;

    uint32_t dataSize = img.width * img.height * img.component;
    const bgfx::Memory* mem = bgfx::copy(img.image.data(), dataSize);

    return bgfx::createTexture2D(
        static_cast<uint16_t>(img.width),
        static_cast<uint16_t>(img.height),
        false,
        1,
        fmt,
        BGFX_TEXTURE_NONE,
        mem
    );
}

// ------------------------------------------------------------
// Ray-triangle intersection (Möller–Trumbore), double-sided.
// Returns true and sets t if hit.
// ------------------------------------------------------------
bool rayTriangleIntersect(
    const float orig[3], const float dir[3],
    const float v0[3], const float v1[3], const float v2[3],
    float& t)
{
    const float EPSILON = 1e-7f;
    float edge1[3] = { v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2] };
    float edge2[3] = { v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2] };

    // crossDirEdge2 = dir x edge2, used to compute the determinant
    float crossDirEdge2[3] = {
        dir[1] * edge2[2] - dir[2] * edge2[1],
        dir[2] * edge2[0] - dir[0] * edge2[2],
        dir[0] * edge2[1] - dir[1] * edge2[0]
    };

    // determinant: if near zero, ray is parallel to the triangle
    float det = edge1[0] * crossDirEdge2[0] + edge1[1] * crossDirEdge2[1] + edge1[2] * crossDirEdge2[2];
    if (fabsf(det) < EPSILON) return false;

    float invDet = 1.0f / det;

    // vecToOrigin: vector from triangle vertex v0 to ray origin
    float vecToOrigin[3] = { orig[0] - v0[0], orig[1] - v0[1], orig[2] - v0[2] };

    // u: first barycentric coordinate, must be in [0,1] to be inside the triangle
    float u = invDet * (vecToOrigin[0] * crossDirEdge2[0] + vecToOrigin[1] * crossDirEdge2[1] + vecToOrigin[2] * crossDirEdge2[2]);
    if (u < 0.0f || u > 1.0f) return false;

    // crossOriginEdge1 = vecToOrigin x edge1, used to compute v and t
    float crossOriginEdge1[3] = {
        vecToOrigin[1] * edge1[2] - vecToOrigin[2] * edge1[1],
        vecToOrigin[2] * edge1[0] - vecToOrigin[0] * edge1[2],
        vecToOrigin[0] * edge1[1] - vecToOrigin[1] * edge1[0]
    };

    // v: second barycentric coordinate, u+v must also stay within [0,1]
    float v = invDet * (dir[0] * crossOriginEdge1[0] + dir[1] * crossOriginEdge1[1] + dir[2] * crossOriginEdge1[2]);
    if (v < 0.0f || u + v > 1.0f) return false;

    // t: distance along the ray to the intersection point
    t = invDet * (edge2[0] * crossOriginEdge1[0] + edge2[1] * crossOriginEdge1[1] + edge2[2] * crossOriginEdge1[2]);
    return t > 1e-4f;
}

// ------------------------------------------------------------
// Transform a point by a 4x4 column-major matrix
// ------------------------------------------------------------
void transformPoint(const float m[16], const float in[3], float out[3])
{
    // homogeneous w component, used to perspective-divide the result
    float w = m[3] * in[0] + m[7] * in[1] + m[11] * in[2] + m[15];
    if (fabsf(w) < 1e-9f) w = 1.0f;
    out[0] = (m[0] * in[0] + m[4] * in[1] + m[8] * in[2] + m[12]) / w;
    out[1] = (m[1] * in[0] + m[5] * in[1] + m[9] * in[2] + m[13]) / w;
    out[2] = (m[2] * in[0] + m[6] * in[1] + m[10] * in[2] + m[14]) / w;
}

// ------------------------------------------------------------
int run3d(TextureBuffer* emuScreenTexBuffer, SharedBool* isPowerOn)
{
    // ------------------------------------------------------------
    // SDL INIT
    // ------------------------------------------------------------
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "paperGB",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // ------------------------------------------------------------
    // BGFX INIT
    // ------------------------------------------------------------
    bgfx::Init init;
    init.type = bgfx::RendererType::OpenGL;
    init.resolution.width = WINDOW_WIDTH;
    init.resolution.height = WINDOW_HEIGHT;
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

    bgfx::setViewRect(0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

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
        GLB_PATH
    );

    if (!loaded)
    {
        std::cout << "Failed to load .glb\n";
        return 1;
    }

    // ------------------------------------------------------------
    // PARSE ANIMATIONS
    // ------------------------------------------------------------
    std::vector<Animation> animations = parseAnimations(gltfModel);

    std::vector<AnimationState> animStates;
    std::vector<NodeTRS> animTRS(gltfModel.nodes.size());

    Uint64 perfFreq = SDL_GetPerformanceFrequency();
    Uint64 lastPerfTick = SDL_GetPerformanceCounter();

    auto triggerAnimation = [&](int animIdx)
        {
            if (animIdx < 0 || animIdx >= (int)animations.size()) return;

            // If this animation is already in the list, restart it
            for (auto& s : animStates)
            {
                if (s.animIndex == animIdx)
                {
                    s.time = 0.0f;
                    s.playing = true;
                    return;
                }
            }

            // Otherwise add a new state
            animStates.push_back({ animIdx, 0.0f, true });
        };

    // Reset only the nodes affected by a given animation
    auto resetAnim = [&](int animIdx)
        {
            if (animIdx < 0 || animIdx >= (int)animations.size()) return;

            for (const auto& ch : animations[animIdx].channels)
                if (ch.nodeIndex >= 0 && ch.nodeIndex < (int)animTRS.size())
                    animTRS[ch.nodeIndex] = NodeTRS{};

            // Remove from active states
            animStates.erase(
                std::remove_if(animStates.begin(), animStates.end(),
                    [animIdx](const AnimationState& s) { return s.animIndex == animIdx; }),
                animStates.end()
            );
        };

    // Look up an animation by name and trigger it, skipping the first N keyframes if already playing
    auto triggerAnimByName = [&](const std::string& animName)
        {
            int foundIdx = -1;
            for (int i = 0; i < (int)animations.size(); ++i)
                if (animations[i].name == animName) { foundIdx = i; break; }

            if (foundIdx == -1) return;

            // Check if any anim is already playing before we clear others
            bool alreadyPlaying = std::any_of(animStates.begin(), animStates.end(),
                [](const AnimationState& s) { return s.playing; });

            // Only clear other directional anims when switching directions
            static const std::vector<std::string> dirNames = {
                "Up", "Down", "Left", "Right", "UpLeft", "UpRight", "DownLeft", "DownRight"
            };
            bool isDirectional = std::find(dirNames.begin(), dirNames.end(), animName) != dirNames.end();
            if (isDirectional)
            {
                for (const auto& name : dirNames)
                {
                    if (name == animName) continue;
                    for (int i = 0; i < (int)animations.size(); ++i)
                        if (animations[i].name == name) { resetAnim(i); break; }
                }
            }

            const auto& firstChannel = animations[foundIdx].channels;
            int skipTo = std::min(5, (int)firstChannel[0].sampler.times.size() - 1);
            float skipTime = (isDirectional && alreadyPlaying && !firstChannel.empty() && (int)firstChannel[0].sampler.times.size() > skipTo)
                ? firstChannel[0].sampler.times[skipTo]
                : 0.0f;
            triggerAnimation(foundIdx);

            // Apply skip time to the state we just triggered
            for (auto& s : animStates)
                if (s.animIndex == foundIdx) { s.time = skipTime; break; }
        };

    // ------------------------------------------------------------
    // Upload all glTF images to bgfx (cache by image index)
    // ------------------------------------------------------------
    std::unordered_map<int, bgfx::TextureHandle> textureCache;
    for (int i = 0; i < (int)gltfModel.textures.size(); ++i)
    {
        int imageIdx = gltfModel.textures[i].source;
        if (textureCache.find(imageIdx) == textureCache.end())
            textureCache[imageIdx] = uploadGltfImage(gltfModel, imageIdx);
    }

    auto getMaterialTexture = [&](int materialIndex) -> bgfx::TextureHandle
        {
            if (materialIndex < 0 || materialIndex >= (int)gltfModel.materials.size())
                return BGFX_INVALID_HANDLE;
            const auto& mat = gltfModel.materials[materialIndex];
            int texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
            if (texIndex < 0)
                return BGFX_INVALID_HANDLE;
            int imageIdx = gltfModel.textures[texIndex].source;
            auto it = textureCache.find(imageIdx);
            if (it == textureCache.end())
                return BGFX_INVALID_HANDLE;
            return it->second;
        };

    // ------------------------------------------------------------
    // Build node parent relationships
    // ------------------------------------------------------------
    std::vector<int> nodeParents(gltfModel.nodes.size(), -1);
    for (size_t i = 0; i < gltfModel.nodes.size(); ++i)
    {
        const auto& node = gltfModel.nodes[i];
        for (int childIdx : node.children)
            nodeParents[childIdx] = int(i);
    }

    std::unordered_set<int> sceneRoots;

    if (!gltfModel.scenes.empty())
    {
        int sceneIndex = gltfModel.defaultScene >= 0 ? gltfModel.defaultScene : 0;
        for (int rootNode : gltfModel.scenes[sceneIndex].nodes)
            sceneRoots.insert(rootNode);
    }

    // If no scene roots found, fall back to nodes with no parent
    if (sceneRoots.empty())
    {
        for (size_t i = 0; i < gltfModel.nodes.size(); ++i)
            if (nodeParents[i] == -1)
                sceneRoots.insert(int(i));
    }

    // ------------------------------------------------------------
    // Map mesh name -> node index (for click-to-animate)
    // ------------------------------------------------------------
    std::unordered_map<std::string, int> meshNameToNodeIndex;
    for (size_t nodeIdx = 0; nodeIdx < gltfModel.nodes.size(); ++nodeIdx)
    {
        const auto& node = gltfModel.nodes[nodeIdx];
        if (node.mesh >= 0)
            meshNameToNodeIndex[gltfModel.meshes[node.mesh].name] = (int)nodeIdx;
    }

    // ------------------------------------------------------------
    // LOAD SHADERS
    // ------------------------------------------------------------
    std::vector<char> vsBuffer;
    bgfx::ShaderHandle vsh = loadShader(VS_BIN_PATH, vsBuffer);
    std::vector<char> fsBuffer;
    bgfx::ShaderHandle fsh = loadShader(FS_BIN_PATH, fsBuffer);
    bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh, true);

    bgfx::UniformHandle s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

    // ------------------------------------------------------------
    // LINE SHADER + LAYOUT for ray visualisation
    // ------------------------------------------------------------
    bgfx::VertexLayout lineLayout;
    lineLayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    std::vector<char> vsLineBuffer, fsLineBuffer;
    bgfx::ShaderHandle vshLine = loadShader(VS_LINE_BIN_PATH, vsLineBuffer);
    bgfx::ShaderHandle fshLine = loadShader(FS_LINE_BIN_PATH, fsLineBuffer);
    bgfx::ProgramHandle lineProgram = bgfx::createProgram(vshLine, fshLine, true);

    // Persistent ray endpoints (world space)
    float g_rayOrigin[3] = { 0.0f, 0.0f, 0.0f };
    float g_rayEnd[3] = { 0.0f, 0.0f, 0.0f };
    bool  g_hasRay = false;
    bool  g_showRay = false; // DEBUG: toggle ray visualisation with R key

    // ------------------------------------------------------------
    // mirrorX matrix (same as render path) — constant, build once
    // ------------------------------------------------------------
    float mirrorX[16];
    bx::mtxIdentity(mirrorX);
    mirrorX[0] = -1.0f;

    // ------------------------------------------------------------
    // PREPARE BUFFERS FOR ALL MESHES
    // ------------------------------------------------------------
    std::vector<MeshInstance> allMeshes;

    // Vertex layout is the same for every primitive, so define it once here
    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    for (size_t nodeIdx = 0; nodeIdx < gltfModel.nodes.size(); ++nodeIdx)
    {
        const auto& node = gltfModel.nodes[nodeIdx];
        if (node.mesh < 0) continue;

        const auto& mesh = gltfModel.meshes[node.mesh];
        const std::string& meshName = mesh.name;

        for (const auto& primitive : mesh.primitives)
        {
            // Guard against missing POSITION attribute
            auto posIt = primitive.attributes.find("POSITION");
            if (posIt == primitive.attributes.end()) {
                std::cerr << "Primitive missing POSITION attribute, skipping." << std::endl;
                continue;
            }

            const auto& posAccessor = gltfModel.accessors.at(posIt->second);
            const auto& posView = gltfModel.bufferViews[posAccessor.bufferView];
            const auto& posBuffer = gltfModel.buffers[posView.buffer];

            const uint8_t* posBase = posBuffer.data.data() + posView.byteOffset + posAccessor.byteOffset;
            size_t posStride = posView.byteStride;
            if (posStride == 0) posStride = sizeof(float) * 3;

            // Guard against missing NORMAL attribute
            auto normIt = primitive.attributes.find("NORMAL");
            if (normIt == primitive.attributes.end()) {
                std::cerr << "Primitive missing NORMAL attribute, skipping." << std::endl;
                continue;
            }

            const auto& normAccessor = gltfModel.accessors.at(normIt->second);
            const auto& normView = gltfModel.bufferViews[normAccessor.bufferView];
            const auto& normBuffer = gltfModel.buffers[normView.buffer];

            const uint8_t* normBase = normBuffer.data.data() + normView.byteOffset + normAccessor.byteOffset;
            size_t normStride = normView.byteStride;
            if (normStride == 0) normStride = sizeof(float) * 3;

            // TEXCOORD_0 (optional)
            const uint8_t* uvBase = nullptr;
            size_t uvStride = sizeof(float) * 2;
            auto uvIt = primitive.attributes.find("TEXCOORD_0");
            if (uvIt != primitive.attributes.end())
            {
                const auto& uvAccessor = gltfModel.accessors.at(uvIt->second);
                const auto& uvView = gltfModel.bufferViews[uvAccessor.bufferView];
                const auto& uvBuffer = gltfModel.buffers[uvView.buffer];
                uvBase = uvBuffer.data.data() + uvView.byteOffset + uvAccessor.byteOffset;
                uvStride = uvView.byteStride ? uvView.byteStride : sizeof(float) * 2;
            }

            uint32_t vertexCount = posAccessor.count;

            const auto& indexAccessor = gltfModel.accessors[primitive.indices];
            const auto& indexView = gltfModel.bufferViews[indexAccessor.bufferView];
            const auto& indexBuffer = gltfModel.buffers[indexView.buffer];

            std::vector<uint16_t> indices16;
            std::vector<uint32_t> indices32;
            bool use32 = false;

            switch (indexAccessor.componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            {
                const uint16_t* ptr = reinterpret_cast<const uint16_t*>(
                    &indexBuffer.data[indexAccessor.byteOffset + indexView.byteOffset]);
                indices16.assign(ptr, ptr + indexAccessor.count);
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            {
                const uint32_t* ptr = reinterpret_cast<const uint32_t*>(
                    &indexBuffer.data[indexAccessor.byteOffset + indexView.byteOffset]);
                indices32.assign(ptr, ptr + indexAccessor.count);
                use32 = true;
                break;
            }
            default:
                std::cerr << "Unsupported index component type!" << std::endl;
                continue;
            }

            std::vector<Vertex> vertices(vertexCount);
            for (uint32_t i = 0; i < vertexCount; ++i)
            {
                const float* pos = reinterpret_cast<const float*>(posBase + posStride * i);
                vertices[i].x = pos[0];
                vertices[i].y = pos[1];
                vertices[i].z = pos[2];

                const float* n = reinterpret_cast<const float*>(normBase + normStride * i);
                vertices[i].nx = n[0];
                vertices[i].ny = n[1];
                vertices[i].nz = n[2];

                if (uvBase)
                {
                    const float* uv = reinterpret_cast<const float*>(uvBase + uvStride * i);
                    vertices[i].u = uv[0];
                    vertices[i].v = uv[1];
                }
                else
                {
                    vertices[i].u = 0.0f;
                    vertices[i].v = 0.0f;
                }
            }

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

            bgfx::TextureHandle tex = getMaterialTexture(primitive.material);

            // ----------------------------------------------------------
            // Build CPU-side mesh for ray testing.
            // Store local-space positions; world positions are updated each frame.
            // ----------------------------------------------------------
            CpuMesh cpuMesh;

            cpuMesh.localPositions.resize(vertexCount * 3);
            cpuMesh.positions.resize(vertexCount * 3);
            for (uint32_t i = 0; i < vertexCount; ++i)
            {
                const float* lp = reinterpret_cast<const float*>(posBase + posStride * i);
                cpuMesh.localPositions[i * 3 + 0] = lp[0];
                cpuMesh.localPositions[i * 3 + 1] = lp[1];
                cpuMesh.localPositions[i * 3 + 2] = lp[2];
            }

            if (use32)
            {
                cpuMesh.indices.assign(indices32.begin(), indices32.end());
            }
            else
            {
                cpuMesh.indices.resize(indices16.size());
                for (size_t ii = 0; ii < indices16.size(); ++ii)
                    cpuMesh.indices[ii] = indices16[ii];
            }

            MeshBuffers buf = { vbh, ibh, (uint32_t)indexAccessor.count, use32, tex };
            allMeshes.push_back({ buf, static_cast<int>(nodeIdx), meshName, std::move(cpuMesh) });
        }
    }

    // ------------------------------------------------------------
    // DYNAMIC SCREEN TEXTURE
    // ------------------------------------------------------------

    // Find the emuScreen mesh instance and replace its texture with a dynamic one
    bgfx::TextureHandle emuScreenTex = bgfx::createTexture2D(
        (uint16_t)emuScreenTexBuffer->width,
        (uint16_t)emuScreenTexBuffer->height,
        false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE
    );

    for (auto& meshInst : allMeshes)
    {
        if (meshInst.meshName == "emuScreen")
            meshInst.buffers.texture = emuScreenTex;
    }

    // ------------------------------------------------------------
    // MAIN LOOP
    // ------------------------------------------------------------
    bool running = true;
    float angleH = 0.0f;
    float angleV = 0.0f;

    bool isDragging = false;
    int lastMouseX = 0;
    int lastMouseY = 0;

    int currentWidth = WINDOW_WIDTH;
    int currentHeight = WINDOW_HEIGHT;

    bool upHeld = false;
    bool downHeld = false;
    bool leftHeld = false;
    bool rightHeld = false;

    // --- Cartridge state machine ---
    enum class CartState { Idle, Ejecting, DialogOpen, Inserting };
    CartState cartState = CartState::Idle;
    bool cartDialogSelected = false;
    std::string cartSelectedPath;
    std::atomic<bool> cartDialogDone{ false };

    // Find CartSpin and Eject indices once
    int cartSpinIdx = -1;
    int cartEjectIdx = -1;
    for (int i = 0; i < (int)animations.size(); ++i)
    {
        if (animations[i].name == "CartSpin") cartSpinIdx = i;
        if (animations[i].name == "Eject")    cartEjectIdx = i;
    }

    std::vector<float> nodeMatrixCache(gltfModel.nodes.size() * 16);

    // Build a parent-first ordered list so each node's parent is always computed before it
    std::vector<int> nodeOrder;
    {
        std::vector<bool> visited(gltfModel.nodes.size(), false);
        std::function<void(int)> visit = [&](int i) {
            if (visited[i]) return;
            int p = nodeParents[i];
            if (p >= 0) visit(p);
            visited[i] = true;
            nodeOrder.push_back(i);
            };
        for (int i = 0; i < (int)gltfModel.nodes.size(); ++i)
            visit(i);
    }

    // Pre-fill the node matrix cache so frame 0 is valid
    for (int i : nodeOrder)
    {
        const auto& node = gltfModel.nodes[i];
        const NodeTRS& trs = animTRS[i];

        float local[16];
        if (!node.matrix.empty())
        {
            for (int j = 0; j < 16; ++j) local[j] = static_cast<float>(node.matrix[j]);
        }
        else
        {
            float tx = trs.hasT ? trs.t[0] : (node.translation.size() >= 3 ? (float)node.translation[0] : 0.0f);
            float ty = trs.hasT ? trs.t[1] : (node.translation.size() >= 3 ? (float)node.translation[1] : 0.0f);
            float tz = trs.hasT ? trs.t[2] : (node.translation.size() >= 3 ? (float)node.translation[2] : 0.0f);
            float qx = trs.hasR ? -trs.r[0] : (node.rotation.size() >= 4 ? (float)node.rotation[0] : 0.0f);
            float qy = trs.hasR ? -trs.r[1] : (node.rotation.size() >= 4 ? (float)node.rotation[1] : 0.0f);
            float qz = trs.hasR ? trs.r[2] : (node.rotation.size() >= 4 ? (float)node.rotation[2] : 0.0f);
            float qw = trs.hasR ? trs.r[3] : (node.rotation.size() >= 4 ? (float)node.rotation[3] : 1.0f);
            float sx = trs.hasS ? trs.s[0] : (node.scale.size() >= 3 ? (float)node.scale[0] : 1.0f);
            float sy = trs.hasS ? trs.s[1] : (node.scale.size() >= 3 ? (float)node.scale[1] : 1.0f);
            float sz = trs.hasS ? trs.s[2] : (node.scale.size() >= 3 ? (float)node.scale[2] : 1.0f);

            float T[16], R[16], S[16], tmp[16];
            bx::mtxTranslate(T, tx, ty, tz);
            bx::mtxFromQuaternion(R, bx::Quaternion(qx, qy, qz, qw));
            bx::mtxScale(S, sx, sy, sz);
            bx::mtxMul(tmp, S, R);
            bx::mtxMul(local, tmp, T);
        }

        int p = nodeParents[i];
        if (p >= 0)
            bx::mtxMul(&nodeMatrixCache[i * 16], local, &nodeMatrixCache[p * 16]);
        else
            memcpy(&nodeMatrixCache[i * 16], local, sizeof(local));
    }

    // Also pre-fill CPU mesh world positions for pick testing
    for (auto& mi : allMeshes)
    {
        float finalMtx[16];
        bx::mtxMul(finalMtx, &nodeMatrixCache[mi.nodeIndex * 16], mirrorX);
        uint32_t vertexCount = (uint32_t)(mi.cpuMesh.localPositions.size() / 3);
        for (uint32_t i = 0; i < vertexCount; ++i)
        {
            transformPoint(finalMtx,
                &mi.cpuMesh.localPositions[i * 3],
                &mi.cpuMesh.positions[i * 3]);
        }
    }

    while (running)
    {
        // --- Delta time ---
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - lastPerfTick) / (float)perfFreq;
        lastPerfTick = now;
        if (dt > 0.1f) dt = 0.1f;

        // --- Advance all active animations ---
        for (auto& s : animStates)
        {
            if (!s.playing) continue;
            const Animation& curAnim = animations[s.animIndex];
            s.time += dt;
            if (s.time >= curAnim.duration)
            {
                s.time = curAnim.duration;
                s.playing = false;
            }
        }

        // --- Rebuild per-node TRS from all active animations ---
        std::fill(animTRS.begin(), animTRS.end(), NodeTRS{});
        for (const auto& s : animStates)
            if (s.animIndex >= 0)
                applyAnimation(animations[s.animIndex], s.time, animTRS);

        // --- Cart state machine ---
        if (cartState == CartState::Ejecting)
        {
            // Wait until the Eject animation has finished playing
            bool ejectDone = true;
            if (cartEjectIdx >= 0)
            {
                for (const auto& s : animStates)
                    if (s.animIndex == cartEjectIdx && s.playing) { ejectDone = false; break; }
            }

            if (ejectDone)
            {
                cartState = CartState::DialogOpen;

                // Start CartSpin looping at normal speed
                if (cartSpinIdx >= 0)
                    triggerAnimByName("CartSpin");

                // Open file dialog on background thread
                std::thread([&]() {
                    OPENFILENAMEA ofn = {};
                    char filePath[MAX_PATH] = {};
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = (HWND)wmi.info.win.window;
                    ofn.lpstrFilter = "GameBoy ROMs\0*.gb\0All Files\0*.*\0";
                    ofn.lpstrFile = filePath;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrTitle = "Select a GameBoy ROM";
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                    bool ok = GetOpenFileNameA(&ofn);
                    cartDialogSelected = ok;
                    cartSelectedPath = ok ? std::string(filePath) : "";
                    cartDialogDone.store(true, std::memory_order_release);
                    }).detach();
            }
        }
        else if (cartState == CartState::DialogOpen)
        {
            // Keep CartSpin looping — but only if dialog is still open
            if (cartSpinIdx >= 0 && !cartDialogDone.load(std::memory_order_acquire))
            {
                AnimationState* spin = nullptr;
                for (auto& s : animStates)
                    if (s.animIndex == cartSpinIdx) { spin = &s; break; }

                if (!spin)
                    triggerAnimation(cartSpinIdx);
                else if (!spin->playing)
                {
                    spin->time = 0.0f;
                    spin->playing = true;
                }
            }

            // Dialog closed
            if (cartDialogDone.load(std::memory_order_acquire))
            {
                cartState = CartState::Inserting;
            }
        }
        else if (cartState == CartState::Inserting)
        {
            if (cartSpinIdx >= 0)
            {
                AnimationState* spin = nullptr;
                for (auto& s : animStates)
                    if (s.animIndex == cartSpinIdx) { spin = &s; break; }

                if (spin && spin->playing)
                {
                    // Advance animation faster
                    float speedModifier = animations[cartSpinIdx].duration - spin->time + 2;
                    spin->time += speedModifier * dt;
                    if (spin->time >= animations[cartSpinIdx].duration)
                    {
                        spin->time = animations[cartSpinIdx].duration;
                        spin->playing = false;
                    }
                }

                // Spin finished — play Insert if a file was selected
                bool spinDone = !spin || !spin->playing;
                if (spinDone)
                {
                    cartState = CartState::Idle;
                    if (cartDialogDone)
                    {
                        triggerAnimByName("Insert");
                        std::cout << "Selected ROM: " << cartSelectedPath << "\n";
                        // TODO: load cartSelectedPath
                        //if cartDialogSelected change emu path to new, otherwise keep original path

                    }
                }
            }
            else
            {
                cartState = CartState::Idle;
            }
        }

        // --- Rebuild node matrix cache using parent-first order (no recursion) ---
        for (int i : nodeOrder)
        {
            const auto& node = gltfModel.nodes[i];
            const NodeTRS& trs = animTRS[i];

            float local[16];
            if (!node.matrix.empty())
            {
                for (int j = 0; j < 16; ++j) local[j] = static_cast<float>(node.matrix[j]);
            }
            else
            {
                float tx = trs.hasT ? trs.t[0] : (node.translation.size() >= 3 ? (float)node.translation[0] : 0.0f);
                float ty = trs.hasT ? trs.t[1] : (node.translation.size() >= 3 ? (float)node.translation[1] : 0.0f);
                float tz = trs.hasT ? trs.t[2] : (node.translation.size() >= 3 ? (float)node.translation[2] : 0.0f);
                float qx = trs.hasR ? -trs.r[0] : (node.rotation.size() >= 4 ? (float)node.rotation[0] : 0.0f);
                float qy = trs.hasR ? -trs.r[1] : (node.rotation.size() >= 4 ? (float)node.rotation[1] : 0.0f);
                float qz = trs.hasR ? trs.r[2] : (node.rotation.size() >= 4 ? (float)node.rotation[2] : 0.0f);
                float qw = trs.hasR ? trs.r[3] : (node.rotation.size() >= 4 ? (float)node.rotation[3] : 1.0f);
                float sx = trs.hasS ? trs.s[0] : (node.scale.size() >= 3 ? (float)node.scale[0] : 1.0f);
                float sy = trs.hasS ? trs.s[1] : (node.scale.size() >= 3 ? (float)node.scale[1] : 1.0f);
                float sz = trs.hasS ? trs.s[2] : (node.scale.size() >= 3 ? (float)node.scale[2] : 1.0f);

                float T[16], R[16], S[16], tmp[16];
                bx::mtxTranslate(T, tx, ty, tz);
                bx::mtxFromQuaternion(R, bx::Quaternion(qx, qy, qz, qw));
                bx::mtxScale(S, sx, sy, sz);
                bx::mtxMul(tmp, S, R);
                bx::mtxMul(local, tmp, T);
            }

            int p = nodeParents[i];
            if (p >= 0)
                bx::mtxMul(&nodeMatrixCache[i * 16], local, &nodeMatrixCache[p * 16]);
            else
                memcpy(&nodeMatrixCache[i * 16], local, sizeof(local));
        }

        SDL_GetWindowSize(window, &currentWidth, &currentHeight);

        // Compute matrices BEFORE event polling so pick ray is always current
        float camX = cosf(angleV) * sinf(angleH) * CAMERA_DISTANCE;
        float camY = sinf(angleV) * CAMERA_DISTANCE;
        float camZ = cosf(angleV) * cosf(angleH) * CAMERA_DISTANCE;

        float view[16];
        float proj[16];
        bx::mtxLookAt(view, { camX, camY, camZ }, { 0.0f, 0.0f, 0.0f });
        bx::mtxProj(proj, CAMERA_FOV_DEG, (float)currentWidth / (float)currentHeight, CAMERA_NEAR, CAMERA_FAR, bgfx::getCaps()->homogeneousDepth);

        SDL_Event e;

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = false;
            }
            else if (e.type == SDL_KEYDOWN && !e.key.repeat)
            {
                bool wasMovementKey = false;
                switch (e.key.keysym.sym)
                {
                case SDLK_r: g_showRay = !g_showRay; std::cout << "Ray visualisation: " << (g_showRay ? "ON" : "OFF") << "\n"; break;
                case SDLK_w: upHeld = true; wasMovementKey = true; break;
                case SDLK_s: downHeld = true; wasMovementKey = true; break;
                case SDLK_a: leftHeld = true; wasMovementKey = true; break;
                case SDLK_d: rightHeld = true; wasMovementKey = true; break;
                case SDLK_SPACE: triggerAnimByName("APress"); break;
                case SDLK_e:     triggerAnimByName("BPress"); break;
                }

                if (wasMovementKey && (upHeld || downHeld || leftHeld || rightHeld))
                {
                    std::string animName;
                    if (upHeld && leftHeld)    animName = "UpLeft";
                    else if (upHeld && rightHeld)   animName = "UpRight";
                    else if (downHeld && leftHeld)  animName = "DownLeft";
                    else if (downHeld && rightHeld) animName = "DownRight";
                    else if (upHeld)                animName = "Up";
                    else if (downHeld)              animName = "Down";
                    else if (rightHeld)             animName = "Right";
                    else if (leftHeld)              animName = "Left";
                    triggerAnimByName(animName);
                }
            }
            else if (e.type == SDL_KEYUP && !e.key.repeat)
            {
                bool wasDirectionalKey = false;
                switch (e.key.keysym.sym)
                {
                case SDLK_w: upHeld = false; wasDirectionalKey = true; break;
                case SDLK_s: downHeld = false; wasDirectionalKey = true; break;
                case SDLK_a: leftHeld = false; wasDirectionalKey = true; break;
                case SDLK_d: rightHeld = false; wasDirectionalKey = true; break;
                case SDLK_SPACE: for (int i = 0; i < (int)animations.size(); ++i) if (animations[i].name == "APress") { resetAnim(i); break; } break;
                case SDLK_e:     for (int i = 0; i < (int)animations.size(); ++i) if (animations[i].name == "BPress") { resetAnim(i); break; } break;
                }

                if (wasDirectionalKey)
                {
                    const Uint8* keyState = SDL_GetKeyboardState(NULL);
                    if (keyState[SDL_SCANCODE_W]) upHeld = true;
                    if (keyState[SDL_SCANCODE_S]) downHeld = true;
                    if (keyState[SDL_SCANCODE_A]) leftHeld = true;
                    if (keyState[SDL_SCANCODE_D]) rightHeld = true;

                    if (!upHeld && !downHeld && !leftHeld && !rightHeld)
                    {
                        for (const std::string& name : { "Up", "Down", "Left", "Right", "UpLeft", "UpRight", "DownLeft", "DownRight" })
                            for (int i = 0; i < (int)animations.size(); ++i)
                                if (animations[i].name == name) { resetAnim(i); break; }
                    }
                    else
                    {
                        std::string animName;
                        if (upHeld && leftHeld)    animName = "UpLeft";
                        else if (upHeld && rightHeld)   animName = "UpRight";
                        else if (downHeld && leftHeld)  animName = "DownLeft";
                        else if (downHeld && rightHeld) animName = "DownRight";
                        else if (upHeld)                animName = "Up";
                        else if (downHeld)              animName = "Down";
                        else if (rightHeld)             animName = "Right";
                        else if (leftHeld)              animName = "Left";
                        triggerAnimByName(animName);
                    }
                }
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                // --- Update CPU mesh world positions for pick testing — only on click ---
                for (auto& mi : allMeshes)
                {
                    float finalMtx[16];
                    bx::mtxMul(finalMtx, &nodeMatrixCache[mi.nodeIndex * 16], mirrorX);
                    uint32_t vertexCount = (uint32_t)(mi.cpuMesh.localPositions.size() / 3);
                    for (uint32_t i = 0; i < vertexCount; ++i)
                    {
                        transformPoint(finalMtx,
                            &mi.cpuMesh.localPositions[i * 3],
                            &mi.cpuMesh.positions[i * 3]);
                    }
                }

                // Build pick ray: origin at camera, direction toward unprojected far point
                float rayOrigin[3] = { camX, camY, camZ };

                // Unproject two points at NDC z=-1 (near) and z=1 (far)
                // using the full inverse-projection path so handedness is correct
                float ndcX = (2.0f * e.button.x / currentWidth) - 1.0f;
                float ndcY = 1.0f - (2.0f * e.button.y / currentHeight);

                float invProj[16];
                bx::mtxInverse(invProj, proj);
                float invView[16];
                bx::mtxInverse(invView, view);

                // Unproject a point on the far plane into world space
                float clipFar[3] = { ndcX, ndcY, 1.0f };
                float viewSpaceFar[3];
                transformPoint(invProj, clipFar, viewSpaceFar);

                float worldSpaceFar[3];
                transformPoint(invView, viewSpaceFar, worldSpaceFar);

                // Direction is from camera origin toward that world space point
                float rayDir[3] =
                {
                    worldSpaceFar[0] - rayOrigin[0],
                    worldSpaceFar[1] - rayOrigin[1],
                    worldSpaceFar[2] - rayOrigin[2]
                };

                float rayLen = sqrtf(rayDir[0] * rayDir[0] + rayDir[1] * rayDir[1] + rayDir[2] * rayDir[2]);
                if (rayLen > 1e-9f) { rayDir[0] /= rayLen; rayDir[1] /= rayLen; rayDir[2] /= rayLen; }

                // Store ray for visualisation
                g_rayOrigin[0] = rayOrigin[0] + rayDir[0] * 0.05f;
                g_rayOrigin[1] = rayOrigin[1] + rayDir[1] * 0.05f;
                g_rayOrigin[2] = rayOrigin[2] + rayDir[2] * 0.05f;
                g_rayEnd[0] = rayOrigin[0] + rayDir[0] * 100.0f;
                g_rayEnd[1] = rayOrigin[1] + rayDir[1] * 100.0f;
                g_rayEnd[2] = rayOrigin[2] + rayDir[2] * 100.0f;
                g_hasRay = true;

                // CPU mesh positions are now updated each frame with mirrorX already applied,
                // so use the unmirrored ray directly for picking.
                float bestT = std::numeric_limits<float>::max();
                std::string bestMesh = "(none)";

                for (const auto& mi : allMeshes)
                {
                    const auto& cpu = mi.cpuMesh;
                    size_t triCount = cpu.indices.size() / 3;
                    for (size_t tri = 0; tri < triCount; ++tri)
                    {
                        uint32_t i0 = cpu.indices[tri * 3 + 0];
                        uint32_t i1 = cpu.indices[tri * 3 + 1];
                        uint32_t i2 = cpu.indices[tri * 3 + 2];

                        float t;
                        if (rayTriangleIntersect(rayOrigin, rayDir,
                            &cpu.positions[i0 * 3],
                            &cpu.positions[i1 * 3],
                            &cpu.positions[i2 * 3], t))
                        {
                            if (t < bestT)
                            {
                                bestT = t;
                                bestMesh = mi.meshName;
                            }
                        }
                    }
                }

                bool firstHitDraggable = DRAGGABLE_MESH_NAMES.count(bestMesh) > 0;

                // --- Trigger animation on click of a non-draggable mesh ---
                if (!firstHitDraggable && bestT < std::numeric_limits<float>::max())
                {
                    auto nodeIt = meshNameToNodeIndex.find(bestMesh);
                    if (nodeIt != meshNameToNodeIndex.end())
                    {
                        if (bestMesh == "switch_low_External_0")
                        {
                            std::lock_guard<std::mutex> lock(isPowerOn->mutex);
                            if (!isPowerOn->value)
                            {
                                animStates.erase(
                                    std::remove_if(animStates.begin(), animStates.end(),
                                        [&](const AnimationState& s) { return s.animIndex >= 0 && animations[s.animIndex].name == "PowerOff"; }),
                                    animStates.end()
                                );
                                triggerAnimByName("PowerOn");
                                isPowerOn->value = true;
                            }
                            else
                            {
                                animStates.erase(
                                    std::remove_if(animStates.begin(), animStates.end(),
                                        [&](const AnimationState& s) { return s.animIndex >= 0 && animations[s.animIndex].name == "PowerOn"; }),
                                    animStates.end()
                                );
                                triggerAnimByName("PowerOff");
                                isPowerOn->value = false;
                            }
                        }
                        else if (bestMesh == "chip_low_Cartridge_0")
                        {
                            if (cartState == CartState::Idle)
                            {
                                animStates.erase(
                                    std::remove_if(animStates.begin(), animStates.end(),
                                        [&](const AnimationState& s) { return s.animIndex >= 0 && animations[s.animIndex].name == "Insert"
                                        || animations[s.animIndex].name == "Eject"
                                        || animations[s.animIndex].name == "CartSpin"; }),
                                    animStates.end()
                                );
                                triggerAnimByName("Eject");
                                cartDialogDone.store(false, std::memory_order_relaxed);
                                cartState = CartState::Ejecting;
                            }
                        }
                    }
                }

                // --- DEBUG ---
                if (g_showRay)
                {
                    std::cout << "=== CLICK at (" << e.button.x << ", " << e.button.y << ") ===\n";
                    std::cout << "  Ray origin : (" << rayOrigin[0] << ", " << rayOrigin[1] << ", " << rayOrigin[2] << ")\n";
                    std::cout << "  Ray dir    : (" << rayDir[0] << ", " << rayDir[1] << ", " << rayDir[2] << ")\n";

                    if (bestT < std::numeric_limits<float>::max())
                    {
                        float hitPt[3] = {
                            rayOrigin[0] + rayDir[0] * bestT,
                            rayOrigin[1] + rayDir[1] * bestT,
                            rayOrigin[2] + rayDir[2] * bestT
                        };
                        std::cout << "  First hit mesh : \"" << bestMesh << "\" t=" << bestT << "\n";
                        std::cout << "  Hit point      : (" << hitPt[0] << ", " << hitPt[1] << ", " << hitPt[2] << ")\n";
                        std::cout << "  Will drag      : " << (firstHitDraggable ? "YES" : "NO") << "\n";
                    }
                    else
                    {
                        std::cout << "  No triangles hit at all\n";
                    }

                    // Collect all per-mesh hits, sort by distance, then print with order
                    struct HitResult { std::string name; float t; bool draggable; };
                    std::vector<HitResult> allHits;

                    for (const auto& mi : allMeshes)
                    {
                        const auto& cpu = mi.cpuMesh;
                        float closestForMesh = std::numeric_limits<float>::max();
                        size_t triCount = cpu.indices.size() / 3;
                        for (size_t tri = 0; tri < triCount; ++tri)
                        {
                            uint32_t i0 = cpu.indices[tri * 3 + 0];
                            uint32_t i1 = cpu.indices[tri * 3 + 1];
                            uint32_t i2 = cpu.indices[tri * 3 + 2];
                            float t;
                            if (rayTriangleIntersect(rayOrigin, rayDir,
                                &cpu.positions[i0 * 3],
                                &cpu.positions[i1 * 3],
                                &cpu.positions[i2 * 3], t) && t < closestForMesh)
                                closestForMesh = t;
                        }
                        if (closestForMesh < std::numeric_limits<float>::max())
                            allHits.push_back({ mi.meshName, closestForMesh, DRAGGABLE_MESH_NAMES.count(mi.meshName) > 0 });
                    }

                    std::sort(allHits.begin(), allHits.end(), [](const HitResult& a, const HitResult& b) {
                        return a.t < b.t;
                        });

                    for (size_t i = 0; i < allHits.size(); ++i)
                    {
                        const auto& h = allHits[i];
                        float hitPt[3] = {
                            rayOrigin[0] + rayDir[0] * h.t,
                            rayOrigin[1] + rayDir[1] * h.t,
                            rayOrigin[2] + rayDir[2] * h.t
                        };
                        std::cout << "  [" << (i + 1) << "] \"" << h.name << "\" t=" << h.t
                            << " hit=(" << hitPt[0] << ", " << hitPt[1] << ", " << hitPt[2] << ")"
                            << (h.draggable ? " [DRAGGABLE]" : "") << "\n";
                    }
                }
                // --- END DEBUG ---

                if (firstHitDraggable)
                {
                    isDragging = true;
                    lastMouseX = e.button.x;
                    lastMouseY = e.button.y;
                }
            }
            else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
            {
                isDragging = false;
            }
            else if (e.type == SDL_MOUSEMOTION)
            {
                if (isDragging)
                {
                    int dx = e.motion.x - lastMouseX;
                    int dy = e.motion.y - lastMouseY;
                    angleH += dx * CAMERA_ORBIT_SPEED;
                    angleV += dy * CAMERA_ORBIT_SPEED;
                    angleV = bx::clamp(angleV, -bx::kPiHalf + 0.01f, bx::kPiHalf - 0.01f);
                    lastMouseX = e.motion.x;
                    lastMouseY = e.motion.y;
                }
            }
        }

        bgfx::setViewRect(0, 0, 0, (uint16_t)currentWidth, (uint16_t)currentHeight);
        bgfx::setViewTransform(0, view, proj);
        bgfx::touch(0);

        // Upload new emuScreen texture if the worker has produced one
        {
            std::lock_guard<std::mutex> lock(emuScreenTexBuffer->mutex);
            if (emuScreenTexBuffer->dirty)
            {
                const bgfx::Memory* mem = bgfx::copy(
                    emuScreenTexBuffer->pixels.data(),
                    (uint32_t)emuScreenTexBuffer->pixels.size()
                );
                bgfx::updateTexture2D(emuScreenTex, 0, 0,
                    0, 0,
                    (uint16_t)emuScreenTexBuffer->width,
                    (uint16_t)emuScreenTexBuffer->height,
                    mem
                );
                emuScreenTexBuffer->dirty = false;
            }
        }

        // Render all meshes
        for (const auto& meshInst : allMeshes)
        {
            // Apply mirrorX on top of cached world matrix
            float finalMtx[16];
            bx::mtxMul(finalMtx, &nodeMatrixCache[meshInst.nodeIndex * 16], mirrorX);

            bgfx::setTransform(finalMtx);
            bgfx::setVertexBuffer(0, meshInst.buffers.vbh);
            bgfx::setIndexBuffer(meshInst.buffers.ibh);

            // Bind texture if available
            if (bgfx::isValid(meshInst.buffers.texture))
                bgfx::setTexture(0, s_texColor, meshInst.buffers.texture);

            //Skip culling on screen because it breaks it
            uint64_t cullFlag = (meshInst.meshName == "emuScreen") ? 0 : BGFX_STATE_CULL_CW;

            // Render state
            bgfx::setState(
                BGFX_STATE_WRITE_RGB |
                BGFX_STATE_WRITE_Z |
                BGFX_STATE_DEPTH_TEST_LESS |
                cullFlag
            );

            bgfx::submit(0, program);
        }

        // Render pick ray for visualisation
        if (g_hasRay && g_showRay)
        {
            LineVertex lineVerts[2] = {
                { g_rayOrigin[0], g_rayOrigin[1], g_rayOrigin[2], 0xff0000ff }, // red = origin
                { g_rayEnd[0],    g_rayEnd[1],    g_rayEnd[2],    0xff00ff00 }  // green = far end
            };

            bgfx::TransientVertexBuffer tvb;
            bgfx::allocTransientVertexBuffer(&tvb, 2, lineLayout);
            memcpy(tvb.data, lineVerts, sizeof(lineVerts));

            float identity[16];
            bx::mtxIdentity(identity);
            bgfx::setTransform(identity);
            bgfx::setVertexBuffer(0, &tvb);
            bgfx::setState(
                BGFX_STATE_WRITE_RGB |
                BGFX_STATE_WRITE_Z |
                BGFX_STATE_DEPTH_TEST_LESS |
                BGFX_STATE_PT_LINES
            );
            bgfx::submit(0, lineProgram);
        }

        // Advance to next frame
        bgfx::frame();
    }

    for (const auto& meshInst : allMeshes)
    {
        bgfx::destroy(meshInst.buffers.vbh);
        bgfx::destroy(meshInst.buffers.ibh);
    }

    for (auto& [idx, tex] : textureCache)
        if (bgfx::isValid(tex))
            bgfx::destroy(tex);

    bgfx::destroy(s_texColor);
    bgfx::destroy(program);
    bgfx::destroy(lineProgram);
    bgfx::shutdown();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}