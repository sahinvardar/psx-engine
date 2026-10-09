#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgte.h>

#include "cube_entity.h"
#include "game_time.h"
#include "renderer.h"
#include "units.h"

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))
#define CUBE_HALF_EDGE WORLD_CENTIMETERS(50)

static const SVECTOR cube_vertices[] = {
	{ -CUBE_HALF_EDGE, -CUBE_HALF_EDGE, -CUBE_HALF_EDGE, 0 },
	{  CUBE_HALF_EDGE, -CUBE_HALF_EDGE, -CUBE_HALF_EDGE, 0 },
	{ -CUBE_HALF_EDGE,  CUBE_HALF_EDGE, -CUBE_HALF_EDGE, 0 },
	{  CUBE_HALF_EDGE,  CUBE_HALF_EDGE, -CUBE_HALF_EDGE, 0 },
	{  CUBE_HALF_EDGE, -CUBE_HALF_EDGE,  CUBE_HALF_EDGE, 0 },
	{ -CUBE_HALF_EDGE, -CUBE_HALF_EDGE,  CUBE_HALF_EDGE, 0 },
	{  CUBE_HALF_EDGE,  CUBE_HALF_EDGE,  CUBE_HALF_EDGE, 0 },
	{ -CUBE_HALF_EDGE,  CUBE_HALF_EDGE,  CUBE_HALF_EDGE, 0 }
};

static const SVECTOR cube_normals[] = {
	{    0,    0, -ONE, 0 },
	{    0,    0,  ONE, 0 },
	{    0, -ONE,    0, 0 },
	{    0,  ONE,    0, 0 },
	{ -ONE,    0,    0, 0 },
	{  ONE,    0,    0, 0 }
};

static const ModelMaterial cube_materials[] = {
	{ 255,  72,  72 },
	{  72, 192, 255 },
	{ 255, 208,  64 },
	{  96, 224, 112 },
	{ 208,  96, 255 },
	{  96, 128, 255 }
};

static const ModelTriangle cube_triangles[] = {
	{ { 0, 1, 2 }, 0, 0 },
	{ { 1, 3, 2 }, 0, 0 },
	{ { 4, 5, 6 }, 1, 1 },
	{ { 5, 7, 6 }, 1, 1 },
	{ { 5, 4, 0 }, 2, 2 },
	{ { 4, 1, 0 }, 2, 2 },
	{ { 6, 7, 3 }, 3, 3 },
	{ { 7, 2, 3 }, 3, 3 },
	{ { 0, 2, 5 }, 4, 4 },
	{ { 2, 7, 5 }, 4, 4 },
	{ { 3, 1, 6 }, 5, 5 },
	{ { 1, 4, 6 }, 5, 5 }
};

static const Model cube_model = {
	.vertices = cube_vertices,
	.normals = cube_normals,
	.materials = cube_materials,
	.triangles = cube_triangles,
	.vertex_count = ARRAY_LENGTH(cube_vertices),
	.normal_count = ARRAY_LENGTH(cube_normals),
	.material_count = ARRAY_LENGTH(cube_materials),
	.triangle_count = ARRAY_LENGTH(cube_triangles)
};

static void cube_entity_run(Entity *entity, TimeDelta delta_time) {
	CubeEntity *cube = (CubeEntity *) entity;
	int32_t pitch = time_scale_rate(cube->pitch_rate, delta_time);
	int32_t yaw = time_scale_rate(cube->yaw_rate, delta_time);
	int32_t roll = time_scale_rate(cube->roll_rate, delta_time);

	assert(pitch >= INT16_MIN && pitch <= INT16_MAX);
	assert(yaw >= INT16_MIN && yaw <= INT16_MAX);
	assert(roll >= INT16_MIN && roll <= INT16_MAX);
	entity_rotate_local(
		entity,
		(int16_t) pitch,
		(int16_t) yaw,
		(int16_t) roll
	);
}

void cube_entity_init(CubeEntity *cube) {
	assert(cube != NULL);
	model_entity_init(&cube->model_entity, &cube_model);
	entity_set_render_method(
		&cube->model_entity.entity,
		renderer_render_model_entity
	);
	cube->pitch_rate = 0;
	cube->yaw_rate = 0;
	cube->roll_rate = 0;
}

Entity *cube_entity_as_entity(CubeEntity *cube) {
	assert(cube != NULL);
	return &cube->model_entity.entity;
}

void cube_entity_set_spin(
	CubeEntity *cube,
	int32_t pitch_rate,
	int32_t yaw_rate,
	int32_t roll_rate
) {
	assert(cube != NULL);
	cube->pitch_rate = pitch_rate;
	cube->yaw_rate = yaw_rate;
	cube->roll_rate = roll_rate;
	entity_set_run_method(
		&cube->model_entity.entity,
		cube_entity_run
	);
}

void cube_entity_stop_spin(CubeEntity *cube) {
	assert(cube != NULL);
	cube->pitch_rate = 0;
	cube->yaw_rate = 0;
	cube->roll_rate = 0;
	entity_set_run_method(&cube->model_entity.entity, NULL);
}
