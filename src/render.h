#ifndef RENDER_H
#define RENDER_H

#include <cstdarg>
#include "raylib.h"

// ---- DEBUG ----

class Debug {
    static const int MAX_LOGS = 20;
    static const int MSG_LEN = 128;

    char messages[MAX_LOGS][MSG_LEN];
    int count = 0;

public:
    void Log(const char *fmt, ...);
    void Clear();
    void Draw(Font font);
};

// ---- SKYBOX ----

namespace Skybox {
    void Load(const char *panoramaPath);
    void Draw();
    void Unload();
}

#endif
