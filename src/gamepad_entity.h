#ifndef GAMEPAD_ENTITY_H
#define GAMEPAD_ENTITY_H

#include <stdint.h>

#include "entity.h"
#include "physics.h"

#define GAMEPAD_BUFFER_LENGTH 34

typedef enum {
	GAMEPAD_PORT_1 = 0,
	GAMEPAD_PORT_2 = 1
} GamepadPort;

typedef struct {
	Entity entity;
	Entity *target;
	PhysicsBody *physics_target;
	uint8_t buffers[2][GAMEPAD_BUFFER_LENGTH];
	GamepadPort port;
	int32_t move_speed_per_second;
	int32_t movement_force;
	int16_t rotation_speed_per_second;
} GamepadEntity;

void gamepad_entity_init(
	GamepadEntity *gamepad,
	GamepadPort port,
	Entity *target
);
void gamepad_entity_set_target(GamepadEntity *gamepad, Entity *target);
void gamepad_entity_set_physics_target(
	GamepadEntity *gamepad,
	PhysicsBody *target
);
void gamepad_entity_set_speeds(
	GamepadEntity *gamepad,
	int32_t move_speed_per_second,
	int16_t rotation_speed_per_second
);
void gamepad_entity_set_movement_force(
	GamepadEntity *gamepad,
	int32_t movement_force
);
void gamepad_entity_stop(GamepadEntity *gamepad);

#endif
