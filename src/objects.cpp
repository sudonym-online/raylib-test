#include "objects.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dirent.h>

static void ParseJsonFile(const char *path, Objects::ObjectTemplate *tpl)
{
    FILE *f = fopen(path, "r");
    if (!f) return;

    char buf[4096];
    int len = fread(buf, 1, sizeof(buf) - 1, f);
    buf[len] = '\0';
    fclose(f);

    char *p = buf;
    while (*p && *p != '{') p++;
    if (!*p) return;
    p++;

    while (*p && tpl->attrCount < Objects::MAX_ATTRS) {
        while (*p && *p != '"') p++;
        if (!*p) break;
        p++;

        char *key = p;
        while (*p && *p != '"') p++;
        if (!*p) break;
        *p = '\0';
        p++;

        while (*p && *p != ':') p++;
        if (!*p) break;
        p++;

        while (*p && *p != '"') p++;
        if (!*p) break;
        p++;

        char *val = p;
        while (*p && *p != '"') p++;
        if (!*p) break;
        *p = '\0';
        p++;

        strncpy(tpl->attrs[tpl->attrCount].key, key, sizeof(tpl->attrs[0].key) - 1);
        strncpy(tpl->attrs[tpl->attrCount].value, val, sizeof(tpl->attrs[0].value) - 1);
        tpl->attrCount++;

        while (*p && *p != ',' && *p != '}') p++;
        if (*p == ',') p++;
    }
}

namespace {

    Objects::ObjectTemplate templates[Objects::MAX_TEMPLATES];
    int templateCount = 0;

    int nextId = 1;

    // constinit: zeroed before any OBJECT_FUNCTION static initializer runs
    constinit Objects::FunctionRegistry functionRegistry = {};

    inline bool isDirectory(const dirent *entry) { return entry->d_type == DT_DIR; }
    inline bool isNavDirectory(const dirent *entry) { return strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0; }
    inline bool isJsonFile(const dirent *entry) { return strcmp(entry->d_name + strlen(entry->d_name) - 5, ".json") == 0; }

    void ScanObjectsDir()
    {
        DIR *dir = opendir("objects");
        if (!dir) return;

        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr && templateCount < Objects::MAX_TEMPLATES) {
            if (!isDirectory(entry)) continue;
            if (isNavDirectory(entry)) continue; // avoids recursion

            char dirpath[256];
            snprintf(dirpath, sizeof(dirpath), "objects/%s", entry->d_name);

            DIR *sub = opendir(dirpath);
            if (!sub) continue;

            struct dirent *file;
            while ((file = readdir(sub)) != nullptr) {
                if (!isJsonFile(file)) continue;

                char filepath[512];
                snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, file->d_name);

                Objects::ObjectTemplate *tpl = &templates[templateCount];
                tpl->attrCount = 0;
                strncpy(tpl->name, entry->d_name, sizeof(tpl->name) - 1);
                ParseJsonFile(filepath, tpl);
                templateCount++;
                break;
            }
            closedir(sub);
        }
        closedir(dir);
    }

}

Objects::ObjectRegistry Objects::registry = {};

void Objects::InitCache()
{
    ScanObjectsDir();
    printf("[Objects] Loaded %d template(s)\n", templateCount);
}

int Objects::Create(const char *name)
{
    for (int i = 0; i < templateCount; i++) {
        if (strcmp(templates[i].name, name) == 0) {
            if (registry.instanceCount >= MAX_INSTANCES) return -1;
            ObjectInstance *inst = &registry.instances[registry.instanceCount];
            inst->id = nextId++;
            inst->attrCount = templates[i].attrCount;
            memcpy(inst->attrs, templates[i].attrs, sizeof(Attribute) * inst->attrCount);
            registry.instanceCount++;
            return inst->id;
        }
    }
    return -1;
}

