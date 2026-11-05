#ifndef OBJECTS_H
#define OBJECTS_H

#include <cstring>
#include "raylib.h"

struct Triangle {
    Vector3             v0, v1, v2;
    Vector3             normal;
};

namespace Objects { struct ObjectRegistry; }

struct StaticBody {
    BoundingBox         aabb;
    Color               tint =              WHITE;
    Model               model;
    Vector3             position;
    Vector3             scale;
    float               rotation;
    int                 triStart;
    int                 triCount;
    int                 instanceId =        0;

    Matrix Transform();
    void UpdateAABB();
    void ExtractTriangles(Objects::ObjectRegistry &reg);
    void draw();
};

namespace Objects {

    const int MAX_ATTRS = 32;
    const int MAX_TEMPLATES = 32;
    const int MAX_INSTANCES = 128;
    const int MAX_TOTAL_TRIANGLES = 65536;
    const int MAX_FUNCTIONS = 64;

    struct Attribute {
        char key[32];
        char value[64];
    };

    struct ObjectTemplate {
        char name[32];
        Attribute attrs[MAX_ATTRS];
        int attrCount;
    };

    struct ObjectInstance {
        int id;
        Attribute attrs[MAX_ATTRS];
        int attrCount;
    };

    struct ObjectRegistry {
        ObjectInstance instances[MAX_INSTANCES];
        int instanceCount;

        StaticBody bodies[MAX_INSTANCES];
        int bodyCount;

        Triangle trianglePool[MAX_TOTAL_TRIANGLES];
        int trianglePoolCount;
    };

    struct ObjectFunction {
        const char      *name;
        void           (*ptr)();
        const void      *signature;
    };

    struct FunctionRegistry {
        ObjectFunction  functions[MAX_FUNCTIONS];
        int             functionCount;
    };

    extern ObjectRegistry registry;

    void InitCache();
    int  Create(const char *name);
    const int *Find(const char *key, const char *value, int *outCount);
    void Set(int id, const char *key, const char *value);
    const char *Get(int id, const char *key);
    int  Spawn(int id, Vector3 position = {0, 0, 0}, Vector3 scale = {1, 1, 1}, float rotation = 0);
    bool Despawn(int id);
    void UnloadAll();

    // one unique address per argument list
    template<class... Args> const void *Signature() { static const char tag = 0; return &tag; }

    template<class... Args> ObjectFunction MakeFunction(const char *name, void (*fn)(int, Args...))
    {
        return {name, (void (*)())fn, Signature<Args...>()};
    }

    bool RegisterFunction(ObjectFunction fn);
    const ObjectFunction *FindFunction(int id, const char *name, const void *signature);

    // runs the function named by the "on.<name>" attribute. arg types must match exactly.
    template<class... Args> bool Use(int id, const char *name, Args... args)
    {
        const ObjectFunction *fn = FindFunction(id, name, Signature<Args...>());
        if (!fn) return false;
        ((void (*)(int, Args...))fn->ptr)(id, args...);
        return true;
    }

}

#define OBJECT_FUNCTION(fn) \
    [[maybe_unused]] static bool fn##Registered = Objects::RegisterFunction(Objects::MakeFunction(#fn, fn))

#endif
