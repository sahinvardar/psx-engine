#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <psxapi.h>
#pragma GCC diagnostic pop

#include <psxpad.h>

#include "gamepad_entity.h"
#include "units.h"

#define ANALOG_DEAD_ZONE 20

static int32_t scale_analog_axis(
	int32_t axis,
	int32_t maximum
) {
	if (axis > -ANALOG_DEAD_ZONE && axis < ANALOG_DEAD_ZONE) {
		return 0;
	}
	return axis * maximum / 128;
}

static int is_supported_pad(const volatile PADTYPE *pad) {
	return pad->stat == 0
		&& (
			pad->type == PAD_ID_DIGITAL
			|| pad->type == PAD_ID_ANALOG_STICK
			|| pad->type == PAD_ID_ANALOG
		);
}

static void gamepad_entity_run(Entity *entity, TimeDelta delta_time) {
	GamepadEntity *gamepad = (GamepadEntity *) entity;
	const volatile PADTYPE *pad =
		(const volatile PADTYPE *) gamepad->buffers[gamepad->port];
	int32_t move_x = 0;
	int32_t move_y = 0;
	int32_t move_z = 0;
	int32_t pitch = 0;
	int32_t yaw = 0;
	int32_t roll = 0;
	int32_t move_amount;
	int32_t rotation_amount;
	uint16_t buttons;

	assert(gamepad->target != NULL);
	assert(delta_time >= 0);
	if (!is_supported_pad(pad)) {
		return;
	}

	move_amount = time_scale_rate(
		gamepad->move_speed_per_second,
		delta_time
	);
	rotation_amount = time_scale_rate(
		gamepad->rotation_speed_per_second,
		delta_time
	);
	assert(rotation_amount <= INT16_MAX);

	buttons = (uint16_t) ~pad->btn;

	if (buttons & PAD_LEFT) {
		move_x -= move_amount;
	}
	if (buttons & PAD_RIGHT) {
		move_x += move_amount;
	}
	if (buttons & PAD_UP) {
		move_z += move_amount;
	}
	if (buttons & PAD_DOWN) {
		move_z -= move_amount;
	}
	if (buttons & PAD_TRIANGLE) {
		move_y -= move_amount;
	}
	if (buttons & PAD_CROSS) {
		move_y += move_amount;
	}

	if (buttons & PAD_SQUARE) {
		yaw -= rotation_amount;
	}
	if (buttons & PAD_CIRCLE) {
		yaw += rotation_amount;
	}
	if (buttons & PAD_L1) {
		pitch -= rotation_amount;
	}
	if (buttons & PAD_R1) {
		pitch += rotation_amount;
	}
	if (buttons & PAD_L2) {
		roll -= rotation_amount;
	}
	if (buttons & PAD_R2) {
		roll += rotation_amount;
	}

	if (
		pad->type == PAD_ID_ANALOG_STICK
		|| pad->type == PAD_ID_ANALOG
	) {
		move_x += scale_analog_axis(
			(int32_t) pad->ls_x - 128,
			move_amount
		);
		move_z -= scale_analog_axis(
			(int32_t) pad->ls_y - 128,
			move_amount
		);
		yaw += scale_analog_axis(
			(int32_t) pad->rs_x - 128,
			rotation_amount
		);
		pitch += scale_analog_axis(
			(int32_t) pad->rs_y - 128,
			rotation_amount
		);
	}

	if (move_x != 0 || move_y != 0 || move_z != 0) {
		entity_move_local(gamepad->target, move_x, move_y, move_z);
	}
	if (pitch != 0 || yaw != 0 || roll != 0) {
		assert(pitch >= INT16_MIN && pitch <= INT16_MAX);
		assert(yaw >= INT16_MIN && yaw <= INT16_MAX);
		assert(roll >= INT16_MIN && roll <= INT16_MAX);
		entity_rotate_local(
			gamepad->target,
			(int16_t) pitch,
			(int16_t) yaw,
			(int16_t) roll
		);
	}
}

void gamepad_entity_init(
	GamepadEntity *gamepad,
	GamepadPort port,
	Entity *target
) {
	assert(gamepad != NULL);
	assert(port == GAMEPAD_PORT_1 || port == GAMEPAD_PORT_2);
	assert(target != NULL);

	entity_init(&gamepad->entity);
	entity_set_run_method(&gamepad->entity, gamepad_entity_run);
	gamepad->target = target;
	gamepad->port = port;
	gamepad->move_speed_per_second = WORLD_METERS(3);
	gamepad->rotation_speed_per_second = 1440;

	for (int pad = 0; pad < 2; pad++) {
		for (int index = 0; index < GAMEPAD_BUFFER_LENGTH; index++) {
			gamepad->buffers[pad][index] = 0xff;
		}
	}

	InitPAD(
		gamepad->buffers[0],
		GAMEPAD_BUFFER_LENGTH,
		gamepad->buffers[1],
		GAMEPAD_BUFFER_LENGTH
	);
	StartPAD();
	ChangeClearPAD(0);
}

void gamepad_entity_set_target(GamepadEntity *gamepad, Entity *target) {
	assert(gamepad != NULL);
	assert(target != NULL);
	gamepad->target = target;
}

void gamepad_entity_set_speeds(
	GamepadEntity *gamepad,
	int32_t move_speed_per_second,
	int16_t rotation_speed_per_second
) {
	assert(gamepad != NULL);
	assert(move_speed_per_second >= 0);
	assert(rotation_speed_per_second >= 0);
	gamepad->move_speed_per_second = move_speed_per_second;
	gamepad->rotation_speed_per_second = rotation_speed_per_second;
}

void gamepad_entity_stop(GamepadEntity *gamepad) {
	assert(gamepad != NULL);
	StopPAD();
	entity_set_run_method(&gamepad->entity, NULL);
}
