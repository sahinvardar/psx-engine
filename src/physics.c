#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgte.h>

#include "physics.h"
#include "units.h"

typedef enum {
	PHYSICS_AXIS_X,
	PHYSICS_AXIS_Y,
	PHYSICS_AXIS_Z
} PhysicsAxis;

static int32_t *vector_component(VECTOR *vector, PhysicsAxis axis) {
	if (axis == PHYSICS_AXIS_X) {
		return &vector->vx;
	}
	if (axis == PHYSICS_AXIS_Y) {
		return &vector->vy;
	}
	return &vector->vz;
}

static int32_t checked_add(int32_t left, int32_t right) {
	int64_t result = (int64_t) left + right;

	assert(result >= INT32_MIN && result <= INT32_MAX);
	return (int32_t) result;
}

static int32_t integrate_rate(
	int32_t rate,
	TimeDelta delta_time,
	int32_t *remainder
) {
	int64_t scaled =
		(int64_t) rate * delta_time
		+ *remainder;
	int64_t whole = scaled / TIME_ONE_SECOND;

	*remainder = (int32_t) (scaled % TIME_ONE_SECOND);
	assert(whole >= INT32_MIN && whole <= INT32_MAX);
	return (int32_t) whole;
}

static void integrate_body(
	PhysicsBody *body,
	const VECTOR *gravity,
	TimeDelta delta_time
) {
	VECTOR acceleration;
	int32_t dx;
	int32_t dy;
	int32_t dz;

	acceleration.vx = checked_add(
		gravity->vx,
		body->accumulated_force.vx / body->mass
	);
	acceleration.vy = checked_add(
		gravity->vy,
		body->accumulated_force.vy / body->mass
	);
	acceleration.vz = checked_add(
		gravity->vz,
		body->accumulated_force.vz / body->mass
	);
	body->accumulated_force = (VECTOR) { 0, 0, 0 };

	body->velocity.vx = checked_add(
		body->velocity.vx,
		integrate_rate(
			acceleration.vx,
			delta_time,
			&body->velocity_remainder.vx
		)
	);
	body->velocity.vy = checked_add(
		body->velocity.vy,
		integrate_rate(
			acceleration.vy,
			delta_time,
			&body->velocity_remainder.vy
		)
	);
	body->velocity.vz = checked_add(
		body->velocity.vz,
		integrate_rate(
			acceleration.vz,
			delta_time,
			&body->velocity_remainder.vz
		)
	);

	if (body->horizontal_damping > 0) {
		int64_t damping =
			(int64_t) body->horizontal_damping * delta_time
			/ TIME_ONE_SECOND;
		int32_t remaining;

		if (damping > TIME_ONE_SECOND) {
			damping = TIME_ONE_SECOND;
		}
		remaining = TIME_ONE_SECOND - (int32_t) damping;
		body->velocity.vx = (int32_t) (
			(int64_t) body->velocity.vx * remaining
			/ TIME_ONE_SECOND
		);
		body->velocity.vz = (int32_t) (
			(int64_t) body->velocity.vz * remaining
			/ TIME_ONE_SECOND
		);
		body->velocity_remainder.vx = 0;
		body->velocity_remainder.vz = 0;
	}

	dx = integrate_rate(
		body->velocity.vx,
		delta_time,
		&body->position_remainder.vx
	);
	dy = integrate_rate(
		body->velocity.vy,
		delta_time,
		&body->position_remainder.vy
	);
	dz = integrate_rate(
		body->velocity.vz,
		delta_time,
		&body->position_remainder.vz
	);
	entity_move_world(body->entity, dx, dy, dz);
}

static int64_t absolute_i64(int64_t value) {
	return value < 0 ? -value : value;
}

static int find_contact(
	const PhysicsBody *first,
	const PhysicsBody *second,
	PhysicsContact *contact,
	PhysicsAxis *axis,
	int32_t *direction
) {
	const VECTOR *first_position = &first->entity->transform.position;
	const VECTOR *second_position = &second->entity->transform.position;
	int64_t delta_x =
		(int64_t) second_position->vx - first_position->vx;
	int64_t delta_y =
		(int64_t) second_position->vy - first_position->vy;
	int64_t delta_z =
		(int64_t) second_position->vz - first_position->vz;
	int64_t overlap_x =
		(int64_t) first->half_extents.vx
		+ second->half_extents.vx
		- absolute_i64(delta_x);
	int64_t overlap_y =
		(int64_t) first->half_extents.vy
		+ second->half_extents.vy
		- absolute_i64(delta_y);
	int64_t overlap_z =
		(int64_t) first->half_extents.vz
		+ second->half_extents.vz
		- absolute_i64(delta_z);
	int64_t penetration;

	if (overlap_x <= 0 || overlap_y <= 0 || overlap_z <= 0) {
		return 0;
	}

	*axis = PHYSICS_AXIS_X;
	*direction = delta_x >= 0 ? 1 : -1;
	penetration = overlap_x;
	contact->normal = (VECTOR) { *direction * ONE, 0, 0 };

	if (overlap_y < penetration) {
		*axis = PHYSICS_AXIS_Y;
		*direction = delta_y >= 0 ? 1 : -1;
		penetration = overlap_y;
		contact->normal = (VECTOR) { 0, *direction * ONE, 0 };
	}
	if (overlap_z < penetration) {
		*axis = PHYSICS_AXIS_Z;
		*direction = delta_z >= 0 ? 1 : -1;
		penetration = overlap_z;
		contact->normal = (VECTOR) { 0, 0, *direction * ONE };
	}

	assert(penetration <= INT32_MAX);
	contact->penetration = (int32_t) penetration;
	return 1;
}

