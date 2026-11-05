#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"


class Player {
public:
    static const int    MAX_INVENTORY_SIZE = 32;
    struct Inventory {
        int             items               [Player::MAX_INVENTORY_SIZE];
        int             hand;
    } inventory;

    struct Movement {
        Vector3         velocity =          {0.0f, 0.0f, 0.0f};
        float           speed =             50.0f;
        float           maxSpeed =          50.0f;
        float           acceleration =      320.0f;
        float           friction =          3.5f;
        float           bufferTime =        0.2f;
        float           bufferTimer =       0.0f;
    } movement;

    struct Collision {
        float           height =            5.0f;
        float           width =             1.0f;
        float           depth =             1.0f;
        BoundingBox     aabb;
        int             bodyCount =         0;
        bool            grounded =          false;
        bool            wasGrounded =       false;
        float           groundTimer =       0.0f;
    } collision;

    struct Visuals {
        Model           armModel;
        Model           heldModel;
        Vector3         heldModelScale =    {1.0f, 1.0f, 1.0f};

        struct ArmConfig {
            float       dist =              0.8f;
            float       height =            -2.8f;
            float       width =             4.5f;
        } armConfig;

        struct HeldModelConfig {
            float       dist =              7.0f;
            float       height =            -2.5f;
            float       side =              4.0f;
        } heldModelConfig;

    } visual;

    Vector3             position =          {0.0f, 0.0f, 0.0f};
    Vector3             handGripPosition =  {0.0f, 0.0f, 0.0f};
    Vector3             forward =           {0.0f, 0.0f, 0.0f};
    Vector3             right =             {0.0f, 0.0f, 0.0f};
    float               yaw =               0.0f;
    float               pitch =             0.0f;

private:
    bool isValid(int itemId);
    void UpdateModelOrientation(Model *model, Camera3D camera);

public:
    void UpdateAABB();
    void DrawArms(Camera3D camera);
    void Stash(int itemId);
    bool Unstash(int itemId);
    void Hold(int itemId);
    void Use(bool down);
};

void inputProcess(float deltaTime, Player &player, Camera3D &camera);

#endif
