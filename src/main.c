#include <assert.h>

#include "camera.h"
#include "cube_entity.h"
#include "entity_system.h"
#include "game_time.h"
#include "gamepad_entity.h"
#include "renderer.h"
#include "spline_entity.h"
#include "units.h"

#define CUBE_GRID_SIZE 3
#define CUBE_GRID_COUNT (CUBE_GRID_SIZE * CUBE_GRID_SIZE)
#define CUBE_EDGE_LENGTH WORLD_METERS(1)
#define CUBE_GAP WORLD_METERS(1)
#define CUBE_CENTER_SPACING (CUBE_EDGE_LENGTH + CUBE_GAP)
#define CUBE_GRID_DEPTH WORLD_METERS(7)

static const SVECTOR spline_points[] = {
	{
		WORLD_METERS(-3),
		WORLD_METERS(-3),
		WORLD_METERS(6),
		0
	},
	{
		WORLD_METERS(3),
		WORLD_METERS(-3),
		WORLD_METERS(6),
		0
	},
	{
		WORLD_METERS(3),
		WORLD_METERS(3),
		WORLD_METERS(8),
		0
	},
	{
		WORLD_METERS(-3),
		WORLD_METERS(3),
		WORLD_METERS(8),
		0
	}
};

int main(void) {
	Camera camera;
	CubeEntity cubes[CUBE_GRID_COUNT];
	EntitySystem entities;
	GamepadEntity gamepad;
	GameTime time;
	CubeEntity moving_cube;
	SplineEntity spline_path;
	SplineFollowerEntity spline_follower;

	renderer_init();
	game_time_init(&time);
	entity_system_init(&entities);

	camera_init(&camera);
	entity_set_position(
		&camera.entity,
		WORLD_METERS(0),
		WORLD_METERS(0),
		WORLD_METERS(0)
	);

	gamepad_entity_init(
		&gamepad,
		GAMEPAD_PORT_1,
		&camera.entity
	);

	assert(
		entity_system_register(&entities, &gamepad.entity)
		== ENTITY_SYSTEM_SUCCESS
	);

	for (int row = 0; row < CUBE_GRID_SIZE; row++) {
		for (int column = 0; column < CUBE_GRID_SIZE; column++) {
			int index = row * CUBE_GRID_SIZE + column;
			Entity *cube_entity;

			cube_entity_init(&cubes[index]);
			cube_entity = cube_entity_as_entity(&cubes[index]);
			entity_set_position(
				cube_entity,
				(column - 1) * CUBE_CENTER_SPACING,
				(row - 1) * CUBE_CENTER_SPACING,
				CUBE_GRID_DEPTH
			);
			cube_entity_set_spin(
				&cubes[index],
				720 + row * 120,
				960 + column * 120,
				480
			);
			assert(
				entity_system_register(&entities, cube_entity)
				== ENTITY_SYSTEM_SUCCESS
			);
		}
	}

	spline_entity_init(
		&spline_path,
		spline_points,
		sizeof(spline_points) / sizeof(spline_points[0]),
		1
	);
	spline_entity_set_color(&spline_path, 64, 255, 255);
	spline_entity_set_subdivisions(&spline_path, 12);

	cube_entity_init(&moving_cube);
	cube_entity_set_spin(&moving_cube, 480, 720, 240);
	spline_follower_entity_init(
		&spline_follower,
		&spline_path,
		cube_entity_as_entity(&moving_cube),
		TIME_SECONDS(12),
		1
	);

	assert(
		entity_system_register(&entities, &spline_path.entity)
		== ENTITY_SYSTEM_SUCCESS
	);
	assert(
		entity_system_register(
			&entities,
			cube_entity_as_entity(&moving_cube)
		) == ENTITY_SYSTEM_SUCCESS
	);
	assert(
		entity_system_register(&entities, &spline_follower.entity)
		== ENTITY_SYSTEM_SUCCESS
	);

	for (;;) {
		game_time_update(&time);
		entity_system_run(&entities, time.delta);
		renderer_draw_entities(&entities, &camera);
		renderer_draw_text(8, 204, "D-PAD MOVE  FACE LOOK");
		renderer_draw_text(8, 216, "TRI/CROSS UP/DOWN");
		renderer_present();
	}

	return 0;
}
