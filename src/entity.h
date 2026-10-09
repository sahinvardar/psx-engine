#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <psxgte.h>

#include "game_time.h"

struct Camera;
struct Entity;

typedef void (*EntityRunMethod)(
	struct Entity *entity,
	TimeDelta delta_time
);
typedef void (*EntityRenderMethod)(
	const struct Entity *entity,
	const struct Camera *camera
);

typedef struct {
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t w;
} Quaternion;

typedef struct {
	VECTOR position;
	Quaternion orientation;
} Transform;

typedef struct Entity {
	Transform transform;
	EntityRunMethod run;
	EntityRenderMethod render;
} Entity;

void entity_init(Entity *entity);
void entity_set_run_method(Entity *entity, EntityRunMethod run);
void entity_set_render_method(Entity *entity, EntityRenderMethod render);
void entity_set_position(Entity *entity, int32_t x, int32_t y, int32_t z);
void entity_set_orientation(Entity *entity, const Quaternion *orientation);
void entity_move_world(Entity *entity, int32_t dx, int32_t dy, int32_t dz);
void entity_move_local(Entity *entity, int32_t dx, int32_t dy, int32_t dz);
void entity_rotate_local(
	Entity *entity,
	int16_t pitch_delta,
	int16_t yaw_delta,
	int16_t roll_delta
);
void entity_build_world_matrix(const Entity *entity, MATRIX *world_matrix);

#endif