const int *Objects::Find(const char *key, const char *value, int *outCount)
{
    static int results[MAX_INSTANCES];
    int count = 0;
    for (int i = 0; i < registry.instanceCount; i++) {
        for (int j = 0; j < registry.instances[i].attrCount; j++) {
            if (strcmp(registry.instances[i].attrs[j].key, key) == 0 &&
                strcmp(registry.instances[i].attrs[j].value, value) == 0) {
                results[count++] = registry.instances[i].id;
                break;
            }
        }
    }
    *outCount = count;
    return count > 0 ? results : nullptr;
}

static Objects::ObjectInstance *findInstance(int id)
{
    for (int i = 0; i < Objects::registry.instanceCount; i++)
        if (Objects::registry.instances[i].id == id) return &Objects::registry.instances[i];
    return nullptr;
}

void Objects::Set(int id, const char *key, const char *value)
{
    ObjectInstance *inst = findInstance(id);
    if (!inst) return;
    for (int j = 0; j < inst->attrCount; j++) {
        if (strcmp(inst->attrs[j].key, key) == 0) {
            strncpy(inst->attrs[j].value, value, sizeof(inst->attrs[0].value) - 1);
            return;
        }
    }
    if (inst->attrCount < MAX_ATTRS) {
        strncpy(inst->attrs[inst->attrCount].key, key, sizeof(inst->attrs[0].key) - 1);
        strncpy(inst->attrs[inst->attrCount].value, value, sizeof(inst->attrs[0].value) - 1);
        inst->attrCount++;
    }
}

const char *Objects::Get(int id, const char *key)
{
    ObjectInstance *inst = findInstance(id);
    if (!inst) return nullptr;
    for (int j = 0; j < inst->attrCount; j++)
        if (strcmp(inst->attrs[j].key, key) == 0) return inst->attrs[j].value;
    return nullptr;
}

bool Objects::RegisterFunction(ObjectFunction fn)
{
    if (functionRegistry.functionCount >= MAX_FUNCTIONS) return false;
    for (int i = 0; i < functionRegistry.functionCount; i++) {
        if (strcmp(functionRegistry.functions[i].name, fn.name) == 0) {
            printf("[Objects] duplicate function '%s'\n", fn.name);
            return false;
        }
    }
    functionRegistry.functions[functionRegistry.functionCount++] = fn;
    return true;
}

const Objects::ObjectFunction *Objects::FindFunction(int id, const char *name, const void *signature)
{
    char key[32];
    snprintf(key, sizeof(key), "on.%s", name);
    const char *target = Get(id, key);
    if (!target) return nullptr;

    for (int i = 0; i < functionRegistry.functionCount; i++) {
        const ObjectFunction *fn = &functionRegistry.functions[i];
        if (strcmp(fn->name, target) != 0) continue;
        if (fn->signature != signature) {
            printf("[Objects] '%s' called with wrong args\n", target);
            return nullptr;
        }
        return fn;
    }
    printf("[Objects] unknown function '%s'\n", target);
    return nullptr;
}

int Objects::Spawn(int id, Vector3 position, Vector3 scale, float rotation)
{
    if (registry.bodyCount >= MAX_INSTANCES) return -1;

    const char *modelPath = Get(id, "model");
    if (!modelPath) return -1;

    StaticBody &body = registry.bodies[registry.bodyCount];
    body.model = LoadModel(modelPath);
    body.position = position;
    body.scale = scale;
    body.rotation = rotation;
    body.UpdateAABB();
    body.ExtractTriangles(registry);
    body.instanceId = id;

    return registry.bodyCount++;
}

bool Objects::Despawn(int id)
{
    for (int i = 0; i < registry.bodyCount; i++) {
        if (registry.bodies[i].instanceId == id) {
            UnloadModel(registry.bodies[i].model);
            registry.bodies[i] = registry.bodies[registry.bodyCount - 1];
            registry.bodyCount--;
            return true;
        }
    }
    return false;
}

void Objects::UnloadAll()
{
    for (int i = 0; i < registry.bodyCount; i++)
        UnloadModel(registry.bodies[i].model);
    registry.bodyCount = 0;
    registry.trianglePoolCount = 0;
    templateCount = 0;
    registry.instanceCount = 0;
    nextId = 1;
}


// ---- STATIC BODY ----

