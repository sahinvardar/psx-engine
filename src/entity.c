#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgte.h>

#include "entity.h"

typedef enum {
	AXIS_X,
	AXIS_Y,
	AXIS_Z
} Axis;

static Quaternion quaternion_multiply(
	const Quaternion *left,
	const Quaternion *right
) {
	Quaternion result;

	result.x = (
		left->w * right->x
		+ left->x * right->w
		+ left->y * right->z
		- left->z * right->y
	) / ONE;
	result.y = (
		left->w * right->y
		- left->x * right->z
		+ left->y * right->w
		+ left->z * right->x
	) / ONE;
	result.z = (
		left->w * right->z
		+ left->x * right->y
		- left->y * right->x
		+ left->z * right->w
	) / ONE;
	result.w = (
		left->w * right->w
		- left->x * right->x
		- left->y * right->y
		- left->z * right->z
	) / ONE;

	return result;
}

static void quaternion_normalize(Quaternion *quaternion) {
	int32_t squared_length =
		quaternion->x * quaternion->x
		+ quaternion->y * quaternion->y
		+ quaternion->z * quaternion->z
		+ quaternion->w * quaternion->w;
	int32_t length = SquareRoot0(squared_length);

	assert(length > 0);
	quaternion->x = quaternion->x * ONE / length;
	quaternion->y = quaternion->y * ONE / length;
	quaternion->z = quaternion->z * ONE / length;
	quaternion->w = quaternion->w * ONE / length;
}

static Quaternion quaternion_axis_rotation(Axis axis, int16_t angle) {
	int32_t half_angle = angle / 2;
	int32_t sine = isin(half_angle);
	Quaternion result = {
		.x = 0,
		.y = 0,
		.z = 0,
		.w = icos(half_angle)
	};

	if (axis == AXIS_X) {
		result.x = sine;
	} else if (axis == AXIS_Y) {
		result.y = sine;
	} else {
		result.z = sine;
	}

	return result;
}

static void quaternion_to_matrix(
	const Quaternion *quaternion,
	MATRIX *matrix
) {
	int32_t xx = quaternion->x * quaternion->x;
	int32_t yy = quaternion->y * quaternion->y;
	int32_t zz = quaternion->z * quaternion->z;
	int32_t xy = quaternion->x * quaternion->y;
	int32_t xz = quaternion->x * quaternion->z;
	int32_t yz = quaternion->y * quaternion->z;
	int32_t xw = quaternion->x * quaternion->w;
	int32_t yw = quaternion->y * quaternion->w;
	int32_t zw = quaternion->z * quaternion->w;

	matrix->m[0][0] = ONE - (2 * (yy + zz) / ONE);
	matrix->m[0][1] = 2 * (xy - zw) / ONE;
	matrix->m[0][2] = 2 * (xz + yw) / ONE;
	matrix->m[1][0] = 2 * (xy + zw) / ONE;
	matrix->m[1][1] = ONE - (2 * (xx + zz) / ONE);
	matrix->m[1][2] = 2 * (yz - xw) / ONE;
	matrix->m[2][0] = 2 * (xz - yw) / ONE;
	matrix->m[2][1] = 2 * (yz + xw) / ONE;
	matrix->m[2][2] = ONE - (2 * (xx + yy) / ONE);
	matrix->t[0] = 0;
	matrix->t[1] = 0;
	matrix->t[2] = 0;
}

static int32_t add_position_delta(int32_t position, int32_t delta) {
	int64_t result = (int64_t) position + delta;

	assert(result >= INT32_MIN && result <= INT32_MAX);
	return (int32_t) result;
}

void entity_init(Entity *entity) {
	assert(entity != NULL);
	entity->transform.position = (VECTOR) { 0, 0, 0 };
	entity->transform.orientation = (Quaternion) { 0, 0, 0, ONE };
	entity->run = NULL;
	entity->render = NULL;
}

void entity_set_run_method(Entity *entity, EntityRunMethod run) {
	assert(entity != NULL);
	entity->run = run;
}

void entity_set_render_method(Entity *entity, EntityRenderMethod render) {
	assert(entity != NULL);
	entity->render = render;
}

void entity_set_position(Entity *entity, int32_t x, int32_t y, int32_t z) {
	assert(entity != NULL);
	entity->transform.position = (VECTOR) { x, y, z };
}

void entity_set_orientation(Entity *entity, const Quaternion *orientation) {
	assert(entity != NULL);
	assert(orientation != NULL);
	assert(orientation->x >= -ONE && orientation->x <= ONE);
	assert(orientation->y >= -ONE && orientation->y <= ONE);
	assert(orientation->z >= -ONE && orientation->z <= ONE);
	assert(orientation->w >= -ONE && orientation->w <= ONE);
	entity->transform.orientation = *orientation;
	quaternion_normalize(&entity->transform.orientation);
}

void entity_move_world(Entity *entity, int32_t dx, int32_t dy, int32_t dz) {
	VECTOR *position;

	assert(entity != NULL);
	position = &entity->transform.position;
	position->vx = add_position_delta(position->vx, dx);
	position->vy = add_position_delta(position->vy, dy);
	position->vz = add_position_delta(position->vz, dz);
}

void entity_move_local(Entity *entity, int32_t dx, int32_t dy, int32_t dz) {
	MATRIX entity_rotation;
	VECTOR local_delta = { dx, dy, dz };
	VECTOR world_delta;

	assert(entity != NULL);
	quaternion_to_matrix(&entity->transform.orientation, &entity_rotation);
	ApplyMatrixLV(&entity_rotation, &local_delta, &world_delta);
	entity_move_world(
		entity,
		world_delta.vx,
		world_delta.vy,
		world_delta.vz
	);
}

void entity_rotate_local(
	Entity *entity,
	int16_t pitch_delta,
	int16_t yaw_delta,
	int16_t roll_delta
) {
	Quaternion delta;
	Quaternion *orientation;

	assert(entity != NULL);
	orientation = &entity->transform.orientation;

	if (pitch_delta != 0) {
		delta = quaternion_axis_rotation(AXIS_X, pitch_delta);
		*orientation = quaternion_multiply(orientation, &delta);
	}
	if (yaw_delta != 0) {
		delta = quaternion_axis_rotation(AXIS_Y, yaw_delta);
		*orientation = quaternion_multiply(orientation, &delta);
	}
	if (roll_delta != 0) {
		delta = quaternion_axis_rotation(AXIS_Z, roll_delta);
		*orientation = quaternion_multiply(orientation, &delta);
	}

	quaternion_normalize(orientation);
}

void entity_build_world_matrix(const Entity *entity, MATRIX *world_matrix) {
	assert(entity != NULL);
	assert(world_matrix != NULL);
	quaternion_to_matrix(&entity->transform.orientation, world_matrix);
	world_matrix->t[0] = entity->transform.position.vx;
	world_matrix->t[1] = entity->transform.position.vy;
	world_matrix->t[2] = entity->transform.position.vz;
}