static void move_body_on_axis(
	PhysicsBody *body,
	PhysicsAxis axis,
	int32_t amount
) {
	VECTOR delta = { 0, 0, 0 };

	*vector_component(&delta, axis) = amount;
	entity_move_world(body->entity, delta.vx, delta.vy, delta.vz);
	*vector_component(&body->position_remainder, axis) = 0;
}

static void stop_inward_velocity(
	PhysicsBody *body,
	PhysicsAxis axis,
	int32_t direction
) {
	int32_t *velocity = vector_component(&body->velocity, axis);

	if (*velocity * (int64_t) direction > 0) {
		*velocity = 0;
		*vector_component(&body->velocity_remainder, axis) = 0;
	}
}

static void resolve_contact(
	PhysicsBody *first,
	PhysicsBody *second,
	const PhysicsContact *contact,
	PhysicsAxis axis,
	int32_t direction
) {
	if (
		first->type == PHYSICS_BODY_DYNAMIC
		&& second->type == PHYSICS_BODY_STATIC
	) {
		move_body_on_axis(
			first,
			axis,
			-direction * contact->penetration
		);
		stop_inward_velocity(first, axis, direction);
	} else if (
		first->type == PHYSICS_BODY_STATIC
		&& second->type == PHYSICS_BODY_DYNAMIC
	) {
		move_body_on_axis(
			second,
			axis,
			direction * contact->penetration
		);
		stop_inward_velocity(second, axis, -direction);
	} else {
		int32_t first_movement = contact->penetration / 2;
		int32_t second_movement =
			contact->penetration - first_movement;
		int32_t *first_velocity =
			vector_component(&first->velocity, axis);
		int32_t *second_velocity =
			vector_component(&second->velocity, axis);

		move_body_on_axis(first, axis, -direction * first_movement);
		move_body_on_axis(second, axis, direction * second_movement);

		if (
			(*first_velocity - *second_velocity)
			* (int64_t) direction > 0
		) {
			int32_t average = (int32_t) (
				((int64_t) *first_velocity + *second_velocity)
				/ 2
			);
			*first_velocity = average;
			*second_velocity = average;
			*vector_component(
				&first->velocity_remainder,
				axis
			) = 0;
			*vector_component(
				&second->velocity_remainder,
				axis
			) = 0;
		}
	}
}

static void dispatch_callbacks(
	PhysicsBody *first,
	PhysicsBody *second,
	const PhysicsContact *contact
) {
	if (first->on_collision != NULL) {
		first->on_collision(first, second, contact);
	}
	if (second->on_collision != NULL) {
		PhysicsContact opposite = *contact;

		opposite.normal.vx = -opposite.normal.vx;
		opposite.normal.vy = -opposite.normal.vy;
		opposite.normal.vz = -opposite.normal.vz;
		second->on_collision(second, first, &opposite);
	}
}

void physics_system_init(PhysicsSystem *system) {
	assert(system != NULL);

	for (int slot = 0; slot < PHYSICS_SYSTEM_CAPACITY; slot++) {
		system->bodies[slot] = NULL;
	}
	system->gravity = (VECTOR) {
		0,
		WORLD_CENTIMETERS(980),
		0
	};
	system->count = 0;
}

void physics_system_set_gravity(
	PhysicsSystem *system,
	int32_t x,
	int32_t y,
	int32_t z
) {
	assert(system != NULL);
	system->gravity = (VECTOR) { x, y, z };
}

PhysicsSystemResult physics_system_register(
	PhysicsSystem *system,
	PhysicsBody *body
) {
	int available_slot = -1;

	assert(system != NULL);
	assert(body != NULL);

	for (int slot = 0; slot < PHYSICS_SYSTEM_CAPACITY; slot++) {
		if (system->bodies[slot] == body) {
			return PHYSICS_SYSTEM_ALREADY_REGISTERED;
		}
		if (system->bodies[slot] == NULL && available_slot < 0) {
			available_slot = slot;
		}
	}
	if (available_slot < 0) {
		return PHYSICS_SYSTEM_FULL;
	}

	system->bodies[available_slot] = body;
	system->count++;
	return PHYSICS_SYSTEM_SUCCESS;
}

