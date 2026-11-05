#include "player.h"
#include "objects.h"
#include "raymath.h"
#include <cmath>
#include <cstdio>

inline bool Player::isValid(int itemId) { return itemId > 0; }

static Vector3 scaleOf(int itemId) {
    Vector3 scale = {1.0f, 1.0f, 1.0f};
    const char *attr = Objects::Get(itemId, "scale");
    if (attr) sscanf(attr, "%f,%f,%f", &scale.x, &scale.y, &scale.z);
    return scale;
}

void Player::UpdateAABB() {
    float bodyHeight = collision.height + 0.5f;
    collision.aabb.min = (Vector3){position.x - collision.width / 2, position.y, position.z - collision.depth / 2};
    collision.aabb.max = (Vector3){position.x + collision.width / 2, position.y + bodyHeight, position.z + collision.depth / 2};
}

void Player::UpdateModelOrientation(Model *model, Camera3D camera) {
    Vector3 dir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

    yaw = atan2f(dir.x, dir.z);

    float horizontalDist = sqrtf(dir.x * dir.x + dir.z * dir.z);
    pitch = -atan2f(dir.y, horizontalDist);

    Matrix rotation = MatrixMultiply(MatrixRotateX(pitch), MatrixRotateY(yaw));
    model->transform = rotation;
}

void Player::DrawArms(Camera3D camera) {
    forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    right = Vector3Normalize(Vector3CrossProduct((Vector3){0, 1, 0}, forward));

    Vector3 baseOffset  = Vector3Add(Vector3Scale(forward, visual.armConfig.dist), Vector3Scale((Vector3){0, 1, 0}, visual.armConfig.height));
    Vector3 leftArmPos  = Vector3Add(camera.position, Vector3Add(baseOffset, Vector3Scale(right, -visual.armConfig.width))); 
    Vector3 rightArmPos = Vector3Add(camera.position, Vector3Add(baseOffset, Vector3Scale(right, visual.armConfig.width)));

    // --- ARMS

    UpdateModelOrientation(&visual.armModel, camera);

    DrawModelEx(visual.armModel, leftArmPos, (Vector3){0, 1, 0}, 0.0f, (Vector3){1, 1, 1}, RED); //  TODO: get a actual left arm model.
    DrawModelEx(visual.armModel, rightArmPos, (Vector3){0, 1, 0}, 0.0f, (Vector3){1, 1, 1}, RED); 

    // --- HOLDING

    if(inventory.hand == 0) return;
    Vector3 handGripOffset = Vector3Add(Vector3Scale(forward, visual.heldModelConfig.dist), Vector3Scale(right, visual.heldModelConfig.side));
    handGripPosition = Vector3Add(camera.position, Vector3Add(baseOffset, handGripOffset));

    UpdateModelOrientation(&visual.heldModel, camera);
    DrawModelEx(visual.heldModel, handGripPosition, (Vector3){0, 1, 0}, 0.0f, visual.heldModelScale, RED);
}

void Player::Stash(int itemId) {
    if (!isValid(itemId)) return;
    for (int i = 0; i < MAX_INVENTORY_SIZE; i++)
        if (inventory.items[i] == itemId) return;

    for (int i = 0; i < MAX_INVENTORY_SIZE; i++) {
        if (inventory.items[i] == 0) {
            inventory.items[i] = itemId;
            break;
        }
    }
    Objects::Despawn(itemId);
}

bool Player::Unstash(int itemId) {
    if (!isValid(itemId)) return false;

    bool found = false;
    for (int i = 0; i < MAX_INVENTORY_SIZE; i++) {
        if (inventory.items[i] == itemId) {
            inventory.items[i] = 0;
            found = true;
            break;
        }
    }
    if (!found) return false;

    Vector3 dropPos = Vector3Add(position, Vector3Scale(forward, 3.0f));
    Objects::Spawn(itemId, dropPos, scaleOf(itemId), 0.0f);
    return true;
}

