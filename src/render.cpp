#include "render.h"
#include "rlgl.h"
#include "raymath.h"
#include <cstdio>
#include <cstring>

// ---- DEBUG ----

void Debug::Log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[MSG_LEN];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (count < MAX_LOGS) count++;

    for (int i = count - 1; i > 0; i--)
        strncpy(messages[i], messages[i - 1], MSG_LEN);

    strncpy(messages[0], buf, MSG_LEN - 1);
    messages[0][MSG_LEN - 1] = '\0';
}

void Debug::Clear()
{
    count = 0;
}

void Debug::Draw(Font font)
{
    int fontSize = 28;
    int lineHeight = fontSize + 6;
    int xPad = 10;
    int yStart = 10;
    int screenW = GetScreenWidth();

    for (int i = 0; i < count; i++) {
        Vector2 size = MeasureTextEx(font, messages[i], fontSize, 1);
        int x = screenW - xPad - (int)size.x;
        int y = yStart + i * lineHeight;
        DrawTextEx(font, messages[i], (Vector2){(float)x, (float)y}, fontSize, 1, BLACK);
    }
}


// ---- SKYBOX ----

namespace {

    Model skyboxModel;
    Shader skyboxShader;

    // Generate a cubemap (6 faces) from an equirectangular panorama texture by
    // rendering the panorama onto each face with the cubemap conversion shader.
    TextureCubemap GenTextureCubemap(Shader shader, Texture2D panorama, int size, int format)
    {
        TextureCubemap cubemap = { 0 };

        rlDisableBackfaceCulling(); // Rendering inside the cube

        unsigned int rbo = rlLoadTextureDepth(size, size, true);
        cubemap.id = rlLoadTextureCubemap(0, size, format, 1);

        unsigned int fbo = rlLoadFramebuffer();
        rlFramebufferAttach(fbo, rbo, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);
        rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X, 0);

        rlEnableShader(shader.id);

        Matrix matFboProjection = MatrixPerspective(90.0 * DEG2RAD, 1.0, rlGetCullDistanceNear(), rlGetCullDistanceFar());
        rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_PROJECTION], matFboProjection);

        Matrix fboViews[6] = {
            MatrixLookAt((Vector3){0.0f, 0.0f, 0.0f}, (Vector3){ 1.0f,  0.0f,  0.0f}, (Vector3){0.0f, -1.0f,  0.0f}),
            MatrixLookAt((Vector3){0.0f, 0.0f, 0.0f}, (Vector3){-1.0f,  0.0f,  0.0f}, (Vector3){0.0f, -1.0f,  0.0f}),
            MatrixLookAt((Vector3){0.0f, 0.0f, 0.0f}, (Vector3){ 0.0f,  1.0f,  0.0f}, (Vector3){0.0f,  0.0f,  1.0f}),
            MatrixLookAt((Vector3){0.0f, 0.0f, 0.0f}, (Vector3){ 0.0f, -1.0f,  0.0f}, (Vector3){0.0f,  0.0f, -1.0f}),
            MatrixLookAt((Vector3){0.0f, 0.0f, 0.0f}, (Vector3){ 0.0f,  0.0f,  1.0f}, (Vector3){0.0f, -1.0f,  0.0f}),
            MatrixLookAt((Vector3){0.0f, 0.0f, 0.0f}, (Vector3){ 0.0f,  0.0f, -1.0f}, (Vector3){0.0f, -1.0f,  0.0f})
        };

        rlViewport(0, 0, size, size);
        rlActiveTextureSlot(0);
        rlEnableTexture(panorama.id);

        for (int i = 0; i < 6; i++) {
            rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_VIEW], fboViews[i]);

            rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X + i, 0);
            rlEnableFramebuffer(fbo);

            rlClearScreenBuffers();
            rlLoadDrawCube();
        }

        rlDisableShader();
        rlDisableTexture();
        rlDisableFramebuffer();
        rlUnloadFramebuffer(fbo);

        rlViewport(0, 0, rlGetFramebufferWidth(), rlGetFramebufferHeight());
        rlEnableBackfaceCulling();

        cubemap.width = size;
        cubemap.height = size;
        cubemap.mipmaps = 1;
        cubemap.format = format;

        return cubemap;
    }

}

void Skybox::Load(const char *panoramaPath)
{
    Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    skyboxModel = LoadModelFromMesh(cube);

    skyboxShader = LoadShader("assets/shaders/skybox.vs", "assets/shaders/skybox.fs");
    skyboxModel.materials[0].shader = skyboxShader;

    int envMap = MATERIAL_MAP_CUBEMAP;
    int doGamma = 0;
    int vflipped = 0;
    SetShaderValue(skyboxShader, GetShaderLocation(skyboxShader, "environmentMap"), &envMap, SHADER_UNIFORM_INT);
    SetShaderValue(skyboxShader, GetShaderLocation(skyboxShader, "doGamma"), &doGamma, SHADER_UNIFORM_INT);
    SetShaderValue(skyboxShader, GetShaderLocation(skyboxShader, "vflipped"), &vflipped, SHADER_UNIFORM_INT);

    Shader cubemapShader = LoadShader("assets/shaders/cubemap.vs", "assets/shaders/cubemap.fs");
    int equirectSlot = 0;
    SetShaderValue(cubemapShader, GetShaderLocation(cubemapShader, "equirectangularMap"), &equirectSlot, SHADER_UNIFORM_INT);

    Texture2D panorama = LoadTexture(panoramaPath);
    skyboxModel.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = GenTextureCubemap(cubemapShader, panorama, 1024, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    UnloadTexture(panorama);
    UnloadShader(cubemapShader);
}

void Skybox::Draw()
{
    // Draw from inside the cube, ignoring the depth buffer, so it stays behind everything
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
        DrawModel(skyboxModel, (Vector3){0, 0, 0}, 1.0f, WHITE);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

void Skybox::Unload()
{
    UnloadShader(skyboxModel.materials[0].shader);
    UnloadTexture(skyboxModel.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
    UnloadModel(skyboxModel);
}
