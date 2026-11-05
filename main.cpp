#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "src/player.h"
#include "src/physics.h"
#include "src/objects.h"
#include "src/render.h"

Debug dbg;
Player player;
Font uiFont;
Texture2D asphaltTex;

Camera3D                camera =            {0};
float                   mouseSensitivity =  0.15f;
bool                    debug =             false;
Vector2                 windowSize =        {1280, 720};

void init() {
    InitWindow(windowSize.x, windowSize.y, "THE LUCKY NEVER DIE");

    uiFont = LoadFontEx("assets/Fonts/Iosevka/IosevkaNerdFontMono-Regular.ttf", 256, nullptr, 0);
    SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
    player.visual.armModel = LoadModel("assets/Arms.obj");

    player.position = (Vector3){0.0f, 120.0f, 4.0f};
    player.UpdateAABB();

    camera.position     = (Vector3){player.position.x, player.position.y + player.collision.height, player.position.z};
    camera.target       = (Vector3){0.0f, 2.0f, 0.0f};
    camera.up           = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy         = 90.0f;
    camera.projection   = CAMERA_PERSPECTIVE;

    DisableCursor();
    SetTargetFPS(60);

    Objects::InitCache();

    dbg.Log("engine initialized [LOG]");

    Skybox::Load("assets/Skybox.png");

    asphaltTex = LoadTexture("objects/Map/Map_Texture.png");

    Model probeModel = LoadModel("objects/Map/Map.obj");
    BoundingBox probeAABB = GetModelBoundingBox(probeModel);
    UnloadModel(probeModel);
    float tileSizeX     = probeAABB.max.x - probeAABB.min.x;
    float tileSizeZ     = probeAABB.max.z - probeAABB.min.z;

    const int MAP_GRID = 10;
    int mapId = Objects::Create("Map");
    for (int gx = 0; gx < MAP_GRID; gx++) {
        for (int gz = 0; gz < MAP_GRID; gz++) {
            Vector3 pos = {
                (gx - (MAP_GRID - 1) / 2.0f) * tileSizeX,
                0.0f,
                (gz - (MAP_GRID - 1) / 2.0f) * tileSizeZ};
            int mapBody = Objects::Spawn(mapId, pos);
            Objects::registry.bodies[mapBody].model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = asphaltTex;
        }
    }
    dbg.Log("spawned %d map tiles [LOG]", MAP_GRID * MAP_GRID);

    int deagle = Objects::Create("Deagle");
    Objects::Set(deagle, "ammo", "24");
    Objects::Spawn(deagle, {2.0f, 0.5f, 0.0f}, {0.4f, 0.4f, 0.4f});
    dbg.Log("spawned Deagle (id=%d) [LOG]", deagle);
}

int main(void) {
    init();

    while (!WindowShouldClose()) {
        Vector2 deltaMouse = GetMouseDelta();
        float deltaTime = GetFrameTime();

        // ------------------- INPUT -------------------
        inputProcess(deltaTime, player, camera);

        // ------------------- MOVEMENT -------------------
        UpdateCameraPro(&camera, (Vector3){0, 0, 0}, (Vector3){deltaMouse.x * mouseSensitivity, deltaMouse.y * mouseSensitivity, 0.0f}, 0.0f);

        // ------------------- PHYSICS & COLLISION -------------------
        physicsProcess(deltaTime, player, camera);
        bool colliding = player.collision.bodyCount > 0;

        float speed = Vector2Length((Vector2){player.movement.velocity.x, player.movement.velocity.z});
        float targetFov = 60.0f + 20.0f * log1pf(speed / 15.0f);
        camera.fovy = Lerp(camera.fovy, targetFov, 1.0f * deltaTime);

        // ------------------- DRAWING -------------------
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // ---- BOTTOM LAYER ----
        BeginMode3D(camera);
            Skybox::Draw();
            for (int i = 0; i < Objects::registry.bodyCount; i++) {
                Objects::registry.bodies[i].draw();
                if (debug) {
                    DrawBoundingBox(Objects::registry.bodies[i].aabb, GREEN);
                }
            }
            if (debug) {
                DrawBoundingBox(player.collision.aabb, RED);
            }
        EndMode3D();

        rlColorMask(false, false, false, false);
        rlClearScreenBuffers();
        rlColorMask(true, true, true, true);

        // ---- TOP LAYER ----
        BeginMode3D(camera);
            player.DrawArms(camera);
        EndMode3D();

        // ------------------- UI -------------------

        dbg.Clear();
        dbg.Log("pitch: %.2f", player.pitch * RAD2DEG);
        dbg.Log("yaw: %.2f",   player.yaw * RAD2DEG);
        dbg.Log("boost: %.2f", getBoost());
        dbg.Log("speed: %.2f", speed);
        dbg.Log("max: %.2f",   player.movement.maxSpeed);

        if (colliding) {
            DrawTextEx(uiFont, "COLLISION DETECTED", (Vector2){10, 10}, 32, 1, RED);
        } else {
            DrawTextEx(uiFont, "NO COLLISION", (Vector2){10, 10}, 32, 1, GREEN);
        }

        dbg.Draw(uiFont);

        EndDrawing();
    }

    // ------------------- CLEANUP -------------------
    UnloadFont(uiFont);
    UnloadTexture(asphaltTex);
    Skybox::Unload();
    UnloadModel(player.visual.armModel);
    Objects::UnloadAll();
    CloseWindow();
    return 0;
}