PhysicsSystemResult physics_system_deregister(
	PhysicsSystem *system,
	PhysicsBody *body
) {
	assert(system != NULL);
	assert(body != NULL);

	for (int slot = 0; slot < PHYSICS_SYSTEM_CAPACITY; slot++) {
		if (system->bodies[slot] == body) {
			system->bodies[slot] = NULL;
			system->count--;
			return PHYSICS_SYSTEM_SUCCESS;
		}
	}
	return PHYSICS_SYSTEM_NOT_REGISTERED;
}

void physics_system_step(PhysicsSystem *system, TimeDelta delta_time) {
	assert(system != NULL);
	assert(delta_time >= 0);

	for (int slot = 0; slot < PHYSICS_SYSTEM_CAPACITY; slot++) {
		PhysicsBody *body = system->bodies[slot];

		if (
			body != NULL
			&& body->enabled
			&& body->type == PHYSICS_BODY_DYNAMIC
		) {
			integrate_body(body, &system->gravity, delta_time);
		}
	}

	for (int first_slot = 0;
		first_slot < PHYSICS_SYSTEM_CAPACITY;
		first_slot++) {
		PhysicsBody *first = system->bodies[first_slot];

		if (first == NULL || !first->enabled) {
			continue;
		}

		for (int second_slot = first_slot + 1;
			second_slot < PHYSICS_SYSTEM_CAPACITY;
			second_slot++) {
			PhysicsBody *second = system->bodies[second_slot];
			PhysicsContact contact;
			PhysicsAxis axis;
			int32_t direction;

			if (
				second == NULL
				|| !second->enabled
				|| (
					first->type == PHYSICS_BODY_STATIC
					&& second->type == PHYSICS_BODY_STATIC
				)
			) {
				continue;
			}

			if (
				find_contact(
					first,
					second,
					&contact,
					&axis,
					&direction
				)
			) {
				resolve_contact(
					first,
					second,
					&contact,
					axis,
					direction
				);
				dispatch_callbacks(first, second, &contact);
			}
		}
	}
}

void physics_body_init(
	PhysicsBody *body,
	Entity *entity,
	PhysicsBodyType type,
	int32_t half_width,
	int32_t half_height,
	int32_t half_depth
) {
	assert(body != NULL);
	assert(entity != NULL);
	assert(half_width > 0);
	assert(half_height > 0);
	assert(half_depth > 0);

	body->entity = entity;
	body->half_extents = (VECTOR) {
		half_width,
		half_height,
		half_depth
	};
	body->velocity = (VECTOR) { 0, 0, 0 };
	body->accumulated_force = (VECTOR) { 0, 0, 0 };
	body->velocity_remainder = (VECTOR) { 0, 0, 0 };
	body->position_remainder = (VECTOR) { 0, 0, 0 };
	body->mass = 1;
	body->horizontal_damping = 0;
	body->type = type;
	body->on_collision = NULL;
	body->user_data = NULL;
	body->enabled = 1;
}

void physics_body_set_velocity(
	PhysicsBody *body,
	int32_t x,
	int32_t y,
	int32_t z
) {
	assert(body != NULL);
	body->velocity = (VECTOR) { x, y, z };
	body->velocity_remainder = (VECTOR) { 0, 0, 0 };
}

void physics_body_set_mass(PhysicsBody *body, int32_t mass) {
	assert(body != NULL);
	assert(mass > 0);
	body->mass = mass;
}

void physics_body_set_horizontal_damping(
	PhysicsBody *body,
	TimeDelta damping
) {
	assert(body != NULL);
	assert(damping >= 0);
	body->horizontal_damping = damping;
}

void physics_body_add_force(
	PhysicsBody *body,
	int32_t x,
	int32_t y,
	int32_t z
) {
	assert(body != NULL);
	assert(body->type == PHYSICS_BODY_DYNAMIC);
	body->accumulated_force.vx = checked_add(
		body->accumulated_force.vx,
		x
	);
	body->accumulated_force.vy = checked_add(
		body->accumulated_force.vy,
		y
	);
	body->accumulated_force.vz = checked_add(
		body->accumulated_force.vz,
		z
	);
}

void physics_body_add_local_force(
	PhysicsBody *body,
	int32_t x,
	int32_t y,
	int32_t z
) {
	MATRIX world_matrix;
	VECTOR local_force = { x, y, z };
	VECTOR world_force;

	assert(body != NULL);
	entity_build_world_matrix(body->entity, &world_matrix);
	ApplyMatrixLV(&world_matrix, &local_force, &world_force);
	physics_body_add_force(
		body,
		world_force.vx,
		world_force.vy,
		world_force.vz
	);
}

void physics_body_set_collision_callback(
	PhysicsBody *body,
	PhysicsCollisionCallback callback,
	void *user_data
) {
	assert(body != NULL);
	body->on_collision = callback;
	body->user_data = user_data;
}