Matrix StaticBody::Transform() {
    Matrix transform = MatrixScale(scale.x, scale.y, scale.z);
    transform = MatrixMultiply(transform, MatrixRotateY(rotation * DEG2RAD));
    return MatrixMultiply(transform, MatrixTranslate(position.x, position.y, position.z));
}

void StaticBody::UpdateAABB() {
    BoundingBox modelAABB = GetModelBoundingBox(model);
    Matrix transform = Transform();

    Vector3 corners[8] = {
            {modelAABB.min.x, modelAABB.min.y, modelAABB.min.z},
            {modelAABB.min.x, modelAABB.min.y, modelAABB.max.z},
            {modelAABB.min.x, modelAABB.max.y, modelAABB.min.z},
            {modelAABB.min.x, modelAABB.max.y, modelAABB.max.z},
            {modelAABB.max.x, modelAABB.min.y, modelAABB.min.z},
            {modelAABB.max.x, modelAABB.min.y, modelAABB.max.z},
            {modelAABB.max.x, modelAABB.max.y, modelAABB.min.z},
            {modelAABB.max.x, modelAABB.max.y, modelAABB.max.z}};

    for (int i = 0; i < 8; i++)
        corners[i] = Vector3Transform(corners[i], transform);

    aabb.min = corners[0];
    aabb.max = corners[0];

    for (int i = 1; i < 8; i++) {
        aabb.min.x = fminf(aabb.min.x, corners[i].x);
        aabb.min.y = fminf(aabb.min.y, corners[i].y);
        aabb.min.z = fminf(aabb.min.z, corners[i].z);
        aabb.max.x = fmaxf(aabb.max.x, corners[i].x);
        aabb.max.y = fmaxf(aabb.max.y, corners[i].y);
        aabb.max.z = fmaxf(aabb.max.z, corners[i].z);
    }
}

void StaticBody::ExtractTriangles(Objects::ObjectRegistry &reg) {
    triOffset = reg.trianglePoolCount;
    triCount = 0;
    Matrix transform = Transform();

    for (int m = 0; m < model.meshCount; m++) {
        Mesh &mesh = model.meshes[m];
        for (int t = 0; t < mesh.triangleCount; t++) {
            if (reg.trianglePoolCount >= Objects::MAX_TOTAL_TRIANGLES) {
                TraceLog(LOG_WARNING, "Triangle pool full — triangles truncated");
                return;
            }

            unsigned int i0, i1, i2;
            if (mesh.indices) {
                i0 = mesh.indices[t * 3 + 0];
                i1 = mesh.indices[t * 3 + 1];
                i2 = mesh.indices[t * 3 + 2];
            } else {
                i0 = (unsigned int)(t * 3 + 0);
                i1 = (unsigned int)(t * 3 + 1);
                i2 = (unsigned int)(t * 3 + 2);
            }

            Vector3 lv0 = {mesh.vertices[i0*3], mesh.vertices[i0*3+1], mesh.vertices[i0*3+2]};
            Vector3 lv1 = {mesh.vertices[i1*3], mesh.vertices[i1*3+1], mesh.vertices[i1*3+2]};
            Vector3 lv2 = {mesh.vertices[i2*3], mesh.vertices[i2*3+1], mesh.vertices[i2*3+2]};

            Vector3 wv0 = Vector3Transform(lv0, transform);
            Vector3 wv1 = Vector3Transform(lv1, transform);
            Vector3 wv2 = Vector3Transform(lv2, transform);

            Vector3 cross = Vector3CrossProduct(Vector3Subtract(wv1, wv0), Vector3Subtract(wv2, wv0));
            float len = Vector3Length(cross);
            if (len < 1e-6f) continue;

            Triangle &tri = reg.trianglePool[reg.trianglePoolCount];
            tri.v0 = wv0;
            tri.v1 = wv1;
            tri.v2 = wv2;
            tri.normal = Vector3Scale(cross, 1.0f / len);

            reg.trianglePoolCount++;
            triCount++;
        }
    }
}

void StaticBody::draw() {
    DrawModelEx(model, position, (Vector3){0, 1, 0}, rotation, scale, tint);
}
