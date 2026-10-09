#include <assert.h>
#include <psxgte.h>

#include "camera.h"
#include "cube_entity.h"
#include "entity_system.h"
#include "game_time.h"
#include "gamepad_entity.h"
#include "physics.h"
#include "plane_entity.h"
#include "renderer.h"
#include "units.h"

#define OBSTACLE_COUNT 5

static const VECTOR obstacle_positions[OBSTACLE_COUNT] = {
	{ WORLD_METERS(-2), WORLD_CENTIMETERS(50), WORLD_METERS(5) },
	{ WORLD_METERS( 2), WORLD_CENTIMETERS(50), WORLD_METERS(5) },
	{ WORLD_METERS(-2), WORLD_CENTIMETERS(50), WORLD_METERS(9) },
	{ WORLD_METERS( 2), WORLD_CENTIMETERS(50), WORLD_METERS(9) },
	{ WORLD_METERS( 0), WORLD_CENTIMETERS(50), WORLD_METERS(11) }
};

static void register_entity(
	EntitySystem *system,
	Entity *entity
) {
	assert(
		entity_system_register(system, entity)
		== ENTITY_SYSTEM_SUCCESS
	);
}

static void register_body(
	PhysicsSystem *system,
	PhysicsBody *body
) {
	assert(
		physics_system_register(system, body)
		== PHYSICS_SYSTEM_SUCCESS
	);
}

int main(void) {
	Camera camera;
	CubeEntity obstacles[OBSTACLE_COUNT];
	CubeEntity player;
	EntitySystem entities;
	GamepadEntity gamepad;
	GameTime time;
	PhysicsBody ground_body;
	PhysicsBody obstacle_bodies[OBSTACLE_COUNT];
	PhysicsBody player_body;
	PhysicsSystem physics;
	PlaneEntity ground;

	renderer_init();
	game_time_init(&time);
	entity_system_init(&entities);
	physics_system_init(&physics);

	camera_init(&camera);
	entity_set_position(
		&camera.entity,
		WORLD_METERS(0),
		WORLD_METERS(-3),
		WORLD_METERS(-5)
	);
	entity_rotate_local(&camera.entity, -256, 0, 0);

	plane_entity_init(&ground);
	entity_set_position(
		plane_entity_as_entity(&ground),
		WORLD_METERS(0),
		WORLD_METERS(1),
		WORLD_METERS(7)
	);
	physics_body_init(
		&ground_body,
		plane_entity_as_entity(&ground),
		PHYSICS_BODY_STATIC,
		WORLD_METERS(6),
		WORLD_CENTIMETERS(10),
		WORLD_METERS(6)
	);
	register_entity(&entities, plane_entity_as_entity(&ground));
	register_body(&physics, &ground_body);

	for (int index = 0; index < OBSTACLE_COUNT; index++) {
		Entity *obstacle;

		cube_entity_init(&obstacles[index]);
		obstacle = cube_entity_as_entity(&obstacles[index]);
		entity_set_position(
			obstacle,
			obstacle_positions[index].vx,
			obstacle_positions[index].vy,
			obstacle_positions[index].vz
		);
		physics_body_init(
			&obstacle_bodies[index],
			obstacle,
			PHYSICS_BODY_STATIC,
			WORLD_CENTIMETERS(50),
			WORLD_CENTIMETERS(50),
			WORLD_CENTIMETERS(50)
		);
		register_entity(&entities, obstacle);
		register_body(&physics, &obstacle_bodies[index]);
	}

	cube_entity_init(&player);
	entity_set_position(
		cube_entity_as_entity(&player),
		WORLD_METERS(0),
		WORLD_CENTIMETERS(50),
		WORLD_METERS(3)
	);
	physics_body_init(
		&player_body,
		cube_entity_as_entity(&player),
		PHYSICS_BODY_DYNAMIC,
		WORLD_CENTIMETERS(50),
		WORLD_CENTIMETERS(50),
		WORLD_CENTIMETERS(50)
	);
	physics_body_set_horizontal_damping(
		&player_body,
		TIME_SECONDS(4)
	);
	register_entity(&entities, cube_entity_as_entity(&player));
	register_body(&physics, &player_body);

	gamepad_entity_init(
		&gamepad,
		GAMEPAD_PORT_1,
		cube_entity_as_entity(&player)
	);
	gamepad_entity_set_physics_target(&gamepad, &player_body);
	gamepad_entity_set_movement_force(
		&gamepad,
		WORLD_METERS(12)
	);
	register_entity(&entities, &gamepad.entity);

	for (;;) {
		game_time_update(&time);
		entity_system_run(&entities, time.delta);
		physics_system_step(&physics, time.delta);
		renderer_draw_entities(&entities, &camera);
		renderer_draw_text(8, 204, "L-STICK FORCE  R-STICK TURN");
		renderer_draw_text(8, 216, "MOVE THE PLAYER CUBE");
		renderer_present();
	}

	return 0;
}
