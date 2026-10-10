# PSX Engine

A first original PlayStation 3D program written in C. It draws a rotating,
colored cube using the Geometry Transformation Engine, ordering tables,
back-face culling and double buffering as found in classic Psy-Q programs.

The project uses the open-source
[PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK) toolchain. Its GPU API
preserves many Psy-Q names and concepts without redistributing Sony's
proprietary SDK. The compiler is GCC targeting the PSX's little-endian MIPS
R3000A CPU.

## Prerequisites

- macOS on Apple Silicon or Intel
- [Docker Desktop](https://www.docker.com/products/docker-desktop/)
- `make` (provided by Apple's Command Line Tools)

The Docker image is intentionally `linux/amd64`, because PSn00bSDK 0.24 only
publishes an x86-64 Linux toolchain. Docker Desktop runs it on both kinds of
Mac.

## Build

Start Docker Desktop, then run:

```sh
make
```

The first build downloads and verifies PSn00bSDK 0.24 inside a local Docker
image. Later builds inspect and reuse that image without invoking
`docker build`. The image is rebuilt only if `Dockerfile` changes or the local
image has been removed. Build and debug commands still use short-lived
containers, which Docker deletes automatically through `--rm`.

Build outputs:

- `build/psx-engine.exe` - standalone PS-X EXE
- `build/psx-engine.bin` - bootable CD image data
- `build/psx-engine.cue` - cue sheet to open in an emulator

Use `make clean` to remove generated build files.

## Run

DuckStation is installed in the standard macOS location, so you can build and
launch the executable directly:

```sh
make run
```

You can alternatively open `build/psx-engine.cue` in a PSX emulator.
DuckStation is convenient for normal testing and provides a GDB remote server
for source-level debugging.

You can also load `build/psx-engine.exe` directly in an emulator that
supports PS-X EXE files.

This project does not include a PlayStation BIOS. Use a BIOS dumped from a
console you own, or use an emulator's supported open BIOS option.

### Boot logo and disc license data

The original BIOS does not load its PlayStation logo as a normal model from the
ISO filesystem. It reads region-specific logo/license data from special disc
sectors. PSn00bSDK cannot distribute Sony's payload, so a slow BIOS boot of an
unlicensed homebrew image may show a broken logo. This does not indicate missing
game models or a damaged executable.

For development, `make run` and `make watch` load `psx-engine.exe` directly and
skip the disc boot sequence. DuckStation's fast boot can likewise skip it when
opening the cue sheet.

To build a custom cube-logo image, legally dump the matching regional license
data from a PlayStation disc you own and save it as:

```text
license.dat
```

The file is ignored by Git. On the next build,
`scripts/generate-cube-license.py` preserves its regional license sector,
replaces the original logo TMD with a six-face colored cube, and writes the
generated `build/license-cube.dat` into the image:

```sh
make
```

You may alternatively configure an absolute input path:

```sh
cmake --preset default -DPSX_LICENSE_FILE=/path/to/license.dat .
cmake --build --preset default
```

To retain the original logo from the dumped disc:

```sh
cmake --preset default -DPSX_CUSTOM_BOOT_CUBE=OFF .
cmake --build --preset default
```

Custom boot meshes are BIOS-dependent:

- NTSC US/Asia BIOS versions accept changed logos.
- PAL BIOS versions through v3.0E accept them.
- PAL v4.0E and later reject changed logos unless region-patched.
- NTSC Japanese BIOS versions reject changed logos.

The boot model is a standard unlit PlayStation TMD stored in sectors 5–11. It
is unrelated to the engine's runtime `Model` structure and is rendered by the
console BIOS before `PSXENGIN.EXE` starts.

License-sector data only supplies the expected boot payload. It does not bypass
the physical wobble-groove authentication on an unmodified console; real
hardware still requires an appropriate legitimate boot method.

## Automatic rebuild and reload

Run:

```sh
make watch
```

The watcher performs an initial build and opens the PS-X EXE in DuckStation.
Whenever a source or CMake file changes, it:

1. Rebuilds `psx-engine.exe`.
2. Stops the DuckStation process it started without showing an exit prompt.
3. Launches the new executable.

If compilation fails, the emulator keeps running the last successful build.
Fix the error and save again to retry. Press Control-C in the terminal to stop
the watcher and its emulator process.

This is automatic **reload**, rather than true in-place hot reload. Replacing
code while a PSX program is running would invalidate pointers, GPU command
buffers and other state in the console's 2 MB of RAM. Restarting the executable
is predictable and takes only a few seconds.

The watcher force-stops only the exact DuckStation process that it launched.
This intentionally skips resume-state creation during development and does not
change DuckStation's global exit-confirmation setting.

If DuckStation is installed elsewhere, pass its executable path:

```sh
make watch DUCKSTATION="/path/to/DuckStation"
```

## Code completion and definition navigation

The project is configured for Microsoft's VS Code C/C++ extension. Generate
the local IntelliSense files with:

```sh
make intellisense
```

This copies the exact PSn00bSDK 0.24 headers from the toolchain image and checks
out the matching SDK source under `.vscode/psn00bsdk/`. That generated folder
is ignored by Git and is not used to compile the game.

After setup, completion, parameter hints, hover information and error
highlighting are available in C files. Use F12 or Command-click to navigate
from calls such as `RotMatrix()` and `ResetGraph()` to their declarations or
implementations. For GTE macros such as `gte_rtpt()`, navigation opens
`inline_c.h`, where the hardware instruction wrapper is defined.

If VS Code was already open when setup completed, run **C/C++: Reset
IntelliSense Database** from the Command Palette or reload the window.

## Debugging in VS Code

DuckStation's GDB remote server can be used by VS Code's Microsoft C/C++
extension. The debugger loads symbols from the unstripped
`build/psx-engine.elf`; DuckStation still runs `build/psx-engine.exe`.

In DuckStation:

1. Open **Settings > Advanced > Debugging**.
2. Enable **GDB Server**.
3. Set **GDB Server Port** to `2345`.
4. Close DuckStation before starting a debug session.

Then open **Run and Debug** in VS Code, select
**PSX: Debug in DuckStation**, and press F5. The launch configuration:

1. Builds the project and its Docker toolchain.
2. Starts the exact DuckStation process used for the session.
3. Holds emulation paused while VS Code connects.
4. Runs Debian's `gdb-multiarch` in the toolchain container.
5. Enables source-level breakpoints, stepping, registers, memory, and local
   variables.

Fast boot can execute `main()` before the debugger finishes attaching. For a
first breakpoint, use a function called every frame, such as
`renderer_present()`. Breakpoints set before pressing F5 are installed before
VS Code resumes the paused emulator.

Stopping the VS Code debug session also stops the DuckStation process launched
for it. DuckStation's remote protocol does not implement GDB's detach packet,
so this configuration intentionally does not leave that emulation session
running. The server implements the core operations needed for CPU debugging,
but it is not a full hardware debugger: use DuckStation's own debug windows for
PSX-specific GPU, DMA, interrupt, and memory inspection.

If DuckStation is installed outside `/Applications`, launch VS Code from a
terminal with `DUCKSTATION` set to the emulator executable path. The debug
launcher also accepts `PSX_GDB_PORT`, but changing it requires updating the
matching port values in `.vscode/launch.json`.

## Project layout

```text
.
├── .vscode/launch.json VS Code GDB launch configuration
├── .vscode/tasks.json  VS Code build and clean commands
├── scripts/docker-gdb.sh Containerized GDB transport
├── scripts/debug-emulator.sh Managed DuckStation debug launcher
├── scripts/watch.sh    Automatic rebuild and emulator reload
├── src/camera.h        Quaternion camera and movement API
├── src/camera.c        Camera view-matrix implementation
├── src/cube_entity.h   Reusable cube entity API
├── src/cube_entity.c   Cube model data and spin behavior
├── src/entity.h        Shared transform and entity movement API
├── src/entity.c        Fixed-point quaternion transform implementation
├── src/entity_system.h Fixed-capacity entity registry
├── src/entity_system.c Entity registration and update dispatch
├── src/gamepad_entity.h Controller-driven entity behavior
├── src/gamepad_entity.c Digital and analog pad polling
├── src/game_time.h     Fixed-point frame timing API
├── src/game_time.c     VBlank delta measurement and rate scaling
├── src/model.h         Indexed models and model-based entities
├── src/model.c         Model entity initialization
├── src/physics.h       AABB bodies, contacts and physics world
├── src/physics.c       Gravity, integration and collision resolution
├── src/plane_entity.h  Reusable ground-plane entity API
├── src/plane_entity.c  Twelve-meter ground model
├── src/renderer.h      Public rendering API
├── src/renderer.c      GPU, GTE, lighting and frame submission
├── src/spline.h        Fixed-point Catmull-Rom spline data
├── src/spline.c        Spline sampling implementation
├── src/spline_entity.h Renderable paths and follower entities
├── src/spline_entity.c Spline visualization and movement behavior
├── src/units.h         Integer world-unit conversion macros
├── src/main.c          Scene setup and update loop
├── CMakeLists.txt      Executable and CD image targets
├── CMakePresets.json   PSn00bSDK cross-compiler configuration
├── Dockerfile          Reproducible macOS build environment
├── Makefile            Short build commands
├── iso.xml             CD image contents
└── system.cnf          PlayStation boot configuration
```

## Psy-Q concepts used

- `DISPENV` and `DRAWENV` describe the displayed and rendered framebuffers.
- Two framebuffers prevent visible tearing while the GPU draws.
- Indexed triangle models share vertices, normals and materials.
- Base entities give cameras and models a common transform and movement API.
- Optional run/render methods provide per-entity behavior and dispatch.
- A fixed-capacity registry updates and renders registered world entities.
- A gamepad entity can move and rotate any target entity.
- Model entities add shared render geometry to a base entity.
- 100 integer world units represent one meter (one unit is one centimeter).
- A normalized quaternion camera supports local movement without gimbal lock.
- The GTE rotates, translates and perspective-projects model vertices.
- GTE normal-color calculations apply ambient and directional face lighting.
- Back-face culling avoids drawing triangles turned away from the camera.
- A 256-entry ordering table draws farther faces before nearer faces.
- `POLY_F3` is Psy-Q's three-point, flat-shaded polygon primitive.
- `setPolyF3`, the `gte_*` macros and `addPrim` construct and queue triangles.
- `DrawOTagEnv`, `DrawSync` and `VSync` submit and synchronize each frame.

PSn00bSDK combines the classic Psy-Q `libgpu.h` declarations into
`psxgpu.h`, but the primitive programming model remains deliberately familiar.

## World units

Game-space positions and dimensions use 100 integer units per meter. Use the
macros from `src/units.h` instead of embedding scaled numbers:

```c
WORLD_METERS(5)
WORLD_CENTIMETERS(180)
WORLD_MILLIMETERS(250)
```

Conversions remain integer-only; no floating-point code is added to the PSX
executable. Values smaller than one world unit (one centimeter with the current
scale) are rounded toward zero. GTE normals, matrices, rotation angles, screen
coordinates and colors retain their own hardware-specific scales.

## Camera

The camera embeds the same base `Entity` used by model entities. Controllers
and gameplay systems can manipulate any entity through the shared API:

```c
entity_set_position(&camera.entity, WORLD_METERS(0), WORLD_METERS(1), 0);
entity_move_world(&camera.entity, WORLD_METERS(1), 0, 0);
entity_move_local(&camera.entity, 0, 0, WORLD_CENTIMETERS(25));
```

Local movement follows the camera orientation, so positive local Z moves
forward. Incremental local rotation is accumulated as quaternion
multiplication rather than Euler orientation:

```c
entity_rotate_local(&camera.entity, pitch_delta, yaw_delta, roll_delta);
```

Angle deltas use the GTE convention of 4096 units per full turn. The API
accepts pitch, yaw and roll deltas for convenience, but it never stores or
reconstructs Euler angles, avoiding their gimbal-lock singularity.

## Entity system

`EntitySystem` stores up to 64 entity pointers without heap allocation.
Registration and deregistration return explicit results such as
`ENTITY_SYSTEM_FULL` and `ENTITY_SYSTEM_NOT_REGISTERED`.

Each entity may define optional run and render methods:

```c
static void spin(Entity *entity, TimeDelta delta_time) {
	int32_t yaw = time_scale_rate(960, delta_time);
	entity_rotate_local(entity, 0, (int16_t) yaw, 0);
}

entity_set_run_method(&cube.entity, spin);
entity_set_render_method(&cube.entity, renderer_render_model_entity);
```

Register the entity after its concrete object has been initialized:

```c
EntitySystemResult result =
	entity_system_register(&entities, &cube.entity);
assert(result == ENTITY_SYSTEM_SUCCESS);
```

The frame first runs entity behavior, then renders registered entities:

```c
game_time_update(&time);
entity_system_run(&entities, time.delta);
renderer_draw_entities(&entities, &camera);
renderer_present();
```

Passing `NULL` to either method setter disables that method. A controller can
hold an `Entity *` and call the common movement functions without knowing
whether it controls a camera, model, player, or another concrete entity type.
Call `entity_system_deregister()` before destroying or reusing registered
entity storage.

## Cube entity

`CubeEntity` owns the shared one-cubic-meter cube model definition and embeds a
`ModelEntity`.
Initialization automatically attaches the model renderer:

```c
CubeEntity cube;
cube_entity_init(&cube);

Entity *entity = cube_entity_as_entity(&cube);
entity_set_position(entity, 0, 0, WORLD_METERS(5));
entity_system_register(&entities, entity);
```

Optional spin behavior is configured in angle units per second:

```c
cube_entity_set_spin(&cube, 720, 960, 480);
cube_entity_stop_spin(&cube);
```

This keeps mesh data and cube-specific behavior out of `main.c`. Additional
concrete entity types can follow the same composition pattern: embed a
`ModelEntity`, attach optional methods during initialization, and expose the
base `Entity *` for shared systems.

The level scene uses cube entities for the player and static obstacles.

## Plane entity

`PlaneEntity` is a reusable 12×12-meter horizontal model. It is subdivided into
one-meter squares with alternating bright materials, providing visible scale
and perspective cues. The level positions it beneath the cubes and pairs it
with a thin static AABB to form the ground.

## Gamepad entity

`GamepadEntity` polls a controller through the PSX BIOS driver from its optional
run method. It can either move an `Entity *` directly or add local forces to a
dynamic `PhysicsBody`. The level attaches port 1 to the player body.

Default digital controls:

| Input | Action |
|---|---|
| D-pad | Move forward, backward and strafe |
| Triangle / Cross | Move up / down |
| Square / Circle | Yaw left / right |
| L1 / R1 | Pitch up / down |
| L2 / R2 | Roll left / right |

Dual Analog and DualShock controllers also use the left stick for movement and
the right stick for yaw and pitch, with a dead zone around the center.
DuckStation must have **Automatically Enable Analog Mode** enabled for port 1;
otherwise the PSX BIOS exposes the emulated DualShock as a digital pad and no
stick bytes are returned. The mapped Analog button can toggle the mode at
runtime.

Retarget the same controller without changing its behavior:

```c
gamepad_entity_set_target(&gamepad, &camera.entity);
gamepad_entity_set_physics_target(&gamepad, &player_body);
```

Direct movement uses meters per second. Physics movement uses force:

```c
gamepad_entity_set_speeds(&gamepad, WORLD_METERS(3), 1440);
gamepad_entity_set_movement_force(&gamepad, WORLD_METERS(12));
```

In physics mode the controller never writes the target position. It accumulates
a local force, then `physics_system_step()` updates velocity, applies damping,
resolves collisions and finally changes the entity transform. The right stick
still rotates that target entity directly.

Rotation is measured in PSX angle units per second.

Before permanently removing the gamepad entity, deregister it and call
`gamepad_entity_stop()` to stop the BIOS pad driver.

## Frame timing

`GameTime` measures elapsed VBlank ticks through `VSync(-1)` and converts them
to signed 16.16 fixed-point seconds. It automatically uses 60 ticks per second
for NTSC and 50 for PAL.

Every entity run method receives the same frame delta:

```c
typedef void (*EntityRunMethod)(Entity *entity, TimeDelta delta_time);
```

Use `time_scale_rate()` to convert a per-second rate into this frame's integer
movement or rotation:

```c
int32_t distance = time_scale_rate(
	WORLD_METERS(3),
	delta_time
);
```

At full speed this moves three meters per second on both NTSC and PAL. If a
frame takes two VBlanks, the returned distance doubles for that frame. The
first frame may receive a zero delta because no VBlank has elapsed yet.

## Splines

`Spline` is a fixed-point Catmull-Rom curve. It passes through every control
point and supports open or closed paths:

```c
static const SVECTOR points[] = {
	{ WORLD_METERS(-3), 0, WORLD_METERS(6), 0 },
	{ WORLD_METERS( 0), 0, WORLD_METERS(4), 0 },
	{ WORLD_METERS( 3), 0, WORLD_METERS(6), 0 }
};

SplineEntity path;
spline_entity_init(&path, points, 3, 0);
```

The last argument selects an open (`0`) or closed (`1`) curve. A spline entity
has a normal entity transform, so the entire path can be moved or rotated. Its
render callback draws a subdivided `LINE_F2` approximation:

```c
spline_entity_set_color(&path, 64, 255, 255);
spline_entity_set_subdivisions(&path, 12);
entity_system_register(&entities, &path.entity);
```

A follower is a separate non-rendered entity that updates any target:

```c
SplineFollowerEntity follower;
spline_follower_entity_init(
	&follower,
	&path,
	cube_entity_as_entity(&cube),
	TIME_SECONDS(12),
	1
);
entity_system_register(&entities, &follower.entity);
```

The duration controls traversal time for the complete path; the final argument
enables looping. Playback can be paused, resumed or restarted. To use the same
path as a camera rail, pass `&camera.entity` as the target. Position follows
the spline; camera orientation remains independently controllable.

Parameter speed is uniform, but physical speed can vary between control points
because Catmull-Rom segments can have different lengths. Constant-distance
motion would require a precomputed arc-length table.

## Physics and collision

`PhysicsSystem` stores up to 32 AABB bodies without heap allocation. Bodies can
be static or dynamic:

```c
PhysicsSystem physics;
PhysicsBody floor_body;
PhysicsBody player_body;

physics_system_init(&physics);

physics_body_init(
	&floor_body,
	floor_entity,
	PHYSICS_BODY_STATIC,
	WORLD_METERS(2),
	WORLD_CENTIMETERS(50),
	WORLD_METERS(2)
);

physics_body_init(
	&player_body,
	player_entity,
	PHYSICS_BODY_DYNAMIC,
	WORLD_CENTIMETERS(50),
	WORLD_CENTIMETERS(50),
	WORLD_CENTIMETERS(50)
);

physics_system_register(&physics, &floor_body);
physics_system_register(&physics, &player_body);
```

Dynamic bodies accumulate forces until the next physics step:

```c
physics_body_set_mass(&player_body, 1);
physics_body_add_force(&player_body, WORLD_METERS(2), 0, 0);
physics_body_add_local_force(&player_body, 0, 0, WORLD_METERS(12));
physics_body_set_horizontal_damping(&player_body, TIME_SECONDS(4));
```

The default gravity is 9.8 meters per second squared along positive Y, matching
the project's screen-oriented coordinate convention. Velocity and gravity use
per-second world units; fractional integration remainders preserve sub-unit
movement over multiple frames.

Step physics after entity behavior and before rendering:

```c
entity_system_run(&entities, time.delta);
physics_system_step(&physics, time.delta);
renderer_draw_entities(&entities, &camera);
```

Optional callbacks receive the other body, contact normal and penetration:

```c
physics_body_set_collision_callback(
	&player_body,
	on_player_collision,
	player_data
);
```

The level registers a thin ground collider and five obstacle cubes as static
bodies. The player cube is dynamic, falls under gravity, receives movement
forces from the gamepad, and collides with the ground and obstacles.

This first implementation uses discrete, axis-aligned boxes and inelastic
response. Collider orientation does not follow visual rotation, and very fast
bodies can tunnel through thin colliders. More advanced rigid-body rotation,
swept collision and spatial partitioning can be added later without changing
the base entity API.

## Build without Docker

If you later build PSn00bSDK natively, add its `bin` directory to `PATH`, set
`PSN00BSDK_LIBS` to its `lib/libpsn00b` directory, and run:

```sh
cmake --preset default .
cmake --build --preset default
```