void Player::Hold(int itemId) {
    if (itemId == -1) {
        if (inventory.hand != 0) {
            UnloadModel(visual.heldModel);
            inventory.hand = 0;
        }
        return;
    }

    if (!isValid(itemId) || inventory.hand == itemId) return;

    const char *modelPath = Objects::Get(itemId, "model");
    if (!modelPath) return; 

    inventory.hand = itemId;
    visual.heldModel = LoadModel(modelPath);

    visual.heldModelScale = scaleOf(itemId);
}

void Player::Use(bool down) {
    if (!down || inventory.hand == 0) return;
    Vector3 eye = {position.x, position.y + collision.height, position.z};
    Objects::Use(inventory.hand, "use", eye, forward);
}


// ---- INPUT ----

Vector2 getKeyVector() {
    return (Vector2){
            (float)IsKeyDown(KEY_W) - (float)IsKeyDown(KEY_S),
            (float)IsKeyDown(KEY_D) - (float)IsKeyDown(KEY_A),
    };
}

void inputProcess(float deltaTime, Player &player, Camera3D &camera) {
    Vector2 keyVector = getKeyVector();

    if (Vector2Length(keyVector) > 0) {
        Vector3 forward = Vector3Subtract(camera.target, camera.position);
        forward.y = 0;
        forward = Vector3Normalize(forward);
        Vector3 right = Vector3CrossProduct(forward, camera.up);
        Vector2 inputDir = Vector2Normalize(keyVector);

        Vector3 moveDir = Vector3Add(
                Vector3Scale(forward, inputDir.x),
                Vector3Scale(right, inputDir.y));

        float prevSpeed = Vector2Length((Vector2){player.movement.velocity.x, player.movement.velocity.z});

        player.movement.velocity.x += moveDir.x * player.movement.acceleration * deltaTime;
        player.movement.velocity.z += moveDir.z * player.movement.acceleration * deltaTime;

        float newSpeed = Vector2Length((Vector2){player.movement.velocity.x, player.movement.velocity.z});
        float maxSpeed = fmaxf(player.movement.maxSpeed, prevSpeed);
        if (newSpeed > maxSpeed) {
            player.movement.velocity.x *= maxSpeed / newSpeed;
            player.movement.velocity.z *= maxSpeed / newSpeed;
        }
    }

    if (IsKeyPressed(KEY_SPACE)) {
        player.movement.bufferTimer = player.movement.bufferTime;
    }

    if (IsKeyPressed(KEY_E)) {
        // stash /unstash
        int count = 0;
        const int *ids = Objects::Find("type", "weapon", &count);
        if (count > 0 && !player.Unstash(ids[0])) player.Stash(ids[0]);
    }
    if (IsKeyPressed(KEY_Q)) {
        // hold item
        int count = 0;
        const int *ids = Objects::Find("type", "weapon", &count);
        if (count > 0) {
            int itemId = ids[0];
            player.Hold(itemId);
        }
    }

    if (IsKeyPressed(KEY_R)) {
        //unhold
        player.Hold(-1);
    }

    player.Use(IsMouseButtonPressed(MOUSE_BUTTON_LEFT));

    // Debug for HeldModelConfig position tweaking
    // if (IsKeyPressed(KEY_I)) {
    //     player.visual.heldModelConfig.dist += 0.1f;
    //     player.visual.armConfig.dist += 0.1f;
    // }
    // if (IsKeyPressed(KEY_K)) {
    //     player.visual.heldModelConfig.dist -= 0.1f;
    //     player.visual.armConfig.dist -= 0.1f;
    // }
    // if (IsKeyPressed(KEY_J)) {
    //     player.visual.heldModelConfig.side += 0.1f;
    //     player.visual.armConfig.width += 0.1f;
    // }
    // if (IsKeyPressed(KEY_L)) {
    //     player.visual.heldModelConfig.side -= 0.1f;
    //     player.visual.armConfig.width -= 0.1f;
    // }
    // if (IsKeyPressed(KEY_U)) {
    //     player.visual.heldModelConfig.height += 0.1f;
    //     player.visual.armConfig.height += 0.1f;
    // }
    // if (IsKeyPressed(KEY_O)) {
    //     player.visual.heldModelConfig.height -= 0.1f;
    //     player.visual.armConfig.height -= 0.1f;
    // }
}
