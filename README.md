# THE LUCKY NEVER DIE

A first-person 3D game prototype in C++ with raylib. It focuses on movement physics and a small engine for objects and items.

**Demo video:** [paste link]

[Screenshot or GIF: paste here]

This is a work in progress. The game idea is a skill-based game where you don't die; you're scored on how fast you clear a level.

## What it does

- **First-person movement:** WASD to move, mouse to look, Space to jump. Jumps are buffered, so a press just before landing still counts.
- **Physics:** gravity, friction, and collision against each triangle of the level meshes. On a slope, the player slides along the surface.
- **Speed boost:** while in the air, a boost builds up based on how far you look up or down. Landing applies it as extra speed. The field of view widens as speed goes up.
- **Object system:** objects are templates with key-value attributes (for example, `ammo = 24`). Each object type can register its own functions, such as a weapon's "use".
- **Items:** a Deagle pistol is placed in the level. Q holds it, E stashes or unstashes it, R lowers it, and left click fires. Firing casts a ray and logs the hit; projectiles, ammo, and recoil are not built yet.
- **Level:** a 10 by 10 grid of map tiles with a skybox.
- **Debug overlay:** pitch, yaw, boost, speed, and a collision indicator.

## Tech stack

- C++20
- [raylib](https://github.com/raysan5/raylib) (graphics, input, models)
- Make
- Blender (models)

## Run it locally

You need `g++`, `make`, `cmake`, and `git`. On Linux, you also need the X11 and OpenGL development packages that raylib uses.

1. Clone this repo, then clone and build raylib inside it:

   ```sh
   git clone https://github.com/raysan5/raylib.git raylib
   cmake -S raylib -B raylib/build
   cmake --build raylib/build
   ```

2. Build and run the game from the project folder:

   ```sh
   make
   ./main
   ```

The game loads its assets from the `assets/` and `objects/` folders, so run it from the project folder.

## What I learned

- **Collision against real meshes.** The player is tested against each triangle of a level mesh, not just its bounding box, and pushed out along the surface. That's also what makes sliding down slopes work.
- **Small details change how movement feels.** A short jump buffer, friction, and widening the field of view with speed make movement feel more responsive, even though each is only a few lines.
- **Designing a small engine.** Instead of hard-coding the pistol, I built an object system with templates, key-value attributes, and registered functions, so a new item only needs its own `.cpp` file.

## What I would improve next

- Finish the pistol: projectiles, ammo, and recoil.
- Build real levels and the scoring from the game concept: clear a level as fast as possible.
- Fix the `raylib` submodule so the project builds from a fresh clone without extra steps.
